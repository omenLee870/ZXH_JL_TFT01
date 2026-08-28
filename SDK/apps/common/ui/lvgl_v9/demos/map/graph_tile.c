#define BOOL_DEFINE_CONFLICT

#include "graph_tile.h"
#include "app_config.h"
#include "asm/psram_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define GRAPH_PATH_MAX 128

/*
 * graph.dat 索引条目 (与 merge_graph.py 的 "<hhIII" 对应, 16 字节)
 */
#pragma pack(push, 1)
typedef struct {
    int16_t tile_x;
    int16_t tile_y;
    uint32_t offset;
    uint32_t node_count;
    uint32_t edge_count;
} graph_index_entry_t;
#pragma pack(pop)

static char graph_path[GRAPH_PATH_MAX];

static graph_index_entry_t *graph_index = NULL;
static uint32_t graph_index_count = 0;

static FILE *graph_fp = NULL;

static graph_tile_t cache[GRAPH_TILE_CACHE_SIZE];
static uint32_t use_counter = 0;

/* PSRAM 辅助函数前置声明 (定义在下方 "PSRAM 分配 + 安全读入" 段) */
static void *graph_psram_malloc(size_t size);
static void  graph_psram_free(void *p);
static int   graph_fread_psram(void *dst, size_t len, FILE *fp);

/*******************************************************
 * 道路名字符串表 (names.bin)
 *******************************************************/
static uint8_t  *names_body = NULL;
static uint32_t *names_offsets = NULL;
static uint32_t  names_count = 0;

bool graph_names_load(const char *path)
{
    graph_names_free();

    char file[256];
    snprintf(file, sizeof(file), "%s/names.bin", path);

    FILE *fp = fopen(file, "r");
    if (fp == NULL) {
        printf("graph_names: open fail %s\n", file);
        return false;
    }

    uint32_t count = 0;
    if (fread(&count, 1, sizeof(count), fp) != sizeof(count)) {
        fclose(fp);
        return false;
    }
    if (count == 0) {
        fclose(fp);
        return false;
    }

    uint32_t n_off = count + 1;
    names_offsets = (uint32_t *)graph_psram_malloc(sizeof(uint32_t) * n_off);
    if (names_offsets == NULL) {
        fclose(fp);
        return false;
    }
    if (graph_fread_psram(names_offsets, sizeof(uint32_t) * n_off, fp) != 0) {
        graph_psram_free(names_offsets);
        names_offsets = NULL;
        fclose(fp);
        return false;
    }

    uint32_t body_len = names_offsets[n_off - 1];   /* 最后一个 offset = body 总长 */
    names_body = (uint8_t *)graph_psram_malloc(body_len);
    if (names_body == NULL) {
        graph_psram_free(names_offsets);
        names_offsets = NULL;
        fclose(fp);
        return false;
    }
    if (graph_fread_psram(names_body, body_len, fp) != 0) {
        graph_psram_free(names_body);
        names_body = NULL;
        graph_psram_free(names_offsets);
        names_offsets = NULL;
        fclose(fp);
        return false;
    }

    names_count = count;
    fclose(fp);
    printf("graph_names: %u names loaded (%s)\n", count - 1, file);
    return true;
}

bool graph_name_lookup(uint16_t id, char *out, int out_size)
{
    if (out == NULL || out_size <= 0) {
        return false;
    }
    out[0] = 0;
    if (names_body == NULL || names_offsets == NULL) {
        return false;
    }
    if ((uint32_t)id >= names_count) {
        return false;
    }

    uint32_t s = names_offsets[id];
    uint32_t e = names_offsets[id + 1];
    uint32_t len = e - s;
    if (len == 0) {
        return true;   /* 空串 */
    }
    if ((int)len >= out_size) {
        len = out_size - 1;
    }
    memcpy(out, names_body + s, len);
    out[len] = 0;
    return true;
}

void graph_names_free(void)
{
    if (names_body) {
        graph_psram_free(names_body);
        names_body = NULL;
    }
    if (names_offsets) {
        graph_psram_free(names_offsets);
        names_offsets = NULL;
    }
    names_count = 0;
}

/*******************************************************
 * PSRAM 分配 + 安全读入
 *
 * SD 卡底层用 DMA 搬数据，直接 fread 到 PSRAM 会踩到
 * DMA 与 cache 一致性的坑（当读取长度非 4 字节对齐时尤其
 * 容易读到坏数据，例如 14 字节的 edge 数组 × 奇数个）。
 * 因此改为：fread 到 SRAM 中转 buffer，再 memcpy 进 PSRAM。
 * 全程只有 CPU 访问 PSRAM，不存在 DMA/cache 一致性问题。
 *******************************************************/
#if TCFG_PSRAM_DEV_ENABLE
static void *graph_psram_malloc(size_t size)
{
    return malloc_psram(size);
}

static void graph_psram_free(void *p)
{
    if (p) {
        free_psram(p);
    }
}
#else
static void *graph_psram_malloc(size_t size)
{
    return malloc(size);
}

static void graph_psram_free(void *p)
{
    free(p);
}
#endif

/* 从 fp 读 len 字节，经 SRAM 中转后拷贝到 PSRAM */
#define GRAPH_STAGE_SIZE 512
static int graph_fread_psram(void *dst, size_t len, FILE *fp)
{
    static uint32_t stage[GRAPH_STAGE_SIZE / 4] __attribute__((aligned(32)));
    uint8_t *s = (uint8_t *)stage;
    uint8_t *d = (uint8_t *)dst;
    size_t left = len;

    while (left > 0) {
        size_t n = left < GRAPH_STAGE_SIZE ? left : GRAPH_STAGE_SIZE;
        if (fread(s, 1, n, fp) != n) {
            return -1;
        }
        memcpy(d, s, n);
        d += n;
        left -= n;
    }
    return 0;
}

/*******************************************************
 * 二分查找索引
 *******************************************************/
static graph_index_entry_t *graph_index_find(int tile_x, int tile_y)
{
    if (graph_index == NULL) {
        return NULL;
    }

    int lo = 0, hi = (int)graph_index_count - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        graph_index_entry_t *e = &graph_index[mid];

        if (e->tile_x < tile_x) {
            lo = mid + 1;
        } else if (e->tile_x > tile_x) {
            hi = mid - 1;
        } else {
            if (e->tile_y < tile_y) {
                lo = mid + 1;
            } else if (e->tile_y > tile_y) {
                hi = mid - 1;
            } else {
                return e;
            }
        }
    }
    return NULL;
}

/*******************************************************
 * 释放单个缓存槽位
 *******************************************************/
static void graph_tile_free(graph_tile_t *tile)
{
    if (tile == NULL) {
        return;
    }
    if (tile->nodes) {
        graph_psram_free(tile->nodes);
        tile->nodes = NULL;
    }
    if (tile->edges) {
        graph_psram_free(tile->edges);
        tile->edges = NULL;
    }
    if (tile->rev_edges) {
        graph_psram_free(tile->rev_edges);
        tile->rev_edges = NULL;
    }
    memset(tile, 0, sizeof(graph_tile_t));
}

/*******************************************************
 * 初始化: 打开 graph.dat, 加载索引到内存
 *******************************************************/
bool graph_tile_init(const char *path)
{
    if (path == NULL) {
        return false;
    }

    strncpy(graph_path, path, sizeof(graph_path) - 1);
    graph_path[sizeof(graph_path) - 1] = 0;
    memset(cache, 0, sizeof(cache));
    use_counter = 0;

    graph_tile_free_all();

    char file[256];
    snprintf(file, sizeof(file), "%s/graph.dat", graph_path);

    graph_fp = fopen(file, "r");
    if (graph_fp == NULL) {
        printf("graph_tile: open %s fail\n", file);
        return false;
    }

    uint32_t count = 0;
    if (fread(&count, 1, sizeof(count), graph_fp) != sizeof(count)) {
        printf("graph_tile: read header fail %s\n", file);
        fclose(graph_fp);
        graph_fp = NULL;
        return false;
    }
    if (count == 0) {
        printf("graph_tile: 0 tiles in %s\n", file);
        fclose(graph_fp);
        graph_fp = NULL;
        return false;
    }

    size_t ibytes = sizeof(graph_index_entry_t) * count;
    graph_index = (graph_index_entry_t *)graph_psram_malloc(ibytes);
    if (graph_index == NULL) {
        printf("graph_tile: malloc index fail (%d bytes)\n", (int)ibytes);
        fclose(graph_fp);
        graph_fp = NULL;
        return false;
    }

    if (graph_fread_psram(graph_index, ibytes, graph_fp) != 0) {
        printf("graph_tile: read index fail\n");
        graph_psram_free(graph_index);
        graph_index = NULL;
        fclose(graph_fp);
        graph_fp = NULL;
        return false;
    }

    graph_index_count = count;
    printf("graph_tile: %d tiles, %s\n", (int)count, file);
    return true;
}

/*******************************************************
 * 查找缓存
 *******************************************************/
graph_tile_t *graph_tile_find(int tile_x, int tile_y)
{
    for (int i = 0; i < GRAPH_TILE_CACHE_SIZE; i++) {
        if (cache[i].used &&
            cache[i].tile_x == tile_x &&
            cache[i].tile_y == tile_y) {
            cache[i].last_used = ++use_counter;
            return &cache[i];
        }
    }
    return NULL;
}

/*******************************************************
 * LRU 分配槽位
 *******************************************************/
static graph_tile_t *graph_tile_alloc_slot(void)
{
    for (int i = 0; i < GRAPH_TILE_CACHE_SIZE; i++) {
        if (!cache[i].used) {
            return &cache[i];
        }
    }

    uint32_t min = 0xffffffff;
    int idx = 0;
    for (int i = 0; i < GRAPH_TILE_CACHE_SIZE; i++) {
        if (cache[i].last_used < min) {
            min = cache[i].last_used;
            idx = i;
        }
    }
    graph_tile_t *result = &cache[idx];
    graph_tile_free(result);
    return result;
}

/*******************************************************
 * 加载 tile
 *******************************************************/
graph_tile_t *graph_tile_load(int tile_x, int tile_y)
{
    graph_tile_t *tile = graph_tile_find(tile_x, tile_y);
    if (tile) {
        return tile;
    }

    graph_index_entry_t *entry = graph_index_find(tile_x, tile_y);
    if (entry == NULL) {
        return NULL;
    }

    tile = graph_tile_alloc_slot();
    if (tile == NULL) {
        return NULL;
    }

    if (fseek(graph_fp, (long)entry->offset, SEEK_SET) != 0) {
        return NULL;
    }

    uint32_t nc = 0, ec = 0, rec = 0;
    if (fread(&nc, 1, sizeof(nc), graph_fp) != sizeof(nc) ||
        fread(&ec, 1, sizeof(ec), graph_fp) != sizeof(ec) ||
        fread(&rec, 1, sizeof(rec), graph_fp) != sizeof(rec)) {
        return NULL;
    }
    if (nc == 0) {
        return NULL;
    }

    tile->nodes = (graph_node_t *)graph_psram_malloc(sizeof(graph_node_t) * nc);
    tile->edges = (graph_edge_t *)graph_psram_malloc(sizeof(graph_edge_t) * ec);
    tile->rev_edges = (graph_edge_t *)graph_psram_malloc(sizeof(graph_edge_t) * rec);
    if (tile->nodes == NULL || (ec > 0 && tile->edges == NULL) ||
        (rec > 0 && tile->rev_edges == NULL)) {
        graph_tile_free(tile);
        return NULL;
    }

    size_t nb = sizeof(graph_node_t) * nc;
    size_t eb = sizeof(graph_edge_t) * ec;
    size_t rb = sizeof(graph_edge_t) * rec;
    if (graph_fread_psram(tile->nodes, nb, graph_fp) != 0 ||
        (ec > 0 && graph_fread_psram(tile->edges, eb, graph_fp) != 0) ||
        (rec > 0 && graph_fread_psram(tile->rev_edges, rb, graph_fp) != 0)) {
        graph_tile_free(tile);
        return NULL;
    }

    tile->tile_x   = tile_x;
    tile->tile_y   = tile_y;
    tile->node_count = nc;
    tile->edge_count = ec;
    tile->rev_edge_count = rec;
    tile->used     = 1;
    tile->last_used = ++use_counter;

    return tile;
}

/*******************************************************
 * 经纬度 → tile 编号
 *******************************************************/
void graph_coord_to_tile(double lat, double lon, int *tile_x, int *tile_y)
{
    if (tile_x) {
        *tile_x = (int)floor(lon * 100);
    }
    if (tile_y) {
        *tile_y = (int)floor(lat * 100);
    }
}

/*******************************************************
 * 释放全部
 *******************************************************/
void graph_tile_free_all(void)
{
    for (int i = 0; i < GRAPH_TILE_CACHE_SIZE; i++) {
        graph_tile_free(&cache[i]);
    }

    if (graph_fp) {
        fclose(graph_fp);
        graph_fp = NULL;
    }

    if (graph_index) {
        graph_psram_free(graph_index);
        graph_index = NULL;
        graph_index_count = 0;
    }

    graph_names_free();
}

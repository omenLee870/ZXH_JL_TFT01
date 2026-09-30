/**
 * @file gui_parse_dyn.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "fs/fs.h"
#include "gui_parse_dyn.h"
#include <string.h>
#include "app_config.h"
#include "sdfile.h"


#if LV_USE_GUI_PARSE_DYN != 0
#if     TCFG_SDFILE_INSERT_FLASH_ENABLE
#define LVGL_MODE_PHY_BASE TCFG_SDFILE_FLASH_DEV_PATY_BASE
#elif   TCFG_SDFILE_EXTERN_FLASH_ENABLE
#define LVGL_MODE_PHY_BASE TCFG_SDFILE_FLASH_DEV_PATY_BASE
#else
#define LVGL_MODE_PHY_BASE 0
#endif

extern u32 sdfile_flash_addr2cpu_addr(u32 offset);
extern u32 sfc1_flash_addr2cpu_addr(u32 offset);


#include "bind.h"
#include "control.h"
#include "dyn_reader.h"
#include "event.h"
#include "prop.h"
#include "res.h"
#include "style.h"

/*********************
 *      DEFINES
 *********************/

#define DYN_RUNTIME_ID_START 0x80000000 // 动态页面ID起始值
#define DYN_RUNTIME_ID_END   0x8FFFFFFF // 动态页面ID结束值

#define DYN_TARGET_LVGL_MAJOR    9U
#define DYN_MAX_SECTION_COUNT    32U
#define DYN_MAX_TREE_DEPTH       64U

/**********************
 *      TYPEDEFS
 **********************/
typedef struct {
    char *path;           /* 动态页面文件路径，用于同一路径的 screen 复用运行时 id。 */
    int32_t runtime_id;   /* 资源链表、screen 管理和删除回收共用的运行时页面 id。 */
    bool preserve;        /* 重载旧 screen 时临时保留 path->id 映射，避免删除回调清掉映射。 */
} dyn_path_id_t;

typedef struct {
    uint32_t *data;
    uint32_t count;
    uint32_t capacity;
} dyn_id_list_t;

typedef struct {
    dyn_section_entry_t sections[DYN_MAX_SECTION_COUNT]; /* section 目录已做范围和 CRC 校验后的结果。 */
    uint16_t section_count;
    uint32_t file_size;
} dyn_file_view_t;

typedef struct {
    uint32_t page_id;
    dyn_view_t data;
} dyn_preview_view_t;

typedef struct {
    /* 这些 view 只记录 dyn 缓冲区中的位置，不拥有实际资源字节；调用方必须先于 free_dyn_data 使用完。 */
    dyn_resource_view_t *resources;
    uint32_t resource_count;
    uint32_t resource_capacity;
    dyn_page_view_t *pages;
    uint32_t page_count;
    uint32_t page_capacity;
    dyn_preview_view_t *previews;
    uint32_t preview_count;
    uint32_t preview_capacity;
} dyn_decoded_file_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

static char *get_dyn_data(const char *path, int32_t *size);
static char *get_dyn_data_ext(const char *path, int32_t *size);
static void free_dyn_data(char *buf);
static dyn_err_t parse_file_view(const uint8_t *buf, uint32_t size, dyn_file_view_t *file);
static dyn_err_t parse_section_dir(const uint8_t *buf, const uint8_t *file_end, uint32_t dir_offset,
                                   uint16_t section_count, uint16_t dir_crc, dyn_file_view_t *file);
static dyn_err_t decode_file_views(const uint8_t *buf, const dyn_file_view_t *file, dyn_decoded_file_t *decoded);
static dyn_err_t decode_resource_section(const uint8_t *p, const uint8_t *end, uint16_t item_count,
        dyn_decoded_file_t *decoded, dyn_id_list_t *resource_ids);
static dyn_err_t decode_page_section(const uint8_t *p, const uint8_t *end, uint16_t item_count,
                                     dyn_decoded_file_t *decoded, dyn_id_list_t *page_ids,
                                     dyn_id_list_t *control_ids, bool *has_page);
static dyn_err_t decode_preview_section(const uint8_t *p, const uint8_t *end, uint16_t item_count,
                                        dyn_decoded_file_t *decoded, dyn_id_list_t *preview_ids);
static dyn_err_t decode_resources(const dyn_decoded_file_t *decoded, int32_t runtime_id);
static dyn_err_t find_preview(const dyn_decoded_file_t *decoded, uint32_t page_id, dyn_view_t *preview);
static dyn_err_t build_staged_obj(const dyn_decoded_file_t *decoded, lv_obj_t *parent,
                                  int32_t temp_runtime_id, lv_obj_t **out_obj);
static dyn_err_t read_resource_item(const uint8_t **p, const uint8_t *section_end, dyn_resource_view_t *resource);
static dyn_err_t read_page_item(const uint8_t **p, const uint8_t *section_end, dyn_page_view_t *page);
static dyn_err_t read_control_record(const uint8_t **p, const uint8_t *page_end, dyn_control_view_t *control);
static dyn_err_t validate_page(const dyn_page_view_t *page, dyn_id_list_t *control_ids);
static dyn_err_t validate_blocks(dyn_view_t blocks, uint8_t block_count);
static dyn_err_t validate_block_items(dyn_block_view_t block);
static bool skip_control_subtree(const dyn_page_view_t *page, const uint8_t **p, const uint8_t *end,
                                 uint16_t *control_index, uint16_t child_count);
static lv_obj_t *parse_page_obj(const dyn_page_view_t *page, lv_obj_t *parent, int32_t runtime_id,
                                bool add_delete_cb);
static void decoded_file_free(dyn_decoded_file_t *decoded);
static bool decoded_add_resource(dyn_decoded_file_t *decoded, dyn_resource_view_t resource);
static bool decoded_add_page(dyn_decoded_file_t *decoded, dyn_page_view_t page);
static bool decoded_add_preview(dyn_decoded_file_t *decoded, uint32_t page_id, dyn_view_t data);
static bool id_list_add_unique(dyn_id_list_t *list, uint32_t id);
static void id_list_free(dyn_id_list_t *list);
static void page_delete_event_cb(lv_event_t *e);
static void dyn_fread_buf_free_cb(lv_event_t *e);
static int32_t dyn_runtime_id_alloc(void);
static int32_t dyn_runtime_id_get(const char *path);
static dyn_path_id_t *dyn_runtime_id_find(int32_t runtime_id);
static void dyn_runtime_id_remove(int32_t runtime_id);
static void log_dyn_error(dyn_err_t err, const char *path);

/**********************
 *  STATIC VARIABLES
 **********************/

static lv_ll_t dyn_path_id_ll;
static bool s_dyn_is_fread_buf = false; /* fread 兜底时为 true，页面对象会注册 LV_EVENT_DELETE 回调释放缓冲 */
static gui_dyn_ext_reader_t s_dyn_ext_reader = NULL; /* 应用层注册的外部读取器（packres 归档等），默认未注册 */

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void gui_dyn_init()
{
    res_list_init();
    lv_ll_init(&dyn_path_id_ll, sizeof(dyn_path_id_t));
}

lv_obj_t *gui_dyn_parse_obj(const char *path, lv_obj_t *parent)
{
    int32_t size;
    char *buf = get_dyn_data(path, &size);
    if (buf == NULL) {
        return NULL;
    }

    dyn_file_view_t file;
    dyn_err_t err = parse_file_view((const uint8_t *)buf, (uint32_t)size, &file);
    if (err != DYN_OK) {
        log_dyn_error(err, path);
        free_dyn_data(buf);
        return NULL;
    }

    /* Object 实例不复用路径级 runtime_id，避免同路径多实例互相释放资源。 */
    int32_t runtime_id = dyn_runtime_id_alloc();
    int32_t temp_runtime_id = dyn_runtime_id_alloc();
    if (res_list_has_page(temp_runtime_id)) {
        res_page_free(temp_runtime_id);
    }

    dyn_decoded_file_t decoded = {0};
    err = decode_file_views((const uint8_t *)buf, &file, &decoded);
    if (err != DYN_OK) {
        log_dyn_error(err, path);
        free_dyn_data(buf);
        return NULL;
    }

    lv_obj_t *obj = NULL;
    err = build_staged_obj(&decoded, parent, temp_runtime_id, &obj);
    if (err != DYN_OK) {
        log_dyn_error(err, path);
        res_page_free(temp_runtime_id);
        decoded_file_free(&decoded);
        free_dyn_data(buf);
        return NULL;
    }

    if (res_list_has_page(temp_runtime_id) && !res_page_rename(temp_runtime_id, runtime_id)) {
        LV_LOG_WARN("rename dyn temp resources failed: %s", path);
        lv_obj_del(obj);
        res_page_free(temp_runtime_id);
        decoded_file_free(&decoded);
        free_dyn_data(buf);
        return NULL;
    }

    if (lv_obj_is_valid(obj)) {
        lv_obj_add_event_cb(obj, page_delete_event_cb, LV_EVENT_DELETE, (void *)(intptr_t)runtime_id);
    }

    /* fread 兜底：页面对象删除时自动释放 dyn 缓冲区 */
    if (s_dyn_is_fread_buf && buf != NULL && lv_obj_is_valid(obj)) {
        lv_obj_add_event_cb(obj, dyn_fread_buf_free_cb, LV_EVENT_DELETE, buf);
        s_dyn_is_fread_buf = false; /* 缓冲区所有权已转交给事件回调 */
    }

    decoded_file_free(&decoded);
    free_dyn_data(buf);
    return obj;
}

gui_scr_t *gui_dyn_parse_scr(const char *path)
{
    int32_t size;
    char *buf = get_dyn_data(path, &size);
    if (buf == NULL) {
        return NULL;
    }

    dyn_file_view_t file;
    dyn_err_t err = parse_file_view((const uint8_t *)buf, (uint32_t)size, &file);
    if (err != DYN_OK) {
        log_dyn_error(err, path);
        free_dyn_data(buf);
        return NULL;
    }

    int32_t runtime_id = dyn_runtime_id_get(path);
    gui_scr_t *old_scr = gui_scr_get(runtime_id);
    if (old_scr && old_scr->scr && lv_obj_is_valid(old_scr->scr)) {
        if (gui_scr_get_act() == old_scr) {
            LV_LOG_WARN("dyn screen is already active, skip reload: %s", path);
            free_dyn_data(buf);
            return old_scr;
        }
    }

    int32_t temp_runtime_id = dyn_runtime_id_alloc();
    if (res_list_has_page(temp_runtime_id)) {
        res_page_free(temp_runtime_id);
    }

    dyn_decoded_file_t decoded = {0};
    err = decode_file_views((const uint8_t *)buf, &file, &decoded);
    if (err != DYN_OK) {
        log_dyn_error(err, path);
        free_dyn_data(buf);
        return NULL;
    }

    lv_obj_t *obj = NULL;

    err = build_staged_obj(&decoded, NULL, temp_runtime_id, &obj);
    if (err != DYN_OK) {
        log_dyn_error(err, path);
        res_page_free(temp_runtime_id);
        decoded_file_free(&decoded);
        free_dyn_data(buf);
        return NULL;
    }

    if (old_scr && old_scr->scr && lv_obj_is_valid(old_scr->scr)) {
        /* 新页面已经完整构建成功后，才删除旧页面并释放旧资源。 */
        dyn_path_id_t *node = dyn_runtime_id_find(runtime_id);
        if (node) {
            node->preserve = true;
        }
        lv_obj_del(old_scr->scr);
        if (res_list_has_page(runtime_id)) {
            res_page_free(runtime_id);
        }
    } else if (res_list_has_page(runtime_id)) {
        res_page_free(runtime_id);
    }

    if (res_list_has_page(temp_runtime_id) && !res_page_rename(temp_runtime_id, runtime_id)) {
        LV_LOG_WARN("rename dyn temp resources failed: %s", path);
        lv_obj_del(obj);
        res_page_free(temp_runtime_id);
        decoded_file_free(&decoded);
        free_dyn_data(buf);
        return NULL;
    }
    lv_obj_add_event_cb(obj, page_delete_event_cb, LV_EVENT_DELETE, (void *)(intptr_t)runtime_id);

    /* fread 兜底：页面对象删除时自动释放 dyn 缓冲区 */
    if (s_dyn_is_fread_buf && buf != NULL && lv_obj_is_valid(obj)) {
        lv_obj_add_event_cb(obj, dyn_fread_buf_free_cb, LV_EVENT_DELETE, buf);
        s_dyn_is_fread_buf = false; /* 缓冲区所有权已转交给事件回调 */
    }

    gui_scr_t *scr = gui_scr_create(runtime_id, "", obj, NULL, NULL);
    if (scr == NULL) {
        lv_obj_del(obj);
        decoded_file_free(&decoded);
        free_dyn_data(buf);
        return NULL;
    }

    decoded_file_free(&decoded);
    free_dyn_data(buf);
    return scr;
}

void gui_dyn_set_event_get_cb(gui_get_event_cb_t cb)
{
    event_set_get_cb(cb);
}

void gui_dyn_set_bind_send_cb(gui_bind_send_cb_t cb)
{
    bind_set_send_cb(cb);
}

void gui_dyn_set_bind_get_cb(gui_bind_get_cb_t cb)
{
    bind_set_get_cb(cb);
}

void gui_dyn_set_bind_get_subject_cb(gui_bind_get_subject_cb_t cb)
{
    bind_set_get_subject_cb(cb);
}

void gui_dyn_set_bind_get_data_cb(gui_bind_get_data_cb_t cb)
{
    bind_set_get_data_cb(cb);
}

void gui_dyn_set_ext_reader(gui_dyn_ext_reader_t reader)
{
    s_dyn_ext_reader = reader;
}

lv_image_dsc_t *gui_dyn_get_preview(const char *path)
{
    int32_t size;
    char *buf = get_dyn_data(path, &size);
    if (buf == NULL) {
        return NULL;
    }

    dyn_file_view_t file;
    dyn_decoded_file_t decoded = {0};
    dyn_view_t preview = {0};
    lv_image_dsc_t *img = NULL;
    dyn_err_t err = parse_file_view((const uint8_t *)buf, (uint32_t)size, &file);
    if (err == DYN_OK) {
        err = decode_file_views((const uint8_t *)buf, &file, &decoded);
    }
    if (err == DYN_OK) {
        err = decoded.page_count > 0 ? find_preview(&decoded, decoded.pages[0].page_id, &preview) : DYN_ERR_BAD_COUNT;
    }
    if (err != DYN_OK) {
        log_dyn_error(err, path);
        goto cleanup;
    }

    img = (lv_image_dsc_t *)res_parse_view(preview.ptr, preview.len);

cleanup:
    decoded_file_free(&decoded);
    free_dyn_data(buf);
    return img;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * @brief 页面删除时释放 fread 兜底分配的 dyn 缓冲区。
 *
 * @param e LVGL 删除事件，user_data 指向需释放的缓冲区指针
 */
static void dyn_fread_buf_free_cb(lv_event_t *e)
{
    void *buf = lv_event_get_user_data(e);
    if (buf != NULL) {
        lv_free(buf);
    }
}

/**
 * @brief 将 dyn 文件完整读入内存（优先 flash XIP 直接映射，失效时回退 fread）。
 *
 * fread 回退的缓冲区通过注册 LV_EVENT_DELETE 回调释放，
 * 确保在页面对象生命周期内缓冲区始终有效。
 *
 * @param path dyn 文件路径
 * @param size 输出缓冲区大小
 * @return 文件数据缓冲区
 */
/**
 * @brief 通过应用层注册的读取器读取 dyn（packres 归档等普通 fopen 打不开的路径）。
 *
 * 缓冲区由本模块用 lv_malloc 分配，与 free_dyn_data() / 页面删除回调的
 * lv_free 配对，内存所有权与 fread 兜底路径完全一致；读取器只负责填数据。
 */
static char *get_dyn_data_ext(const char *path, int32_t *size)
{
    int32_t need;
    char *buf;

    if (s_dyn_ext_reader == NULL) {
        return NULL;
    }

    need = s_dyn_ext_reader(path, NULL, 0, NULL);
    if (need <= 0) {
        LV_LOG_WARN("dyn ext reader: get size failed: %s", path);
        return NULL;
    }

    buf = lv_malloc((size_t)need);
    if (buf == NULL) {
        LV_LOG_ERROR("dyn ext reader: lv_malloc %d failed", (int)need);
        return NULL;
    }

    if (s_dyn_ext_reader(path, buf, need, size) != 0) {
        lv_free(buf);
        return NULL;
    }

    s_dyn_is_fread_buf = true;
    return buf;
}

static char *get_dyn_data(const char *path, int32_t *size)
{
    s_dyn_is_fread_buf = false;

    /* 先问应用层注册的外部读取器（packres 归档等）。
     *
     * 必须放在 fopen 之前：实测 vitfat 的 FS 对归档路径 …/C/watch1/watch1.dyn
     * 会 fopen 成功，但返回的是归档文件本身的 sclust/fsize，不是内部 .dyn 的数据。
     * 读取器只接管它认得路径（返回 >0），其余返回 <=0，交回下面的原有逻辑
     * （含 flash XIP 直读），因此未注册或路径不匹配时行为与改造前一致。 */
    if (s_dyn_ext_reader != NULL) {
        char *ext = get_dyn_data_ext(path, size);
        if (ext != NULL) {
            return ext;
        }
    }

    FILE *fp = fopen(path, "r");
    if (!fp) {
        printf("%s: open dyn file failed: %s\n", __func__, path);
        return NULL;
    }

    struct vfs_attr file_attr = {0};
    fget_attrs(fp, &file_attr);
    *size = (int32_t)file_attr.fsize;

    char *addr = NULL;
#if TCFG_SDFILE_EXTERN_FLASH_ENABLE
    addr = (char *)sfc1_flash_addr2cpu_addr(file_attr.sclust + LVGL_MODE_PHY_BASE);
#elif TCFG_SDFILE_INSERT_FLASH_ENABLE
    addr = (char *)sdfile_flash_addr2cpu_addr(file_attr.sclust + LVGL_MODE_PHY_BASE);
#endif

    /* 校验 flash XIP 地址是否指向有效的 dyn 数据（magic "JL"） */
    if (addr != NULL && file_attr.fsize >= 2 &&
        addr[0] == 'J' && addr[1] == 'L') {
        fclose(fp);
        return addr;
    }

    /* Flash XIP 地址映射失败或无映射，回退到 fread 方式读入堆内存 */
    fclose(fp);

    LV_LOG_WARN("dyn XIP unmapped (addr=%p, sclust=0x%x), use fread: %s",
                (void *)addr, (unsigned int)file_attr.sclust, path);

    fp = fopen(path, "r");
    if (!fp) {
        LV_LOG_ERROR("dyn reopen for fread failed: %s", path);
        return NULL;
    }

    addr = lv_malloc(file_attr.fsize);
    if (addr) {
        if (fread(addr, 1, file_attr.fsize, fp) != (size_t)file_attr.fsize) {
            LV_LOG_ERROR("dyn fread failed: %s", path);
            lv_free(addr);
            addr = NULL;
        } else {
            s_dyn_is_fread_buf = true;
        }
    }
    fclose(fp);
    return addr;
}
/**
 * @brief 释放 dyn 文件数据缓冲区。
 *
 * Flash XIP 直接映射时无需释放（数据在 flash 中）。
 * fread 回退路径：在页面对象场景中，缓冲区所有权已转移给
 * LV_EVENT_DELETE 回调（s_dyn_is_fread_buf 已置 false），此处为无操作；
 * 在 gui_dyn_get_preview 等无页面对象的场景中，直接释放。
 *
 * @param buf 文件数据缓冲区
 */
static void free_dyn_data(char *buf)
{
    if (s_dyn_is_fread_buf && buf != NULL) {
        lv_free(buf);
    }
}

static dyn_err_t parse_file_view(const uint8_t *buf, uint32_t size, dyn_file_view_t *file)
{
    if (size < JLDYN_FILE_HEADER_SIZE) {
        return DYN_ERR_SHORT;
    }

    const uint8_t *p = buf;
    const uint8_t *end = buf + size;
    uint8_t magic0;
    uint8_t magic1;
    uint16_t version;
    uint8_t tag[4];
    uint16_t header_size;
    uint8_t target_lvgl_major;
    uint8_t reserved;
    uint32_t file_size;
    uint32_t section_dir_offset;
    uint16_t section_count;
    uint16_t section_dir_crc16;

    if (!dyn_read_u8(&p, end, &magic0) || !dyn_read_u8(&p, end, &magic1) ||
        !dyn_read_le16(&p, end, &version) || !dyn_read_u8(&p, end, &tag[0]) ||
        !dyn_read_u8(&p, end, &tag[1]) || !dyn_read_u8(&p, end, &tag[2]) ||
        !dyn_read_u8(&p, end, &tag[3]) || !dyn_read_le16(&p, end, &header_size) ||
        !dyn_read_u8(&p, end, &target_lvgl_major) || !dyn_read_u8(&p, end, &reserved) ||
        !dyn_read_le32(&p, end, &file_size) || !dyn_read_le32(&p, end, &section_dir_offset) ||
        !dyn_read_le16(&p, end, &section_count) || !dyn_read_le16(&p, end, &section_dir_crc16)) {
        return DYN_ERR_SHORT;
    }

    if (magic0 != 'J' || magic1 != 'L') {
        return DYN_ERR_MAGIC;
    }
    if (version != JLDYN_PROTOCOL_VERSION || header_size != JLDYN_FILE_HEADER_SIZE ||
        memcmp(tag, "W005", 4) != 0 || reserved != 0) {
        return DYN_ERR_VERSION;
    }
    if (target_lvgl_major != DYN_TARGET_LVGL_MAJOR) {
        return DYN_ERR_LVGL_MISMATCH;
    }
    if (file_size != size || file_size < JLDYN_FILE_HEADER_SIZE) {
        return DYN_ERR_RANGE;
    }
    if (section_dir_offset != dyn_align4(header_size)) {
        return DYN_ERR_ALIGN;
    }
    if (section_count == 0 || section_count > DYN_MAX_SECTION_COUNT) {
        return DYN_ERR_BAD_COUNT;
    }

    /* 文件头只描述 section 目录位置，真正的页面/资源内容在 decode_file_views 中单次展开。 */
    lv_memset(file, 0, sizeof(*file));
    file->file_size = file_size;
    file->section_count = section_count;
    return parse_section_dir(buf, buf + file_size, section_dir_offset, section_count, section_dir_crc16, file);
}

static dyn_err_t parse_section_dir(const uint8_t *buf, const uint8_t *file_end, uint32_t dir_offset,
                                   uint16_t section_count, uint16_t dir_crc, dyn_file_view_t *file)
{
    uint32_t file_size = (uint32_t)(file_end - buf);
    uint32_t dir_size = (uint32_t)section_count * JLDYN_SECTION_DIR_ENTRY_SIZE;
    uint32_t dir_end_offset = dir_offset + dir_size;
    if (dir_offset > file_size || dir_end_offset < dir_offset || dir_end_offset > file_size) {
        return DYN_ERR_RANGE;
    }
    if (dyn_crc16(buf + dir_offset, dir_size) != dir_crc) {
        return DYN_ERR_CRC;
    }

    const uint8_t *p = buf + dir_offset;
    const uint8_t *end = p + dir_size;
    for (uint16_t i = 0; i < section_count; i++) {
        dyn_section_entry_t *entry = &file->sections[i];
        uint16_t reserved;
        if (!dyn_read_u8(&p, end, &entry->section_type) || !dyn_read_u8(&p, end, &entry->section_version) ||
            !dyn_read_le16(&p, end, &reserved) || !dyn_read_le32(&p, end, &entry->section_offset) ||
            !dyn_read_le32(&p, end, &entry->section_size) || !dyn_read_le16(&p, end, &entry->item_count) ||
            !dyn_read_le16(&p, end, &entry->section_crc16)) {
            return DYN_ERR_SHORT;
        }

        if (reserved != 0 || (entry->section_offset & 3U) != 0 || (entry->section_size & 3U) != 0) {
            return DYN_ERR_ALIGN;
        }
        if (entry->section_version > 1U &&
            (entry->section_type == JLDYN_SECTION_PAGE || entry->section_type == JLDYN_SECTION_RESOURCE ||
             entry->section_type == JLDYN_SECTION_PREVIEW)) {
            return DYN_ERR_UNSUPPORTED_SECTION_VERSION;
        }
        /* 先确认 section 范围在文件内，CRC 校验本身也不能越界读。 */
        if (entry->section_offset > file_size || entry->section_offset + entry->section_size < entry->section_offset ||
            entry->section_offset + entry->section_size > file_size) {
            return DYN_ERR_RANGE;
        }
        if (dyn_crc16(buf + entry->section_offset, entry->section_size) != entry->section_crc16) {
            return DYN_ERR_CRC;
        }
    }
    return DYN_OK;
}

static dyn_err_t decode_file_views(const uint8_t *buf, const dyn_file_view_t *file, dyn_decoded_file_t *decoded)
{
    dyn_id_list_t resource_ids = {0};
    dyn_id_list_t page_ids = {0};
    dyn_id_list_t control_ids = {0};
    dyn_id_list_t preview_ids = {0};
    bool has_page = false;
    dyn_err_t err = DYN_OK;

    if (buf == NULL || file == NULL || decoded == NULL) {
        return DYN_ERR_RANGE;
    }

    /* 单次扫描 section，完成结构校验并缓存轻量视图，避免构建和预览流程重复解析文件。 */
    for (uint16_t i = 0; i < file->section_count && err == DYN_OK; i++) {
        const dyn_section_entry_t *entry = &file->sections[i];
        const uint8_t *p = buf + entry->section_offset;
        const uint8_t *end = p + entry->section_size;

        switch (entry->section_type) {
        case JLDYN_SECTION_RESOURCE:
            err = decode_resource_section(p, end, entry->item_count, decoded, &resource_ids);
            break;
        case JLDYN_SECTION_PAGE:
            err = decode_page_section(p, end, entry->item_count, decoded, &page_ids, &control_ids, &has_page);
            break;
        case JLDYN_SECTION_PREVIEW:
            err = decode_preview_section(p, end, entry->item_count, decoded, &preview_ids);
            break;
        default:
            break;
        }
    }

    if (err == DYN_OK && !has_page) {
        err = DYN_ERR_BAD_COUNT;
    }

    id_list_free(&resource_ids);
    id_list_free(&page_ids);
    id_list_free(&control_ids);
    id_list_free(&preview_ids);
    if (err != DYN_OK) {
        decoded_file_free(decoded);
    }
    return err;
}

static dyn_err_t decode_resource_section(const uint8_t *p, const uint8_t *end, uint16_t item_count,
        dyn_decoded_file_t *decoded, dyn_id_list_t *resource_ids)
{
    for (uint16_t item = 0; item < item_count; item++) {
        dyn_resource_view_t resource;
        dyn_err_t err = read_resource_item(&p, end, &resource);
        if (err != DYN_OK) {
            return err;
        }
        /* 资源 id 在整个 dyn 文件内唯一，避免运行时 res_get 命中歧义。 */
        if (!id_list_add_unique(resource_ids, resource.resource_id)) {
            return DYN_ERR_DUPLICATE_ID;
        }
        if (!decoded_add_resource(decoded, resource)) {
            return DYN_ERR_ALLOC;
        }
    }
    return p == end ? DYN_OK : DYN_ERR_BAD_COUNT;
}

static dyn_err_t decode_page_section(const uint8_t *p, const uint8_t *end, uint16_t item_count,
                                     dyn_decoded_file_t *decoded, dyn_id_list_t *page_ids,
                                     dyn_id_list_t *control_ids, bool *has_page)
{
    if (item_count > 0) {
        *has_page = true;
    }

    for (uint16_t item = 0; item < item_count; item++) {
        dyn_page_view_t page;
        dyn_err_t err = read_page_item(&p, end, &page);
        if (err != DYN_OK) {
            return err;
        }
        /* 页面和控件在构建前完成树结构校验，后续创建 LVGL 对象时不再重复扫描 page section。 */
        if (!id_list_add_unique(page_ids, page.page_id)) {
            return DYN_ERR_DUPLICATE_ID;
        }
        err = validate_page(&page, control_ids);
        if (err != DYN_OK) {
            return err;
        }
        if (!decoded_add_page(decoded, page)) {
            return DYN_ERR_ALLOC;
        }
    }
    return p == end ? DYN_OK : DYN_ERR_BAD_COUNT;
}

static dyn_err_t decode_preview_section(const uint8_t *p, const uint8_t *end, uint16_t item_count,
                                        dyn_decoded_file_t *decoded, dyn_id_list_t *preview_ids)
{
    for (uint16_t item = 0; item < item_count; item++) {
        uint32_t preview_page_id;
        uint32_t data_len;
        dyn_view_t data;
        if (!dyn_read_le32(&p, end, &preview_page_id) || !dyn_read_le32(&p, end, &data_len) ||
            !dyn_read_bytes_view(&p, end, data_len, &data) || !dyn_skip_padding4(&p, end, data_len)) {
            return DYN_ERR_RANGE;
        }
        if (!id_list_add_unique(preview_ids, preview_page_id)) {
            return DYN_ERR_DUPLICATE_ID;
        }
        if (!decoded_add_preview(decoded, preview_page_id, data)) {
            return DYN_ERR_ALLOC;
        }
    }
    return p == end ? DYN_OK : DYN_ERR_BAD_COUNT;
}

static dyn_err_t decode_resources(const dyn_decoded_file_t *decoded, int32_t runtime_id)
{
    for (uint32_t i = 0; i < decoded->resource_count; i++) {
        if (!res_page_add_resource(runtime_id, decoded->resources[i])) {
            return DYN_ERR_ALLOC;
        }
    }
    return DYN_OK;
}

static dyn_err_t find_preview(const dyn_decoded_file_t *decoded, uint32_t page_id, dyn_view_t *preview)
{
    for (uint32_t i = 0; i < decoded->preview_count; i++) {
        if (decoded->previews[i].page_id == page_id) {
            *preview = decoded->previews[i].data;
            return DYN_OK;
        }
    }
    return DYN_ERR_BAD_COUNT;
}

static dyn_err_t build_staged_obj(const dyn_decoded_file_t *decoded, lv_obj_t *parent,
                                  int32_t temp_runtime_id, lv_obj_t **out_obj)
{
    dyn_err_t err;

    if (decoded == NULL || out_obj == NULL) {
        return DYN_ERR_RANGE;
    }
    *out_obj = NULL;
    if (decoded->page_count == 0) {
        return DYN_ERR_BAD_COUNT;
    }

    /* 先把资源挂到临时 id；对象完整构建成功后再重命名，避免失败时污染旧页面资源。 */
    err = decode_resources(decoded, temp_runtime_id);
    if (err == DYN_OK) {
        /* 入口页沿用文件内第一个 page item，避免构建流程再次扫描 page section。 */
        *out_obj = parse_page_obj(&decoded->pages[0], parent, temp_runtime_id, false);
        if (*out_obj == NULL) {
            err = DYN_ERR_BAD_TREE;
        }
    }

    return err;
}

static dyn_err_t read_resource_item(const uint8_t **p, const uint8_t *section_end, dyn_resource_view_t *resource)
{
    uint8_t r0;
    uint8_t r1;
    uint8_t r2;
    uint32_t data_len;
    if (!dyn_read_le32(p, section_end, &resource->resource_id) || !dyn_read_u8(p, section_end, &resource->resource_type) ||
        !dyn_read_u8(p, section_end, &r0) || !dyn_read_u8(p, section_end, &r1) || !dyn_read_u8(p, section_end, &r2) ||
        !dyn_read_le32(p, section_end, &data_len)) {
        return DYN_ERR_SHORT;
    }
    if (r0 != 0 || r1 != 0 || r2 != 0) {
        return DYN_ERR_VERSION;
    }
    if (!dyn_read_bytes_view(p, section_end, data_len, &resource->data) || !dyn_skip_padding4(p, section_end, data_len)) {
        return DYN_ERR_RANGE;
    }
    return DYN_OK;
}

static dyn_err_t read_page_item(const uint8_t **p, const uint8_t *section_end, dyn_page_view_t *page)
{
    const uint8_t *start = *p;
    uint32_t page_name_len;
    if (!dyn_read_le32(p, section_end, &page->page_id) || !dyn_read_le32(p, section_end, &page->page_size) ||
        !dyn_read_le16(p, section_end, &page->control_count)) {
        return DYN_ERR_SHORT;
    }

    uint16_t reserved;
    if (!dyn_read_le16(p, section_end, &reserved) || !dyn_read_le32(p, section_end, &page_name_len)) {
        return DYN_ERR_SHORT;
    }
    if (reserved != 0 || page->control_count == 0 || page->page_size < JLDYN_PAGE_ITEM_HEADER_SIZE ||
        (page->page_size & 3U) != 0 || !dyn_can_read(start, section_end, page->page_size)) {
        return DYN_ERR_RANGE;
    }

    const uint8_t *page_end = start + page->page_size;
    if (!dyn_read_bytes_view(p, page_end, page_name_len, &page->page_name) || !dyn_skip_padding4(p, page_end, page_name_len)) {
        return DYN_ERR_RANGE;
    }
    page->controls.ptr = *p;
    page->controls.len = (uint32_t)(page_end - *p);
    *p = page_end;
    return DYN_OK;
}

static dyn_err_t read_control_record(const uint8_t **p, const uint8_t *page_end, dyn_control_view_t *control)
{
    const uint8_t *start = *p;
    if (!dyn_read_u8(p, page_end, &control->control_type) || !dyn_read_u8(p, page_end, &control->block_count) ||
        !dyn_read_le16(p, page_end, &control->child_count) || !dyn_read_le32(p, page_end, &control->control_id) ||
        !dyn_read_le32(p, page_end, &control->record_size)) {
        return DYN_ERR_SHORT;
    }

    if (control->record_size < JLDYN_CONTROL_RECORD_HEADER_SIZE || (control->record_size & 3U) != 0 ||
        !dyn_can_read(start, page_end, control->record_size)) {
        return DYN_ERR_RANGE;
    }

    const uint8_t *record_end = start + control->record_size;
    control->blocks.ptr = *p;
    control->blocks.len = (uint32_t)(record_end - *p);
    *p = record_end;
    return DYN_OK;
}

static dyn_err_t validate_page(const dyn_page_view_t *page, dyn_id_list_t *control_ids)
{
    const uint8_t *p = page->controls.ptr;
    const uint8_t *end = page->controls.ptr + page->controls.len;
    uint16_t stack[DYN_MAX_TREE_DEPTH];
    uint32_t depth = 0;

    for (uint16_t i = 0; i < page->control_count; i++) {
        dyn_control_view_t control;
        dyn_err_t err = read_control_record(&p, end, &control);
        if (err != DYN_OK) {
            return err;
        }
        if (control.control_id == 0) {
            return DYN_ERR_RANGE;
        }
        if (!id_list_add_unique(control_ids, control.control_id)) {
            return DYN_ERR_DUPLICATE_ID;
        }
        err = validate_blocks(control.blocks, control.block_count);
        if (err != DYN_OK) {
            return err;
        }

        if (i > 0) {
            if (depth == 0 || stack[depth - 1] == 0) {
                return DYN_ERR_BAD_TREE;
            }
            stack[depth - 1]--;
        }
        if (control.child_count > 0) {
            if (depth >= DYN_MAX_TREE_DEPTH) {
                return DYN_ERR_BAD_TREE;
            }
            stack[depth++] = control.child_count;
        }
        while (depth > 0 && stack[depth - 1] == 0) {
            depth--;
        }
    }

    if (p != end || depth != 0) {
        return DYN_ERR_BAD_TREE;
    }
    return DYN_OK;
}

static dyn_err_t validate_blocks(dyn_view_t blocks, uint8_t block_count)
{
    const uint8_t *p = blocks.ptr;
    const uint8_t *end = blocks.ptr + blocks.len;
    for (uint8_t i = 0; i < block_count; i++) {
        dyn_block_view_t block;
        if (!dyn_read_block(&p, end, &block)) {
            return DYN_ERR_RANGE;
        }
        if (dyn_block_has_item_stream(block.block_type)) {
            dyn_err_t err = validate_block_items(block);
            if (err != DYN_OK) {
                return err;
            }
        }
    }
    if ((end - p) > 3) {
        return DYN_ERR_BAD_COUNT;
    }
    while (p < end) {
        if (*p++ != 0) {
            return DYN_ERR_ALIGN;
        }
    }
    return DYN_OK;
}

static dyn_err_t validate_block_items(dyn_block_view_t block)
{
    const uint8_t *p = block.items.ptr;
    const uint8_t *end = block.items.ptr + block.items.len;
    for (uint16_t i = 0; i < block.item_count; i++) {
        dyn_item_view_t item;
        if (!dyn_read_item(&p, end, &item)) {
            return DYN_ERR_RANGE;
        }
    }
    return p == end ? DYN_OK : DYN_ERR_BAD_COUNT;
}

static bool skip_control_subtree(const dyn_page_view_t *page, const uint8_t **p, const uint8_t *end,
                                 uint16_t *control_index, uint16_t child_count)
{
    uint32_t remaining = child_count;

    while (remaining > 0) {
        dyn_control_view_t control;
        if (*control_index + 1 >= page->control_count) {
            return false;
        }

        (*control_index)++;
        if (read_control_record(p, end, &control) != DYN_OK) {
            return false;
        }

        remaining--;
        remaining += control.child_count;
        if (remaining > page->control_count) {
            return false;
        }
    }

    return true;
}

static lv_obj_t *parse_page_obj(const dyn_page_view_t *page, lv_obj_t *parent, int32_t runtime_id,
                                bool add_delete_cb)
{
    const uint8_t *p = page->controls.ptr;
    const uint8_t *end = page->controls.ptr + page->controls.len;
    lv_obj_t *root = NULL;
    lv_obj_t *obj_stack[DYN_MAX_TREE_DEPTH];
    uint16_t child_stack[DYN_MAX_TREE_DEPTH];
    uint32_t depth = 0;

    /* 控件记录按先序遍历存储；child_stack 记录每层还剩多少子节点未消费。 */
    for (uint16_t i = 0; i < page->control_count; i++) {
        dyn_control_view_t control;
        if (read_control_record(&p, end, &control) != DYN_OK) {
            goto fail;
        }

        lv_obj_t *parent_obj = (i == 0) ? parent : (depth > 0 ? obj_stack[depth - 1] : NULL);
        if (i > 0 && parent_obj == NULL) {
            goto fail;
        }

        if (i > 0) {
            if (child_stack[depth - 1] == 0) {
                goto fail;
            }
            child_stack[depth - 1]--;
        }

        control_support_t support = control_get_support(control.control_type);
        /* 未知或当前固件未启用的非根控件按整棵子树跳过，避免单个控件拖垮整页构建。 */
        if (support != CONTROL_SUPPORT_AVAILABLE) {
            if (support == CONTROL_SUPPORT_DISABLED) {
                LV_LOG_WARN("skip disabled control type %d", (int)control.control_type);
            } else {
                LV_LOG_WARN("skip unknown control type %d", (int)control.control_type);
            }
            if (i == 0 || !skip_control_subtree(page, &p, end, &i, control.child_count)) {
                goto fail;
            }
            while (depth > 0 && child_stack[depth - 1] == 0) {
                depth--;
            }
            continue;
        }

        lv_obj_t *obj = control_gen(control.control_type, control.blocks, parent_obj);
        if (obj == NULL) {
            goto fail;
        }
        if (i == 0) {
            root = obj;
        }
        if (!control_apply_blocks(control.blocks, control.block_count, obj, runtime_id)) {
            goto fail;
        }

        if (control.child_count > 0) {
            if (depth >= DYN_MAX_TREE_DEPTH) {
                goto fail;
            }
            obj_stack[depth] = obj;
            child_stack[depth] = control.child_count;
            depth++;
        }
        while (depth > 0 && child_stack[depth - 1] == 0) {
            depth--;
        }
    }

    if (p != end || depth != 0) {
        goto fail;
    }

    if (add_delete_cb && root != NULL && lv_obj_is_valid(root)) {
        lv_obj_add_event_cb(root, page_delete_event_cb, LV_EVENT_DELETE, (void *)(intptr_t)runtime_id);
    }
    return root;

fail:
    if (root != NULL && lv_obj_is_valid(root)) {
        lv_obj_del(root);
    }
    return NULL;
}

static void decoded_file_free(dyn_decoded_file_t *decoded)
{
    if (decoded == NULL) {
        return;
    }

    if (decoded->resources != NULL) {
        lv_free(decoded->resources);
        decoded->resources = NULL;
    }
    if (decoded->pages != NULL) {
        lv_free(decoded->pages);
        decoded->pages = NULL;
    }
    if (decoded->previews != NULL) {
        lv_free(decoded->previews);
        decoded->previews = NULL;
    }
    decoded->resource_count = 0;
    decoded->resource_capacity = 0;
    decoded->page_count = 0;
    decoded->page_capacity = 0;
    decoded->preview_count = 0;
    decoded->preview_capacity = 0;
}

static bool decoded_add_resource(dyn_decoded_file_t *decoded, dyn_resource_view_t resource)
{
    if (decoded->resource_count == decoded->resource_capacity) {
        uint32_t new_capacity = decoded->resource_capacity == 0 ? 8U : decoded->resource_capacity * 2U;
        dyn_resource_view_t *new_data = lv_realloc(decoded->resources, new_capacity * sizeof(dyn_resource_view_t));
        if (new_data == NULL) {
            return false;
        }
        decoded->resources = new_data;
        decoded->resource_capacity = new_capacity;
    }

    decoded->resources[decoded->resource_count++] = resource;
    return true;
}

static bool decoded_add_page(dyn_decoded_file_t *decoded, dyn_page_view_t page)
{
    if (decoded->page_count == decoded->page_capacity) {
        uint32_t new_capacity = decoded->page_capacity == 0 ? 4U : decoded->page_capacity * 2U;
        dyn_page_view_t *new_data = lv_realloc(decoded->pages, new_capacity * sizeof(dyn_page_view_t));
        if (new_data == NULL) {
            return false;
        }
        decoded->pages = new_data;
        decoded->page_capacity = new_capacity;
    }

    decoded->pages[decoded->page_count++] = page;
    return true;
}

static bool decoded_add_preview(dyn_decoded_file_t *decoded, uint32_t page_id, dyn_view_t data)
{
    if (decoded->preview_count == decoded->preview_capacity) {
        uint32_t new_capacity = decoded->preview_capacity == 0 ? 4U : decoded->preview_capacity * 2U;
        dyn_preview_view_t *new_data = lv_realloc(decoded->previews, new_capacity * sizeof(dyn_preview_view_t));
        if (new_data == NULL) {
            return false;
        }
        decoded->previews = new_data;
        decoded->preview_capacity = new_capacity;
    }

    decoded->previews[decoded->preview_count].page_id = page_id;
    decoded->previews[decoded->preview_count].data = data;
    decoded->preview_count++;
    return true;
}

static bool id_list_add_unique(dyn_id_list_t *list, uint32_t id)
{
    for (uint32_t i = 0; i < list->count; i++) {
        if (list->data[i] == id) {
            return false;
        }
    }

    if (list->count == list->capacity) {
        uint32_t new_capacity = list->capacity == 0 ? 16U : list->capacity * 2U;
        uint32_t *new_data = lv_realloc(list->data, new_capacity * sizeof(uint32_t));
        if (new_data == NULL) {
            return false;
        }
        list->data = new_data;
        list->capacity = new_capacity;
    }

    list->data[list->count++] = id;
    return true;
}

static void id_list_free(dyn_id_list_t *list)
{
    if (list->data != NULL) {
        lv_free(list->data);
        list->data = NULL;
    }
    list->count = 0;
    list->capacity = 0;
}

/**
 * @brief 页面删除事件回调。
 *
 * @param e LVGL 删除事件
 */
static void page_delete_event_cb(lv_event_t *e)
{
    int32_t page_id = (int32_t)(intptr_t)lv_event_get_user_data(e);
    res_page_free(page_id); //释放页面资源
    gui_scr_t *scr = gui_scr_get(page_id);
    if (scr != NULL) {
        scr->scr = NULL;
        scr->is_act = false;
        gui_scr_del(scr);
    }
    /* 如果此次删除是为重载做准备则保留映射，否则清理映射 */
    dyn_path_id_t *node = dyn_runtime_id_find(page_id);
    if (node && node->preserve) {
        node->preserve = false;
    } else {
        dyn_runtime_id_remove(page_id);
    }
}

static int32_t dyn_runtime_id_alloc(void)
{
    static int32_t next_id = DYN_RUNTIME_ID_START;
    if (next_id == DYN_RUNTIME_ID_END) {
        next_id = DYN_RUNTIME_ID_START;
    }
    return next_id++;
}

static int32_t dyn_runtime_id_get(const char *path)
{
    dyn_path_id_t *node = lv_ll_get_head(&dyn_path_id_ll);
    while (node) {
        if (node->path != NULL && strcmp(node->path, path) == 0) {
            return node->runtime_id;
        }
        node = lv_ll_get_next(&dyn_path_id_ll, node);
    }

    int32_t runtime_id = dyn_runtime_id_alloc();
    dyn_path_id_t *new_node = lv_ll_ins_tail(&dyn_path_id_ll);
    if (new_node) {
        size_t len = strlen(path) + 1;
        new_node->path = lv_malloc(len);
        if (new_node->path == NULL) {
            lv_ll_remove(&dyn_path_id_ll, new_node);
            lv_free(new_node);
            return runtime_id;
        }
        lv_memcpy(new_node->path, path, len);
        new_node->runtime_id = runtime_id;
        new_node->preserve = false;
    }
    return runtime_id;
}

static dyn_path_id_t *dyn_runtime_id_find(int32_t runtime_id)
{
    dyn_path_id_t *node = lv_ll_get_head(&dyn_path_id_ll);
    while (node) {
        if (node->runtime_id == runtime_id) {
            return node;
        }
        node = lv_ll_get_next(&dyn_path_id_ll, node);
    }
    return NULL;
}

static void dyn_runtime_id_remove(int32_t runtime_id)
{
    dyn_path_id_t *node = lv_ll_get_head(&dyn_path_id_ll);
    while (node) {
        dyn_path_id_t *next = lv_ll_get_next(&dyn_path_id_ll, node);
        if (node->runtime_id == runtime_id) {
            if (node->path) {
                lv_free(node->path);
                node->path = NULL;
            }
            lv_ll_remove(&dyn_path_id_ll, node);
            lv_free(node);
            break;
        }
        node = next;
    }
}

static void log_dyn_error(dyn_err_t err, const char *path)
{
    if (err != DYN_OK) {
        LV_LOG_WARN("parse dyn failed(%d): %s", (int)err, path);
    }
}

#endif /*LV_USE_GUI_PARSE_DYN*/

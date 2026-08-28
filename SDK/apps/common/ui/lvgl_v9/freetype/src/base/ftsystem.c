/****************************************************************************
 *
 * ftsystem.c
 *
 *   ANSI-specific FreeType low-level system interface (body).
 *
 * Copyright (C) 1996-2026 by
 * David Turner, Robert Wilhelm, and Werner Lemberg.
 *
 * This file is part of the FreeType project, and may only be used,
 * modified, and distributed under the terms of the FreeType project
 * license, LICENSE.TXT.  By continuing to use, modify, or distribute
 * this file you indicate that you have read the license and
 * understand and accept it fully.
 *
 */

/**************************************************************************
 *
 * This file contains the default interface used by FreeType to access
 * low-level, i.e. memory management, i/o access as well as thread
 * synchronisation.  It can be replaced by user-specific routines if
 * necessary.
 *
 */


#include <ft2build.h>
#include FT_CONFIG_CONFIG_H
#include <freetype/internal/ftdebug.h>
#include <freetype/internal/ftstream.h>
#include <freetype/ftsystem.h>
#include <freetype/fterrors.h>
#include <freetype/fttypes.h>
#include "fs/fs.h"

#define USE_FS_ACCELERATE 1
#define LV_TTF_FILE_NAME_LEN  (15)
#define SEEK_BUF_LEN  4*1024

#if USE_FS_ACCELERATE
#include "utils/uthash/uthash.h"

// 每个打开的文件在 Hash 表中的节点
typedef struct _ttf_file_node {
    char filename[LV_TTF_FILE_NAME_LEN]; // 注意：确保Key唯一性
    FT_FILE  *file;                      // 共享的文件句柄
    unsigned long size;
    int ref_count;                       // 引用计数
    u8 *seek_buf;
    UT_hash_handle hh;
} ttf_file_node_t;

//每个Stream独有的上下文信息
//FILE* 是共享的，为每个Stream维护它自己认为的文件指针位置
typedef struct _stream_context {
    FT_FILE      *file;       // 指向共享的 file node
    unsigned long current_pos;// 当前这个 stream 逻辑上的文件偏移量
    ttf_file_node_t *node;    // 反向指向 hash 节点，方便 close 时查找
} stream_context_t;

static ttf_file_node_t *__file_handles = NULL;

#endif
/**************************************************************************
 *
 *                      MEMORY MANAGEMENT INTERFACE
 *
 */

/**************************************************************************
 *
 * It is not necessary to do any error checking for the
 * allocation-related functions.  This will be done by the higher level
 * routines like ft_mem_alloc() or ft_mem_realloc().
 *
 */


/**************************************************************************
 *
 * @Function:
 *   ft_alloc
 *
 * @Description:
 *   The memory allocation function.
 *
 * @Input:
 *   memory ::
 *     A pointer to the memory object.
 *
 *   size ::
 *     The requested size in bytes.
 *
 * @Return:
 *   The address of newly allocated block.
 */
FT_CALLBACK_DEF(void *)
ft_alloc(FT_Memory  memory,
         long       size)
{
    FT_UNUSED(memory);

    return ft_smalloc((size_t)size);
}


/**************************************************************************
 *
 * @Function:
 *   ft_realloc
 *
 * @Description:
 *   The memory reallocation function.
 *
 * @Input:
 *   memory ::
 *     A pointer to the memory object.
 *
 *   cur_size ::
 *     The current size of the allocated memory block.
 *
 *   new_size ::
 *     The newly requested size in bytes.
 *
 *   block ::
 *     The current address of the block in memory.
 *
 * @Return:
 *   The address of the reallocated memory block.
 */
FT_CALLBACK_DEF(void *)
ft_realloc(FT_Memory  memory,
           long       cur_size,
           long       new_size,
           void      *block)
{
    FT_UNUSED(memory);
    FT_UNUSED(cur_size);

    return ft_srealloc(block, (size_t)new_size);
}


/**************************************************************************
 *
 * @Function:
 *   ft_free
 *
 * @Description:
 *   The memory release function.
 *
 * @Input:
 *   memory ::
 *     A pointer to the memory object.
 *
 *   block ::
 *     The address of block in memory to be freed.
 */
FT_CALLBACK_DEF(void)
ft_free(FT_Memory  memory,
        void      *block)
{
    FT_UNUSED(memory);

    ft_sfree(block);
}


/**************************************************************************
 *
 *                    RESOURCE MANAGEMENT INTERFACE
 *
 */

#ifndef FT_CONFIG_OPTION_DISABLE_STREAM_SUPPORT

/**************************************************************************
 *
 * The macro FT_COMPONENT is used in trace mode.  It is an implicit
 * parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log
 * messages during execution.
 */
#undef  FT_COMPONENT
#define FT_COMPONENT  io

/* We use the macro STREAM_FILE for convenience to extract the       */
/* system-specific stream handle from a given FreeType stream object */
#define STREAM_FILE( stream )  ( (FT_FILE*)stream->descriptor.pointer )


/**************************************************************************
 *
 * @Function:
 *   ft_ansi_stream_close
 *
 * @Description:
 *   The function to close a stream.
 *
 * @Input:
 *   stream ::
 *     A pointer to the stream object.
 */
FT_CALLBACK_DEF(void)
ft_ansi_stream_close(FT_Stream  stream)
{

#if USE_FS_ACCELERATE
    stream_context_t *ctx = (stream_context_t *)stream->descriptor.pointer;
    if (!ctx) {
        return;
    }

    ttf_file_node_t *node = ctx->node;

    /*printf("ft_ansi_stream_close----ref_count before = %d\n", node->ref_count);*/

    // 1. 减少引用计数
    node->ref_count--;

    // 2. 释放 Stream 独有的上下文内存
    ft_sfree(ctx);
    stream->descriptor.pointer = NULL;

    // 3. 如果引用计数归零，才真正关闭文件并从 Hash 中删除
    if (node->ref_count <= 0) {
        /*printf("ft_ansi_stream_close----closing real file handle %p\n", node->file);*/
        ft_fclose(node->file);

        if (node->seek_buf) {
            ft_sfree(node->seek_buf);
        }
        // 从 Hash 表移除
        HASH_DEL(__file_handles, node);
        ft_sfree(node);
    } else {
        /*printf("ft_ansi_stream_close----keeping file handle open, ref_count = %d\n", node->ref_count);*/
    }

#else
    ft_fclose(STREAM_FILE(stream));
    stream->descriptor.pointer = NULL;
#endif

    stream->size               = 0;
    stream->base               = NULL;
}


/**************************************************************************
 *
 * @Function:
 *   ft_ansi_stream_io
 *
 * @Description:
 *   The function to open a stream.
 *
 * @Input:
 *   stream ::
 *     A pointer to the stream object.
 *
 *   offset ::
 *     The position in the data stream to start reading.
 *
 *   buffer ::
 *     The address of buffer to store the read data.
 *
 *   count ::
 *     The number of bytes to read from the stream.
 *
 * @Return:
 *   The number of bytes actually read.  If `count' is zero (that is,
 *   the function is used for seeking), a non-zero return value
 *   indicates an error.
 */
FT_CALLBACK_DEF(unsigned long)
ft_ansi_stream_io(FT_Stream       stream,
                  unsigned long   offset,
                  unsigned char  *buffer,
                  unsigned long   count)
{
#if USE_FS_ACCELERATE
    stream_context_t *ctx = (stream_context_t *)stream->descriptor.pointer;
    FT_FILE *file = ctx->file;

    // 因为文件句柄是共享的，其他 Stream 可能刚刚移动过文件指针。
    // 所以不能依赖 stream->pos 和 底层文件指针的一致性。
    // 必须强制 seek 到我们需要的 offset。
    if (count == 0) {
        // Seek only mode
        // 更新逻辑上的位置
        ctx->current_pos = offset;

        // 实际上也要移动底层指针，以便下一次 read 如果不 seek 也能对（虽然我们会每次都 seek）
        // 或者为了优化，可以检查 if (ctx->current_pos != offset)
        ft_fseek(file, (long)offset, SEEK_SET);
        return 0;
    }

    // Read mode
    // 1. 移动到底层文件的正确位置
    // 优化：如果 ctx->current_pos 已经是 offset，理论上可以跳过 fseek，
    // 为了绝对安全（防止外部干扰或 bug），始终fseek
    if (ctx->current_pos != offset) {
        ft_fseek(file, (long)offset, SEEK_SET);
        ctx->current_pos = offset;
    }

    // 2. 读取数据
    unsigned long bytes_read = (unsigned long)ft_fread(buffer, 1, count, file);

    // 3. 更新逻辑位置
    ctx->current_pos += bytes_read;

    return bytes_read;

#else

    FT_FILE  *file = STREAM_FILE(stream);

    if (offset > stream->size && !count) {
        return 1;
    }

    if (stream->pos != offset) {
        ft_fseek(file, (long)offset, SEEK_SET);
    }

    if (!count) {
        return 0;
    }

    return (unsigned long)ft_fread(buffer, 1, count, file);
#endif
}





/* documentation is in ftstream.h */
FT_BASE_DEF(FT_Error)
FT_Stream_Open(FT_Stream    stream,
               const char  *filepathname)
{
    FT_FILE  *file;

    /*printf("-----filepathname = %s\n",filepathname);*/

    if (!stream) {
        return FT_THROW(Invalid_Stream_Handle);
    }

    stream->descriptor.pointer = NULL;
    stream->pathname.pointer   = (char *)filepathname;
    stream->base               = NULL;
    stream->pos                = 0;
    stream->read               = NULL;
    stream->close              = NULL;

#if USE_FS_ACCELERATE
    ttf_file_node_t *p = NULL;
    int name_len = strlen(filepathname);

    if (name_len < LV_TTF_FILE_NAME_LEN) {
        // 处理短文件名情况，或者直接 fallback 到普通打开
        goto open_new_file;
    }

    const char *key_ptr = &filepathname[name_len - LV_TTF_FILE_NAME_LEN];

    // 查找 Hash
    HASH_FIND_STR(__file_handles, key_ptr, p);

    if (p) {

        /*printf("\n[cache hit] %s\n", filepathname);*/

        // 1. 增加引用计数
        p->ref_count++;

        // 2. 分配 Stream 独有的上下文
        stream_context_t *ctx = (stream_context_t *)ft_smalloc(sizeof(stream_context_t));
        if (!ctx) {
            p->ref_count--; // 回滚
            return FT_THROW(Out_Of_Memory);
        }

        ctx->file = p->file;
        ctx->current_pos = 0; // 新打开的 stream 初始位置为 0
        ctx->node = p;

        // 3. 设置 Stream 属性
        stream->size = p->size;
        stream->descriptor.pointer = ctx; // 存入自定义上下文，而不是直接的 FILE*
        stream->read  = ft_ansi_stream_io;
        stream->close = ft_ansi_stream_close;

        return FT_Err_Ok;
    }

open_new_file:
#endif

    //文件未打开，全新打开
#ifdef CONFIG_JLFAT_ENABLE
    char path[128];
    path[long_file_name_encode(filepathname, (unsigned char *)path, sizeof(path))] = '\0';
    file = ft_fopen(path, "r");
#else
    file = ft_fopen(filepathname, "r");
#endif

    if (!file) {
        FT_ERROR(("FT_Stream_Open: could not open `%s'\n", filepathname));
        return FT_THROW(Cannot_Open_Resource);
    }

    // 获取文件大小
    /*unsigned long fsize = (unsigned long)ft_ftell(file); // 注意：ft_ftell 行为依赖具体实现，通常需要 fseek(END) 再 ftell*/
    // 修正文件大小获取逻辑：
    /*ft_fseek(file, 0, SEEK_END);*/
    /*fsize = (unsigned long)ft_ftell(file);*/
    /*ft_fseek(file, 0, SEEK_SET); // 重置到开头*/

    //使用flen获取长度
    unsigned long fsize = (unsigned long)flen(file);

    if (!fsize) {
        FT_ERROR(("FT_Stream_Open: opened `%s' but zero-sized\n", filepathname));
        ft_fclose(file);
        return FT_THROW(Cannot_Open_Stream);
    }

#if USE_FS_ACCELERATE
    // 1. 创建 Hash 节点
    p = (ttf_file_node_t *)ft_smalloc(sizeof(ttf_file_node_t));
    if (!p) {
        ft_fclose(file);
        return FT_THROW(Out_Of_Memory);
    }

    p->file = file;
    p->size = fsize;
    p->ref_count = 1; // 初始引用为 1
    strcpy(p->filename, key_ptr);

    //fseek加速:
    p->seek_buf = ft_smalloc(SEEK_BUF_LEN);
    if (p->seek_buf) {
        fsave_fat_table(file, SEEK_BUF_LEN, p->seek_buf);
    }

    HASH_ADD_KEYPTR(hh, __file_handles, p->filename, strlen(p->filename), p);

    // 2. 创建 Stream 独有上下文
    stream_context_t *ctx = (stream_context_t *)ft_smalloc(sizeof(stream_context_t));
    if (!ctx) {
        // 清理 hash 和 file
        HASH_DEL(__file_handles, p);
        ft_sfree(p);
        ft_fclose(file);
        return FT_THROW(Out_Of_Memory);
    }

    ctx->file = file;
    ctx->current_pos = 0;
    ctx->node = p;

    stream->size = fsize;
    stream->descriptor.pointer = ctx;
#else
    stream->size = fsize;
    stream->descriptor.pointer = file;
#endif

    stream->read  = ft_ansi_stream_io;
    stream->close = ft_ansi_stream_close;

    FT_TRACE1(("FT_Stream_Open: opened `%s' (%ld bytes) successfully\n", filepathname, stream->size));
    return FT_Err_Ok;
}

#endif /* !FT_CONFIG_OPTION_DISABLE_STREAM_SUPPORT */

#ifdef FT_DEBUG_MEMORY

extern FT_Int
ft_mem_debug_init(FT_Memory  memory);

extern void
ft_mem_debug_done(FT_Memory  memory);

#endif


/* documentation is in ftobjs.h */

FT_BASE_DEF(FT_Memory)
FT_New_Memory(void)
{
    FT_Memory  memory;


    memory = (FT_Memory)ft_smalloc(sizeof(*memory));
    if (memory) {
        memory->user    = NULL;
        memory->alloc   = ft_alloc;
        memory->realloc = ft_realloc;
        memory->free    = ft_free;
#ifdef FT_DEBUG_MEMORY
        ft_mem_debug_init(memory);
#endif
    }

    return memory;
}


/* documentation is in ftobjs.h */

FT_BASE_DEF(void)
FT_Done_Memory(FT_Memory  memory)
{
#ifdef FT_DEBUG_MEMORY
    ft_mem_debug_done(memory);
#endif
    ft_sfree(memory);
}


/* END */

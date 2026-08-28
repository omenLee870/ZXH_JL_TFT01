#include "asm/cpu.h"
#include "asm/power_interface.h"
#include "system/includes.h"
#include "os/os_api.h"
#include "power/p11_cbuf.h"

#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".p11_cbuf.data.bss")
#pragma data_seg(".p11_cbuf.data")
#pragma const_seg(".p11_cbuf.text.const")
#pragma code_seg(".p11_cbuf.text")
#endif

static spinlock_t cbuf_lock;

#define     p11_cbuf_spin_lock()        spin_lock(&cbuf_lock)
#define     p11_cbuf_spin_unlock()      spin_unlock(&cbuf_lock)



#define CBUF_ENTER_CRITICAL()                   \
    do{                                         \
        p11_cbuf_spin_lock();                   \
        ipc_spin_lock(IPC_SPIN_LOCK_EVENT_CBUF);\
    }while(0)



#define CBUF_EXIT_CRITICAL()                      \
    do{                                           \
        ipc_spin_unlock(IPC_SPIN_LOCK_EVENT_CBUF);\
        p11_cbuf_spin_unlock();                   \
    }while(0)



/* --------------------------------------------------------------------------*/
/**
 * @brief 适用范围:全局
 * @brief cbuffer 通道开关
 *
 * @param [in] cbuffer cbuffer 句柄
 * @param [in] index 通道索引
 * @param [in] enable 通道开关
 */
/* ----------------------------------------------------------------------------*/
void p11cbuf_mult_entry_enable(p11_cbuffer_t *cbuffer, int index, int enable)
{
    if (!cbuffer) {
        return;
    }

    cbuffer = (p11_cbuffer_t *)(((int)(cbuffer)) + P11_RAM_BASE);

    if ((!cbuffer->child_count) || index >= cbuffer->child_count) {
        return;
    }

    if (!cbuffer->child) {
        return;
    }


    cbuffer_child_t *child;
    child = (cbuffer_child_t *)((int)cbuffer->child + P11_RAM_BASE);


    CBUF_ENTER_CRITICAL();
    if (!enable) { //关操作时候需要释放空间
        child[index].en  = !!enable;
        child[index].offset  = 0;
        u32 min_offset = (u32) - 1;
        for (int i = 0; i < cbuffer->child_count; i++) {
            if (child[i].en && child[i].offset < min_offset) {
                min_offset = child[i].offset;
            }
        }

        for (int i = 0; i < cbuffer->child_count; i++) {
            if (child[i].en) {
                child[i].offset -= min_offset;
            }
        }

        if (min_offset > cbuffer->data_len) {
            min_offset = cbuffer->data_len;
        }


        cbuffer->tmp_len =  cbuffer->data_len -= min_offset;
        cbuffer->read_ptr += min_offset;

        if ((u32)cbuffer->read_ptr >= (u32)cbuffer->end) {
            cbuffer->read_ptr = (u8 *)cbuffer->begin + ((u32)cbuffer->read_ptr - (u32)cbuffer->end);
        }
    } else {
        child[index].en  = !!enable;
        child[index].offset  = 0;
    }
    CBUF_EXIT_CRITICAL();
}


/* --------------------------------------------------------------------------*/
/**
 * @brief 适用范围:全局
 * @brief 获取存储数据的字节长度
 *
 * @param [in] cbuffer cbuffer 句柄
 *
 * @return 存储数据的字节长度
 */
/* ----------------------------------------------------------------------------*/
u32 p11_cbuf_get_data_len(p11_cbuffer_t *cbuffer)
{
    if (!cbuffer) {
        return 0;
    }
    cbuffer = (p11_cbuffer_t *)(((int)(cbuffer)) + P11_RAM_BASE);
    return cbuffer->data_len;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 适用范围:全局
 * @brief 获取存储数据的字节长度
 *
 * @param [in] cbuffer cbuffer 句柄
 * @param [in] index 当前通道的索引
 * @return 存储数据的字节长度
 */
/* ----------------------------------------------------------------------------*/
u32 p11_cbuf_mult_read_get_data_len(p11_cbuffer_t *cbuffer, int index)
{

    u32 len;
    if (!cbuffer) {
        return 0;
    }

    cbuffer = (p11_cbuffer_t *)(((int)(cbuffer)) + P11_RAM_BASE);

    if (index >= cbuffer->child_count) {
        return 0;
    }

    CBUF_ENTER_CRITICAL();
    cbuffer_child_t *child;
    child = (cbuffer_child_t *)((int)cbuffer->child + P11_RAM_BASE);

    if (child[index].en) {
        len = cbuffer->data_len - child[index].offset;
    } else {
        len = 0;
    }
    CBUF_EXIT_CRITICAL();
    return len;
}


/* --------------------------------------------------------------------------*/
/**
 * @brief 适用范围:memcpy管理+cbuf_read_alloc系列管理函数
 * @brief 预分配待读取数据的空间,并把读取到的数据存入buf数组
 *
 * @param [in] cbuffer cbuffer 句柄
 * @param [in] index 当前通道的索引
 * @param [out] buf 存储读取的数据的目标buf数组
 * @param [in] len 要读取的数据的字节长度
 *
 * @return
 */
/* ----------------------------------------------------------------------------*/
u32 p11_cbuf_mult_read_alloc_len(p11_cbuffer_t *cbuffer, int index, void *buf, u32 len)
{
    u32 r_len = len;
    u32 copy_len;
    u8 *read_ptr;

    if (!cbuffer) {
        return 0;
    }

    if (buf == NULL) {
        printf("ERROR :%s %d buff is NULL!!!\n", __func__, __LINE__);
        return 0;
    }

    cbuffer = (p11_cbuffer_t *)(((int)(cbuffer)) + P11_RAM_BASE);

    if ((!cbuffer->child_count) || index >= cbuffer->child_count) {
        return 0;
    }


    if (!cbuffer->child) {
        return 0;
    }

    CBUF_ENTER_CRITICAL();
    cbuffer_child_t *child;
    child = (cbuffer_child_t *)((int)cbuffer->child + P11_RAM_BASE);

    if (!child[index].en) {
        CBUF_EXIT_CRITICAL();
        return 0;
    }


    read_ptr =  cbuffer->read_ptr + child[index].offset;

    if ((u32) read_ptr >= (u32)cbuffer->end) {
        read_ptr = (u8 *)cbuffer->begin + ((u32)read_ptr - (u32)cbuffer->end);
    }

    if (cbuffer->data_len - child[index].offset < len) {
        memset(buf, 0, len);
        CBUF_EXIT_CRITICAL();
        return 0;
    }

    copy_len = (u32)cbuffer->end - (u32)read_ptr;

    if (copy_len > len) {
        copy_len = len;
    }
    len -= copy_len;

    memcpy(buf, read_ptr + P11_RAM_BASE, copy_len);
    if (len == 0) {
        /* cbuffer->read_ptr += copy_len; */
    } else {
        memcpy((u8 *)buf + copy_len, cbuffer->begin + P11_RAM_BASE, len);
    }
    CBUF_EXIT_CRITICAL();
    return r_len;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 适用范围:memcpy管理+cbuf_read_alloc系列管理函数
 * @brief 更新cbuf管理handle的读指针位置和数据长度
 *
 * @param [in] cbuffer cbuffer 句柄
 * @param [in] index 当前通道的索引
 * @param [in] len 要更新的数据的字节长度
 */
/* ----------------------------------------------------------------------------*/
void p11_cbuf_mult_read_alloc_len_updata(p11_cbuffer_t *cbuffer, int index, u32 len)
{
    if (!cbuffer) {
        return ;
    }

    cbuffer = (p11_cbuffer_t *)(((int)(cbuffer)) + P11_RAM_BASE);


    if ((!cbuffer->child_count) || index >= cbuffer->child_count) {
        return ;
    }



    if (!cbuffer->child) {
        return ;
    }

    CBUF_ENTER_CRITICAL();

    cbuffer_child_t *child;
    child = (cbuffer_child_t *)((int)cbuffer->child + P11_RAM_BASE);

    if (!child[index].en) {
        CBUF_EXIT_CRITICAL();
        return;
    }


    if (len + child[index].offset > cbuffer->data_len) {
        len = cbuffer->data_len - child[index].offset;
    }

    child[index].offset += len;
    u32 min_offset = (u32) - 1;

    for (int i = 0; i < cbuffer->child_count; i++) {
        if (child[i].en && child[i].offset < min_offset) {
            min_offset = child[i].offset;
        }
    }
    for (int i = 0; i < cbuffer->child_count; i++) {
        if (child[i].en) {
            child[i].offset -= min_offset;
        }
    }

    if (min_offset > cbuffer->data_len) {
        ASSERT(0);
        /* min_offset = cbuffer->data_len; */
    }


    cbuffer->tmp_len =  cbuffer->data_len -= min_offset;
    cbuffer->read_ptr += min_offset;


    if ((u32)cbuffer->read_ptr >= (u32)cbuffer->end) {
        cbuffer->read_ptr = (u8 *)cbuffer->begin + ((u32)cbuffer->read_ptr - (u32)cbuffer->end);
    }


    CBUF_EXIT_CRITICAL();
}


#define P11_CBUF_TEST  0

#include "p11_app_msg.h"

#if P11_CBUF_TEST


static p11_cbuffer_t *cbuffer;
static void master_read_p11(void *p)
{
    static int index_1;
    static int index_2;
    static int index_3;

    u8 test[50];
    int len = p11_cbuf_mult_read_alloc_len(cbuffer, 0, test, 10);
    if (len) {
        p11_cbuf_mult_read_alloc_len_updata(cbuffer, 0, len);
        printf("index 0 read len %d ,left %d,total %d\n", len, p11_cbuf_mult_read_get_data_len(cbuffer, 0), p11_cbuf_get_data_len(cbuffer));
        put_buf(test, len);
        index_1 += len;
        if (index_1 == 50) {
            p11cbuf_mult_entry_enable(cbuffer, 1, 0);
            printf("stop index 1\n");
        }
        if (index_1 == 100) {
            p11cbuf_mult_entry_enable(cbuffer, 1, 1);
            printf("open index 1\n");
        }
    }


    printf("%s %d %x\n", __func__, __LINE__, (int)cbuffer);

    len =   p11_cbuf_mult_read_alloc_len(cbuffer, 1, test, 25);
    if (len) {
        p11_cbuf_mult_read_alloc_len_updata(cbuffer, 1, len);
        printf("index 1 read len %d ,left %d,total %d\n", len, p11_cbuf_mult_read_get_data_len(cbuffer, 1), p11_cbuf_get_data_len(cbuffer));
        put_buf(test, len);

    }

    len = p11_cbuf_mult_read_alloc_len(cbuffer, 2, test, 5);
    if (len) {
        p11_cbuf_mult_read_alloc_len_updata(cbuffer, 2, len);
        printf("index 2 read len %d ,left %d,total %d\n", len, p11_cbuf_mult_read_get_data_len(cbuffer, 2), p11_cbuf_get_data_len(cbuffer));
        put_buf(test, len);
    }

    task_post_msg2p11(NULL, 1, MSG_P11_SYS_KICK);

}










static void test_handler(void *priv, u8 *buf, u32 len)
{
    if (buf == NULL) {
        printf("ERROR :%s %d buff is NULL!!!\n", __func__, __LINE__);
        return;
    }
    int msg[16];
    printf("msg: %d, len: %d\n", msg[0], len);
    ASSERT(len < sizeof(msg));
    memcpy(msg, buf, len);
    switch (msg[0]) {
    case MSG_P11_SYS_RAM_INIT:
        printf("MSG_P11_SYS_RAM_INIT %x\n", msg[1]);
        cbuffer = (p11_cbuffer_t *)msg[1];
        sys_timer_add(NULL, master_read_p11, 1000);
        break;
    }
}

REGISTER_P2M_MSG_HANDLER(NULL, MSG_APP, test_handler);


#endif

#ifndef __P11_CBUF__H__
#define __P11_CBUF__H__

/* --------------------------------------------------------------------------*/
/**
 * @brief cbuffer 私有成员结构体
 */
/* ----------------------------------------------------------------------------*/
typedef struct _cbuffer_child {
    u16 en    : 1;
    u16 offset: 15;
} cbuffer_child_t;



/* --------------------------------------------------------------------------*/
/**
 * @brief cbuffer结构体
 */
/* ----------------------------------------------------------------------------*/
typedef struct _p11_cbuffer {
    u8  *begin;
    u8  *end;
    u8  *read_ptr;
    u8  *write_ptr;
    u8  *tmp_ptr ;
    u16 tmp_len;
    u16 data_len;
    u16 total_len;
    u8  child_count;
    cbuffer_child_t *child;
} p11_cbuffer_t;


void p11cbuf_mult_entry_enable(p11_cbuffer_t *cbuffer, int index, int enable);
u32 p11_cbuf_get_data_len(p11_cbuffer_t *cbuffer);
u32 p11_cbuf_mult_read_get_data_len(p11_cbuffer_t *cbuffer, int index);
u32 p11_cbuf_mult_read_alloc_len(p11_cbuffer_t *cbuffer, int index, void *buf, u32 len);
void p11_cbuf_mult_read_alloc_len_updata(p11_cbuffer_t *cbuffer, int index, u32 len);



#endif

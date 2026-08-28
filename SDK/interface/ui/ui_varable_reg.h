#ifndef _UI_VARABLE_REG_H_
#define _UI_VARABLE_REG_H_

#include "typedef.h"


struct ui_varable_reg_t {
    u32 begin_addr;
    u8 len;
    u16 msg_id;
    void (*set_value_cb)(void);
};


//UI变量寄存器仅支持 byte、半word、word 3种变量类型
#define DEFINE_UI_VARIABLE_REG(MSG_ID,VARIABLE_TYPE,VARIABLE_NAME,SET_VALUE_CALLBACK) \
extern u32 ui_varable_reg_begin_##MSG_ID,ui_varable_reg_end_##MSG_ID;\
SEC_USED(.ui_varable_reg_tab)  static struct ui_varable_reg_t varable_reg_##MSG_ID = \
{\
.begin_addr =(u32)&ui_varable_reg_begin_##MSG_ID,\
.len=sizeof(VARIABLE_TYPE),\
.msg_id=MSG_ID,\
.set_value_cb=SET_VALUE_CALLBACK,\
};\
__attribute__((section(".ui_varable_reg_"#MSG_ID),used))  volatile VARIABLE_TYPE VARIABLE_NAME


extern void ui_set_varable_reg_value(u16 id, void *ptr, u32 len);

#endif

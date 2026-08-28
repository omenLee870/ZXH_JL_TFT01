#define  REGISTER_UI_VARIABLE_REG(MSG_ID) \
		    . = ALIGN(4);\
            PROVIDE(ui_varable_reg_begin_##MSG_ID = .); \
            KEEP(*(.ui_varable_reg_##MSG_ID)) \
            PROVIDE(ui_varable_reg_end_##MSG_ID = .);




PROVIDE(ui_varable_reg_begin = .);

//根据实际变量寄存器的类型对齐，连续对齐保持可节省内存空间
REGISTER_UI_VARIABLE_REG(10)
REGISTER_UI_VARIABLE_REG(11)
REGISTER_UI_VARIABLE_REG(12)


PROVIDE(ui_varable_reg_end = .);


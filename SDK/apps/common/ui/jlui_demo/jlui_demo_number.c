#include "jlui_demo_cfg.h"

#if (defined JLUI_DEMO_NUMBER_ENABLE && JLUI_DEMO_NUMBER_ENABLE)

#include "app_config.h"
#include "system/includes.h"
#include "ui_api.h"
#include "ui.h"
#include "typedef.h"
#include "app_task.h"

#define JLUI_DEMO_PAGE_ID		PAGE_5
#define JLUI_DEMO_NUMBER_ID		0xA3C00

#define MIN_NUMBER	0
#define MAX_NUMBER	10
static int cur_num = 0;

int jlui_demo_number()
{
    JLUI_DEMO_SWITCH(JLUI_DEMO_PAGE_ID);
    struct ui_number *num = (struct ui_number *)ui_core_get_element_by_id(JLUI_DEMO_NUMBER_ID);
    if (num) {
        struct unumber unum;
        unum.type = TYPE_NUM;
        unum.numbs = 1;
        unum.number[0] = cur_num;
        unum.number[1] = cur_num;
        ui_number_update(num, &unum);
    }
    cur_num ++;
    printf("cur_num: %d\n", cur_num);

    JLUI_DEMO_REFRESH();

    if (cur_num > MAX_NUMBER) {
        cur_num = 0;
        return 0;
    } else {
        return 1 * 1000;
    }
}

#else

int jlui_demo_number()
{
    return 0;
}

#endif

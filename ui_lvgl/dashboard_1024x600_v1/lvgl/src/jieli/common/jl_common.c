/**
 * @file jl_common.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "jl_common.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void jl_common_init(void)
{
#if LV_USE_JL_GUI_COMP
#if LV_USE_GUI_SCR
    gui_scr_init();
#if LV_USE_GUI_SCR_STACK
    gui_scr_stack_init();
#endif
#endif
#endif
#if LV_USE_GUI_PARSE_DYN
    gui_dyn_init();
#endif
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

#include "app_config.h"

// *INDENT-OFF*

#if CONFIG_LVGL_UI_ENABLE 

c_SRC_FILES += \
    $(shell find apps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/custom -name '*.c' -print) \
    $(shell find apps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/generated -name '*.c' -print) \
    apps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/ui_action.c
	

#endif

// *INDENT-OFF*
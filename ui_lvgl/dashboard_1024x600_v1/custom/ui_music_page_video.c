#if !LV_USE_GUIBUILDER_SIMULATOR
#include "app_msg.h"
#include "lv_ui.h"
#include "app_config.h"
#include "fs/fs.h"
#include "avctp_user.h"
#include "custom.h"
#include "camera_display_ui.h"

//注册页面加载卸载回调
static int gui_src_action_page_video(int action)
{
    printf("--->%s()----->%d, action: %d\n", __func__, __LINE__, action);

    switch (action) {
    case GUI_SCREEN_ACTION_LOAD:
#if TCFG_CAMERA_DVP_ENABLE && TCFG_VIDEO0_ENABLE
        camera_display_ui_init(1024, 600, 0, 0);
#endif
        break;
    case GUI_SCREEN_ACTION_UNLOAD:
#if TCFG_CAMERA_DVP_ENABLE && TCFG_VIDEO0_ENABLE
        camera_display_ui_deinit();
#endif
        break;
    }
    return 0;
}
REGISTER_UI_SCREEN_ACTION_HANDLER(GUI_SCREEN_PAGE_VIDEO)
.onchange = gui_src_action_page_video,
};
#endif

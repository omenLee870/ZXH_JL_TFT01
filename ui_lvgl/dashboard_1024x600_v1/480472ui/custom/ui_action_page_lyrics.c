#include "custom.h"
#include "lvgl.h"
#include "gui_guider.h"

#if !LV_USE_GUIBUILDER_SIMULATOR
#include "app_core.h"
#include "app_config.h"
#include "action.h"
#include "ui.h"
#include "fs/fs.h"
#endif

#if !LV_USE_GUIBUILDER_SIMULATOR

#if 0

#define CONFIG_AVI_PATH "storage/sd0/C/avi/moto.avi"
#define CONFIG_FONT_TTF_PATH "storage/sd0/C/ui/lyrics.ttf"


/*

 * @file lv_example_lyrics.c

 *

 * 演示了如何调用 lv_lyrics 控件和蓝牙播歌实现简单的歌词播放 demo

 */

#include "../../lv_examples.h"

#if LV_USE_LYRICS

#include "os/os_api.h"
#include "lvgl.h"

#define CONFIG_UI_RES_PATH CONFIG_ROOT_PATH // SD 路径


#define LYRICS_LETTER_NUM_MAX 256

static char lyrics_example_text[LYRICS_LETTER_NUM_MAX] = "  ";

static lv_obj_t *curr_obj_0;

static lv_obj_t *curr_obj_1;

static char *test_text = "龘12345bjg";

static int lyrics_path_init_flag = 0;

static void lyrics_example_play_anim_cb(void *obj, int32_t v)
{
    lv_lyrics_info *lyrics_info = (lv_lyrics_info *)(((lv_obj_t *)obj)->user_data);
    //lv_lyrics_set_pos(obj, v, v);

    //lv_lyrics_set_zoom(obj, 128, 128);

    //lv_lyrics_set_zoom(obj, 256 + 128, 256 + 128);
    lv_lyrics_set_zoom(obj, v, v);
    lv_lyrics_fontimg_redarw(obj);
}

void lyrics_example_clean(void);

static void lv_example_lyrics_letter( lv_obj_t *dest_scr, const char *text, uint16_t font_size, const char *font_file)
{
    lyrics_example_clean();
    size_t text_len = strlen(text);
    uint32_t *letter_buf = (uint32_t *)lv_mem_alloc(sizeof(uint32_t) * text_len);    // 空间大小最大不会超过 text_len * 4
    uint16_t letter_num = 0;
    // 解析字符串中的每一个 unicode

    int ofs = 0;

    while (ofs < text_len) {
        uint32_t letter;
        uint32_t letter_next;
        printf("[chili] %s %d\n", __func__, __LINE__);
        _lv_txt_encoded_letter_next_2(text, &letter, &letter_next, &ofs);
        printf("ofs = %d; letter = 0x%08x; letter_next = 0x%08x.", ofs, letter, letter_next);
        letter_buf[letter_num] = letter;
        printf("info.letter_buf[%d] = 0x%08x.", letter_num, letter_buf[letter_num]);
        letter_num++;
        assert(letter_num < LYRICS_LETTER_NUM_MAX);   // 歌词字符数量过大，有需求再修改
    }

    printf("letter_num = %d.", letter_num);
    uint16_t lyrics_0_len = (letter_num / 2);

    curr_obj_0 = lv_lyrics_create(dest_scr, font_file, font_size,0,letter_buf, lyrics_0_len);
    if (curr_obj_0 == NULL) {
        printf("lv_lyrics_create is fail!!");
        goto example_error0;
    }

    jlvg_color_t color = {0xff,0xff,0xff,0xff};
    lv_lyrics_set_color(curr_obj_0, color);
    lv_obj_clear_flag(curr_obj_0, LV_OBJ_FLAG_CLICKABLE);

    lv_lyrics_set_pos(curr_obj_0, 450, 300);

    //lv_lyrics_set_rotation(curr_obj_0, -450, 0, 0);
    curr_obj_1 = lv_lyrics_create(dest_scr, font_file, font_size ,0,letter_buf + lyrics_0_len, (letter_num - lyrics_0_len));

    if (curr_obj_1 == NULL) {
        printf("lv_lyrics_create is fail!!");
        goto example_error1;
    }

    lv_lyrics_set_color(curr_obj_1, color);

    lv_lyrics_set_pos(curr_obj_1, 450, 350);
    lv_obj_clear_flag(curr_obj_1, LV_OBJ_FLAG_CLICKABLE);

    lv_anim_t lyrics_example_play_anim0;
    lv_anim_init(&lyrics_example_play_anim0);  // 初时化动画变量
    lv_anim_set_var(&lyrics_example_play_anim0, curr_obj_0); //设置动画关联的对象img
    lv_anim_set_exec_cb(&lyrics_example_play_anim0, lyrics_example_play_anim_cb); //设置动画执行的回调函数set_angle
    lv_anim_set_values(&lyrics_example_play_anim0, 128, 256);
    lv_anim_set_time(&lyrics_example_play_anim0, 3000);
    lv_anim_set_playback_time(&lyrics_example_play_anim0, 2000);
    lv_anim_set_repeat_count(&lyrics_example_play_anim0, LV_ANIM_REPEAT_INFINITE);//设置动画重复次数，这里是无穷次
    lv_anim_start(&lyrics_example_play_anim0); //启动动画播放

    lv_anim_t lyrics_example_play_anim1;
    lv_anim_init(&lyrics_example_play_anim1);  // 初时化动画变量
    lv_anim_set_var(&lyrics_example_play_anim1, curr_obj_1); //设置动画关联的对象img
    lv_anim_set_exec_cb(&lyrics_example_play_anim1, lyrics_example_play_anim_cb); //设置动画执行的回调函数set_angle
    lv_anim_set_values(&lyrics_example_play_anim1, 128, 256);
    lv_anim_set_time(&lyrics_example_play_anim1, 3000);
    lv_anim_set_playback_time(&lyrics_example_play_anim1, 2000);
    lv_anim_set_repeat_count(&lyrics_example_play_anim1, LV_ANIM_REPEAT_INFINITE);//设置动画重复次数，这里是无穷次
    lv_anim_start(&lyrics_example_play_anim1); //启动动画播放

    lv_mem_free(letter_buf);
    return;

example_error1:

    lv_lyrics_destructor(curr_obj_0);

example_error0:

    lv_mem_free(letter_buf);

}



//释放歌词

void lyrics_example_clean(void)

{

     if (curr_obj_0 != NULL) {
        lv_anim_del(curr_obj_0,NULL);
        lv_lyrics_destructor(curr_obj_0);
        curr_obj_0 = NULL;
    }



     if (curr_obj_1 != NULL) {
        lv_anim_del(curr_obj_1,NULL);
        lv_lyrics_destructor(curr_obj_1);
        curr_obj_1 = NULL;
     }

}



//更新歌词

void lv_example_lyrics_text_input(char *new_text)

{

    if(!lyrics_path_init_flag){
        FILE *fd = fopen(CONFIG_FONT_TTF_PATH, "r");
        if (fd) {
            fclose(fd);
            lyrics_path_init_flag = 1;
        }else{
            printf("open lyrics path error!!!!!\n");
            return;
        }
    }


    size_t text_len = strlen(new_text);
    if (text_len > LYRICS_LETTER_NUM_MAX) {

        printf("error: new_text is too len. len = %d.", text_len);

        return;

    }

    printf("[%s] new lyrics is [%s]", __func__, new_text);
    memcpy(lyrics_example_text, new_text, text_len);
    lyrics_example_text[text_len] = '\0';
    lv_example_lyrics_letter(lv_layer_top(),lyrics_example_text, 48, CONFIG_FONT_TTF_PATH);
}



//调用示例

//在lvgl以外线程使用lvgl_rpc_post_func(),歌词创建在lv_layer_top()页面,test_text为输入歌词
//lvgl_rpc_post_func(lv_example_lyrics_text_input, 2, lv_layer_top() , test_text);


#endif

#endif

#endif
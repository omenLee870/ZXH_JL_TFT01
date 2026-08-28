#ifndef __RUBIKS_CUBE_H__
#define __RUBIKS_CUBE_H__

#ifdef __cplusplus
extern "C" {
#endif

struct rubiks_cube_screen_info {
    int screen_width;    // 屏幕宽
    int screen_height;   // 屏幕高
    int side;            // 边长
    int gamepad;         // 控制面旋转幅度的量
};

int rubiks_cube_init(int width, int height, void *cfg_task, void *draw_cube);     //初始化魔方游戏
int rubiks_cube_uninit(void);   //退出魔方游戏
void restore_cube(void);        //复原魔方
void OnMove(int x, int y);      //输入设备MOVE事件中调用
void OnMouseUp(int x, int y);   //输入设备UP事件中调用
void OnMouseDown(int x, int y);
int is_cube_restored(void);     //1:复原完成
void random_cube(void);         //随机打乱魔方

void draw_quadrilateral(float points[8], uint8_t color[4]);
void display_cube(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif


#endif // __RUBIKS_CUBE_H__

# 倒车影像卡死定位日志

`lv_video_diag.h` 中 `LV_VIDEO_DIAG_ENABLE=1` 默认开启。fb5 视频控件创建后
开始输出，删除后停止输出。定位完成后改为 `0` 可关闭。

每 2 秒由 app_core 的系统定时器打印 `[VIDEO_DIAG]`。逐帧路径只更新状态，
不会逐帧打印，也没有新增锁或等待。UI 或 GPU 任务阻塞时，app_core 仍可采样；
如果 app_core、调度或串口也阻塞，则采样日志也可能停止。

字段：`seq` 是该 lane 的状态更新次数；`age_ms` 是当前状态持续时间；
`wait=1` 表示正在执行/等待；`detail` 为 fb 编号或 ioctl/post 返回值。
正在执行/等待超过 1 秒时标为 `STALL`。快照并非原子采样，需比较连续多次输出，
不能把单条 `STALL` 直接当成根因。`wait=0` 的状态持续很久可以表示正常空闲。

| lane / stage | 所在位置 |
| --- | --- |
| ui / flush_sem | UI 等待上一帧推屏信号量；同时查看 push 的状态 |
| ui / render_start、render_draw | 更新视频源或 LVGL 收集/渲染绘制任务 |
| ui / flush_build、flush_post | 构建链表或投递推屏任务 |
| push / gpu_gate | 等待 GPU 推屏独占门 |
| push / gpu_run | GPU 任务执行 |
| push / lcd_wait_before、lcd_wait_after | 等待 LCD 传输完成；结合 lcd_busy 查看 |
| push / lcd_area、lcd_draw | 设置 LCD 区域或提交画面 |
| push / task_free、push_sem_post | 释放任务链或归还完成信号量 |
| frame / fb_get、fb_put | 取出或归还视频帧 |
| frame / fb_no_frame | 当前没有可用帧；结合 seq 是否继续增长判断 |
| frame / frame_set_src、image_set_src | 更新视频图像描述或设置 LVGL 图像源 |
| frame / delete_objects、fb_close | 删除视频控件或关闭 fb |
| drain / drain_sem、drain_lcd | 等待异步渲染排空或 LCD 完成 |

复现时从开机开始保存串口日志，反复进入/退出倒车页。卡住后保留至少 10 秒日志，
再复位。记录是否只有影像停止、是否切屏键也失效，以及重启是否手动触发。
本改动还把取帧空指针检查移到解引用之前，防止无帧时访问空指针；没有改动
现有显示时序、等待时长或恢复策略。需要重新编译、烧录后才会看到新日志。

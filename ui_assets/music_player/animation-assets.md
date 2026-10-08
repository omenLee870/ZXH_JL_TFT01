# 音乐动画素材

JieLiUI 的资源目录为 `ui_lvgl/dashboard_1024x600_v1/import/image/music/`。

- `dog_headphones_clean.png`：216×142，去除固定音符的小狗，使用内置 imagegen 编辑透明 PNG。
- `music_note_right.png` / `music_note_left.png`：24×32，两个相反朝向的独立青色音符；SVG 源文件在 `source/`。
- `die_display.png`：原 `die.png` 等比例缩小至 96×96，便于绕中心旋转；原图保留。

图像编辑提示词：移除小狗右侧独立青色音符；保留白色小狗的姿势、五官、粉色脸颊、黑色轮廓、青色耳机及原构图，背景透明，不添加其他元素。

内置工具使用的提示词原文：

> Use case: precise-object-edit. Edit target: attached transparent cartoon dog with cyan headphones. Remove ONLY the tiny cyan music note at the far right of the image (separate detached note). Leave that area fully transparent. Preserve the exact same white dog, pose, face, pink cheeks, black outlines, cyan headphones, colors, and dimensions/proportions. Keep original composition and transparent margins in a landscape canvas of ratio 216:142. Do not redraw, restyle, recenter or enlarge the dog. Do not add notes or any other elements. Transparent background, no checkerboard.

布局通过 JieLiUI 本地 MCP 更新；UI 动画在 custom 显示模块，蓝牙歌曲信息及切换规则在 SDK common 业务模块。

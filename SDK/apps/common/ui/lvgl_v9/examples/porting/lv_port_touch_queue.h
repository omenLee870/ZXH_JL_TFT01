/**
 * @file lv_port_touch_queue.h
 * @brief 保留触摸按下/抬起边沿，仅合并连续移动；由调用方保护跨核访问。
 */
#ifndef LV_PORT_TOUCH_QUEUE_H
#define LV_PORT_TOUCH_QUEUE_H
#include <stdint.h>
#define LV_TOUCH_QUEUE_CAPACITY 64u
struct lv_touch_sample { int32_t x,y; uint32_t sampled_ms; uint8_t pressed,edge; };
struct lv_touch_queue {
    struct lv_touch_sample items[LV_TOUCH_QUEUE_CAPACITY], current;
    unsigned head,count,peak,merged,rejected;
    int32_t last_x,last_y;
    uint8_t last_pressed;
};
/** @return -1 表示队列满，采集任务须重试本次样本，不能用新状态覆盖失败的边沿。 */
static int lv_touch_queue_push(struct lv_touch_queue *q,int32_t x,int32_t y,uint8_t pressed)
{
    pressed=!!pressed;
    uint8_t edge=pressed!=q->last_pressed;
    if (!edge && (!pressed || (x==q->last_x && y==q->last_y))) return 0;
    if (!edge && q->count) {
        struct lv_touch_sample *tail=&q->items[(q->head+q->count-1)%LV_TOUCH_QUEUE_CAPACITY];
        if (!tail->edge) {tail->x=x;tail->y=y;q->last_x=x;q->last_y=y;q->merged++;return 0;}
    }
    if (q->count==LV_TOUCH_QUEUE_CAPACITY) {q->rejected++;return -1;}
    struct lv_touch_sample *sample=&q->items[(q->head+q->count)%LV_TOUCH_QUEUE_CAPACITY];
    sample->x=x;sample->y=y;sample->pressed=pressed;sample->edge=edge;
    sample->sampled_ms=0;
    q->count++;if(q->count>q->peak)q->peak=q->count;
    q->last_x=x;q->last_y=y;q->last_pressed=pressed;
    return 0;
}
static void lv_touch_queue_read(struct lv_touch_queue *q,struct lv_touch_sample *out,uint8_t *more)
{
    if (q->count) {q->current=q->items[q->head];q->head=(q->head+1)%LV_TOUCH_QUEUE_CAPACITY;q->count--;}
    *out=q->current;*more=!!q->count;
}
#endif

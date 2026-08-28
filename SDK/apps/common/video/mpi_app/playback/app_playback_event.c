#include "app_playback_mpi_priv.h"

#define Q_APP_PLAYBACK_EVENT_MASK   BIT(17)

app_playback_ret_t app_playback_register_event_handler(
    app_playback_handle_t handle,
    void *priv,
    app_playback_event_handler_t handler)
{
    const char *owner;

    if (handle == NULL || handler == NULL) {
        return APP_PLAYBACK_ERR_INVALID_ARG;
    }

    owner = APP_PLAYBACK_EVENT_TASK_NAME;

    handle->event_handler = handler;
    handle->event_priv = priv;
    handle->event_owner = owner;

    return APP_PLAYBACK_OK;
}


app_playback_ret_t app_playback_post_event(app_playback_handle_t handle,
        int argc,
        int *argv)
{
    int err;
    int msg[32];

    if (handle == NULL) {
        return APP_PLAYBACK_ERR_INVALID_ARG;
    }

    if (handle->event_handler == NULL || handle->event_owner == NULL) {
        return APP_PLAYBACK_OK;
    }

    if (argc <= 0 || argv == NULL) {
        return APP_PLAYBACK_ERR_INVALID_ARG;
    }

    if ((u32)(argc + 3) > ARRAY_SIZE(msg)) {
        return APP_PLAYBACK_ERR_INVALID_ARG;
    }

    msg[0] = (int)handle->event_handler;
    msg[1] = BIT(10) | (1 + argc);
    msg[2] = (int)handle->event_priv;
    memcpy(msg + 3, argv, argc * sizeof(int));

    err = os_taskq_post_type(handle->event_owner,
                             Q_CALLBACK | Q_APP_PLAYBACK_EVENT_MASK,
                             argc + 3,
                             msg);
    if (err != OS_NO_ERR && err != OS_ERR_NONE) {
        printf("failed to post playback event to task %s, err=%d\n", handle->event_owner, err);
        return APP_PLAYBACK_ERR_IO;
    }

    return APP_PLAYBACK_OK;
}

static void app_playback_event_task(void *arg)
{
    int msg[32];

    (void)arg;

    while (1) {
        if (os_taskq_pend("taskq", msg, ARRAY_SIZE(msg)) != OS_TASKQ) {
            continue;
        }
    }
}

static int app_playback_mpi_init(void)
{
    int ret;

    ret = thread_fork(APP_PLAYBACK_EVENT_TASK_NAME,
                      APP_PLAYBACK_EVENT_TASK_PRIO,
                      APP_PLAYBACK_EVENT_TASK_STACK,
                      APP_PLAYBACK_EVENT_TASK_Q_SIZE,
                      NULL,
                      app_playback_event_task,
                      NULL);
    if (ret != OS_NO_ERR && ret != OS_ERR_NONE) {
        printf("failed to create playback event task, ret=%d\n", ret);
    }

    return 0;
}
late_initcall(app_playback_mpi_init);
/****************************************************************************
 * packages/demos/knowledge_cards/tests/test_mocks.c
 *
 * 测试环境 Mock 实现
 * 为 pthread、semaphore、libuv、media API 提供 stub
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include <stddef.h>

/* pthread stubs */

typedef unsigned long pthread_t;
typedef struct {
    int stacksize;
} pthread_attr_t;

int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine)(void *), void *arg)
{
    (void)thread;
    (void)attr;
    (void)start_routine;
    (void)arg;
    return 0;
}

int pthread_join(pthread_t thread, void **retval)
{
    (void)thread;
    (void)retval;
    return 0;
}

int pthread_attr_init(pthread_attr_t *attr)
{
    (void)attr;
    return 0;
}

int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize)
{
    (void)attr;
    (void)stacksize;
    return 0;
}

/* semaphore stubs */

typedef struct {
    int value;
} sem_t;

int sem_init(sem_t *sem, int pshared, unsigned int value)
{
    (void)pshared;
    sem->value = (int)value;
    return 0;
}

int sem_wait(sem_t *sem)
{
    (void)sem;
    return 0;
}

int sem_post(sem_t *sem)
{
    (void)sem;
    return 0;
}

int sem_destroy(sem_t *sem)
{
    (void)sem;
    return 0;
}

/* libuv stubs */

typedef struct uv_loop_s {
    void *data;
} uv_loop_t;

static uv_loop_t g_default_loop;

uv_loop_t *uv_default_loop(void)
{
    return &g_default_loop;
}

/* media API stubs */

typedef void* media_recorder_t;

media_recorder_t media_uv_recorder_open(void *loop, const char *stream_name,
                                         void *callback, void *cookie)
{
    (void)loop;
    (void)stream_name;
    (void)callback;
    (void)cookie;
    /* Return a non-NULL sentinel */
    return (media_recorder_t)0xDEADBEEF;
}

int media_uv_recorder_listen(media_recorder_t recorder, void *callback)
{
    (void)recorder;
    (void)callback;
    return 0;
}

int media_uv_recorder_prepare(media_recorder_t recorder, void *reserved,
                               const char *format, void *callback,
                               void *cookie, void *extra)
{
    (void)recorder;
    (void)reserved;
    (void)format;
    (void)callback;
    (void)cookie;
    (void)extra;
    return 0;
}

int media_uv_recorder_start(media_recorder_t recorder, void *callback, void *cookie)
{
    (void)recorder;
    (void)callback;
    (void)cookie;
    return 0;
}

int media_uv_recorder_stop(media_recorder_t recorder, void *callback, void *cookie)
{
    (void)recorder;
    (void)callback;
    (void)cookie;
    return 0;
}

int media_uv_recorder_close(media_recorder_t recorder, void *callback)
{
    (void)recorder;
    (void)callback;
    return 0;
}

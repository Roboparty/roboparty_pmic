#ifndef TEST_RTTHREAD_H
#define TEST_RTTHREAD_H

struct rt_mutex { int unused; };

#define RT_WAITING_FOREVER (-1)
#define RT_IPC_FLAG_PRIO 0

int rt_mutex_init(struct rt_mutex *mutex, const char *name, int flag);
int rt_mutex_take(struct rt_mutex *mutex, int timeout);
int rt_mutex_release(struct rt_mutex *mutex);
void rt_enter_critical(void);
void rt_exit_critical(void);
void rt_thread_mdelay(int milliseconds);

#endif

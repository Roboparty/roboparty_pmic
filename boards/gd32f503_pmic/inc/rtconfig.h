#ifndef __RTTHREAD_CFG_H__
#define __RTTHREAD_CFG_H__

#define RT_NAME_MAX                     8
#define RT_ALIGN_SIZE                   8
#define RT_CPUS_NR                      1
#define RT_THREAD_PRIORITY_32
#define RT_THREAD_PRIORITY_MAX          32
#define RT_TICK_PER_SECOND              1000
#define RT_USING_OVERFLOW_CHECK
#define IDLE_THREAD_STACK_SIZE          256

#define RT_USING_SEMAPHORE
#define RT_USING_HEAP
#define RT_USING_SMALL_MEM
#define RT_USING_SMALL_MEM_AS_HEAP
#define RT_USING_MUTEX

#define RT_USING_DEVICE
#define RT_USING_DEVICE_IPC
#define RT_USING_SERIAL
#define RT_USING_SERIAL_V1
#define RT_SERIAL_RB_BUFSZ              64
#define RT_USING_PIN

#define RT_USING_CONSOLE
#define RT_CONSOLEBUF_SIZE              128
#define RT_CONSOLE_DEVICE_NAME          "uart3"

#define RT_USING_COMPONENTS_INIT
#define RT_USING_USER_MAIN
#define RT_MAIN_THREAD_STACK_SIZE       2048
#define RT_MAIN_THREAD_PRIORITY         4

#define RT_VER_NUM                      0x50300
#define RT_BACKTRACE_LEVEL_MAX_NR        32

#define ARCH_ARM
#define ARCH_ARM_CORTEX_M
#define ARCH_ARM_CORTEX_M33

#endif /* __RTTHREAD_CFG_H__ */

cmake_minimum_required(VERSION 3.22)
project(ChargeBAT_GD32 C ASM)

option(PMIC_GD32_HARDWARE_VERIFIED "Enable charging after GD32 board verification" OFF)

set(CMAKE_C_STANDARD 11)
set(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")
if(DEFINED ENV{RTT_ROOT} AND EXISTS "$ENV{RTT_ROOT}/src/scheduler_up.c")
    set(RTT_ROOT "$ENV{RTT_ROOT}")
else()
    set(RTT_ROOT "${REPO_ROOT}/thirdparty/rt-thread")
endif()
set(SDK "${REPO_ROOT}/thirdparty/GD32F50x_Firmware_Library_V1.0.3/Firmware")
if(NOT EXISTS "${RTT_ROOT}/src/scheduler_up.c" OR NOT EXISTS "${SDK}/CMSIS/GD/GD32F50x/Include/gd32f50x.h")
    message(FATAL_ERROR "RT-Thread 5.3.0 and GD32F50x SDK are required")
endif()

set(RTT_KERNEL_SOURCES
    ${RTT_ROOT}/src/clock.c ${RTT_ROOT}/src/components.c
    ${RTT_ROOT}/src/cpu_up.c ${RTT_ROOT}/src/defunct.c
    ${RTT_ROOT}/src/idle.c ${RTT_ROOT}/src/ipc.c
    ${RTT_ROOT}/src/irq.c ${RTT_ROOT}/src/kservice.c
    ${RTT_ROOT}/src/mem.c ${RTT_ROOT}/src/object.c
    ${RTT_ROOT}/src/scheduler_comm.c ${RTT_ROOT}/src/scheduler_up.c
    ${RTT_ROOT}/src/thread.c ${RTT_ROOT}/src/timer.c
    ${RTT_ROOT}/src/klibc/kerrno.c ${RTT_ROOT}/src/klibc/kstdio.c
    ${RTT_ROOT}/src/klibc/kstring.c ${RTT_ROOT}/src/klibc/rt_vsnprintf_tiny.c
)
set(RTT_DRIVER_SOURCES
    ${RTT_ROOT}/components/drivers/core/device.c
    ${RTT_ROOT}/components/drivers/serial/dev_serial.c
    ${RTT_ROOT}/components/drivers/ipc/completion_up.c
    ${RTT_ROOT}/components/drivers/ipc/completion_comm.c
    ${RTT_ROOT}/components/drivers/ipc/ringbuffer.c
)
add_executable(${PROJECT_NAME}
    bootloader/startup_gd32f50x.S
    board.c src/main.c
    drivers/gpio.c drivers/serial.c drivers/adc.c drivers/pwm.c drivers/flash.c
    drivers/ota_storage_flash.c
    ${REPO_ROOT}/boards/stm32f103_pmic/drivers/syscalls.c
    ${REPO_ROOT}/boards/stm32f103_pmic/drivers/sysmem.c
    ${REPO_ROOT}/common/drivers/src/i2c_soft.c
    ${REPO_ROOT}/common/drivers/src/ch224.c
    ${REPO_ROOT}/common/drivers/src/tmp100.c
    ${REPO_ROOT}/common/ota/src/ota.c
    ${REPO_ROOT}/common/app/src/app_main.c
    ${REPO_ROOT}/common/app/src/app_state.c
    ${REPO_ROOT}/common/app/src/app_config.c
    ${REPO_ROOT}/common/app/src/app_charge.c
    ${REPO_ROOT}/common/app/src/app_host.c
    ${SDK}/CMSIS/GD/GD32F50x/Source/system_gd32f50x.c
    ${SDK}/GD32F50x_standard_peripheral/Source/gd32f50x_gpio.c
    ${SDK}/GD32F50x_standard_peripheral/Source/gd32f50x_rcu.c
    ${SDK}/GD32F50x_standard_peripheral/Source/gd32f50x_usart.c
    ${SDK}/GD32F50x_standard_peripheral/Source/gd32f50x_adc.c
    ${SDK}/GD32F50x_standard_peripheral/Source/gd32f50x_timer.c
    ${SDK}/GD32F50x_standard_peripheral/Source/gd32f50x_fmc.c
    ${SDK}/GD32F50x_standard_peripheral/Source/gd32f50x_misc.c
    ${RTT_KERNEL_SOURCES} ${RTT_DRIVER_SOURCES}
    ${RTT_ROOT}/libcpu/arm/cortex-m33/cpuport.c
    ${RTT_ROOT}/libcpu/arm/cortex-m33/trustzone.c
    ${RTT_ROOT}/libcpu/arm/cortex-m33/context_gcc.S
)
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/drivers
    ${REPO_ROOT}/common/app/inc ${REPO_ROOT}/common/drivers/inc
    ${REPO_ROOT}/common/bsp
    ${REPO_ROOT}/common/ota/inc
    ${SDK}/../Template ${SDK}/CMSIS ${SDK}/CMSIS/GD/GD32F50x/Include
    ${SDK}/GD32F50x_standard_peripheral/Include
    ${RTT_ROOT}/include ${RTT_ROOT}/components/drivers/include
    ${RTT_ROOT}/libcpu/arm/cortex-m33
)
target_compile_definitions(${PROJECT_NAME} PRIVATE
    GD32F50X GD32F503 GD32F50X_HD PMIC_LEFT_LAYOUT
    __RTTHREAD__ __RT_KERNEL_SOURCE__ boot_main=rtthread_startup
)
if(PMIC_GD32_HARDWARE_VERIFIED)
    target_compile_definitions(${PROJECT_NAME} PRIVATE PMIC_GD32_HARDWARE_VERIFIED)
endif()
target_compile_options(${PROJECT_NAME} PRIVATE
    -mcpu=cortex-m33 -mthumb -mfloat-abi=soft -Os
    -ffunction-sections -fdata-sections
)
target_link_options(${PROJECT_NAME} PRIVATE
    -mcpu=cortex-m33 -mthumb -mfloat-abi=soft
    -T${CMAKE_CURRENT_SOURCE_DIR}/linker_scripts/link.ld
    --specs=nano.specs -nostartfiles -Wl,--gc-sections
    -Wl,-Map=ChargeBAT_GD32.map -Wl,-u,_printf_float
    -Wl,--print-memory-usage
)
target_link_libraries(${PROJECT_NAME} PRIVATE m)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}> ChargeBAT_GD32.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ChargeBAT_GD32.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}>
)

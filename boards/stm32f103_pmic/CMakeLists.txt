cmake_minimum_required(VERSION 3.22)

project(ChargeBAT C ASM)

option(PMIC_LEFT_LAYOUT "Build for the left schematic without OLED or SPI flash" OFF)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

set(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")
if(DEFINED ENV{RTT_ROOT} AND EXISTS "$ENV{RTT_ROOT}/src/scheduler_up.c")
    set(RTT_ROOT "$ENV{RTT_ROOT}")
else()
    set(RTT_ROOT "${REPO_ROOT}/thirdparty/rt-thread")
endif()

if(NOT EXISTS "${RTT_ROOT}/src/scheduler_up.c")
    message(FATAL_ERROR "RT-Thread 5.3.0 not found at ${RTT_ROOT}; set RTT_ROOT")
endif()

set(CMSIS_ROOT "${REPO_ROOT}/thirdparty/STM32F1xx_CMSIS")

set(RTT_KERNEL_SOURCES
    ${RTT_ROOT}/src/clock.c
    ${RTT_ROOT}/src/components.c
    ${RTT_ROOT}/src/cpu_up.c
    ${RTT_ROOT}/src/defunct.c
    ${RTT_ROOT}/src/idle.c
    ${RTT_ROOT}/src/ipc.c
    ${RTT_ROOT}/src/irq.c
    ${RTT_ROOT}/src/kservice.c
    ${RTT_ROOT}/src/mem.c
    ${RTT_ROOT}/src/object.c
    ${RTT_ROOT}/src/scheduler_comm.c
    ${RTT_ROOT}/src/scheduler_up.c
    ${RTT_ROOT}/src/thread.c
    ${RTT_ROOT}/src/timer.c
    ${RTT_ROOT}/src/klibc/kerrno.c
    ${RTT_ROOT}/src/klibc/kstdio.c
    ${RTT_ROOT}/src/klibc/kstring.c
    ${RTT_ROOT}/src/klibc/rt_vsnprintf_tiny.c
)

set(RTT_PORT_SOURCES
    ${RTT_ROOT}/libcpu/arm/cortex-m3/cpuport.c
    ${RTT_ROOT}/libcpu/arm/cortex-m3/context_gcc.S
)

set(RTT_DRIVER_SOURCES
    ${RTT_ROOT}/components/drivers/core/device.c
    ${RTT_ROOT}/components/drivers/serial/dev_serial.c
    ${RTT_ROOT}/components/drivers/pin/dev_pin.c
    ${RTT_ROOT}/components/drivers/ipc/completion_up.c
    ${RTT_ROOT}/components/drivers/ipc/completion_comm.c
    ${RTT_ROOT}/components/drivers/ipc/ringbuffer.c
)

if(PMIC_LEFT_LAYOUT)
    set(PMIC_LAYOUT_SOURCES)
else()
    set(PMIC_LAYOUT_SOURCES
        drivers/w25q128.c
        drivers/ota_storage_w25q.c
        ${REPO_ROOT}/common/ota/src/ota.c
        ${REPO_ROOT}/common/drivers/src/oled.c
        ${REPO_ROOT}/common/drivers/src/key.c
        ${REPO_ROOT}/common/app/src/app_ui.c
    )
endif()

add_executable(${PROJECT_NAME}
    startup_stm32f103xb.s
    board.c
    drivers/serial.c
    drivers/pin.c
    drivers/gpio.c
    drivers/adc.c
    drivers/pwm.c
    drivers/flash.c
    drivers/syscalls.c
    drivers/sysmem.c
    src/main.c

    ${REPO_ROOT}/common/drivers/src/i2c_soft.c
    ${REPO_ROOT}/common/drivers/src/ch224.c
    ${REPO_ROOT}/common/drivers/src/tmp100.c
    ${REPO_ROOT}/common/drivers/src/bms_tws.c

    ${REPO_ROOT}/common/app/src/app_main.c
    ${REPO_ROOT}/common/app/src/app_state.c
    ${REPO_ROOT}/common/app/src/app_config.c
    ${REPO_ROOT}/common/app/src/app_charge.c
    ${REPO_ROOT}/common/app/src/app_bms.c
    ${REPO_ROOT}/common/app/src/app_host.c
    ${PMIC_LAYOUT_SOURCES}
    ${RTT_KERNEL_SOURCES}
    ${RTT_PORT_SOURCES}
    ${RTT_DRIVER_SOURCES}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${REPO_ROOT}/common/app/inc
    ${REPO_ROOT}/common/drivers/inc
    ${REPO_ROOT}/common/bsp
    ${REPO_ROOT}/common/ota/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/drivers
    ${RTT_ROOT}/include
    ${RTT_ROOT}/components/drivers/include
    ${RTT_ROOT}/libcpu/arm/cortex-m3
    ${CMSIS_ROOT}/Device/ST/STM32F1xx/Include
    ${CMSIS_ROOT}/Include
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    STM32F103xB
    __RTTHREAD__
    __RT_KERNEL_SOURCE__
)

if(PMIC_LEFT_LAYOUT)
    target_compile_definitions(${PROJECT_NAME} PRIVATE PMIC_LEFT_LAYOUT)
endif()

target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:-Og>
    $<$<CONFIG:Release>:-Os>
)

target_link_libraries(${PROJECT_NAME} PRIVATE m)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}> ${CMAKE_PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${CMAKE_PROJECT_NAME}.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}>
    COMMENT "Generating HEX/BIN"
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    ADDITIONAL_CLEAN_FILES "${CMAKE_PROJECT_NAME}.map"
)

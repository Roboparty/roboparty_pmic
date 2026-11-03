cmake_minimum_required(VERSION 3.20)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
project(roboparty_pmic C CXX ASM)

set(RTT_ROOT "$ENV{RTT_ROOT}" CACHE PATH "RT-Thread 5.3.0 root")
if(NOT RTT_ROOT OR NOT EXISTS "${RTT_ROOT}/src/scheduler_up.c")
    set(RTT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/rt-thread" CACHE PATH "RT-Thread 5.3.0 root" FORCE)
endif()
set(GD32_SDK "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/GD32F50x_Firmware_Library_V1.0.3")
if(NOT EXISTS "${RTT_ROOT}/src/scheduler_up.c")
    message(FATAL_ERROR "RTT_ROOT must point to RT-Thread 5.3.0")
endif()
if(NOT EXISTS "${GD32_SDK}/Firmware")
    message(FATAL_ERROR "GD32F50x V1.0.3 SDK is missing")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_library(${PROJECT_NAME} STATIC
    src/main.c
    src/scud_bms_rtt.cpp
)
target_include_directories(${PROJECT_NAME} PRIVATE
    inc boards/gd32f503_pmic
    ${GD32_SDK}/Firmware/CMSIS/GD/GD32F50x/Include
    ${GD32_SDK}/Firmware/GD32F50x_standard_peripheral/Include
    ${RTT_ROOT}/include ${RTT_ROOT}/components/finsh
    ${RTT_ROOT}/components/drivers/include
    ${RTT_ROOT}/libcpu/arm/cortex-m33
)
target_compile_definitions(${PROJECT_NAME} PRIVATE GD32F503 GD32F50X_HD)
target_compile_options(${PROJECT_NAME} PRIVATE -mcpu=cortex-m33 -mthumb -mfloat-abi=soft)


target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)


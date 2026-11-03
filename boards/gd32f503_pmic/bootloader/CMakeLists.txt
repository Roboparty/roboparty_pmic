cmake_minimum_required(VERSION 3.22)
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_SIZE arm-none-eabi-size)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
project(GD32F503_BOOT C ASM)

set(SDK "${CMAKE_CURRENT_SOURCE_DIR}/../../../thirdparty/GD32F50x_Firmware_Library_V1.0.3")
set(OTA_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../common/ota")

add_executable(boot
    startup_gd32f50x.S
    boot_main.c
    ${SDK}/Firmware/CMSIS/GD/GD32F50x/Source/system_gd32f50x.c
    ${SDK}/Firmware/GD32F50x_standard_peripheral/Source/gd32f50x_fmc.c
    ${SDK}/Firmware/GD32F50x_standard_peripheral/Source/gd32f50x_misc.c
    ${OTA_ROOT}/src/ota.c
)

target_include_directories(boot PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${OTA_ROOT}/inc
    ${SDK}/Template
    ${SDK}/Firmware/CMSIS
    ${SDK}/Firmware/CMSIS/GD/GD32F50x/Include
    ${SDK}/Firmware/GD32F50x_standard_peripheral/Include
)

target_compile_definitions(boot PRIVATE GD32F50X GD32F503 GD32F50X_HD)
target_compile_options(boot PRIVATE
    -mcpu=cortex-m33 -mthumb -mfloat-abi=soft -Os -g0
    -ffunction-sections -fdata-sections
)
target_link_options(boot PRIVATE
    -mcpu=cortex-m33 -mthumb -mfloat-abi=soft
    -T${CMAKE_CURRENT_SOURCE_DIR}/link.ld
    --specs=nano.specs
    -nostartfiles
    -Wl,--gc-sections
    -Wl,-Map=boot.map
    -Wl,--print-memory-usage
)

add_custom_command(TARGET boot POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:boot> boot.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:boot> boot.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:boot>
)

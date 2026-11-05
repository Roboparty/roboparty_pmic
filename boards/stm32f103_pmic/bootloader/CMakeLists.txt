cmake_minimum_required(VERSION 3.22)
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_SIZE arm-none-eabi-size)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
project(STM32F103_BOOT C ASM)

option(PMIC_LEFT_LAYOUT "Build for the current STM32F103C8 hardware without SPI flash" ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMSIS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../thirdparty/STM32F1xx_CMSIS")
set(OTA_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../common/ota")

if(PMIC_LEFT_LAYOUT)
    set(BOOT_OTA_SOURCES)
else()
    set(BOOT_OTA_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/../drivers/w25q128.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../drivers/ota_storage_w25q.c
        ${OTA_ROOT}/src/ota.c
    )
endif()

add_executable(boot
    startup_stm32f103xb.s
    boot_main.c
    ${BOOT_OTA_SOURCES}
)

target_include_directories(boot PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../drivers
    ${OTA_ROOT}/inc
    ${CMSIS_ROOT}/Device/ST/STM32F1xx/Include
    ${CMSIS_ROOT}/Include
)

target_compile_definitions(boot PRIVATE STM32F103xB)
if(PMIC_LEFT_LAYOUT)
    target_compile_definitions(boot PRIVATE PMIC_LEFT_LAYOUT)
endif()
target_compile_options(boot PRIVATE
    -mcpu=cortex-m3 -mthumb -Os -g0
    -ffunction-sections -fdata-sections
)
target_link_options(boot PRIVATE
    -mcpu=cortex-m3 -mthumb
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

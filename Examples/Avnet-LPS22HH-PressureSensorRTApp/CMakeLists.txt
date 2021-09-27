# This code is based on a sample from Microsoft (see license below),
# with modifications made by MediaTek.
# Modified version of CMakeLists.txt from Microsoft Azure Sphere sample code:
# https://github.com/Azure/azure-sphere-samples/blob/master/Samples/HelloWorld/HelloWorld_RTApp_MT3620_BareMetal/CMakeLists.txt

#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required (VERSION 3.11)

# Configurations
project(AvnetAlsPt19RTApp C ASM)
SET(CMAKE_ASM_FLAGS "-mcpu=cortex-m4")
azsphere_configure_tools(TOOLS_REVISION "20.07")
add_compile_definitions(OSAI_AZURE_RTOS)
add_compile_definitions(OSAI_ENABLE_DMA)
# When place CODE_REGION in FLASH instead of TCM, please enable this definition:
# add_compile_definitions(M4_ENABLE_XIP_FLASH)
add_link_options(-specs=nano.specs -specs=nosys.specs)

# Executable
add_executable(${PROJECT_NAME}
                ../../mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_adc.c
                ../../mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_dma.c
                ../../mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_eint.c
                ../../mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_gpio.c
                ../../mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_gpioif.c
                ../../mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_gpt.c
                ../../mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_i2c.c
                ../../mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_i2s.c
                ../../mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_lp.c
                ../../mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_mbox.c
                ../../mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_mbox_shared_mem.c
                ../../mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_pwm.c
                ../../mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_spim.c
                ../../mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_uart.c
                ../../mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_wdt.c
                ./mbox_logic.c 
                ./rtcoremain.c
                ./tx_initialize_low_level.S)

# Include Folders
include_directories(${PROJECT_NAME} PUBLIC
                    ./)
target_include_directories(${PROJECT_NAME} PUBLIC
                            ../../mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/inc
                            ./)

# Libraries
set(OSAI_AZURE_RTOS 1)
set(THREADX_ARCH "cortex_m4")
set(THREADX_TOOLCHAIN "gnu")
add_subdirectory(../../mt3620_m4_software/MT3620_M4_Driver ./lib/MT3620_M4_Driver)
add_subdirectory(threadx)
target_link_libraries(${PROJECT_NAME} MT3620_M4_Driver threadx)

# Linker, Image
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_DEPENDS ${CMAKE_SOURCE_DIR}/linker.ld)
azsphere_target_add_image_package(${PROJECT_NAME})
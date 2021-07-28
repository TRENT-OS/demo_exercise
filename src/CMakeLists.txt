#
# Demo Exercise
#
# Copyright (C) 2021, Hensoldt Cyber GmbH
#

cmake_minimum_required(VERSION 3.7.2)

os_set_config_file(system_config.h)

project(system_config C)
add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_LIST_DIR})

#-------------------------------------------------------------------------------
project(demo_exercise C)

TimeServer_DeclareCAmkESComponent(
    TimeServer
)

RPi_SPI_Flash_DeclareCAmkESComponent(
    RPi_SPI_Flash
    LIBS
        TimeServer_client
)

StorageServer_DeclareCAmkESComponent(
    StorageServer
)

NIC_RPi_DeclareCAmkESComponent(
    NIC_RPi
)

DeclareCAmkESComponent(
    Ticker
    SOURCES
        components/Ticker/Ticker.c
    C_FLAGS
        -Wall
        -Werror
    LIBS
        lib_debug
        os_core_api
)

DeclareCAmkESComponent(
    NetworkStack
    INCLUDES
        include/util
    SOURCES
        components/NetworkStack/NetworkStack.c
    C_FLAGS
        -Wall -Werror
        -DCFG_ETH_ADDR=ETH_ADDR
        -DCFG_ETH_GATEWAY_ADDR=ETH_GATEWAY_ADDR
        -DCFG_ETH_SUBNET_MASK=ETH_SUBNET_MASK
    LIBS
        system_config
        os_core_api
        lib_debug
        os_network_lib
        TimeServer_client
)

DeclareCAmkESComponent(
    TestApp
    INCLUDES
        include/util
    SOURCES
        components/TestApp/src/TestApp.c
    C_FLAGS
        -Wall
        -Werror
    LIBS
        system_config
        lib_debug
        os_core_api
        os_filesystem
        os_network_api
)

DeclareAndCreateCamkESSystem(demo_exercise.camkes)
GenerateSimulateScript()
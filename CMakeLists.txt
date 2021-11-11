#
# Demo Exercise
#
# Copyright (C) 2021, Hensoldt Cyber GmbH
#

cmake_minimum_required(VERSION 3.7.2)

#-------------------------------------------------------------------------------
os_set_config_file(
    "system_config.h"
    CONFIG_PROJECT_NAME "system_config"
)

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

DeclareCAmkESComponent(
    TestApp
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
)

DeclareAndCreateCamkESSystem(demo_exercise.camkes)
GenerateSimulateScript()
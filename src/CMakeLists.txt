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

SdHostController_DeclareCAmkESComponent(
    SdHostController
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
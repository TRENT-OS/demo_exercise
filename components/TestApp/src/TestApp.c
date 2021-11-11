/*
 * Copyright (C) 2021, Hensoldt Cyber GmbH
 */

#include "OS_FileSystem.h"

#include "lib_debug/Debug.h"
#include <string.h>

#include <camkes.h>

//------------------------------------------------------------------------------
static OS_FileSystem_Config_t spiffsCfg_1 =
{
    .type = OS_FileSystem_Type_SPIFFS,
    .size = OS_FileSystem_USE_STORAGE_MAX,
    .storage = IF_OS_STORAGE_ASSIGN(
        storage_rpc_1,
        storage_dp_1),
};

static OS_FileSystem_Config_t spiffsCfg_2 =
{
    .type = OS_FileSystem_Type_SPIFFS,
    .size = OS_FileSystem_USE_STORAGE_MAX,
    .storage = IF_OS_STORAGE_ASSIGN(
        storage_rpc_2,
        storage_dp_2),
};

//------------------------------------------------------------------------------
static void
test_OS_FileSystem(OS_FileSystem_Config_t* cfg)
{
    OS_Error_t ret;
    OS_FileSystem_Handle_t hFs;

    static const char* fileName = "testfile.txt";
    static const uint8_t fileData[] =
    {
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77
    };
    static const off_t fileSize = sizeof(fileData);
    static OS_FileSystemFile_Handle_t hFile;

    // Init file system
    if ((ret = OS_FileSystem_init(&hFs, cfg)) != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_FileSystem_init() failed, code %d", ret);
    }

    // Format file system
    if ((ret = OS_FileSystem_format(hFs)) != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_FileSystem_format() failed, code %d", ret);
    }

    // Mount file system
    if ((ret = OS_FileSystem_mount(hFs)) != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_FileSystem_mount() failed, code %d", ret);
    }

    // Open file
    if((ret = OS_FileSystemFile_open(hFs, &hFile, fileName,
                                OS_FileSystem_OpenMode_RDWR,
                                OS_FileSystem_OpenFlags_CREATE)) != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_FileSystemFile_open() failed, code %d", ret);
    }

    // Write to the file
    off_t to_write, written;
    to_write = fileSize;
    written = 0;

    while (to_write > 0)
    {
        if((ret = OS_FileSystemFile_write(hFs, hFile, written, sizeof(fileData), fileData)) != OS_SUCCESS)
        {
            Debug_LOG_ERROR("OS_FileSystemFile_write() failed, code %d", ret);
        }

        written  += sizeof(fileData);
        to_write -= sizeof(fileData);
    }

    // Read from the file
    uint8_t buf[sizeof(fileData)];
    off_t to_read, read;
    to_read = fileSize;
    read = 0;

    while (to_read > 0)
    {
        if((ret = OS_FileSystemFile_read(hFs, hFile, read, sizeof(buf), buf)) != OS_SUCCESS)
        {
            Debug_LOG_ERROR("OS_FileSystemFile_read() failed, code %d", ret);
        }

        if(memcmp(fileData, buf, sizeof(buf)))
            Debug_LOG_ERROR("File content read does not equal file content to be written.");

        read    += sizeof(buf);
        to_read -= sizeof(buf);
    }

    // Close file
    if((ret = OS_FileSystemFile_close(hFs, hFile)) != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_FileSystemFile_close() failed, code %d", ret);
    }

    // Clean up
    if((ret = OS_FileSystem_unmount(hFs)) != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_FileSystem_unmount() failed, code %d", ret);
    }

    if ((ret = OS_FileSystem_free(hFs)) != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_FileSystem_free() failed, code %d", ret);
    }
}

//------------------------------------------------------------------------------
int run()
{
    // Work on file system 1 (residing on partition 1)
    test_OS_FileSystem(&spiffsCfg_1);

    // Work on file system 2 (residing on partition 2)
    test_OS_FileSystem(&spiffsCfg_2);

    Debug_LOG_INFO("Demo completed successfully.");

    return 0;
}
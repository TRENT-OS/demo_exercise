/*
 * Copyright (C) 2021, Hensoldt Cyber GmbH
 */

#include "OS_FileSystem.h"

#include "lib_debug/Debug.h"
#include <string.h>

#include "OS_Error.h"
#include "OS_Network.h"
#include "OS_NetworkStackClient.h"
#include "loop_defines.h"

#include <camkes.h>

//----------------------------------------------------------------------
// Storage
//----------------------------------------------------------------------

static OS_FileSystem_Config_t fatCfg_1 =
{
    .type = OS_FileSystem_Type_FATFS,
    .size = OS_FileSystem_USE_STORAGE_MAX,
    .storage = IF_OS_STORAGE_ASSIGN(
        storage_rpc_1,
        storage_dp_1),
};

static OS_FileSystem_Config_t fatCfg_2 =
{
    .type = OS_FileSystem_Type_FATFS,
    .size = OS_FileSystem_USE_STORAGE_MAX,
    .storage = IF_OS_STORAGE_ASSIGN(
        storage_rpc_2,
        storage_dp_2),
};

static char fileData[5];

//------------------------------------------------------------------------------
static void
test_OS_FileSystem(OS_FileSystem_Config_t* cfg)
{
    OS_Error_t ret;
    OS_FileSystem_Handle_t hFs;

    static const char* fileName = "testfile.txt";
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

//----------------------------------------------------------------------
// Network
//----------------------------------------------------------------------

extern OS_Error_t OS_NetworkAPP_RT(OS_Network_Context_t ctx);

void init_client_api()
{
    static OS_NetworkStackClient_SocketDataports_t config;

    config.number_of_sockets = OS_NETWORK_MAXIMUM_SOCKET_NO;
    static OS_Dataport_t dataports[OS_NETWORK_MAXIMUM_SOCKET_NO] = {0};


    int i = 0;

#define LOOP_COUNT OS_NETWORK_MAXIMUM_SOCKET_NO
#define LOOP_ELEMENT                                                     \
    GEN_ID(OS_Dataport_t t) = OS_DATAPORT_ASSIGN(GEN_ID(NwAppDataPort)); \
    dataports[i] = GEN_ID(t);                                            \
    i++;
#include "loop.h"

    config.dataport = dataports;
    OS_NetworkStackClient_init(&config);
}

//------------------------------------------------------------------------------
int run()
{
    //----------------------------------------------------------------------
    // Network Test
    //----------------------------------------------------------------------

    init_client_api();

    Debug_LOG_INFO("Starting test_app_server...");

    char buffer[4096];
    OS_NetworkAPP_RT(NULL);    /* Must be actually called by OS Runtime */

    OS_NetworkServer_Socket_t  srv_socket =
    {
        .domain = OS_AF_INET,
        .type   = OS_SOCK_STREAM,
        .listen_port = 5555,
        .backlog = 1,
    };

    /* Gets filled when accept is called */
    OS_NetworkSocket_Handle_t  os_socket_handle ;
    /* Gets filled when server socket create is called */
    OS_NetworkServer_Handle_t  os_nw_server_handle ;

    OS_Error_t err = OS_NetworkServerSocket_create(NULL, &srv_socket,
                                                   &os_nw_server_handle);

    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("server_socket_create() failed, code %d", err);
        return -1;
    }

    Debug_LOG_INFO("launching echo server");

    for (;;)
    {
        err = OS_NetworkServerSocket_accept(os_nw_server_handle, &os_socket_handle);
        if (err != OS_SUCCESS)
        {
            Debug_LOG_ERROR("socket_accept() failed, error %d", err);
            return -1;
        }

        /*
            As of now the nw stack behavior is as below:
            Keep reading data until you receive one of the return values:
             a. err = OS_ERROR_CONNECTION_CLOSED and length = 0 indicating end of data read
                      and connection close
             b. err = OS_ERROR_GENERIC  due to error in read
             c. err = OS_SUCCESS and length = 0 indicating no data to read but there is still
                      connection
             d. err = OS_SUCCESS and length >0 , valid data

            Take appropriate actions based on the return value rxd.


            Only a single socket is supported and no multithreading !!!
            Once we accept an incoming connection, start reading data from the client and echo back
            the data rxd.
        */
        memset(buffer, 0, sizeof(buffer));

        Debug_LOG_INFO("starting server read loop");
        /* Keep calling read until we receive CONNECTION_CLOSED from the stack */
        for (;;)
        {
            Debug_LOG_DEBUG("read...");
            size_t n = 0;
            // Try to read as much as fits into the buffer
            err = OS_NetworkSocket_read(os_socket_handle, buffer, sizeof(buffer), &n);
            if (OS_SUCCESS != err)
            {
                Debug_LOG_ERROR("socket_read() failed, error %d", err);
                break;
            }

            err = OS_NetworkSocket_write(os_socket_handle, buffer, n, &n);
            if (err != OS_SUCCESS)
            {
                Debug_LOG_ERROR("socket_write() failed, error %d", err);
                break;
            }

            memcpy(fileData, buffer, sizeof(fileData));
        }
            // ----------------------------------------------------------------------
            // Storage Test
            // ----------------------------------------------------------------------

            // Work on file system 1 (residing on partition 1)
            test_OS_FileSystem(&fatCfg_1);

            // Work on file system 2 (residing on partition 2)
            test_OS_FileSystem(&fatCfg_2);

            Debug_LOG_INFO("Demo completed successfully.");

        switch (err)
        {
        /* This means end of read as socket was closed. Exit now and close handle*/
        case OS_ERROR_CONNECTION_CLOSED:
            Debug_LOG_INFO("connection closed by server");
            OS_NetworkSocket_close(os_socket_handle);
            continue;
        /* Any other value is a failure in read, hence exit and close handle  */
        default :
            Debug_LOG_ERROR("server socket failure, error %d", err);
            OS_NetworkSocket_close(os_socket_handle);
            continue;
        } //end of switch
    }
    return -1;
}
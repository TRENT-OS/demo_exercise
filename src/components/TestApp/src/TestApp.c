/*
 * Copyright (C) 2021, Hensoldt Cyber GmbH
 */

#include "OS_FileSystem.h"

#include "lib_debug/Debug.h"
#include <string.h>
#include <math.h>

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

static char fileData[250];

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

    // New network client stuff
    OS_Network_Socket_t cli_socket =
    {
        .domain = OS_AF_INET,
        .type   = OS_SOCK_STREAM,
        .name   = "10.0.0.10",
        .port   = 8888
    };

    /* This creates a socket API and gives a handle which can be used
       for further communication. */
    OS_NetworkSocket_Handle_t handle[OS_NETWORK_MAXIMUM_SOCKET_NO];
    OS_Error_t err;
    int socket_max = 0;
    int i;

    for (i = 0; i < OS_NETWORK_MAXIMUM_SOCKET_NO; i++)
    {
        err = OS_NetworkSocket_create(NULL, &cli_socket, &handle[i]);
        if (err != OS_SUCCESS)
        {
            Debug_LOG_ERROR("client_socket_create() failed, code %d for %d socket", err, i);
            break;
        }
    }
    socket_max = i;

    Debug_LOG_INFO("Send request to host...");

    char* request = "GET / HTTP/1.0\r\nHost: " CFG_TEST_HTTP_SERVER
                    "\r\nConnection: close\r\n\r\n";

    const size_t len_request = strlen(request);
    size_t len = len_request;

    /* Send the request to the host */
    for (int i = 0; i < socket_max; i++)
    {
        size_t offs = 0;
        Debug_LOG_INFO("Writing request to socket %d for %.*s", i, 17, request);
        request[13] = 'a' + i;
        do
        {
            const size_t lenRemaining = len_request - offs;
            size_t len_io = lenRemaining;

            err = OS_NetworkSocket_write(handle[i], &request[offs], len_io, &len_io);

            if (err != OS_SUCCESS)
            {
                Debug_LOG_ERROR("socket_write() failed, code %d", err);
                OS_NetworkSocket_close(handle[i]);
                return OS_ERROR_GENERIC;
            }

            /* fatal error, this must not happen. API broken*/
            Debug_ASSERT(len_io <= lenRemaining);

            offs += len_io;
        } while (offs < len_request);
    }

    Debug_LOG_INFO("read response...");

    int flag = 0;

    do
    {
        for (int i = 0; i < socket_max; i++)
        {
            len = sizeof(buffer);
            memset(buffer, 0, sizeof(buffer));
            OS_Error_t err = OS_ERROR_CONNECTION_CLOSED;
            if (!(flag & (1 << i))) {
                err = OS_NetworkSocket_read(handle[i], buffer, len, &len);
            }

            switch (err)
            {
            /* This means end of read or nothing further to read as socket was
             * closed */
            case OS_ERROR_CONNECTION_CLOSED:
                Debug_LOG_INFO("socket_read() reported connection closed for handle %d", i);
                flag |= 1 << i; /* terminate loop and close handle*/
                break;

            /* Continue further reading */
            case OS_SUCCESS:
                Debug_LOG_INFO("chunk read, length %d, handle %d", len, i);
                memcpy(fileData, buffer, sizeof(fileData));
                continue;

            /* Error case, break and close the handle */
            default:
                Debug_LOG_INFO("socket_read() failed for handle %d, error %d", i, err);
                flag |= 1 << i; /* terminate loop and close handle */
                break;
            }
        }
    } while (flag != pow(2, socket_max) - 1);

    // ----------------------------------------------------------------------
    // Storage Test
    // ----------------------------------------------------------------------

    // Work on file system 1 (residing on partition 1)
    test_OS_FileSystem(&fatCfg_1);

    // Work on file system 2 (residing on partition 2)
    test_OS_FileSystem(&fatCfg_2);

    Debug_LOG_INFO("Demo completed successfully.");

    for (int i = 0; i < socket_max; i++)
    {
        /* Close the socket communication */
        err = OS_NetworkSocket_close(handle[i]);
        if (err != OS_SUCCESS)
        {
            Debug_LOG_ERROR("close() failed for handle %d, code %d", i, err);
            return OS_ERROR_GENERIC;
        }
    }
    return OS_SUCCESS;
}
/**
 * Copyright (C) 2021, Hensoldt Cyber GmbH
 *
 * OS libraries configurations
 */


#pragma once


//-----------------------------------------------------------------------------
// Debug
//-----------------------------------------------------------------------------
#if !defined(NDEBUG)
#   define Debug_Config_STANDARD_ASSERT
#   define Debug_Config_ASSERT_SELF_PTR
#else
#   define Debug_Config_DISABLE_ASSERT
#   define Debug_Config_NO_ASSERT_SELF_PTR
#endif

#define Debug_Config_LOG_LEVEL                  Debug_LOG_LEVEL_INFO
#define Debug_Config_INCLUDE_LEVEL_IN_MSG
#define Debug_Config_LOG_WITH_FILE_LINE


//-----------------------------------------------------------------------------
// Memory
//-----------------------------------------------------------------------------
#define Memory_Config_USE_STDLIB_ALLOC


//-----------------------------------------------------------------------------
// StorageServer
//-----------------------------------------------------------------------------

// 129 MiB reserved for MBR and BOOT partition
#define MBR_STORAGE_SIZE            (1*1024*1024)
#define BOOT_STORAGE_SIZE           (128*1024*1024)

// 32 MiB for TestApp
#define FILESYSTEM_1_STORAGE_OFFSET     (MBR_STORAGE_SIZE + BOOT_STORAGE_SIZE)
#define FILESYSTEM_1_STORAGE_SIZE       (16*1024*1024)
#define FILESYSTEM_2_STORAGE_OFFSET     (FILESYSTEM_1_STORAGE_OFFSET + FILESYSTEM_1_STORAGE_SIZE)
#define FILESYSTEM_2_STORAGE_SIZE       (16*1024*1024)

//-----------------------------------------------------------------------------
// NIC driver
//-----------------------------------------------------------------------------
#define NIC_DRIVER_RINGBUFFER_NUMBER_ELEMENTS 16
#define NIC_DRIVER_RINGBUFFER_SIZE                                             \
    (NIC_DRIVER_RINGBUFFER_NUMBER_ELEMENTS * 4096)

//-----------------------------------------------------------------------------
// Network Stack
//-----------------------------------------------------------------------------

#ifndef OS_NETWORK_MAXIMUM_SOCKET_NO
#define OS_NETWORK_MAXIMUM_SOCKET_NO 1
#endif

#define ETH_ADDR                  "10.0.0.11"
#define ETH_GATEWAY_ADDR          "10.0.0.1"
#define ETH_SUBNET_MASK           "255.255.255.0"

#define CFG_TEST_HTTP_SERVER "10.0.0.10"
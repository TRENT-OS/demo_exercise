/*
 *  OS Network Stack CAmkES wrapper
 *
 *  Copyright (C) 2021, Hensoldt Cyber GmbH
 *
 */

#include "system_config.h"

#include "lib_debug/Debug.h"
#include "OS_Error.h"
#include "OS_NetworkStack.h"
#include "TimeServer.h"
#include <camkes.h>
#include "OS_Dataport.h"
#include "loop_defines.h"

static const if_OS_Timer_t timer =
    IF_OS_TIMER_ASSIGN(
        timeServer_rpc,
        timeServer_notify);

static const OS_NetworkStack_AddressConfig_t config =
{
    .dev_addr      = CFG_ETH_ADDR,
    .gateway_addr  = CFG_ETH_GATEWAY_ADDR,
    .subnet_mask   = CFG_ETH_SUBNET_MASK
};

//------------------------------------------------------------------------------
// network stack's PicoTCP OS adaption layer calls this.
uint64_t
Timer_getTimeMs(void)
{
    OS_Error_t err;
    uint64_t ms;

    if ((err = TimeServer_getTime(&timer, TimeServer_PRECISION_MSEC,
                                  &ms)) != OS_SUCCESS)
    {
        Debug_LOG_ERROR("TimeServer_getTime() failed with %d", err);
        ms = 0;
    }

    return ms;
}

//------------------------------------------------------------------------------
int run(void)
{
    Debug_LOG_INFO("[NwStack '%s'] starting", get_instance_name());

    #define LOOP_ELEMENT \
        { \
            .notify_write      = GEN_EMIT(e_write), \
            .wait_write        = GEN_WAIT(c_write), \
            .notify_read       = GEN_EMIT(e_read), \
            .wait_read         = GEN_WAIT(c_read), \
            .notify_connection = GEN_EMIT(e_conn), \
            .wait_connection   = GEN_WAIT(c_conn), \
            .buf               = OS_DATAPORT_ASSIGN(GEN_ID(nwStack_port)), \
            .accepted_handle   = -1, \
        },

    static OS_NetworkStack_SocketResources_t socks[OS_NETWORK_MAXIMUM_SOCKET_NO] = {
        #define LOOP_COUNT OS_NETWORK_MAXIMUM_SOCKET_NO
        #include "loop.h" // places LOOP_ELEMENT here for LOOP_COUNT times
    };

    static const OS_NetworkStack_CamkesConfig_t camkes_config =
    {
        .notify_init_done        = nwStack_event_ready_emit,
        .wait_loop_event         = event_tick_or_data_wait,

        .internal =
        {
            .notify_loop        = event_internal_emit,

            .allocator_lock     = allocatorMutex_lock,
            .allocator_unlock   = allocatorMutex_unlock,

            .nwStack_lock       = nwstackMutex_lock,
            .nwStack_unlock     = nwstackMutex_unlock,

            .socketCB_lock      = socketControlBlockMutex_lock,
            .socketCB_unlock    = socketControlBlockMutex_unlock,

            .stackTS_lock       = stackThreadSafeMutex_lock,
            .stackTS_unlock     = stackThreadSafeMutex_unlock,

            .number_of_sockets = OS_NETWORK_MAXIMUM_SOCKET_NO,
            .sockets           = socks,
        },

        .drv_nic =
        {
            .from =
            {
                .io = (void**)( &(nic_port_from)),
                .size = NIC_DRIVER_RINGBUFFER_NUMBER_ELEMENTS
            },
            .to = OS_DATAPORT_ASSIGN(nic_port_to),

            .rpc =
            {
                .dev_read       = nic_driver_rx_data,
                .dev_write      = nic_driver_tx_data,
                .get_mac        = nic_driver_get_mac_address,
            }
        },

        .app =
        {
            .notify_init_done   = nwStack_event_ready_emit,

        }
    };

    // The Ticker component sends us a tick every second. Currently there is
    // no dedicated interface to enable and disable the tick. because we don't
    // need this. OS_NetworkStack_run() is not supposed to return.

    OS_Error_t ret = OS_NetworkStack_run(&camkes_config, &config);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_FATAL("[NwStack '%s'] OS_NetworkStack_run() failed, error %d",
                        get_instance_name(), ret);
        return -1;
    }

    // actually, OS_NetworkStack_run() is not supposed to return with
    // OS_SUCCESS. We have to assume this is a graceful shutdown for some
    // reason
    Debug_LOG_WARNING("[NwStack '%s'] graceful termination",
                      get_instance_name());

    return 0;
}
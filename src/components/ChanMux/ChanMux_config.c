/*
 *  Channel MUX
 *
 *  Copyright (C) 2021, HENSOLDT Cyber GmbH
 */

#include "system_config.h"
#include "ChanMux/ChanMux.h"
#include "ChanMuxNic.h"
#include <camkes.h>


//------------------------------------------------------------------------------
static unsigned int
resolveChannel(
    unsigned int  sender_id,
    unsigned int  chanNum_local)
{
    switch (sender_id)
    {
    //----------------------------------
    case CHANMUX_ID_NIC:
        switch (chanNum_local)
        {
        //----------------------------------
        case CHANMUX_CHANNEL_NIC_CTRL:
            return CHANMUX_CHANNEL_NIC_CTRL;
        //----------------------------------
        case CHANMUX_CHANNEL_NIC_DATA:
            return CHANMUX_CHANNEL_NIC_DATA;
        //----------------------------------
        default:
            break;
        }
        break;
    //----------------------------------
    default:
        break;
    }

    return INVALID_CHANNEL;
}


//------------------------------------------------------------------------------
static struct
{
    uint8_t ctrl[128];
    // FIFO is big enough to store 1 minute of network "background" traffic.
    // Value found by manual testing, may differ in less noisy networks
    uint8_t data[1024 * PAGE_SIZE];
} nic_fifo[1];

static struct
{
    ChanMux_Channel_t ctrl;
    ChanMux_Channel_t data;
} nic_channel[1];


//------------------------------------------------------------------------------
static const ChanMux_ChannelCtx_t channelCtx[] =
{

    CHANNELS_CTX_NIC_CTRL_DATA(
        CHANMUX_CHANNEL_NIC_CTRL,
        CHANMUX_CHANNEL_NIC_DATA,
        0,
        nwDriver_ctrl_portRead,
        nwDriver_ctrl_portWrite,
        nwDriver_data_portRead,
        nwDriver_data_portWrite,
        nwDriver_ctrl_eventHasData_emit,
        nwDriver_data_eventHasData_emit)
};


//------------------------------------------------------------------------------
// this is used by the ChanMux component
const ChanMux_Config_t cfgChanMux =
{
    .resolveChannel = &resolveChannel,
    .numChannels    = ARRAY_SIZE(channelCtx),
    .channelCtx     = channelCtx,
};

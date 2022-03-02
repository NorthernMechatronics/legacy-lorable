/*
 * BSD 3-Clause License
 *
 * Copyright (c) 2021, Northern Mechatronics, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <am_bsp.h>
#include <am_mcu_apollo.h>
#include <am_util.h>

#include <FreeRTOS.h>
#include <FreeRTOS_CLI.h>
#include <queue.h>


#include <wsf_types.h>
#include <wsf_trace.h>
#include <wsf_buf.h>
#include <wsf_timer.h>
#include <wsf_msg.h>

#include <hci_handler.h>

#include <dm_handler.h>
#include <l2c_handler.h>
#include <att_handler.h>
#include <smp_handler.h>
#include <l2c_api.h>
#include <att_api.h>
#include <smp_api.h>
#include <app_api.h>
#include <app_ui.h>

#include "console_task.h"
#include "ble.h"
#include "amota_cli.h"

static portBASE_TYPE amota_command(char *pcWriteBuffer, size_t xWriteBufferLen,
                                 const char *pcCommandString);

CLI_Command_Definition_t amota_command_definition = {
    (const char *const) "amota",
    (const char *const) "amota:\tAMOTA Framework.\r\n",
    amota_command, -1};

static portBASE_TYPE amota_command(char *pcWriteBuffer, size_t xWriteBufferLen,
                                 const char *pcCommandString)
{
    const char *pcParameterString;
    portBASE_TYPE xParameterStringLength;

    pcWriteBuffer[0] = 0x0;

    pcParameterString =
        FreeRTOS_CLIGetParameter(pcCommandString, 1, &xParameterStringLength);
    if (pcParameterString == NULL)
    {
        return pdFALSE;
    }

    if (strncmp(pcParameterString, "start", xParameterStringLength) == 0)
    {
        AppAdvStart(APP_MODE_AUTO_INIT);
    }
    else if (strncmp(pcParameterString, "stop", xParameterStringLength) == 0)
    {
        AppAdvStop();
    }
    else if (strncmp(pcParameterString, "connected", xParameterStringLength) == 0)
    {
        dmConnId_t connId = AppConnIsOpen();

        if (connId == DM_CONN_ID_NONE)
        {
            strcat(pcWriteBuffer, "AMOTA: not connected");
        }
        else
        {
            strcat(pcWriteBuffer, "AMOTA: connected");
        }
    }


    return pdFALSE;
}

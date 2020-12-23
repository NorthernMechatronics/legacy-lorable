/*
 * BSD 3-Clause License
 *
 * Copyright (c) 2020, Northern Mechatronics, Inc.
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
#include <stdio.h>

#include <am_bsp.h>
#include <am_mcu_apollo.h>
#include <am_util.h>

#include <LmHandler.h>
#include <LmHandlerMsgDisplay.h>
#include <LmhpCompliance.h>
#include <LmhpClockSync.h>
#include <LmhpRemoteMcastSetup.h>
#include <NvmCtxMgmt.h>
#include <board.h>
#include <timer.h>

#include "lorawan.h"
#include "lorawan_cli.h"
#include "console_task.h"
#include "task_message.h"

TaskHandle_t lorawan_task_handle;
QueueHandle_t LoRaWANTaskQueue;

uint8_t psLmDataBuffer[LM_BUFFER_SIZE];
LmHandlerAppData_t LmAppData;

static LmHandlerParams_t LmParameters;
static LmHandlerCallbacks_t LmCallbacks;

static LmhpComplianceParams_t LmComplianceParams;

static volatile bool TransmitPending = false;
static volatile bool MacProcessing = false;
static volatile bool ClockSynchronized = false;
static volatile bool McSessionStarted = false;

/*
 * Board ID is called by the LoRaWAN stack to
 * uniquely identify this device.
 * 
 * This example uses 4 bytes from the processor ID
 * and another 4 bytes that are user-defined. 
 */
void BoardGetUniqueId(uint8_t *id)
{
  am_util_id_t i;

  am_util_id_device(&i);

  id[0] = 0x01;
  id[1] = 0x02;
  id[2] = 0x03;
  id[3] = 0x04;
  id[4] = (uint8_t) (i.sMcuCtrlDevice.ui32ChipID0);
  id[5] = (uint8_t) (i.sMcuCtrlDevice.ui32ChipID0 >> 8);
  id[6] = (uint8_t) (i.sMcuCtrlDevice.ui32ChipID0 >> 16);
  id[7] = (uint8_t) (i.sMcuCtrlDevice.ui32ChipID0 >> 24);
}

static void TclProcessCommand(LmHandlerAppData_t *appData)
{
	// Only handle the reset command as that is indicative of the
	// beginning of compliance testing.  All the other compliance test
	// commands are handle by the Compliance state machine
    switch(appData->Buffer[0])
    {
    case 0x01:
        am_util_stdio_printf("Tcl: LoRaWAN MAC layer reset requested\r\n");
    	break;
    case 0x05:
        am_util_stdio_printf("Tcl: Duty cycle set to %d\r\n", appData->Buffer[1]);
        break;
    }
}

/*
 * LoRaMAC Application Layer Callbacks
 */
static void OnClassChange(DeviceClass_t deviceClass)
{
    DisplayClassUpdate(deviceClass);
    switch (deviceClass)
    {
    default:
    case CLASS_A:
    {
    	McSessionStarted = false;
    }
    	break;
    case CLASS_B:
    {
    	LmHandlerAppData_t appData = { .Buffer = NULL, .BufferSize = 0, .Port = 0, };
    	LmHandlerSend(&appData, LORAMAC_HANDLER_UNCONFIRMED_MSG);
    	McSessionStarted = true;
    }
    	break;
    case CLASS_C:
    {
    	McSessionStarted = true;
    }
    	break;
    }
}

static void OnMacProcess(void)
{
    // this is called inside an IRQ
    MacProcessing = true;
}

static void OnJoinRequest(LmHandlerJoinParams_t *params)
{
    am_util_stdio_printf("\r\n");
    DisplayJoinRequestUpdate(params);

    if (params->Status == LORAMAC_HANDLER_ERROR) {
        LmHandlerJoin();
    } else {
        am_util_stdio_printf("LoRaWAN join successful\r\n\r\n");

        LmAppData.Port = LM_APPLICATION_PORT;
        LmAppData.BufferSize = 0;
        LmAppData.Buffer = psLmDataBuffer;

        task_message_t TaskMessage;
        TaskMessage.ui32Event = SEND;
        TaskMessage.psContent = &LmAppData;
        xQueueSend(LoRaWANTaskQueue, &TaskMessage, portMAX_DELAY);
    }

    nm_console_print_prompt();
}

static void OnMacMlmeRequest(LoRaMacStatus_t status, MlmeReq_t *mlmeReq,
                             TimerTime_t nextTxDelay)
{
    am_util_stdio_printf("\r\n");
    DisplayMacMlmeRequestUpdate(status, mlmeReq, nextTxDelay);
    nm_console_print_prompt();
}

static void OnMacMcpsRequest(LoRaMacStatus_t status, McpsReq_t *mcpsReq,
                             TimerTime_t nextTxDelay)
{
    am_util_stdio_printf("\r\n");
    DisplayMacMcpsRequestUpdate(status, mcpsReq, nextTxDelay);
    nm_console_print_prompt();
}

static void OnNetworkParametersChange(CommissioningParams_t *params)
{
    am_util_stdio_printf("\r\n");
    DisplayNetworkParametersUpdate(params);
    nm_console_print_prompt();
}

static void OnRxData(LmHandlerAppData_t *appData, LmHandlerRxParams_t *params)
{
    am_util_stdio_printf("\r\n");
    DisplayRxUpdate(appData, params);
    nm_console_print_prompt();

    switch (appData->Port) {
    case 0:
        am_util_stdio_printf("MAC command received\r\n");
    	break;

    case 3:
        if (appData->BufferSize == 1) {
            switch (appData->Buffer[0]) {
            case 0:
                LmHandlerRequestClass(CLASS_A);
                break;
            case 1:
                LmHandlerRequestClass(CLASS_B);
                break;
            case 2:
                LmHandlerRequestClass(CLASS_C);
                break;
            default:
                break;
            }
        }
        break;

    case LM_APPLICATION_PORT:
    {
    }
        break;

    case 224:
    	TclProcessCommand(appData);
    	break;
    }
}

static void OnSysTimeUpdate( bool isSynchronized, int32_t timeCorrection )
{
	ClockSynchronized = isSynchronized;
}

static void OnTxData(LmHandlerTxParams_t *params)
{
    am_util_stdio_printf("\r\n");
    DisplayTxUpdate(params);
    nm_console_print_prompt();
}

static void lorawan_handle_uplink()
{
	if (TransmitPending) {
		if (LmHandlerIsBusy() == true) {
			return;
		}

		TransmitPending = false;

	    LmAppData.Port = LM_APPLICATION_PORT;

		LmHandlerSend(&LmAppData, LORAMAC_HANDLER_UNCONFIRMED_MSG);
    }
}

static void lorawan_handle_command()
{
    task_message_t TaskMessage;

    // do not block on message receive as the LoRa MAC state machine decides
    // when it is appropriate to sleep.  We also do not explicitly go to
    // sleep directly and simply do a task yield.  This allows other timing
    // critical radios such as BLE to run their state machines.
    if (xQueueReceive(LoRaWANTaskQueue, &TaskMessage, 0) == pdPASS) {
        switch (TaskMessage.ui32Event) {
        case JOIN:
            LmHandlerJoin();
            break;
        case SEND:
            TransmitPending = true;
            break;
        }
    }
}

static void lorawan_setup()
{
    BoardInitMcu();
    BoardInitPeriph();

    LmParameters.Region = LORAMAC_REGION_US915;
    LmParameters.AdrEnable = true;
    LmParameters.TxDatarate = DR_3;
    LmParameters.PublicNetworkEnable = true;
    LmParameters.DataBufferMaxSize = LM_BUFFER_SIZE;
    LmParameters.DataBuffer = psLmDataBuffer;

    switch (LmParameters.Region) {
    case LORAMAC_REGION_EU868:
    case LORAMAC_REGION_RU864:
    case LORAMAC_REGION_CN779:
        LmParameters.DutyCycleEnabled = true;
        break;
    default:
        LmParameters.DutyCycleEnabled = false;
        break;
    }

    memset(&LmCallbacks, 0, sizeof(LmHandlerCallbacks_t));
    // these are mandatory
    LmCallbacks.OnMacProcess = OnMacProcess;
    LmCallbacks.OnJoinRequest = OnJoinRequest;
    LmCallbacks.OnNetworkParametersChange = OnNetworkParametersChange;
    LmCallbacks.OnMacMlmeRequest = OnMacMlmeRequest;
    LmCallbacks.OnMacMcpsRequest = OnMacMcpsRequest;
    LmCallbacks.OnSysTimeUpdate = OnSysTimeUpdate;
    LmCallbacks.OnTxData = OnTxData;
    LmCallbacks.OnRxData = OnRxData;
    LmCallbacks.OnClassChange = OnClassChange;

    LmHandlerInit(&LmCallbacks, &LmParameters);
    LmHandlerSetSystemMaxRxError(20);

    LmHandlerPackageRegister(PACKAGE_ID_COMPLIANCE,
                             &LmComplianceParams);

    LmHandlerPackageRegister(PACKAGE_ID_CLOCK_SYNC,
                             NULL);

    LmHandlerPackageRegister(PACKAGE_ID_REMOTE_MCAST_SETUP,
                             NULL);
}

void lorawan_task(void *pvParameters)
{
    FreeRTOS_CLIRegisterCommand(&LoRaWANCommandDefinition);
    LoRaWANTaskQueue = xQueueCreate(10, sizeof(task_message_t));

    am_util_stdio_printf("\r\n\r\nLoRaWAN Application State Machine Started\r\n\r\n");
    nm_console_print_prompt();

    lorawan_setup();

    TransmitPending = false;
    while (1) {
        lorawan_handle_command();
        lorawan_handle_uplink();
        LmHandlerProcess();

        if (MacProcessing) {
            taskENTER_CRITICAL();
            MacProcessing = 0;
            taskEXIT_CRITICAL();
        } else {
            taskYIELD();
        }
    }
}


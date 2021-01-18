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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <am_bsp.h>
#include <am_mcu_apollo.h>
#include <am_util.h>

#include <LmHandler.h>
#include <LmHandlerMsgDisplay.h>
#include <LmhpClockSync.h>
#include <LmhpCompliance.h>
#include <LmhpRemoteMcastSetup.h>
#include <board.h>

#include "lorawan.h"
#include "lorawan_cli.h"
#include "lorawan_config.h"
#include "task_message.h"

#define LORAWAN_EVENT_JOIN  0x01

TaskHandle_t  lorawan_task_handle;
QueueHandle_t lorawan_task_queue;

static QueueHandle_t lorawan_transmit_queue;

uint8_t AppDataBuffer[LORAWAN_APP_DATA_BUFFER_MAX_SIZE];

static volatile uint8_t IsMacProcessPending = 0;
static volatile uint8_t IsTxFramePending    = 0;

static LmHandlerCallbacks_t   LmHandlerCallbacks;
static LmHandlerParams_t      LmHandlerParams;
static LmhpComplianceParams_t LmhpComplianceParams;
static LmHandlerAppData_t     LmHandlerAppData;

static void lorawan_handler();

static void UplinkProcess(void);

static void OnMacProcessNotify(void);
static void OnNvmDataChange(LmHandlerNvmContextStates_t state, uint16_t size);
static void OnNetworkParametersChange(CommissioningParams_t *params);
static void OnMacMcpsRequest(LoRaMacStatus_t status, McpsReq_t *mcpsReq,
                             TimerTime_t nextTxIn);
static void OnMacMlmeRequest(LoRaMacStatus_t status, MlmeReq_t *mlmeReq,
                             TimerTime_t nextTxIn);
static void OnJoinRequest(LmHandlerJoinParams_t *params);
static void OnTxData(LmHandlerTxParams_t *params);
static void OnRxData(LmHandlerAppData_t *appData, LmHandlerRxParams_t *params);
static void OnClassChange(DeviceClass_t deviceClass);
static void OnBeaconStatusChange(LoRaMAcHandlerBeaconParams_t *params);
static void OnSysTimeUpdate(bool isSynchronized, int32_t timeCorrection);

void BoardGetUniqueId(uint8_t *id)
{
    am_util_id_t i;

    am_util_id_device(&i);
    id[0] = 0x01;
    id[1] = 0x02;
    id[2] = 0x03;
    id[3] = 0x04;
    id[4] = (uint8_t)(i.sMcuCtrlDevice.ui32ChipID0);
    id[5] = (uint8_t)(i.sMcuCtrlDevice.ui32ChipID0 >> 8);
    id[6] = (uint8_t)(i.sMcuCtrlDevice.ui32ChipID0 >> 16);
    id[7] = (uint8_t)(i.sMcuCtrlDevice.ui32ChipID0 >> 24);
}

void lorawan_join()
{
    task_message_t task_message;
    task_message.ui32Event = LORAWAN_EVENT_JOIN;
    xQueueSend(lorawan_task_queue, &task_message, portMAX_DELAY);
}

void lorawan_send(lorawan_transaction_t *transaction)
{
    xQueueSend(lorawan_transmit_queue, transaction, portMAX_DELAY);
}

static void lorawan_setup()
{
    BoardInitMcu();
    BoardInitPeriph();

    LmHandlerCallbacks.GetBatteryLevel           = BoardGetBatteryLevel;
    LmHandlerCallbacks.GetTemperature            = NULL;
    LmHandlerCallbacks.GetRandomSeed             = BoardGetRandomSeed;
    LmHandlerCallbacks.OnMacProcess              = OnMacProcessNotify;
    LmHandlerCallbacks.OnNvmDataChange           = OnNvmDataChange;
    LmHandlerCallbacks.OnNetworkParametersChange = OnNetworkParametersChange;
    LmHandlerCallbacks.OnMacMcpsRequest          = OnMacMcpsRequest;
    LmHandlerCallbacks.OnMacMlmeRequest          = OnMacMlmeRequest;
    LmHandlerCallbacks.OnJoinRequest             = OnJoinRequest;
    LmHandlerCallbacks.OnTxData                  = OnTxData;
    LmHandlerCallbacks.OnRxData                  = OnRxData;
    LmHandlerCallbacks.OnClassChange             = OnClassChange;
    LmHandlerCallbacks.OnBeaconStatusChange      = OnBeaconStatusChange;
    LmHandlerCallbacks.OnSysTimeUpdate           = OnSysTimeUpdate;

    LmHandlerParams.Region              = ACTIVE_REGION;
    LmHandlerParams.AdrEnable           = LORAWAN_ADR_STATE;
    LmHandlerParams.TxDatarate          = LORAWAN_DEFAULT_DATARATE;
    LmHandlerParams.PublicNetworkEnable = LORAWAN_PUBLIC_NETWORK;
    LmHandlerParams.DutyCycleEnabled    = LORAWAN_DUTYCYCLE_ON;
    LmHandlerParams.DataBufferMaxSize   = LORAWAN_APP_DATA_BUFFER_MAX_SIZE;
    LmHandlerParams.DataBuffer          = AppDataBuffer;

    LmhpComplianceParams.AdrEnabled       = LORAWAN_ADR_STATE;
    LmhpComplianceParams.DutyCycleEnabled = LORAWAN_DUTYCYCLE_ON;
    LmhpComplianceParams.StopPeripherals  = NULL;
    LmhpComplianceParams.StartPeripherals = NULL;

    LmHandlerAppData.Buffer     = AppDataBuffer;
    LmHandlerAppData.BufferSize = 0;
    LmHandlerAppData.Port       = 0;

    LmHandlerInit(&LmHandlerCallbacks, &LmHandlerParams);
    LmHandlerSetSystemMaxRxError(20);
    LmHandlerPackageRegister(PACKAGE_ID_COMPLIANCE, &LmhpComplianceParams);
}

void lorawan_task(void *pvParameters)
{
    FreeRTOS_CLIRegisterCommand(&LoRaWANCommandDefinition);
    lorawan_task_queue = xQueueCreate(10, sizeof(task_message_t));
    lorawan_transmit_queue = xQueueCreate(10, sizeof(lorawan_transaction_t));

    lorawan_setup();

    while (1) {
        lorawan_handler();
        LmHandlerProcess();
        UplinkProcess();

        taskENTER_CRITICAL();
        if (IsMacProcessPending) {
            IsMacProcessPending = 0;
        }
        else
        {
        	taskYIELD();
        }
        taskEXIT_CRITICAL();
    }
}

static void OnMacProcessNotify(void)
{
    IsMacProcessPending = 1;
}

static void OnNvmDataChange(LmHandlerNvmContextStates_t state, uint16_t size)
{
    DisplayNvmDataChange(state, size);
}

static void OnNetworkParametersChange(CommissioningParams_t *params)
{
    DisplayNetworkParametersUpdate(params);
}

static void OnMacMcpsRequest(LoRaMacStatus_t status, McpsReq_t *mcpsReq,
                             TimerTime_t nextTxIn)
{
    DisplayMacMcpsRequestUpdate(status, mcpsReq, nextTxIn);
}

static void OnMacMlmeRequest(LoRaMacStatus_t status, MlmeReq_t *mlmeReq,
                             TimerTime_t nextTxIn)
{
    DisplayMacMlmeRequestUpdate(status, mlmeReq, nextTxIn);
}

static void OnJoinRequest(LmHandlerJoinParams_t *params)
{
    DisplayJoinRequestUpdate(params);
    if (params->Status == LORAMAC_HANDLER_ERROR) {
        LmHandlerJoin();
    } else {
        LmHandlerRequestClass(LORAWAN_DEFAULT_CLASS);
    }
}

static void OnTxData(LmHandlerTxParams_t *params)
{
    DisplayTxUpdate(params);
}

static void OnRxData(LmHandlerAppData_t *appData, LmHandlerRxParams_t *params)
{
    DisplayRxUpdate(appData, params);

    switch (appData->Port) {
    case LORAWAN_APP_PORT:
        break;
    default:
        break;
    }
}

static void OnClassChange(DeviceClass_t deviceClass)
{
    DisplayClassUpdate(deviceClass);

    LmHandlerAppData_t appData = {.Buffer = NULL, .BufferSize = 0, .Port = 0};
    LmHandlerSend(&appData, LORAMAC_HANDLER_UNCONFIRMED_MSG);
}

static void OnBeaconStatusChange(LoRaMAcHandlerBeaconParams_t *params)
{
    switch (params->State) {
    case LORAMAC_HANDLER_BEACON_RX:
    case LORAMAC_HANDLER_BEACON_LOST:
    case LORAMAC_HANDLER_BEACON_NRX:
    default:
        break;
    }

    DisplayBeaconUpdate(params);
}

static void OnSysTimeUpdate(bool isSynchronized, int32_t timeCorrection)
{
}

static void UplinkProcess(void)
{
    lorawan_transaction_t transaction;
    if (xQueuePeek(lorawan_transmit_queue, &transaction, 0) == pdPASS)
    {
        if (LmHandlerIsBusy() == true)
        {
            return;
        }

        if (xQueueReceive(lorawan_transmit_queue, &transaction, 0) == pdPASS)
        {
            LmHandlerAppData.Port = transaction.port;
            LmHandlerAppData.BufferSize = transaction.length;
            LmHandlerAppData.Buffer = transaction.buffer;

            LmHandlerSend(&LmHandlerAppData, transaction.message_type);
        }
    }
}

static void lorawan_handler()
{
    task_message_t task_message;

    // do not block on message receive as the LoRa MAC state machine decides
    // when it is appropriate to sleep.  We also do not explicitly go to
    // sleep directly and simply do a task yield.  This allows other timing
    // critical radios such as BLE to run their state machines.
    if (xQueueReceive(lorawan_task_queue, &task_message, 0) == pdPASS) {
        switch (task_message.ui32Event) {
        case LORAWAN_EVENT_JOIN:
            LmHandlerJoin();
            break;
        }
    }
}



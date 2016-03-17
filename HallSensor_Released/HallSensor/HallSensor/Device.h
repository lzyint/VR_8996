/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    device.h

Abstract:

    This file contains the device definitions.

Environment:

    User-mode Driver Framework 2.0

--*/

#include <wdm.h>
#include <wdf.h>
#include <SmartCoverConsts.h>

#define COVER_STATE_MONITOR_PERIOD_MS 2000 /* 2s */
#define GPIO_PIN_OPERATION_TIMEOUT_MS 500 /* 500ms */

#define COVER_STATE 0x00
#define UNCOVER_STATE 0x01

//
// The device context performs the same job as
// a WDM device extension in the driver frameworks
//
typedef struct _DEVICE_CONTEXT
{
	WDFDEVICE pDevice;
	UNICODE_STRING ResourceString;
	WDFIOTARGET pGpioIoTarget;
	KEVENT hCoverStateMonitorCloseEvent;
	SHELL_COVER_STATE CoverState;
} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

//
// This macro will generate an inline function called GetDeviceContext
// which will be used to get a pointer to the device context memory
// in a type safe manner.
//
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, GetDeviceContext)

EVT_WDF_DRIVER_DEVICE_ADD OnEvtDeviceAdd;
EVT_WDF_DEVICE_PREPARE_HARDWARE OnEvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE OnEvtDeviceReleaseHardware;
EVT_WDF_DEVICE_D0_ENTRY OnEvtDeviceD0Entry;
EVT_WDF_DEVICE_D0_EXIT OnEvtDeviceD0Exit;
EVT_WDF_INTERRUPT_WORKITEM  OnInterruptWorkItem;
EVT_WDF_INTERRUPT_ISR OnEvtInterruptIsr;
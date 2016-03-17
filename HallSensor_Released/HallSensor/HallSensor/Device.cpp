/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    Device.c - Device handling events for example driver.

Abstract:

   This file contains the device entry points and callbacks.
    
Environment:

    User-mode Driver Framework 2.0

--*/

#include "Device.h"
#include "Trace.h"
#include <strsafe.h>
//#include <stdlib.h>
#include <gpio.h>
#define RESHUB_USE_HELPER_ROUTINES
#include <reshub.h>
#include <SmartCoverWrapperWNF.h>
#include "Device.tmh"

NTSTATUS OpenGpioIoTarget(_In_ PDEVICE_CONTEXT pDeviceContext, _In_ WDFCMRESLIST pResourcesTranslated);
NTSTATUS ReadGpioIo(_In_ PDEVICE_CONTEXT pDeviceContext, _Out_ BYTE *pReadData);
NTSTATUS WriteGpioIo(_In_ PDEVICE_CONTEXT pDeviceContext, _In_ BYTE WriteData);
NTSTATUS UpdateCoverState(_In_ PDEVICE_CONTEXT pDeviceContext);
BOOLEAN OnEvtInterruptIsr(_In_ WDFINTERRUPT Interrupt, _In_ ULONG MessageID);
VOID OnInterruptWorkItem(_In_ WDFINTERRUPT WdfInterrupt, _In_ WDFOBJECT WdfDevice);

NTSTATUS OnEvtDeviceAdd(
	_In_ WDFDRIVER pDriver,
	_Inout_ PWDFDEVICE_INIT pDeviceInit)
	/*++
	Routine Description:

	EvtDeviceAdd is called by the framework in response to AddDevice
	call from the PnP manager. We create and initialize a device object to
	represent a new instance of the device.

	Arguments:

	Driver - Handle to a framework driver object created in DriverEntry

	DeviceInit - Pointer to a framework-allocated WDFDEVICE_INIT structure.

	Return Value:

	NTSTATUS

	--*/
{
	NTSTATUS Status = STATUS_SUCCESS;
	WDFDEVICE pDevice = NULL;
	PDEVICE_CONTEXT pDeviceContext = NULL;
	WDF_OBJECT_ATTRIBUTES ObjectAttributes = { 0 };
	//WDF_WORKITEM_CONFIG WorkitemConfig = { 0 };
	WDF_PNPPOWER_EVENT_CALLBACKS PnpPowerCallbacks = { 0 };

	UNREFERENCED_PARAMETER(pDriver);

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "OnEvtDeviceAdd read only entered!!!!!!\n");
	SENSOR_FunctionEnter();

	//
	// Add PNP and Power callback
	//
	WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&PnpPowerCallbacks);
	PnpPowerCallbacks.EvtDevicePrepareHardware = OnEvtDevicePrepareHardware;
	PnpPowerCallbacks.EvtDeviceReleaseHardware = OnEvtDeviceReleaseHardware;
	PnpPowerCallbacks.EvtDeviceD0Entry = OnEvtDeviceD0Entry;
	PnpPowerCallbacks.EvtDeviceD0Exit = OnEvtDeviceD0Exit;

	WdfDeviceInitSetPnpPowerEventCallbacks(pDeviceInit, &PnpPowerCallbacks);

	//
	// Allocate device context and create device object.
	//
	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&ObjectAttributes, DEVICE_CONTEXT);

	Status = WdfDeviceCreate(&pDeviceInit,
		&ObjectAttributes,
		&pDevice);
	if (!NT_SUCCESS(Status))
	{
		TraceError("%!FUNC! WdfDeviceCreate failed with %!STATUS!.", Status);
		goto Exit;
	}

	pDeviceContext = GetDeviceContext(pDevice);
	if (NULL == pDeviceContext)
	{
		TraceError("%!FUNC! Invalid device context.");
		Status = STATUS_UNSUCCESSFUL;
		goto Exit;
	}

	//
	// Initialize the device context.
	//
	memset(pDeviceContext, 0, sizeof(DEVICE_CONTEXT));

	pDeviceContext->pDevice = pDevice;
	pDeviceContext->CoverState = SHELL_COVER_STATE_UNKNOWN;

	KeInitializeEvent(&pDeviceContext->hCoverStateMonitorCloseEvent, SynchronizationEvent, FALSE);

	//
	// Create cover state monitor workitem.
	//
	WDF_OBJECT_ATTRIBUTES_INIT(&ObjectAttributes);
	ObjectAttributes.ParentObject = pDevice;

Exit:
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "OnEvtDeviceAdd exit with 0x%lx!!!!!!\n", Status);
	SENSOR_FunctionExit(Status);
	return Status;
}

NTSTATUS OnEvtDevicePrepareHardware(
	_In_  WDFDEVICE pDevice,
	_In_  WDFCMRESLIST pResourcesRaw,
	_In_  WDFCMRESLIST pResourcesTranslated)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PDEVICE_CONTEXT pDeviceContext = NULL;

	UNREFERENCED_PARAMETER(pResourcesRaw);

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "OnEvtDevicePrepareHardware entered!!!!!!\n");
	SENSOR_FunctionEnter();

	if ((NULL == pDevice) || (NULL == pResourcesTranslated))
	{
		TraceError("%!FUNC! Invalid parameters.");
		Status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	pDeviceContext = GetDeviceContext(pDevice);
	if (NULL == pDeviceContext)
	{
		TraceError("%!FUNC! Invalid device context.");
		Status = STATUS_UNSUCCESSFUL;
		goto Exit;
	}
	pDeviceContext->pDevice = pDevice;

	Status = OpenGpioIoTarget(pDeviceContext, pResourcesTranslated);
	if (!NT_SUCCESS(Status))
	{
		TraceError("%!FUNC! Failed to open GPIO I/O target with %!STATUS!.", Status);
		goto Exit;
	}

Exit:
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "OnEvtDevicePrepareHardware exit with 0x%lx!!!!!!\n", Status);
	SENSOR_FunctionExit(Status);
	return Status;
}

NTSTATUS OnEvtDeviceReleaseHardware(
	_In_  WDFDEVICE pDevice,
	_In_  WDFCMRESLIST pResourcesTranslated)
{
	NTSTATUS Status = STATUS_SUCCESS;
	
	UNREFERENCED_PARAMETER(pDevice);
	UNREFERENCED_PARAMETER(pResourcesTranslated);

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "OnEvtDeviceReleaseHardware entered!!!!!!\n");
	SENSOR_FunctionEnter();

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "OnEvtDeviceReleaseHardware exit with 0x%lx!!!!!!\n", Status);
	SENSOR_FunctionExit(Status);
	return Status;
}

NTSTATUS OnEvtDeviceD0Entry(
	_In_ WDFDEVICE pDevice,
	_In_ WDF_POWER_DEVICE_STATE PreviousState)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PDEVICE_CONTEXT pDeviceContext = NULL;

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "OnEvtDeviceD0Entry entered!!!!!!\n");
	SENSOR_FunctionEnter();

	if (NULL == pDevice)
	{
		TraceError("%!FUNC! Invalid parameter.");
		Status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	pDeviceContext = GetDeviceContext(pDevice);
	if (NULL == pDevice)
	{
		TraceError("%!FUNC! Invalid device context.");
		Status = STATUS_UNSUCCESSFUL;
		goto Exit;
	}

	TraceInformation("%!FUNC! Device is coming from %d state.", PreviousState);

Exit:
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "OnEvtDeviceD0Entry exit with 0x%lx!!!!!!\n", Status);
	SENSOR_FunctionExit(Status);
	return Status;
}

NTSTATUS OnEvtDeviceD0Exit(
	_In_ WDFDEVICE pDevice,
	_In_ WDF_POWER_DEVICE_STATE TargetState)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PDEVICE_CONTEXT pDeviceContext = NULL;

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "OnEvtDeviceD0Exit entered!!!!!!\n");
	SENSOR_FunctionEnter();

	if (NULL == pDevice)
	{
		TraceError("%!FUNC! Invalid parameter.");
		Status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	pDeviceContext = GetDeviceContext(pDevice);
	if (NULL == pDevice)
	{
		TraceError("%!FUNC! Invalid device context.");
		Status = STATUS_UNSUCCESSFUL;
		goto Exit;
	}

	TraceInformation("%!FUNC! Device is going to %d state.", TargetState);

	KeSetEvent(&pDeviceContext->hCoverStateMonitorCloseEvent, 0, TRUE);

Exit:
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "OnEvtDeviceD0Exit exit with 0x%lx!!!!!!\n", Status);
	SENSOR_FunctionExit(Status);
	return Status;
}

NTSTATUS OpenGpioIoTarget(
	_In_ PDEVICE_CONTEXT pDeviceContext,
	_In_ WDFCMRESLIST pResourcesTranslated)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PCM_PARTIAL_RESOURCE_DESCRIPTOR pDescriptor = NULL;
	PCM_PARTIAL_RESOURCE_DESCRIPTOR pDescriptorRaw = NULL;
	ULONG ResourceCount = 0;
	ULONG ResourceIndex = 0;

	ULONG InterruptResourceCount = 0;
	WDFINTERRUPT WdfInterrupt;
	WDF_INTERRUPT_CONFIG InterruptConfiguration;

	ULONG GpioIoConnectionResourceCount = 0;
	LARGE_INTEGER GpioIoConnectionId = { 0 };
	WDF_OBJECT_ATTRIBUTES ObjectAttributes = { 0 };
	WDF_IO_TARGET_OPEN_PARAMS OpenParams = { 0 };
	//HRESULT rc = S_OK;
	DECLARE_UNICODE_STRING_SIZE(ResourceString, RESOURCE_HUB_PATH_SIZE);

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "OpenGpioIoTarget entered!!!!!!\n");
	SENSOR_FunctionEnter();

	if ((NULL == pDeviceContext) || (NULL == pResourcesTranslated))
	{
		TraceError("%!FUNC! Invalid parameters.");
		Status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	//
	// Get HW resource from ACPI and set up IO Target
	//

	// Walk Resources - verify we have I2C
	ResourceCount = WdfCmResourceListGetCount(pResourcesTranslated);
	DbgPrintEx(0, 0, "ResourceCount %d.\n", ResourceCount);

	for (ResourceIndex = 0; ResourceIndex < ResourceCount; ResourceIndex++)
	{
		pDescriptorRaw = WdfCmResourceListGetDescriptor(pResourcesTranslated, ResourceIndex);
		pDescriptor = WdfCmResourceListGetDescriptor(pResourcesTranslated, ResourceIndex);

		switch (pDescriptor->Type)
		{
			// Check we have I2C bus assigned in ACPI
			case CmResourceTypeConnection:
			{
				DbgPrintEx(0, 0, "Hall %!FUNC! CmResourceTypeConnectione found.\n");

				if ((CM_RESOURCE_CONNECTION_CLASS_GPIO == pDescriptor->u.Connection.Class)
					&& (CM_RESOURCE_CONNECTION_TYPE_GPIO_IO == pDescriptor->u.Connection.Type))
				{
					GpioIoConnectionId.LowPart = pDescriptor->u.Connection.IdLowPart;
					GpioIoConnectionId.HighPart = pDescriptor->u.Connection.IdHighPart;

					GpioIoConnectionResourceCount += 1;
				}

				break;
			}
			case CmResourceTypeInterrupt:
			{
				InterruptResourceCount = +1;
				DbgPrintEx(0, 0, "Hall %!FUNC! GPIO interrupt resource found.\n");
				WDF_INTERRUPT_CONFIG_INIT(&InterruptConfiguration, OnEvtInterruptIsr, NULL);
				InterruptConfiguration.InterruptRaw = pDescriptorRaw;
				InterruptConfiguration.InterruptTranslated = pDescriptor;
				// Configure an interrupt work item which runs at IRQL = PASSIVE_LEVEL
				// Note: to configure to run at IRQL = DISPATCH_LEVEL, set up an InterruptDpc instead of an InterruptWorkItem
				InterruptConfiguration.EvtInterruptWorkItem = OnInterruptWorkItem;
				InterruptConfiguration.PassiveHandling = TRUE;
				Status = WdfInterruptCreate(pDeviceContext->pDevice, &InterruptConfiguration, WDF_NO_OBJECT_ATTRIBUTES, &WdfInterrupt);
				if (!NT_SUCCESS(Status))
				{
					TraceError("Hall %!FUNC! WdfInterruptCreate failed %!STATUS!", Status);
				}

				DbgPrintEx(0, 0, "%!FUNC! HW Interrupt Number: = %X \n", (ULONG)pDescriptor->u.Interrupt.Vector);
				break;
			}

			default:
				break;
		}
	}

	if (1 != GpioIoConnectionResourceCount)
	{
		Status = STATUS_UNSUCCESSFUL;
		TraceError("%!FUNC! Did not find GPIO resource with %!STATUS!.", Status);
		goto Exit;
	}

	// Set up GPIO I/O Target. To be used to issue GPIO write transfers.
	WDF_OBJECT_ATTRIBUTES_INIT(&ObjectAttributes);
	ObjectAttributes.ParentObject = pDeviceContext->pDevice;

	pDeviceContext->pGpioIoTarget = NULL;
	Status = WdfIoTargetCreate(pDeviceContext->pDevice,
		&ObjectAttributes,
		&pDeviceContext->pGpioIoTarget);

	if (!NT_SUCCESS(Status))
	{
		TraceError("%!FUNC! GPIO WdfIoTargetCreate failed with %!STATUS!.", Status);
		goto Exit;
	}

	RESOURCE_HUB_CREATE_PATH_FROM_ID(&ResourceString, GpioIoConnectionId.LowPart, GpioIoConnectionId.HighPart);

	

	// Connect to GPIO I/O Target
	WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(&OpenParams,
		&ResourceString,
		GENERIC_READ);

	Status = WdfIoTargetOpen(pDeviceContext->pGpioIoTarget, &OpenParams);
	if (!NT_SUCCESS(Status))
	{
		TraceError("%!FUNC! GPIO WdfIoTargetOpen failed with %!STATUS!.", Status);
		goto Exit;
	}

Exit:
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "OpenGpioIoTarget exit with 0x%lx!!!!!!\n", Status);
	SENSOR_FunctionExit(Status);
	return Status;
}

NTSTATUS ReadGpioIo(
	_In_ PDEVICE_CONTEXT pDeviceContext,
	_Out_ BYTE *pReadData)
{
	NTSTATUS Status = STATUS_SUCCESS;
	WDF_OBJECT_ATTRIBUTES ObjectAttributes = { 0 };
	WDF_REQUEST_SEND_OPTIONS SendOptions = { 0 };
	WDFREQUEST pIoctlRequest = NULL;
	WDFMEMORY pMemory = NULL;
	BYTE ReadData = 0;

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "ReadGpioIo entered!!!!!!\n");
	SENSOR_FunctionEnter();

	if ((NULL == pDeviceContext) || (NULL == pReadData))
	{
		TraceError("%!FUNC! Invalid parameters.");
		Status = STATUS_UNSUCCESSFUL;
		goto Exit;
	}

	WDF_OBJECT_ATTRIBUTES_INIT(&ObjectAttributes);
	Status = WdfRequestCreate(&ObjectAttributes,
		pDeviceContext->pGpioIoTarget,
		&pIoctlRequest);
	if (!NT_SUCCESS(Status))
	{
		TraceError("%!FUNC! GPIO WdfRequestCreate failed with %!STATUS!.", Status);
		goto Exit;
	}
	// Set up a WDF memory object for the IOCTL request
	WDF_OBJECT_ATTRIBUTES_INIT(&ObjectAttributes);
	ObjectAttributes.ParentObject = pIoctlRequest;
	Status = WdfMemoryCreatePreallocated(&ObjectAttributes,
		&ReadData,
		sizeof(ReadData),
		&pMemory);
	if (!NT_SUCCESS(Status))
	{
		TraceError("%!FUNC! GPIO WdfMemoryCreatePreallocated failed with %!STATUS!.", Status);
		goto Exit;
	}

	Status = WdfIoTargetFormatRequestForIoctl(pDeviceContext->pGpioIoTarget,
		pIoctlRequest,
		IOCTL_GPIO_READ_PINS,
		NULL,
		0,
		pMemory,
		0);
	if (!NT_SUCCESS(Status))
	{
		TraceError("%!FUNC! GPIO WdfIoTargetFormatRequestForIoctl failed with %!STATUS!.", Status);
		goto Exit;
	}

	WDF_REQUEST_SEND_OPTIONS_INIT(&SendOptions,
		WDF_REQUEST_SEND_OPTION_SYNCHRONOUS);

	WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&SendOptions,
		WDF_REL_TIMEOUT_IN_MS(GPIO_PIN_OPERATION_TIMEOUT_MS));

	if (!WdfRequestSend(pIoctlRequest, pDeviceContext->pGpioIoTarget, &SendOptions))
	{
		Status = WdfRequestGetStatus(pIoctlRequest);
		if (!NT_SUCCESS(Status))
		{
			TraceError("%!FUNC! GPIO WdfRequestSend synchronously failed with %!STATUS!.", Status);
			goto Exit;
		}
	}

	*pReadData = ReadData;

Exit:
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "ReadGpioIo exit with 0x%lx!!!!!!\n", Status);
	SENSOR_FunctionExit(Status);
	return Status;
}

NTSTATUS WriteGpioIo(
	_In_ PDEVICE_CONTEXT pDeviceContext,
	_In_ BYTE WriteData)
{
	NTSTATUS Status = STATUS_SUCCESS;
	WDF_OBJECT_ATTRIBUTES ObjectAttributes = { 0 };
	WDF_REQUEST_SEND_OPTIONS SendOptions = { 0 };
	WDFREQUEST pIoctlRequest = NULL;
	WDFMEMORY pMemory = NULL;

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "WriteGpioIo entered!!!!!!\n");
	SENSOR_FunctionEnter();

	if (NULL == pDeviceContext)
	{
		TraceError("%!FUNC! Invalid parameter.");
		Status = STATUS_UNSUCCESSFUL;
		goto Exit;
	}

	WDF_OBJECT_ATTRIBUTES_INIT(&ObjectAttributes);
	Status = WdfRequestCreate(&ObjectAttributes,
		pDeviceContext->pGpioIoTarget,
		&pIoctlRequest);
	if (!NT_SUCCESS(Status))
	{
		TraceError("%!FUNC! GPIO WdfRequestCreate failed with %!STATUS!.", Status);
		goto Exit;
	}

	// Set up a WDF memory object for the IOCTL request
	WDF_OBJECT_ATTRIBUTES_INIT(&ObjectAttributes);
	ObjectAttributes.ParentObject = pIoctlRequest;
	Status = WdfMemoryCreatePreallocated(&ObjectAttributes,
		&WriteData,
		sizeof(WriteData),
		&pMemory);
	if (!NT_SUCCESS(Status))
	{
		TraceError("%!FUNC! GPIO WdfMemoryCreatePreallocated failed with %!STATUS!.", Status);
		goto Exit;
	}

	Status = WdfIoTargetFormatRequestForIoctl(pDeviceContext->pGpioIoTarget,
		pIoctlRequest,
		IOCTL_GPIO_WRITE_PINS,
		pMemory,
		0,
		NULL,
		0);
	if (!NT_SUCCESS(Status))
	{
		TraceError("%!FUNC! GPIO WdfIoTargetFormatRequestForIoctl failed with %!STATUS!.", Status);
		goto Exit;
	}

	WDF_REQUEST_SEND_OPTIONS_INIT(&SendOptions,
		WDF_REQUEST_SEND_OPTION_SYNCHRONOUS);

	WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&SendOptions,
		WDF_REL_TIMEOUT_IN_MS(GPIO_PIN_OPERATION_TIMEOUT_MS));

	if (!WdfRequestSend(pIoctlRequest, pDeviceContext->pGpioIoTarget, &SendOptions))
	{
		Status = WdfRequestGetStatus(pIoctlRequest);
		if (!NT_SUCCESS(Status))
		{
			TraceError("%!FUNC! GPIO WdfRequestSend synchronously failed with %!STATUS!.", Status);
			goto Exit;
		}
	}

Exit:
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "WriteGpioIo exit with 0x%lx!!!!!!\n", Status);
	SENSOR_FunctionExit(Status);
	return Status;
}

NTSTATUS UpdateCoverState(
	_In_ PDEVICE_CONTEXT pDeviceContext)
{
	NTSTATUS Status = STATUS_SUCCESS;
	BYTE CoverState = 0;
	BOOLEAN IsUpdate = FALSE;

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "UpdateCoverState entered!!!!!!\n");
	SENSOR_FunctionEnter();

	if (NULL == pDeviceContext)
	{
		TraceError("%!FUNC! Invalid device context.");
		Status = STATUS_UNSUCCESSFUL;
		goto Exit;
	}

	Status = ReadGpioIo(pDeviceContext, &CoverState);
	if (!NT_SUCCESS(Status))
	{
		TraceError("%!FUNC! Failed to read cover state with 0x%lx.", Status);
		goto Exit;
	}

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Current cover sate [%ld], read state [%d]!!!!!!\n", pDeviceContext->CoverState, CoverState);
	TraceInformation("%!FUNC! Current cover sate [%ld], read state [%d].", pDeviceContext->CoverState, CoverState);

	if (0 == CoverState)
	{
		if ((SHELL_COVER_STATE_UNKNOWN == pDeviceContext->CoverState) 
			|| (SHELL_COVER_STATE_OPEN == pDeviceContext->CoverState))
		{
			pDeviceContext->CoverState = SHELL_COVER_STATE_CLOSED;
			IsUpdate = TRUE;
		}
	}
	else
	{
		if ((SHELL_COVER_STATE_UNKNOWN == pDeviceContext->CoverState)
			|| (SHELL_COVER_STATE_CLOSED == pDeviceContext->CoverState))
		{
			pDeviceContext->CoverState = SHELL_COVER_STATE_OPEN;
			IsUpdate = TRUE;
		}
	}

	if (TRUE == IsUpdate)
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Update cover state data to [%ld]!!!!!!\n", pDeviceContext->CoverState);
		TraceInformation("%!FUNC! Update cover state data to [%ld].", pDeviceContext->CoverState);
		Status = CoverPublishStateData(pDeviceContext->CoverState);
		if (!NT_SUCCESS(Status))
		{
			TraceError("%!FUNC! Failed to publish CLOSED cover state data with %!STATUS!.", Status);
			goto Exit;
		}
	}
	else
	{
		TraceInformation("%!FUNC! Cover state not changed.");
	}

Exit:
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "UpdateCoverState exit with 0x%lx!!!!!!\n", Status);
	SENSOR_FunctionExit(Status);
	return Status;
}

BOOLEAN
OnEvtInterruptIsr(
_In_ WDFINTERRUPT Interrupt,
_In_ ULONG MessageID
)

/*++

Routine Description:

This routine is the interrupt service routine for the sample device

N.B. This driver assumes that the interrupt line is not shared with any
other device. Hence it always claims the interrupt.

Arguments:

Interupt - Supplies a handle to interrupt object (WDFINTERRUPT) for this
device.

MessageID - Supplies the MSI message ID for MSI-based interrupts.

Return Value:

Always TRUE.

--*/

{
	UNREFERENCED_PARAMETER(Interrupt);
	UNREFERENCED_PARAMETER(MessageID);

	BOOLEAN result = FALSE;

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "OnEvtInterruptIsr working!!!!!!\n");

	result = WdfInterruptQueueDpcForIsr(Interrupt);
	if (result == FALSE)
	{
		DbgPrintEx(0, 0, "the callback function was previously queued and has not executed.\n");
	}

	return result;
}

VOID
OnInterruptWorkItem(
_In_ WDFINTERRUPT WdfInterrupt,
_In_ WDFOBJECT WdfDevice
)

/*++

Routine Description:

This routine is the DPC callback for the ISR. This routine is unused.

Arguments:

Interupt - Supplies a handle to interrupt object (WDFINTERRUPT) for this
device.

Device - Supplies a handle to the framework device object.

Return Value:

None.

--*/

{
	UNREFERENCED_PARAMETER(WdfInterrupt);
	UNREFERENCED_PARAMETER(WdfDevice);

	NTSTATUS Status = STATUS_SUCCESS;
	PDEVICE_CONTEXT pDeviceContext = NULL;

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "OnInterruptWorkItem working!!!!!!\n");
	SENSOR_FunctionEnter();

	if (NULL == WdfDevice)
	{
		TraceError("%!FUNC! Invalid parameter.");
		Status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	pDeviceContext = GetDeviceContext(WdfDevice);
	if (NULL == pDeviceContext)
	{
		TraceError("%!FUNC! Invalid device context.");
		Status = STATUS_UNSUCCESSFUL;
		goto Exit;
	}

	Status = UpdateCoverState(pDeviceContext);
	if (!NT_SUCCESS(Status))
	{
		TraceError("%!FUNC! Failed to update cover state with %!STATUS!.", Status);
	}

Exit:
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "OnInterruptWorkItem exit with 0x%lx!!!!!!\n", Status);
	SENSOR_FunctionExit(Status);
	return;
}

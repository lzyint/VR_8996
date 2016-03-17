/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    Driver.c

Abstract:

    This file contains the driver entry points and callbacks.

Environment:

    User-mode Driver Framework 2.0

--*/

#include "Driver.h"
#include "Device.h"
#include "Trace.h"
#include "Driver.tmh"


NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:
    DriverEntry initializes the driver and is the first routine called by the
    system after the driver is loaded. DriverEntry specifies the other entry
    points in the function driver, such as EvtDevice and DriverUnload.

Parameters Description:

    DriverObject - represents the instance of the function driver that is loaded
    into memory. DriverEntry must initialize members of DriverObject before it
    returns to the caller. DriverObject is allocated by the system before the
    driver is loaded, and it is released by the system after the system unloads
    the function driver from memory.

    RegistryPath - represents the driver specific path in the Registry.
    The function driver can use the path to store driver related data between
    reboots. The path does not store hardware instance specific data.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise.

--*/
{
    WDF_DRIVER_CONFIG config;
	NTSTATUS Status;
    WDF_OBJECT_ATTRIBUTES attributes;

    //
    // Initialize WPP Tracing
    //
    WPP_INIT_TRACING(DriverObject, NULL);

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "DriverEntry entered!!!!!!\n");
	SENSOR_FunctionEnter();

    //
    // Register a cleanup callback so that we can call WPP_CLEANUP when
    // the framework driver object is deleted during driver unload.
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
	attributes.EvtCleanupCallback = OnEvtDriverContextCleanup;

    WDF_DRIVER_CONFIG_INIT(&config, OnEvtDeviceAdd);

	Status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             &attributes,
                             &config,
                             WDF_NO_HANDLE
                             );

	if (!NT_SUCCESS(Status)) 
	{
		TraceError("%!FUNC! WdfDriverCreate failed with %!STATUS!.", Status);
        WPP_CLEANUP(DriverObject);
		return Status;
    }

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "DriverEntry exit with 0x%lx!!!!!!\n", Status);
	SENSOR_FunctionExit(Status);

	return Status;
}

VOID
OnEvtDriverContextCleanup(
    _In_ WDFOBJECT DriverObject
    )
/*++
Routine Description:

    Free all the resources allocated in DriverEntry.

Arguments:

    DriverObject - handle to a WDF Driver object.

Return Value:

    VOID.

--*/
{
    //UNREFERENCED_PARAMETER(DriverObject);

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "OnEvtDriverContextCleanup called!!!!!!\n");
	SENSOR_FunctionEnter();

	SENSOR_FunctionExit(STATUS_SUCCESS);
    //
    // Stop WPP Tracing
    //
    WPP_CLEANUP(WdfDriverWdmGetDriverObject((WDFDRIVER)DriverObject));
}

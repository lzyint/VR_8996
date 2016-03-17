/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    driver.h

Abstract:

    This file contains the driver definitions.

Environment:

    User-mode Driver Framework 2.0

--*/


#include <wdm.h>
#include <wdf.h>

WDF_EXTERN_C_START
DRIVER_INITIALIZE DriverEntry;
EVT_WDF_OBJECT_CONTEXT_CLEANUP OnEvtDriverContextCleanup;
WDF_EXTERN_C_END
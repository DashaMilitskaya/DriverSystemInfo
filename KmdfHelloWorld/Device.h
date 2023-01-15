
#pragma once
#include <ntddk.h>
#include <wdf.h>
#include "Queue.h"
#include <initguid.h>
#include <Wdmsec.h>
#define NT_STATUS_SUCCESSFUL 0

//
// The device context performs the same job as
// a WDM device extension in the driver frameworks
//
typedef struct _DEVICE_CONTEXT
{
    ULONG PrivateDeviceData;  // just a placeholder

} DEVICE_CONTEXT, * PDEVICE_CONTEXT;

//
// This macro will generate an inline function called DeviceGetContext
// which will be used to get a pointer to the device context memory
// in a type safe manner.
//
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, DeviceGetContext)

//
// Function to initialize the device and its callbacks
//

NTSTATUS SytemCreateControlDevice(WDFDRIVER * Driver);
VOID NonPnpEvtDeviceFileCreate(
    IN WDFDEVICE            Device,
    IN WDFREQUEST Request,
    IN WDFFILEOBJECT        FileObject
);
VOID NonPnpEvtFileClose(
    IN WDFFILEOBJECT    FileObject
);
#pragma once
#include <ntddk.h>
#include <wdf.h>
#include "ioCtlCods.h"
#include "GdtTable.h"
#include "IdtTable.h"
#include "hookSystemCallx64.h"

WDF_EXTERN_C_START

typedef struct _QUEUE_CONTEXT {

    ULONG PrivateDeviceData;  // just a placeholder

} QUEUE_CONTEXT, * PQUEUE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(QUEUE_CONTEXT, QueueGetContext);

NTSTATUS SystemTestQueueInitialize(
    _In_ WDFDEVICE Device
);

//
// Events from the IoQueue object
//
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL SystemTestEvtIoDeviceControl;
EVT_WDF_IO_QUEUE_IO_STOP SystemTestEvtIoStop;
EXTERN_C_END
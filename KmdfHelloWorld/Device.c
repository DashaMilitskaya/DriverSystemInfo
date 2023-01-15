
#include "Device.h"
DECLARE_CONST_UNICODE_STRING(MyDeviceName, L"\\Device\\WdfDevice");
DECLARE_CONST_UNICODE_STRING(ioDeviceName, L"\\??\\MyIoLink");

NTSTATUS SytemCreateControlDevice(
   WDFDRIVER*           Driver
    ) {

    PWDFDEVICE_INIT  deviceInit = NULL;
    NTSTATUS  status;
    WDF_OBJECT_ATTRIBUTES  objectAttribs;
    WDFDEVICE device;
    PDEVICE_CONTEXT deviceContext;

    

    deviceInit = WdfControlDeviceInitAllocate(
        *Driver,
        &SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_RW_RES_R
    );

    if (deviceInit == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        return;
    }

   

    DbgPrint("\n\nWdfControlDeviceInitAllocate \n\n");
    
    status = WdfDeviceInitAssignName(
        deviceInit,
        &MyDeviceName
    );

    if (!NT_SUCCESS(status)) {
        return status;
    }

    DbgPrint("\n\nWdfDeviceInitAssignName \n\n");


    WdfDeviceInitSetIoType(deviceInit, WdfDeviceIoBuffered);
    
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&objectAttribs, DEVICE_CONTEXT);

    WDF_FILEOBJECT_CONFIG fileConfig;
    WDF_FILEOBJECT_CONFIG_INIT(
        &fileConfig,
        NonPnpEvtDeviceFileCreate,
        NonPnpEvtFileClose,
        WDF_NO_EVENT_CALLBACK // not interested in Cleanup
    );

    WdfDeviceInitSetFileObjectConfig(deviceInit,
        &fileConfig,
        WDF_NO_OBJECT_ATTRIBUTES);

    status = WdfDeviceCreate(
        &deviceInit,
        &objectAttribs,
        &device
    );

    if (!NT_SUCCESS(status)) {
        WdfDeviceInitFree(deviceInit);
        deviceInit = NULL;
        return status;
    }
    DbgPrint("\n\nWdfDeviceCreate \n\n");
    /*
    ***
    Create a default I/O queue for the device
    ***
    */
    //TO_DO Create simbolic link not interfase
    deviceContext = DeviceGetContext(device);

    //
    // Initialize the context.
    //
    deviceContext->PrivateDeviceData = 0;

    //
    // Create a device interface so that applications can find and talk
    // to us.
    //

   status = WdfDeviceCreateSymbolicLink(device, &ioDeviceName);
    if (NT_SUCCESS(status)) {
        //
        // Initialize the I/O Package and any Queues
        //
        DbgPrint("\n\n Interface Created");
        status = SystemTestQueueInitialize(device);
    }

    WdfControlFinishInitializing(device);
    
    return status;
}

VOID NonPnpEvtDeviceFileCreate(
    IN WDFDEVICE            Device,
    IN WDFREQUEST Request,
    IN WDFFILEOBJECT        FileObject
) {
    WdfRequestComplete(Request, NT_STATUS_SUCCESSFUL);
    return;
}


VOID
NonPnpEvtFileClose(
    IN WDFFILEOBJECT    FileObject
) {
    return;
}
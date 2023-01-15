#pragma once
#include <ntddk.h>
#include <wdf.h>
#include <intrin.h>
#include <windef.h>
#include <ntdef.h>
#include "Queue.h"
#include "Device.h"

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD SystemTestEvtDeviceAdd;
EVT_WDF_OBJECT_CONTEXT_CLEANUP SystemTestEvtDriverContextCleanup;
EXTERN_C VOID HookSyscallEntryPoint();
EXTERN_C VOID HookInterrupt();
void DriverUnload(
    _In_ DRIVER_OBJECT* DriverObject
);

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT     DriverObject,
    _In_ PUNICODE_STRING    RegistryPath
)
{
    
   /* IDT sIDT;
    RtlZeroMemory(&sIDT, sizeof(sIDT));
    __sidt(&sIDT);
    //IDTENTRY* idt_entries = (IDTENTRY*)*(PULONG64)&sIDT.LowIDTbase;
    //DbgPrint("\n\n%p\n\n", idt_entries);
    DbgPrint("\n\n IDT_BASE: %p  IDT_LIMIT: %d \n\n", sIDT.LowIDTbase, sIDT.IDTLimit);
    DbgPrint("%p", __readgsqword(0x38));
    DbgPrint("\n\nLoad driver %wZ\n\n", &DriverObject->DriverName);
    DbgPrint("\n\nRegistry path %wZ\n\n", RegistryPath);
    DbgPrint("\n\nDriverEntry\n\n");
    
    PIDTENTRY idt_entries = KeGetPcr()->IdtBase;
    DbgPrint("\n\n%p\n\n", idt_entries);*/


    DriverObject->DriverUnload = DriverUnload;
    //showIdt(KeGetCurrentProcessorIndex());
  
    WDF_DRIVER_CONFIG config;
    NTSTATUS status;
    WDF_OBJECT_ATTRIBUTES attributes;
    WDFDRIVER driver;
    //
    // Register a cleanup callback so that we can call WPP_CLEANUP when
    // the framework driver object is deleted during driver unload.
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.EvtCleanupCallback = SystemTestEvtDriverContextCleanup;

    WDF_DRIVER_CONFIG_INIT(&config,
        SystemTestEvtDeviceAdd
    );
    
    //SystemCreateDevice();

    status = WdfDriverCreate(
        DriverObject,
        RegistryPath,
        &attributes,
        &config,
        &driver
    );
    SytemCreateControlDevice(&driver);

    DbgPrint("\n\n Driver Entry Name: %wZ \n\n", DriverObject->DriverName);
    if (!NT_SUCCESS(status)) {
        return status;
    }
    DbgPrint("\n my HookSyscallx64 address: %p \n", HookSyscallEntryPoint);
    DbgPrint("\n my Hook Interrupt: %p \n", HookInterrupt);
    setMyInterrupt(0x4c, HookInterrupt);
    showInt3();
    return status;
   
}

NTSTATUS
SystemTestEvtDeviceAdd(
    _In_    WDFDRIVER       Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit
)

{
    DbgPrint("\n\n SystemTestEvtDeviceAdd \n\n");
    NTSTATUS status;

    UNREFERENCED_PARAMETER(Driver);

    //PAGED_CODE();

    status = SytemCreateControlDevice(&Driver);
    return status;
}


VOID
SystemTestEvtDriverContextCleanup(
    _In_ WDFOBJECT DriverObject
)
{
    UNREFERENCED_PARAMETER(DriverObject);

    //PAGED_CODE();
}

void DriverUnload(
    _In_ DRIVER_OBJECT* DriverObject
)
{
    UnHookSystemCallx64();
    
}
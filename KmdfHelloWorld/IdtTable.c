#pragma once
#include "IdtTable.h"
ULONG_PTR oldInterrupt;

NTSTATUS EnumIDTableAddress(WDFREQUEST Request, size_t InputBufferLength, size_t OutputBufferLength, _Out_opt_ size_t* Resquestlength)
{
    if (!InputBufferLength || !OutputBufferLength)
        return STATUS_BUFFER_TOO_SMALL;
    NTSTATUS status = STATUS_SUCCESS;
    size_t InputBufferlength = 0;
    PPROCESSOR_NUMBER InputBuffer = NULL;
    status = WdfRequestRetrieveInputBuffer(Request, 1, &InputBuffer, &InputBufferlength);
    if (!NT_SUCCESS(status))
        return status;
    PROCESSOR_NUMBER i = *InputBuffer;
    USHORT NumberGroups = KeQueryActiveGroupCount();
    if (i.Group >= NumberGroups)
        i.Group = NumberGroups - 1;
    showIdt(Request, InputBufferLength, OutputBufferLength, Resquestlength, i);

    return STATUS_SUCCESS;
}

NTSTATUS showIdt(WDFREQUEST Request, size_t InputBufferLength, size_t OutputBufferLength, size_t* Resquestlength, PROCESSOR_NUMBER num)
//NTSTATUS showIdt(int num)
{
    NTSTATUS status = STATUS_SUCCESS;
    GDTINFO idtr = { 0 };
    PROCESSOR_NUMBER TargetProcessor;
    PROCESSOR_NUMBER CurrentProcessor;
    
    KeGetCurrentProcessorNumberEx(&CurrentProcessor);
    RtlCopyMemory(&TargetProcessor, &num, sizeof(PROCESSOR_NUMBER));
    
    GROUP_AFFINITY TargetGroupAffinityThread;
    GROUP_AFFINITY UserGroupAffinityThread; // affinity of user mode thread

    TargetGroupAffinityThread.Group = TargetProcessor.Group;
    TargetGroupAffinityThread.Mask = (KAFFINITY)((1ULL) << TargetProcessor.Number);

    LARGE_INTEGER interval;
    interval.LowPart = 10;
    interval.HighPart = 0;
    interval.QuadPart = 0;
    KeLowerIrql(PASSIVE_LEVEL);
    KeSetSystemGroupAffinityThread(&TargetGroupAffinityThread, &UserGroupAffinityThread);
    KeDelayExecutionThread(KernelMode, FALSE, &interval);
    

   // KeSetSystemAffinityThread(num); устарело с Vindows Vista
    //PKIDTENTRY64 idt = KeGetPcr()->IdtBase;
    
    __sidt(&idtr);
    //KeRevertToUserAffinityThread();
    PKIDTENTRY64 idt = idtr.Base;
    KeLowerIrql(PASSIVE_LEVEL);
    KeRevertToUserGroupAffinityThread(&UserGroupAffinityThread);
    KeDelayExecutionThread(KernelMode, FALSE, &interval);

    DWORD maxNum = 0;
    if (idtr.Limit % sizeof(KIDTENTRY64) == 0) {//idtr.Pad[0] == 0xfff.
        maxNum = idtr.Limit / sizeof(KIDTENTRY64);
    }
    else {
        maxNum = idtr.Limit / sizeof(KIDTENTRY64);
        maxNum++;
    }


    size_t OutputBufferlength = 0;
    PIDTABLE_STRUCT buffer = NULL;
    status = WdfRequestRetrieveOutputBuffer(Request, maxNum * sizeof(IDTABLE_STRUCT), &buffer, &OutputBufferlength);
    if (!NT_SUCCESS(status))
    {
        if (STATUS_BUFFER_TOO_SMALL == status)
        {
            status = WdfRequestRetrieveOutputBuffer(Request, sizeof(WORD), &buffer, &OutputBufferlength);
            if (NT_SUCCESS(status))
            {
                *(PDWORD64)buffer = maxNum * sizeof(IDTABLE_STRUCT);
                *Resquestlength = OutputBufferlength;
                status = STATUS_SUCCESS;
            }
        }
        return status;
    }
    RtlZeroMemory(buffer, OutputBufferlength);
    *Resquestlength = maxNum * sizeof(IDTABLE_STRUCT);
    

    if (MmIsAddressValid(idt))
    {
        for (USHORT i = 0; i < maxNum; i++)
        {
            SIZE_T ISR = 0;
            PKIDTENTRY64 pkidte_t = (idt + i);
            ISR = pkidte_t->OffsetHigh;
            ISR = (ISR << 32);
            ISR |= (DWORD)(pkidte_t->OffsetLow + (pkidte_t->OffsetMiddle << 16));
            buffer->CPUNum = num.Number;
            buffer->InterruptAddress = ISR;
            buffer->InterruptNums = i;
            if (i <= 20)
                DbgPrint("Index: %d CPU: 0x%02x Name:%s ISR: 0x%llX\n", num, i, IST_NAME[i], ISR);
            else if (i > 20 && i <= 32)
                DbgPrint("Index: %d CPU: 0x%02x Name:%s ISR:0x%llX\n", num, i, IST_NAME[21], ISR);
            else
                DbgPrint("Index: %d CPU: 0x%02x   Name:%s  ISR:0x%llX\n", num, i, IST_NAME[22], ISR);
            buffer++;
        }
    }
    else
        status = STATUS_UNSUCCESSFUL;

    return status;
}


ULONG64 ClearWP(void) {

    ULONG64 reg;
    reg = __readcr0();
    __writecr0(reg & 0xFFFEFFFF);
    return reg;
}

void WriteCR0(ULONG64 reg) {
    __writecr0(reg);
}

NTSTATUS setMyInterruptCp(ULONG index, ULONG_PTR address) {
    ULONG64 cr0;
    //DbgBreakPoint();
    DbgPrint("\n New Interrupt Address: %p <> Index: %d", address, index);

    GDTINFO idtr = { 0 };
    __sidt(&idtr);
    //PKIDTENTRY64 idt_base = KeGetPcr()->IdtBase;
    PKIDTENTRY64 idt_base = idtr.Base;

    DbgPrint("\n IDT_BASE: %p \n", idt_base);
    DbgPrint("ProcessorNumberToSetInterrupt %d \n", KeGetCurrentProcessorIndex());
    

    PKIDTENTRY64 pidt_item = (idt_base + index);
    
    ULONG_PTR ISR = pidt_item->OffsetHigh;
    ISR = (ISR << 32);
    ISR |= (DWORD)(pidt_item->OffsetLow + (pidt_item->OffsetMiddle << 16));
    oldInterrupt = ISR;

    cr0 = ClearWP();
    //change address

    pidt_item->OffsetHigh = (ULONG)(address >> 32);
    pidt_item->OffsetMiddle = (USHORT)(address >> 16);
    pidt_item->OffsetLow = (USHORT)(address);
    pidt_item->Type = 0xE;
    pidt_item->Present = 1;
    pidt_item->Dpl = 3;
    pidt_item->Selector = 0x10;
    pidt_item->IstIndex = 0;

    WriteCR0(cr0);
}

NTSTATUS showInt3(){
    ULONG64 cr0;
    UINT index = 0x4c;
   // DbgPrint("\n New Interrupt Address: %p <> Index: %d", address, index);
    
    GDTINFO idtr = { 0 };
    PKIDTENTRY64 idt_base = KeGetPcr()->IdtBase;
    __sidt(&idtr);

    DbgPrint("\n IDT_BASE: %p \n", idt_base);

    PKIDTENTRY64 pidt_item = (idt_base + index);

    ULONG_PTR ISR = pidt_item->OffsetHigh;
    ISR = (ISR << 32);
    ISR |= (DWORD)(pidt_item->OffsetLow + (pidt_item->OffsetMiddle << 16));
    oldInterrupt = ISR;
    
    DbgPrint("ISR:%p, Type: %x, Present: %x, Dpl:%x, Selector: %x", ISR, pidt_item->Type, pidt_item->Present, 
        pidt_item->Dpl, pidt_item->Selector);

    return STATUS_SUCCESS;
}


void setMyInterrupt(ULONG index, ULONG_PTR address) {
    //DbgBreakPoint();

    USHORT NumberOfGroups = KeQueryActiveGroupCount();
    for (USHORT gr = 0; gr < NumberOfGroups; gr++) {
        ULONG NumberOfProcessors = KeQueryActiveProcessorCountEx(gr);
        for (ULONG pr = 0; pr < NumberOfProcessors; pr++) {

            GROUP_AFFINITY TargetGroupAffinityThread;
            GROUP_AFFINITY UserGroupAffinityThread; // affinity of user mode thread
            TargetGroupAffinityThread.Group = gr;
            TargetGroupAffinityThread.Mask = (KAFFINITY)(((KAFFINITY)1ULL) << pr);
            LARGE_INTEGER interval;
            interval.LowPart = 10;
            interval.HighPart = 0;
            interval.QuadPart = 0;
            KeLowerIrql(PASSIVE_LEVEL);
            KeSetSystemGroupAffinityThread(&TargetGroupAffinityThread, &UserGroupAffinityThread);
            KeDelayExecutionThread(KernelMode, FALSE, &interval);
            setMyInterruptCp(index, address);//to hook IDT_item

            KeLowerIrql(PASSIVE_LEVEL);
            KeRevertToUserGroupAffinityThread(&UserGroupAffinityThread);
            KeDelayExecutionThread(KernelMode, FALSE, &interval);
        }
    }

}
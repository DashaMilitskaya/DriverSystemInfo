#pragma once
#include "GdtTable.h"


typedef struct _IDT {

    UINT16 IDTLimit;
    UINT32 LowIDTbase;
    UINT32 HiIDTbase;

}IDT, * PIDT;

typedef struct _IDTABLE_STRUCT
{
    int CPUNum;
    SIZE_T InterruptNums;
    DWORD64 InterruptAddress;
}IDTABLE_STRUCT, * PIDTABLE_STRUCT;

typedef union _KIDTENTRY64
{
    struct
    {
        USHORT OffsetLow;
        USHORT Selector;
        USHORT IstIndex : 3;
        USHORT Reserved0 : 5;
        USHORT Type : 5;
        USHORT Dpl : 2;
        USHORT Present : 1;
        USHORT OffsetMiddle;
        ULONG OffsetHigh;
        ULONG Reserved1;
    };
    UINT64 Alignment;
} IDTENTRY, * PIDTENTRY, KIDTENTRY64, * PKIDTENTRY64;

static const char* const IST_NAME[] =
{
    "Divide Error",
    "Debug",
    "NMI Interrupt Not applicable",
    "Breakpoint",
    "Overflow",
    "BOUND Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun", // Intel reserved
    "Invalid TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection",
    "Page-Fault",
    "Intel reserved",
    "x87 FPU Floating-Point",
    "Alignment Check",
    "Machine-Check",
    "SIMD Floating-Point",
    "Virtualization",
    "Intel reserved",
    "User Defined"
};
NTSTATUS EnumIDTableAddress(WDFREQUEST Request, size_t InputBufferLength, size_t OutputBufferLength, _Out_opt_ size_t* Resquestlength);
NTSTATUS showIdt(WDFREQUEST Request, size_t InputBufferLength, size_t OutputBufferLength, size_t* Resquestlength, PROCESSOR_NUMBER num);
void setMyInterrupt(ULONG index, ULONG_PTR address);
NTSTATUS showInt3();
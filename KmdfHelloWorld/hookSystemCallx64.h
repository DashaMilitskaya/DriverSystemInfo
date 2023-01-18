
/*
void __writemsr(
   unsigned long Register,
   unsigned __int64 Value
);
ULONGLONG  KiSystemCall64 = __readmsr(0xC0000082);		// Get the address of nt!KeSystemCall64

we can change address in msr
*/
#pragma once
#include <ntddk.h>
#include <wdf.h>
#include <intrin.h>
#include <windef.h>
#include <ntdef.h>

#define NumOfSyscalls 0x1000

typedef union _callContect {
	struct {
		UINT64 num; //8 байт
		UINT64 rax;	//8 байт

		/*Register Arguments*/
		UINT64 r10;	//8 байт arg1
		UINT64 rdx;	//8 байт arg2
		UINT64 r8;	//8 байт arg3
		UINT64 r9;	//8 байт arg4
		
		BYTE stackArgsBytes;
		
		BYTE stackArgs[50];
		
		//The stack argument is placed
	};
	
}CALL_CONTEXT64, *PCALL_CONTEXT64;

//48 байт + 160 = 208байт

// CALL_CONTEXT64 ContextTable[NumOfSyscalls];

NTSTATUS hookSystemCallx64();
void UnHookSystemCallx64();
void KernelPrintHookedItem();
void SaveOldKiSystemCall64();
PVOID InitArgumentTable();

/*dps nt!KeServiceDescriptorTable L10
fffff803`44250740  fffff803`44192640 nt!KiServiceTable
fffff803`44250748  00000000`00000000
fffff803`44250750  00000000`000001b9
fffff803`44250758  fffff803`4419340c nt!KiArgumentTable
fffff803`44250760  00000000`00000000
fffff803`44250768  00000000`00000000
fffff803`44250770  00000000`00000000
fffff803`44250778  00000000`00000000
fffff803`44250780  00000000`00000000
fffff803`44250788  00000000`00000000
fffff803`44250790  00000000`00c11e84
fffff803`44250798  00007ffc`91a45410 ntdll!KiUserCallbackDispatch
*/

typedef struct _SDT {
	PULONG KiServiceTable;
	ULONG64 reserved;
	ULONG64 syscallNumber;
	PCHAR   KiArgumentTable;

}SDT, *PSDT;

NTSTATUS PrintHookedItem(WDFREQUEST Request, size_t InputBufferLength, size_t OutputBufferLength, size_t* Resquestlength);
#include "hookSystemCallx64.h"

PUINT64 countersPtr;
ULONG_PTR oldKiSystemCall64;


PCHAR   KiArgumentTable;
EXTERN_C VOID HookSyscallEntryPoint();
PCALL_CONTEXT64 ContextTable = NULL;

PVOID GetKeServiceDescriptorTable64()
{
	PUCHAR StartSearchAddress = (PUCHAR)__readmsr(0xC0000082);
	PUCHAR EndSearchAddress = StartSearchAddress + 0x500;
	PUCHAR i = NULL;
	UCHAR b1 = 0, b2 = 0, b3 = 0;
	ULONG_PTR ulv1 = 0;
	PVOID address = 0;
	for (i = StartSearchAddress; i < EndSearchAddress; i++)
	{
		if (MmIsAddressValid(i) && MmIsAddressValid(i + 1) && MmIsAddressValid(i + 2))
		{
			b1 = *i;
			b2 = *(i + 1);
			b3 = *(i + 2);
			if (b1 == 0x4c && b2 == 0x8d && b3 == 0x15)
			{
				memcpy(&ulv1, i + 3, 4);
				address = (PVOID)((ULONG_PTR)ulv1 + (ULONG_PTR)i + 7);
				return address;
			}
		}
	}
	return 0;
}

void writeLSTAR(ULONG_PTR newSystemCallx64) {
	USHORT NumberOfGroups = KeQueryActiveGroupCount();
	for (USHORT gr = 0; gr < NumberOfGroups; gr++) {
		ULONG NumberOfProcessors = KeQueryActiveProcessorCountEx(gr);
		for (ULONG pr = 0; pr < NumberOfProcessors; pr++) {

			GROUP_AFFINITY TargetGroupAffinityThread;
			GROUP_AFFINITY UserGroupAffinityThread; // affinity of user mode thread
			TargetGroupAffinityThread.Group = gr;
			TargetGroupAffinityThread.Mask = (KAFFINITY)((1ULL) << pr);

			KeSetSystemGroupAffinityThread(&TargetGroupAffinityThread, &UserGroupAffinityThread);

			__writemsr(0xC0000082, newSystemCallx64); //to hook SysCall64

			KeRevertToUserGroupAffinityThread(&UserGroupAffinityThread);

		}
	}

}
PSDT KeServiceDescriptorTable;
ULONG64 NumSyscalls;
PVOID InitArgumentTable() {
	KeServiceDescriptorTable = (PSDT)GetKeServiceDescriptorTable64();
	KiArgumentTable = KeServiceDescriptorTable->KiArgumentTable;
	NumSyscalls =  KeServiceDescriptorTable->syscallNumber;
	ContextTable = (PCALL_CONTEXT64)ExAllocatePool(NonPagedPool, NumSyscalls * sizeof(CALL_CONTEXT64));
	if(ContextTable)
		RtlZeroMemory(ContextTable, NumSyscalls * sizeof(CALL_CONTEXT64));
	return ContextTable;
}

void SaveOldKiSystemCall64() {
	oldKiSystemCall64 = __readmsr(0xC0000082);
}

NTSTATUS hookSystemCallx64() {
	if (ContextTable) return STATUS_SUCCESS;
	
	//KeLowerIrql(PASSIVE_LEVEL);
	DbgPrint("Hooking KiSystemCall64");
	if (InitArgumentTable()) {
		oldKiSystemCall64 = __readmsr(0xC0000082);
		writeLSTAR(HookSyscallEntryPoint); //To hook Syscallx64
	}
	return STATUS_SUCCESS;
	
}

void UnHookSystemCallx64() {
	if (ContextTable) {
		writeLSTAR(oldKiSystemCall64);//To unhook Syscallx64
		ExFreePool(ContextTable);
	}
		
	ContextTable = NULL;
}

void KernelPrintHookedItem() {
	for (UINT i = 0; i < NumOfSyscalls; i++) {
		
		CALL_CONTEXT64 item = ContextTable[i];
		
		DbgPrint("\n %llx %llx %llx %llx %llx %llx NumofStackBytes: %x\n", ContextTable[i].num, ContextTable[i].rax, 
														ContextTable[i].r10, ContextTable[i].rdx, 
			ContextTable[i].r8, ContextTable[i].r9, ContextTable[i].stackArgsBytes);
	}
}

NTSTATUS PrintHookedItem(WDFREQUEST Request, size_t InputBufferLength, size_t OutputBufferLength, size_t* Resquestlength) {
	if (ContextTable == NULL) return STATUS_ACCESS_VIOLATION;
	NTSTATUS status;
	size_t OutputBufferlength = 0;
	PCALL_CONTEXT64 buffer = NULL;
	UINT realNumOfSyscalls = KeServiceDescriptorTable->syscallNumber;
	status = WdfRequestRetrieveOutputBuffer(Request, realNumOfSyscalls * sizeof(CALL_CONTEXT64), &buffer, &OutputBufferlength);
	if (!NT_SUCCESS(status))
	{
		if (STATUS_BUFFER_TOO_SMALL == status)
		{
			status = WdfRequestRetrieveOutputBuffer(Request, sizeof(WORD), &buffer, &OutputBufferlength);
			if (NT_SUCCESS(status))
			{
				*(PDWORD64)buffer = realNumOfSyscalls * sizeof(CALL_CONTEXT64);
				*Resquestlength = OutputBufferlength;
				status = STATUS_SUCCESS;
			}
		}
		return status;
	}
	RtlZeroMemory(buffer, OutputBufferlength);
	*Resquestlength = realNumOfSyscalls * sizeof(CALL_CONTEXT64);

	memcpy(buffer, ContextTable, OutputBufferLength);
	return status;
	/*
	
	for (UINT i = 0; i < NumOfSyscalls; i++) {

		CALL_CONTEXT64 item = ContextTable[i];

		DbgPrint("\n %llx %llx %llx %llx %llx %llx NumofStackBytes: %x\n", ContextTable[i].num, ContextTable[i].rax,
			ContextTable[i].r10, ContextTable[i].rdx,
			ContextTable[i].r8, ContextTable[i].r9, ContextTable[i].stackArgsBytes);
	}*/
}
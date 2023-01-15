
#include "GdtTable.h"
#pragma intrinsic(_sgdt) 

/*
Code - and Data-Segment Types
*/
const char SegmentTypes[][256] = {
	"<Reserved>",//Data Read-Only：Data RO，： <Reserved>（UINT64）Reserved
	"Data RO AC",//Data Read-Only, accessed
	"Data RW",//Data Read/Write
	"Data RW AC",//Data Read/Write, accessed
	"Data RO ED",//Data Read-Only, expand-down
	"Data RO ED AC",//Data Read-Only, expand-down, accessed
	"Data RW ED",//Data Read/Write, expand-down
	"Data RW ED AC",//Data Read/Write, expand-down, accessed

	"Code EO",//Code Execute-Only
	"Code EO AC",//Code Execute-Only, accessed
	"Code RE",//Code Execute/Read
	"Code RE AC",//Code Execute/Read, accessed
	"Code EO CO",//Code Execute-Only, conforming
	"Code EO CO AC",//Code Execute-Only, conforming, accessed
	"Code RE CO",//Code Execute/Read, conforming
	"Code RE CO AC",//Code Execute/Read, conforming, accessed
	"TSS32 Busy ",
	"TSS32 Avl"
};

NTSTATUS EnumGdtTable(WDFREQUEST Request, size_t InputBufferLength, size_t OutputBufferLength, _Out_opt_ size_t* Resquestlength){
	if (!InputBufferLength || !OutputBufferLength)
		return STATUS_BUFFER_TOO_SMALL;
	NTSTATUS status = STATUS_SUCCESS;

	size_t InputBufferlength = 0;
	PPROCESSOR_NUMBER InputBuffer = NULL;
	status = WdfRequestRetrieveInputBuffer(Request, 1, &InputBuffer, &InputBufferlength);
	if (!NT_SUCCESS(status))
		return status;
	PROCESSOR_NUMBER i = *InputBuffer;
	showGdt(Request, InputBufferLength, OutputBufferLength, Resquestlength, i);

	return STATUS_SUCCESS;
}

NTSTATUS showGdt(WDFREQUEST Request, size_t InputBufferLength, size_t OutputBufferLength, _Out_opt_ size_t* Resquestlength, PROCESSOR_NUMBER num){
	GDTINFO GdtLimit = { 0 };
	//KeSetSystemAffinityThread(i + 1);
	
	PROCESSOR_NUMBER TargetProcessor;
	PROCESSOR_NUMBER CurrentProcessor;

	KeGetCurrentProcessorNumberEx(&CurrentProcessor);
	RtlCopyMemory(&TargetProcessor, &num, sizeof(PROCESSOR_NUMBER));

	GROUP_AFFINITY TargetGroupAffinityThread;
	GROUP_AFFINITY UserGroupAffinityThread; // affinity of user mode thread


	
	TargetGroupAffinityThread.Group = TargetProcessor.Group;
	TargetGroupAffinityThread.Mask = (KAFFINITY)((1ULL) << TargetProcessor.Number);
	KeSetSystemGroupAffinityThread(&TargetGroupAffinityThread, &UserGroupAffinityThread);
	
	
	PKGDTENTRY64 pkgdte = KeGetPcr()->GdtBase;
	_sgdt(&GdtLimit);

	
	KeRevertToUserGroupAffinityThread(&UserGroupAffinityThread);
	

	//KeRevertToUserAffinityThread();
	SIZE_T maxNum = (GdtLimit.Limit + 1) / sizeof(KGDTENTRY64);

	/*
	KdPrint(("Sel        Base             Limit             Type   DPl Size Gran Pres Long Flags\n"));
	KdPrint(("---- ---------------- ---------------- ------------- --- ---- ---- ---- ---- --------\n"));
	KdPrint(("\n"));
	*/
	

	NTSTATUS status = STATUS_SUCCESS;
	size_t OutputBufferlength = 0;
	PGPTABLE_STRUCT buffer = NULL;
	status = WdfRequestRetrieveOutputBuffer(Request, (maxNum - 1) * sizeof(GPTABLE_STRUCT), &buffer, &OutputBufferlength);
	if (!NT_SUCCESS(status))
	{
		if (STATUS_BUFFER_TOO_SMALL == status)
		{
			status = WdfRequestRetrieveOutputBuffer(Request, sizeof(WORD), &buffer, &OutputBufferlength);
			if (NT_SUCCESS(status))
			{
				*(PDWORD64)buffer = (maxNum - 1) * sizeof(GPTABLE_STRUCT);
				*Resquestlength = OutputBufferlength;
				status = STATUS_SUCCESS;
			}
		}
		return status;
	}
	RtlZeroMemory(buffer, OutputBufferlength);
	*Resquestlength = (maxNum - 1) * sizeof(GPTABLE_STRUCT);

	for (SIZE_T index = 1; index < maxNum; index++)
	{
		PKGDTENTRY64 pkgdteEntry = &pkgdte[index];
		SIZE_T Base = 0;
		SIZE_T Limit = 0;
		LONG  Type = 0;
		char* size = NULL;
		char* Granularity = NULL;
		char* Present = NULL;
		char* LongMode = NULL;
		int    Flags = 0;

		Base = pkgdteEntry->Bits.BaseHigh;
		Base = (Base << 24);
		Base += (pkgdteEntry->BaseLow + (pkgdteEntry->Bits.BaseMiddle << 16));

		Limit = pkgdteEntry->LimitLow + (pkgdteEntry->Bits.LimitHigh << 16);

		if (pkgdteEntry->Bits.DefaultBig && Base)
		{
			
			Base += 0xffffffff00000000;
		}

		if (pkgdteEntry->Bits.DefaultBig && pkgdteEntry->Bits.Granularity)
		{
		
			SIZE_T t = Limit;
			Limit = (Limit << 12);
			Limit += PAGE_SIZE - 1;
		}

		Type = pkgdteEntry->Bits.Type;
		_bittestandreset(&Type, 4);

		if (pkgdteEntry->Bits.DefaultBig)
		{
			size = "Bg  ";//Big 
		}
		else
		{
			size = "Nb  ";//Not Big
		
		}
		if (pkgdteEntry->Bits.Granularity)
		{
			Granularity = "Pg  ";//Page 
		}
		else
		{
			Granularity = "By  ";//Byte
		}

		if (pkgdteEntry->Bits.Present)
		{
			Present = "P   ";//Present 
		}
		else
		{
			Present = "NP  ";//NO Present
		}

		if (pkgdteEntry->Bits.LongMode)
		{
			LongMode = "Lo  ";//Long 
		}
		else
		{
			LongMode = "Nl  ";//NO long 
		}

		Flags = (pkgdteEntry->Bytes.Flags2 >> 4);//Segment limit
		Flags = Flags << 8;
		Flags = Flags + pkgdteEntry->Bytes.Flags1;

		KdPrint(("%04x %p %p %13s %03x %s %s %s %s 0x%04x\n",
			index * 8, //sizeof (KGDTENTRY)
			Base,
			Limit,
			SegmentTypes[Type],
			pkgdteEntry->Bits.Dpl,
			size,
			Granularity,
			Present,
			LongMode,
			Flags
			));
		buffer->Base = Base;
		buffer->Flags = Flags;
		buffer->Limit = Limit;
		buffer->Index = index * 8;
		buffer->DPl = pkgdteEntry->Bits.Dpl;
		RtlCopyMemory(buffer->SegmentTypes, SegmentTypes[Type], strlen(SegmentTypes[Type]));
		RtlCopyMemory(buffer->Granularity, Granularity, 2);
		RtlCopyMemory(buffer->Present, Present, 2);
		RtlCopyMemory(buffer->LongMode, LongMode, 2);
		RtlCopyMemory(buffer->size, size, 2);
		buffer++;
	}
		return STATUS_SUCCESS;



}
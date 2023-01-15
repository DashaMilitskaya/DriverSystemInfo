﻿#pragma once
#include <ntddk.h>
#include <wdf.h>
#include <intrin.h>
#include <windef.h>
#include <ntdef.h>

typedef struct
{
	WORD Limit;
#ifdef _WIN64
	DWORD64 Base;
#else
	DWORD Base;
#endif // _WIN64
} GDTINFO;

#ifdef _WIN64

typedef struct _AMD64_DESCRIPTOR {
	USHORT  Pad[3];
	USHORT  Limit;
	ULONG64 Base;
} AMD64_DESCRIPTOR, * PAMD64_DESCRIPTOR;

typedef union _KGDTENTRY64 {
	struct {
		USHORT  LimitLow;
		USHORT  BaseLow;
		union {
			struct {
				UCHAR   BaseMiddle;
				UCHAR   Flags1;
				UCHAR   Flags2;
				UCHAR   BaseHigh;
			} Bytes;

			struct {
				ULONG   BaseMiddle : 8;
				ULONG   Type : 5;
				ULONG   Dpl : 2;
				ULONG   Present : 1;
				ULONG   LimitHigh : 4;
				ULONG   System : 1;
				ULONG   LongMode : 1;
				ULONG   DefaultBig : 1;
				ULONG   Granularity : 1;
				ULONG   BaseHigh : 8;
			} Bits;
		};

		//ULONG BaseUpper;*/
		//ULONG MustBeZero;
	};

	//ULONG64 Alignment;
} KGDTENTRY64, * PKGDTENTRY64;

typedef struct _GPTABLE_STRUCT
{
	SIZE_T Index;
	SIZE_T Base;
	SIZE_T Limit;
	LONG  Type;
	char SegmentTypes[20];
	ULONG DPl;
	char size[3];
	char Granularity[3];
	char Present[3];
	char LongMode[3];
	int    Flags;
}GPTABLE_STRUCT, * PGPTABLE_STRUCT;
#else 
// Special Registers for i386
typedef struct _X86_DESCRIPTOR {
	USHORT  Pad;
	USHORT  Limit;
	ULONG   Base;
} X86_DESCRIPTOR, * PX86_DESCRIPTOR;

// GDT Entry
typedef struct _KGDTENTRY {
	USHORT  LimitLow;
	USHORT  BaseLow;
	union {
		struct {
			UCHAR   BaseMid;
			UCHAR   Flags1;     // Declare as bytes to avoid alignment
			UCHAR   Flags2;     // Problems.
			UCHAR   BaseHi;
		} Bytes;
		struct {
			ULONG   BaseMid : 8;
			ULONG   Type : 5;
			ULONG   Dpl : 2;
			ULONG   Pres : 1;
			ULONG   LimitHi : 4;
			ULONG   Sys : 1;
			ULONG   Reserved_0 : 1;//LongMode
			ULONG   Default_Big : 1;//(default operation size/default stack pointer size and/or upper bound) flag。
			ULONG   Granularity : 1;
			ULONG   BaseHi : 8;
		} Bits;
	} HighWord;
} KGDTENTRY, * PKGDTENTRY;

#endif

NTSTATUS showGdt(WDFREQUEST Request, size_t InputBufferLength, size_t OutputBufferLength, _Out_opt_ size_t* Resquestlength, PROCESSOR_NUMBER num);
NTSTATUS EnumGdtTable(WDFREQUEST Request, size_t InputBufferLength, size_t OutputBufferLength, _Out_opt_ size_t* Resquestlength);

#ifdef _X86_
__forceinline PKPCR KeGetPcr(VOID)
{
	return (PKPCR)__readfsdword(FIELD_OFFSET(KPCR, SelfPcr));
}
#endif

#include RADPCH
#include "Base.h"
#include "CPUCount.h"

#if defined(RAD_OPT_APPLE) || defined(_WIN64)

#pragma message ("CPUCount - Stubbed Out")

unsigned char CPUCount(unsigned int *a, unsigned int *b, unsigned int *c)
{ // rewrite using mach later.
	*a = *b = *c = 1;
	return USER_CONFIG_ISSUE;
}

#else

#if defined(RAD_OPT_LINUX)
	#define LINUX
#endif

// Copyright (c) 2005 Intel Corporation
// All Rights Reserved
//
// CPUCount.cpp : Detects three forms of hardware multi-threading support across IA-32 platform
//					The three forms of HW multithreading are: Multi-processor, Multi-core, and
//					HyperThreading Technology.
//					This application enumerates all the logical processors enabled by OS and BIOS,
//					determine the HW topology of these enabled logical processors in the system
//					using information provided by CPUID instruction.
//					A multi-processing system can support any combination of the three forms of HW
//					multi-threading support. The relevant topology can be identified using a
//					three level decomposition of the "initial APIC ID" into
//					Package_id, core_id, and SMT_id. Such decomposition provides a three-level map of
//					the topology of hardware resources and
//					allow multi-threaded software to manage shared hardware resources in
//					the platform to reduce resource contention

//					Multicore detection algorithm for processor and cache topology requires
//					all leaf functions of CPUID instructions be available. System administrator
//					must ensure BIOS settings is not configured to restrict CPUID functionalities.
//-------------------------------------------------------------------------------------------------

#define HWD_MT_BIT         0x10000000     // EDX[28]  Bit 28 is set if HT or multi-core is supported
#define NUM_LOGICAL_BITS   0x00FF0000     // EBX[23:16] Bit 16-23 in ebx contains the number of logical
// processors per physical processor when execute cpuid with
// eax set to 1
#define NUM_CORE_BITS      0xFC000000     // EAX[31:26] Bit 26-31 in eax contains the number of cores minus one
// per physical processor when execute cpuid with
// eax set to 4.


#define INITIAL_APIC_ID_BITS  0xFF000000  // EBX[31:24] Bits 24-31 (8 bits) return the 8-bit unique
// initial APIC ID for the processor this code is running on.

unsigned int  CpuIDSupported(void);
unsigned int  GenuineIntel(void);
unsigned int  HWD_MTSupported(void);
unsigned int  MaxLogicalProcPerPhysicalProc(void);
unsigned int  MaxCorePerPhysicalProc(void);
unsigned int  find_maskwidth(unsigned int);
unsigned char GetAPIC_ID(void);
unsigned char GetNzbSubID(unsigned char,
						  unsigned char,
						  unsigned char);

// Define constant �LINUX� to compile under Linux
#ifdef LINUX
// 	The Linux source code listing can be compiled using Linux kernel verison 2.6
//	or higher (e.g. RH 4AS-2.8 using GCC 3.4.4).
//	Due to syntax variances of Linux affinity APIs with earlier kernel versions
//	and dependence on glibc library versions, compilation on Linux environment
//	with older kernels and compilers may require kernel patches or compiler upgrades.

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sched.h>
#define DWORD unsigned long
#else
#include <windows.h>
#endif
#include <stdio.h>
#include <assert.h>

#if 0

int main(void)
{

	unsigned int  TotAvailLogical   = 0,  // Number of available logical CPU per CORE
		TotAvailCore  = 0,      // Number of available cores per physical processor
		PhysicalNum   = 0;      // Total number of physical processors

	unsigned char StatusFlag = 0;
	int MaxLPPerCore;
	if (CpuIDSupported() < 4) { // CPUID does not report leaf 4 information
		printf("\nUser Warning:\n CPUID Leaf 4 is not supported or disabled. Please check  \
			   \n BIOS and correct system configuration error if leaf 4 is disabled. \n");

	}
	StatusFlag = CPUCount(&TotAvailLogical, &TotAvailCore, &PhysicalNum);
	if( USER_CONFIG_ISSUE == StatusFlag) {
		printf("User Configuration Error: Not all logical processors in the system are enabled \
			   while running this process. Please rerun this application after make corrections. \n");
		exit(1);
	}

	printf("\n----Counting Hardware MultiThreading Capabilities and Availability ---------- \n\n");
	printf("This application displays information on three forms of hardware multithreading\n");
	printf("capability and their availability to apps. The three forms of capabilities are:\n");
	printf("multi-processor (MP), Multi-core (core), and HyperThreading Technology (HT).\n");
	printf("\nHardware capability results represents the maximum number provided in hardware.\n");
	printf("Note, Bios/OS or experienced user can make configuration changes resulting in \n");
	printf("less-than-full HW capabilities are available to applications.\n");
	printf("For best result, the operator is responsible to configure the BIOS/OS such that\n");
	printf("full hardware multi-threading capabilities are enabled.\n");
	printf("\n---------------------------------------------------------------------------- \n\n\n");

	printf("\nCapabilities:\n\n");

	switch(StatusFlag)
	{

	case MULTI_CORE_AND_HT_NOT_CAPABLE:
		printf("\tHyper-Threading Technology: not capable  \n\tMulti-core: Yes \n\tMulti-processor: ");
		if (PhysicalNum > 1) printf("yes\n"); else printf("No\n");
		break;

	case SINGLE_CORE_AND_HT_NOT_CAPABLE:
		printf("\tHyper-Threading Technology: Not capable  \n\tMulti-core: No \n\tMulti-processor: ");
		if (PhysicalNum > 1) printf("yes\n"); else printf("No\n");
		break;

	case SINGLE_CORE_AND_HT_DISABLED:
		printf("\tHyper-Threading Technology: Disabled  \n\tMulti-core: No \n\tMulti-processor: ");
		if (PhysicalNum > 1) printf("yes\n"); else printf("No\n");
		break;

	case SINGLE_CORE_AND_HT_ENABLED:
		printf("\tHyper-Threading Technology: Enabled  \n\tMulti-core: No \n\tMulti-processor: ");
		if (PhysicalNum > 1) printf("yes\n"); else printf("No\n");
		break;

	case MULTI_CORE_AND_HT_DISABLED:
		printf("\tHyper-Threading Technology: Disabled  \n\tMulti-core: Yes \n\tMulti-processor: ");
		if (PhysicalNum > 1) printf("yes\n"); else printf("No\n");
		break;

	case MULTI_CORE_AND_HT_ENABLED:
		printf("\tHyper-Threading Technology: Enabled  \n\tMulti-core: Yes \n\tMulti-processor: ");
		if (PhysicalNum > 1) printf("yes\n"); else printf("No\n");
		break;

	}



	printf("\n\nHardware capability and its availability to applications: \n");
	printf("\n  System wide availability: %d physical processors, %d cores, %d logical processors\n", \
		PhysicalNum, TotAvailCore, TotAvailLogical);

	MaxLPPerCore = MaxLogicalProcPerPhysicalProc() / MaxCorePerPhysicalProc() ;
	printf("  Multi-core capabililty : %d cores per package \n", MaxCorePerPhysicalProc());
	printf("  HT capability: %d logical processors per core \n",  MaxLPPerCore);
	assert (PhysicalNum * MaxCorePerPhysicalProc() >= TotAvailCore);
	assert (PhysicalNum * MaxLogicalProcPerPhysicalProc() >= TotAvailLogical);
	if( PhysicalNum * MaxCorePerPhysicalProc() > TotAvailCore) printf("\n  Not all cores in the system are enabled for this application.\n");
	else printf("\n  All cores in the system are enabled for this application.\n");

	printf("\n\nRelationships between OS affinity mask, Initial APIC ID, and 3-level sub-IDs: \n");
	printf("\n%s", g_s3Levels);
	printf("\n\nPress Enter To Continue\n");
	getchar();
	return 0;
}

#endif

//
// CpuIDSupported will return 0 if CPUID instruction is unavailable. Otherwise, it will return
// the maximum supported standard function.
//
unsigned int CpuIDSupported(void)
{
	unsigned int MaxInputValue;
	// If CPUID instruction is supported
#ifdef LINUX
	try
	{
		MaxInputValue = 0;
		// call cpuid with eax = 0
		asm volatile
			(
			"pushl %%ebx\n\t"
			"xorl %%eax, %%eax\n\t"
			"cpuid\n\t"
			"popl %%ebx\n\t"
			: "=a" (MaxInputValue)
			:
			: "%ecx", "%edx"
			);
	}
	catch (...)
	{
		return(0);                   // cpuid instruction is unavailable
	}
#else //Win32
	try
	{
		MaxInputValue = 0;
		// call cpuid with eax = 0
		__asm
		{
			xor eax, eax
				cpuid
				mov MaxInputValue, eax
		}
	}
	catch (...)
	{
		return(0);                   // cpuid instruction is unavailable
	}
#endif

	return MaxInputValue;

}


//
// GenuineIntel will return 0 if the processor is not a Genuine Intel Processor
//
unsigned int GenuineIntel(void)
{
#ifdef LINUX
	unsigned int VendorIDb = 0,VendorIDd = 0, VendorIDc = 0;

	try
		// If CPUID instruction is supported
	{
		// Get vendor id string
		asm volatile
			(
			//get the vendor string
			// call cpuid with eax = 0
			"pushl %%ebx\n\t"
			"xorl %%eax, %%eax\n\t"
			"cpuid\n\t"
			"movl %%ebx, %%eax\n\t"
			"popl %%ebx\n\t"
			: 	"=a" (VendorIDb),
			"=d" (VendorIDd),
			"=c" (VendorIDc)
			:
			:
			);
	}

	catch(...)
	{
		return(0);                   // cpuid instruction is unavailable
	}

	return ( (VendorIDb == 'uneG') &&
		(VendorIDd  == 'Ieni') &&
		(VendorIDc  == 'letn'));

#else
	unsigned int VendorID[3] = {0, 0, 0};
	try    // If CPUID instruction is supported
	{
		__asm
		{
			xor eax, eax			// call cpuid with eax = 0
				cpuid					// Get vendor id string
				mov VendorID, ebx
				mov VendorID + 4, edx
				mov VendorID + 8, ecx
		}
	}
	catch (...)
	{
		return(0);      		unsigned int MaxInputValue =0;
		// cpuid instruction is unavailable
	}
	return ( (VendorID[0] == 'uneG') &&
		(VendorID[1] == 'Ieni') &&
		(VendorID[2] == 'letn'));
#endif
}



//
// Function returns the maximum cores per physical package. Note that the number of
// AVAILABLE cores per physical to be used by an application might be less than this
// maximum value.
//

unsigned int MaxCorePerPhysicalProc(void)
{

	unsigned int Regeax        = 0;

	if (!HWD_MTSupported()) return (unsigned int) 1;  // Single core
#ifdef LINUX
	{
		asm volatile
			(
			"pushl %ebx\n\t"
			"xorl %eax, %eax\n\t"
			"cpuid\n\t"
			"popl %ebx\n\t"
			"cmpl $4, %eax\n\t"			// check if cpuid supports leaf 4
			"jl .single_core\n\t"		// Single core
			"movl $4, %eax\n\t"
			"movl $0, %ecx\n\t"			// start with index = 0; Leaf 4 reports
			);								// at least one valid cache level
		asm
			(
			"pushl %%ebx\n\t"
			"cpuid\n\t"
			"popl %%ebx\n\t"
			: "=a" (Regeax)
			:
			: "%ecx", "%edx"
			);
		asm
			(
			"jmp .multi_core\n"
			".single_core:\n\t"
			"xor %eax, %eax\n"
			".multi_core:"
			);
	}
#else
	__asm
	{
		xor eax, eax
			cpuid
			cmp eax, 4			// check if cpuid supports leaf 4
			jl single_core		// Single core
			mov eax, 4
			mov ecx, 0			// start with index = 0; Leaf 4 reports
			cpuid				// at least one valid cache level
			mov Regeax, eax
			jmp multi_core

single_core:
		xor eax, eax

multi_core:

	}
#endif
	return (unsigned int)((Regeax & NUM_CORE_BITS) >> 26)+1;

}





//
// The function returns 0 when the hardware multi-threaded bit is not set.
//
unsigned int HWD_MTSupported(void)
{


	unsigned int Regedx      = 0;


	if ((CpuIDSupported() >= 1) && GenuineIntel())
	{
#ifdef LINUX
		asm volatile
			(
			"pushl %%ebx\n\t"
			"movl $1,%%eax\n\t"
			"cpuid\n\t"
			"popl %%ebx\n\t"
			: "=d" (Regedx)
			:
		: "%eax","%ecx"
			);
#else
		__asm
		{
			mov eax, 1
				cpuid
				mov Regedx, edx
		}
#endif
	}

	return (Regedx & HWD_MT_BIT);


}

//
// Function returns the maximum logical processors per physical package. Note that the number of
// AVAILABLE logical processors per physical to be used by an application might be less than this
// maximum value.
//
unsigned int MaxLogicalProcPerPhysicalProc(void)
{

	unsigned int Regebx = 0;

	if (!HWD_MTSupported()) return (unsigned int) 1;
#ifdef LINUX
	asm volatile
		(
		"pushl %%ebx\n\t"
		"movl $1,%%eax\n\t"
		"cpuid\n\t"
		"movl %%ebx, %%eax\n\t"
		"popl %%ebx\n\t"
		: "=a" (Regebx)
		:
	: "%ecx","%edx"
		);
#else
	__asm
	{
		mov eax, 1
			cpuid
			mov Regebx, ebx
	}
#endif
	return (unsigned int) ((Regebx & NUM_LOGICAL_BITS) >> 16);

}


unsigned char GetAPIC_ID(void)
{

	unsigned int Regebx = 0;
#ifdef LINUX
	asm volatile
		(
		"pushl %%ebx\n\t"
		"movl $1, %%eax\n\t"
		"cpuid\n\t"
		"movl %%ebx, %%eax\n\t"
		"popl %%ebx\n\t"
		: "=a" (Regebx)
		:
	: "%ecx","%edx"
		);

#else
	__asm
	{
		mov eax, 1
			cpuid
			mov Regebx, ebx
	}
#endif

	return (unsigned char) ((Regebx & INITIAL_APIC_ID_BITS) >> 24);

}


//
// Determine the width of the bit field that can represent the value count_item.
//
unsigned int find_maskwidth(unsigned int CountItem)
{
	unsigned int MaskWidth,
		count = CountItem;
#ifdef LINUX
	asm volatile
		(
#ifdef __x86_64__		// define constant to compile
		"push %%rcx\n\t"		// under 64-bit Linux
		"push %%rax\n\t"
#else
		"pushl %%ecx\n\t"
		"pushl %%eax\n\t"
#endif
		//		"movl $count, %%eax\n\t" //done by Assembler below
		"xorl %%ecx, %%ecx"
		//		"movl %%ecx, MaskWidth\n\t" //done by Assembler below
		: "=c" (MaskWidth)
		: "a" (count)
		//		: "%ecx", "%eax" We don't list these as clobbered because we don't want the assembler
		//to put them back when we are done
		);
	asm
		(
		"decl %%eax\n\t"
		"bsrw %%ax,%%cx\n\t"
		"jz next\n\t"
		"incw %%cx\n\t"
		//		"movl %%ecx, MaskWidth\n" //done by Assembler below
		: "=c" (MaskWidth)
		:
	);
	asm
		(
		"next:\n\t"
#ifdef __x86_64__
		"pop %rax\n\t"
		"pop %rcx"
#else
		"popl %eax\n\t"
		"popl %ecx"
#endif
		);

#else
	__asm
	{
		mov eax, count
			mov ecx, 0
			mov MaskWidth, ecx
			dec eax
			bsr cx, ax
			jz next
			inc cx
			mov MaskWidth, ecx
next:

	}
#endif
	return MaskWidth;
}


//
// Extract the subset of bit field from the 8-bit value FullID.  It returns the 8-bit sub ID value
//
unsigned char GetNzbSubID(unsigned char FullID,
						  unsigned char MaxSubIDValue,
						  unsigned char ShiftCount)
{
	unsigned int MaskWidth;
	unsigned char MaskBits;

	MaskWidth = find_maskwidth((unsigned int) MaxSubIDValue);
	MaskBits  = (0xff << ShiftCount) ^
		((unsigned char) (0xff << (ShiftCount + MaskWidth)));

	return (FullID & MaskBits);
}




//
//
//
unsigned char CPUCount(unsigned int *TotAvailLogical,
					   unsigned int *TotAvailCore,
					   unsigned int *PhysicalNum)
{
	unsigned char StatusFlag = 0;
	unsigned int numLPEnabled = 0;
	DWORD dwAffinityMask;
	int j = 0, MaxLPPerCore;
	unsigned char apicID, PackageIDMask;
	unsigned char tblPkgID[256], tblCoreID[256], tblSMTID[256];

	*TotAvailCore = 1;
	*PhysicalNum  = 1;

#ifdef LINUX
	//we need to make sure that this process is allowed to run on
	//all of the logical processors that the OS itself can run on.
	//A process could acquire/inherit affinity settings that restricts the
	// current process to run on a subset of all logical processor visible to OS.

	// Linux doesn't easily allow us to look at the Affinity Bitmask directly,
	// but it does provide an API to test affinity maskbits of the current process
	// against each logical processor visible under OS.
	int sysNumProcs = sysconf(_SC_NPROCESSORS_CONF); //This will tell us how many
	//CPUs are currently enabled.

	//this will tell us which processors this process can run on.
	cpu_set_t allowedCPUs;
	sched_getaffinity(0, sizeof(allowedCPUs), &allowedCPUs);

	for (int i = 0; i < sysNumProcs; i++ )
	{
		if ( CPU_ISSET(i, &allowedCPUs) == 0 )
		{
			StatusFlag = USER_CONFIG_ISSUE;
			return StatusFlag;
		}
	}
#else
	DWORD dwProcessAffinity, dwSystemAffinity;
	GetProcessAffinityMask(GetCurrentProcess(),
		&dwProcessAffinity,
		&dwSystemAffinity);
	if (dwProcessAffinity != dwSystemAffinity)  // not all CPUs are enabled
	{
		StatusFlag = USER_CONFIG_ISSUE;
		return StatusFlag;
	}
#endif

	// Assumwe that cores within a package have the SAME number of
	// logical processors.  Also, values returned by
	// MaxLogicalProcPerPhysicalProc and MaxCorePerPhysicalProc do not have
	// to be power of 2.

	MaxLPPerCore = MaxLogicalProcPerPhysicalProc() / MaxCorePerPhysicalProc();
	dwAffinityMask = 1;

#ifdef LINUX
	cpu_set_t currentCPU;
	while ( j < sysNumProcs )
	{
		CPU_ZERO(&currentCPU);
		CPU_SET(j, &currentCPU);
		if ( sched_setaffinity (0, sizeof(currentCPU), &currentCPU) == 0 )
		{
			sleep(0);  // Ensure system to switch to the right CPU
#else
	while (dwAffinityMask && dwAffinityMask <= dwSystemAffinity)
	{
		if (SetThreadAffinityMask(GetCurrentThread(), dwAffinityMask))
		{
			Sleep(0);  // Ensure system to switch to the right CPU
#endif
			apicID = GetAPIC_ID();


			// Store SMT ID and core ID of each logical processor
			// Shift vlaue for SMT ID is 0
			// Shift value for core ID is the mask width for maximum logical
			// processors per core

			tblSMTID[j]  = GetNzbSubID(apicID, MaxLPPerCore, 0);
			tblCoreID[j] = GetNzbSubID(apicID,
				MaxCorePerPhysicalProc(),
				(unsigned char) find_maskwidth(MaxLPPerCore));

			// Extract package ID, assume single cluster.
			// Shift value is the mask width for max Logical per package

			PackageIDMask = (unsigned char) (0xff <<
				find_maskwidth(MaxLogicalProcPerPhysicalProc()));

			tblPkgID[j] = apicID & PackageIDMask;
			numLPEnabled ++;   // Number of available logical processors in the system.

		} // if

		j++;
		dwAffinityMask = 1 << j;
	} // while

	// restore the affinity setting to its original state
#ifdef LINUX
	sched_setaffinity (0, sizeof(allowedCPUs), &allowedCPUs);
	sleep(0);
#else
	SetThreadAffinityMask(GetCurrentThread(), dwProcessAffinity);
	Sleep(0);
#endif
	*TotAvailLogical = numLPEnabled;

	//
	// Count available cores (TotAvailCore) in the system
	//
	unsigned char CoreIDBucket[256];
	DWORD ProcessorMask, pCoreMask[256];
	unsigned int i, ProcessorNum;

	CoreIDBucket[0] = tblPkgID[0] | tblCoreID[0];
	ProcessorMask = 1;
	pCoreMask[0] = ProcessorMask;

	for (ProcessorNum = 1; ProcessorNum < numLPEnabled; ProcessorNum++)
	{
		ProcessorMask <<= 1;
		for (i = 0; i < *TotAvailCore; i++)
		{
			// Comparing bit-fields of logical processors residing in different packages
			// Assuming the bit-masks are the same on all processors in the system.
			if ((tblPkgID[ProcessorNum] | tblCoreID[ProcessorNum]) == CoreIDBucket[i])
			{
				pCoreMask[i] |= ProcessorMask;
				break;
			}

		}  // for i

		if (i == *TotAvailCore)   // did not match any bucket.  Start a new one.
		{
			CoreIDBucket[i] = tblPkgID[ProcessorNum] | tblCoreID[ProcessorNum];
			pCoreMask[i] = ProcessorMask;

			(*TotAvailCore)++;	// Number of available cores in the system

		}

	}  // for ProcessorNum


	//
	// Count physical processor (PhysicalNum) in the system
	//
	unsigned char PackageIDBucket[256];
	DWORD pPackageMask[256];

	PackageIDBucket[0] = tblPkgID[0];
	ProcessorMask = 1;
	pPackageMask[0] = ProcessorMask;

	for (ProcessorNum = 1; ProcessorNum < numLPEnabled; ProcessorNum++)
	{
		ProcessorMask <<= 1;
		for (i = 0; i < *PhysicalNum; i++)
		{
			// Comparing bit-fields of logical processors residing in different packages
			// Assuming the bit-masks are the same on all processors in the system.
			if (tblPkgID[ProcessorNum]== PackageIDBucket[i])
			{
				pPackageMask[i] |= ProcessorMask;
				break;
			}

		}  // for i

		if (i == *PhysicalNum)   // did not match any bucket.  Start a new one.
		{
			PackageIDBucket[i] = tblPkgID[ProcessorNum];
			pPackageMask[i] = ProcessorMask;

			(*PhysicalNum)++;	// Total number of physical processors in the system

		}

	}  // for ProcessorNum

	//
	// Check to see if the system is multi-core
	// Check if the system is hyper-threading
	//
	if (*TotAvailCore > *PhysicalNum)
	{
		// Multi-core
		if (MaxLPPerCore == 1)
			StatusFlag = MULTI_CORE_AND_HT_NOT_CAPABLE;
		else if (numLPEnabled > *TotAvailCore)
			StatusFlag = MULTI_CORE_AND_HT_ENABLED;
		else StatusFlag = MULTI_CORE_AND_HT_DISABLED;

	}
	else
	{
		// Single-core
		if (MaxLPPerCore == 1)
			StatusFlag = SINGLE_CORE_AND_HT_NOT_CAPABLE;
		else if (numLPEnabled > *TotAvailCore)
			StatusFlag = SINGLE_CORE_AND_HT_ENABLED;
		else StatusFlag = SINGLE_CORE_AND_HT_DISABLED;


	}

	return StatusFlag;
}

#endif




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

// Status Flag
#define MULTI_CORE_AND_HT_NOT_CAPABLE		1
#define SINGLE_CORE_AND_HT_NOT_CAPABLE		2
#define SINGLE_CORE_AND_HT_DISABLED			3
#define USER_CONFIG_ISSUE					4
#define SINGLE_CORE_AND_HT_ENABLED			5
#define MULTI_CORE_AND_HT_ENABLED			6
#define MULTI_CORE_AND_HT_DISABLED			7
#define HYPERTHREADING_ENABLED              5

unsigned char CPUCount(unsigned int *logical, // Number of available logical CPU per CORE
					   unsigned int *cores, // Number of available cores per physical processor
					   unsigned int *physical); // Total number of physical processors

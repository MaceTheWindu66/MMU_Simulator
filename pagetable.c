// Starting code version 1.0 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>

#include "mmu.h"
#include "pagetable.h"
#include "memsim.h"

int pageToEvict = 1;
char diskSim[100];

typedef struct{
	int ptStartPA;
	int present;
} ptRegister;

//Bitfield struct
//This bitfield will make obtaining values from the PTE, such as protection bit, valid bit, PFN, etc much easier
typedef struct {

	uint8_t referenced : 1;
	uint8_t present : 1;
	uint8_t protection : 1;
	uint8_t valid : 1;
	uint8_t PFN : 2;
	
} PT_Entry;

//Since the physical memeory (physmem) is an array of chars, we need a union to convert this into a single char.

typedef union{
	
	PT_Entry entry;
	char c;
	
}PTE_Converter;


// Page table root pointer register values 
// One stored for each process, swapped in with process)
ptRegister ptRegVals[NUM_PROCESSES]; 

/*
 * Public Interface:
 */
 
int isMapped(int pid, int VPN){
	char* physmem = Memsim_GetPhysMem();
	PTE_Converter converter;
	converter.c = physmem[ptRegVals[pid].ptStartPA + VPN];
	return converter.entry.valid;

}

int getValFromDisk(int va){
	return diskSim[va];
}


int getPageToEvict(){
	return pageToEvict;
}

int getPresBit(int pid, int VPN){
	char* physmem = Memsim_GetPhysMem();
	PTE_Converter converter;
	converter.c = physmem[ptRegVals[pid].ptStartPA + VPN];
	return converter.entry.present;
}

/* 

* Flush the file to disk, and then flush the disk to ensure the data is written.

* Seeks over the file to the end, and then flushes the file to disk.

* Returns -1 if there was an error, and 0 if the seek was successful.

*/

int Disk_Flush(int fd, FILE* swapFile) {

	fflush(swapFile);

	off_t end_offset = lseek(fd, 0, SEEK_END);

	if(end_offset == -1){

		printf("Error on disk. Could not write to disk.\n");

		return -1;

	}

	fflush(swapFile);

	return 0;

}



/*

* Write a PAGE_SIZE worth of bytes from the physical memory starting at the targPFN to the offset in the file.

* Returns the offset in the file where the data was written.

*/

int Disk_Write(FILE* swapFile, int targPFN, int verbose) {


	//assert(PAGE_START(targPFN) % PAGE_SIZE == 0); // only frame-aligned access from this file

	char* physmem = Memsim_GetPhysMem();
	PTE_Converter converter;
	

	int diskFrameNum = ftell(swapFile);
	

	fwrite(&physmem[targPFN * 16], sizeof(char), PAGE_SIZE, swapFile);

	fflush(swapFile);
	
	converter.c = physmem[(targPFN * 16) + 1];
	converter.entry.present = 0;
	physmem[(targPFN * 16) + 1] = converter.c;
	

	if (verbose) {

		printf("Swapped Frame %d to disk at offset %d.\n", targPFN, diskFrameNum);

	}

	return diskFrameNum;

}



/*

* Read a PAGE_SIZE worth of bytes from the offset in the file to the physical memory starting at the targPFN.

* Returns the number of bytes read.

*/

int Disk_ReadFrame(int diskAddr, int targPFN) {

	//assert(PAGE_START(targPFN) % PAGE_SIZE == 0); // only frame-aligned access from this file

	FILE* swapFile = MMU_GetSwapFileHandle();

	int fd = fileno(swapFile);

	char* physmem = Memsim_GetPhysMem();

	if (lseek(fd, PAGE_START(diskAddr), SEEK_SET) == -1) {

		printf("Error on disk. Could not read from disk.\n");

		return -1;

	} 

	fread(&physmem[PAGE_START(targPFN)], sizeof(char), PAGE_SIZE, swapFile);
	
	for(int i = 1; i < 15; i++){
		physmem[(targPFN * PAGE_SIZE) + i] = diskSim[(diskAddr) + i];
	}
	
	printf("Swapped disk frame %d into physical frame %d.\n", diskAddr, targPFN);
	
	PTE_Converter converter;
	converter.c = physmem[PAGE_START(targPFN)];
	
	converter.entry.PFN = targPFN;
	
	physmem[PAGE_START(targPFN)] = converter.c;
	
}



void PT_SetPTE(int pid, int VPN, int PFN, int valid, int protection, int present, int referenced) {
	char* physmem = Memsim_GetPhysMem();
	
	PTE_Converter converter;
	PT_Entry ptEntry;
	
	ptEntry.PFN = PFN;
	ptEntry.valid = valid;
	ptEntry.protection = protection;
	ptEntry.present = present;
	ptEntry.referenced = referenced;
	
	converter.entry = ptEntry;
	
	physmem[ptRegVals[pid].ptStartPA + VPN] = converter.c;
	
	return;
}

int PT_GetRWBit(int pid, int va){
	char* physmem = Memsim_GetPhysMem();
	int VPN = VPN(va);
	PTE_Converter converter;
	
	converter.c = physmem[ptRegVals[pid].ptStartPA + VPN];
	
	return converter.entry.protection;
	
}




int PT_PageTableInit(int pid, int pa){
	char* physmem = Memsim_GetPhysMem();
	
	ptRegVals[pid].ptStartPA = pa;
	
	return 0;

 }

 void PT_PageTableCreate(int pid, int pa){
 	// Store the physical address
	ptRegVals[pid].ptStartPA = pa;
 }

 int PT_PageTableExists(int pid){
 	// check if present
 	//printf("Start PA of PID %d exists at %d\n", pid, ptRegVals[pid].ptStartPA);
	return ptRegVals[pid].ptStartPA != -1;	
	
 }

/* Gets the location of the start of the page table. If it is on disk, brings it into memory. */
int PT_GetRootPtrRegVal(int pid){
	return ptRegVals[pid].ptStartPA;
}

/*
 * Evicts the next page. 
 * Updates the corresponding information in the page table, returns the location that was evicted.
 * 
 * The supplied input and output used in autotest.sh *RR tests, uses the round-robin algorithm. 
 * You may also implement the simple and powerful Least Recently Used (LRU) policy, 
 * or another fair algorithm.
 */
int PTNextEvictionRR() {
	
	int nextPgToEvict = pageToEvict;

	pageToEvict++;

	pageToEvict = pageToEvict % NUM_PAGES; //wraps around to first page if it goes over
	
	return nextPgToEvict;

}

//Returns diskFrame written to and PFN of evicted page
int PT_Evict(int targPFN, int diskFrameNum, int verbose) {

	char* physmem = Memsim_GetPhysMem();

	FILE* swapFile = MMU_GetSwapFileHandle();

	int fd = fileno(swapFile);
	

	if (Disk_Flush(fd, swapFile) == -1) {
		return -1;

	} else { 
		
		targPFN = PTNextEvictionRR(); // call the RR policy decider
		if(targPFN == 0){
			targPFN = PTNextEvictionRR();
		}
		diskFrameNum = Disk_Write(swapFile, targPFN, verbose);
		
		for(int i = 0; i < 16; i++){
			diskSim[(diskFrameNum) + i] = physmem[(PAGE_SIZE * targPFN) + i];
		}
	
	}
	
	
	return targPFN;

}


/*
 * Searches through the process's page table. If an entry is found containing the specified VPN, 
 * return the address of the start of the corresponding physical page frame in physical memory. 
 *
 * If the physical page is not present, first swaps in the phyical page from the physical disk,
 * and returns the physical address.
 * 
 * Otherwise, returns -1.
 */

int PT_VPNtoPA(int pid, int VPN){
	
	char* physmem = Memsim_GetPhysMem();
	
	//a union type def to convert a page table entry (bitfield) to a char
	PTE_Converter converter;
	
	converter.c = physmem[ptRegVals[pid].ptStartPA + VPN];
	

	// Extract the valid bit from the page table entry
	int valid_bit = converter.entry.valid;


	// Check if the page table entry is valid
	if (valid_bit == 1) {
		// Extract the physical frame number from the page table entry
		int pfn = converter.entry.PFN;
		int pa = pfn * PAGE_SIZE;

		return pa;
	} else {
		// Page is not valid, return -1 to indicate failure

		return -1;
	}
	
}

/*
 * Finds the page table entry corresponding to the VPN, and checks
 * to see if the protection bit is set to 1 (readable and writable).
 * If it is 1, it returns TRUE, and FALSE if it is not found or is 0.
 */
int PT_PIDHasWritePerm(int pid, int VPN){
	char* physmem = Memsim_GetPhysMem();
	
	PTE_Converter converter;
	
	converter.c = physmem[ptRegVals[pid].ptStartPA + VPN];
	
	return converter.entry.protection;
}

/* Initialize the register values for each page table location (per process). */
void PT_Init() {
	for(int i = 0; i < NUM_PROCESSES; i++) {
		// initialize to 0 to show no page table
		ptRegVals[i].ptStartPA = -1;
		ptRegVals[i].present = 0;
	}
}

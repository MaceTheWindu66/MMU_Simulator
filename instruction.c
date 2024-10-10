// Starting code version v1.0

#include <stdio.h>
#include <stdint.h>

#include "memsim.h"
#include "pagetable.h"
#include "mmu.h"

/*
 * Searches the memory for a free page, and assigns it to the process's virtual address. If value is
 * 0, the page has read only permissions, and if it is 1, it has read/write permissions.
 * If the process does not already have a page table, one is assigned to it. If there are no more empty
 * pages, the mapping fails.
 */
int Instruction_Map(int pid, int va, int value_in){
    int pa;
    char* physmem = Memsim_GetPhysMem();

    int rwBit = PT_GetRWBit(pid, va);

    if (value_in != 0 && value_in != 1) {
        printf("Invalid value for map instruction. Value must be 0 or 1.\n");
        return 1;
    }
    int targPFN;
    int override = 0;
    
    if (PT_PageTableExists(pid) == 0) {
        pa = Memsim_FirstFreePFN();
         if (pa == -1) {
         	targPFN = 0;
        	targPFN = PT_Evict(targPFN, 0, 1);
        	PT_SetPTE((pid-1), VPN(va), PFN(pa), 1, 0, 0, 0);
        	PT_PageTableInit(pid, pa);
        	printf("Put page table for PID %d into physical frame %d.\n", pid, targPFN);
        	targPFN = PT_Evict(targPFN, 0, 1);
        	PFNFree(targPFN);
        	override = 1;
    	} else {
		PT_PageTableInit(pid, pa);
		printf("Put page table for PID %d into physical frame %d.\n", pid, PFN(pa));
        }
    }
    
    if(override == 0){
    //check if pa has been mapped to
	    if ((pa = PT_VPNtoPA(pid, VPN(va))) != -1) {
	    	//If protection is being changed, update permission.
	    	if(rwBit != value_in){
	    		printf("Updating permissions of virtual address %d\n", va);
	    		PT_SetPTE(pid, VPN(va), PFN(pa), 1, value_in, 1, 0);
	    		return 0;
	    	// If not, give an error saying a mapping has been made with the same permission
	    	} else {
			printf("Error: Virtual page already mapped into physical frame %d with the rwBit of %d.\n", PFN(pa), rwBit);
			return 1;
		}
	    }
    }

    pa = Memsim_FirstFreePFN();
    

    int vpn = VPN(va);
    PT_SetPTE(pid, vpn, PFN(pa), 1, value_in, 1, 0);

    printf("Mapped virtual address %d (page %d) into physical frame %d.\n", va, vpn, PFN(pa));

    return 0;
}


/**
* If the virtual address is valid and has write permissions for the process, store
* value in the virtual address specified.
*/
int Instruction_Store(int pid, int va, int value_in){
	int pa;
	char* physmem = Memsim_GetPhysMem();

	int valid = isMapped(pid, VPN(va));

	if(isMapped == 0){
		printf("Address has not been mapped to. Invalid instruction. \n");
		return 1;
	}

	if (value_in < 0 || value_in > MAX_VA) { //check for a valid value (instructions validate the value_in)
		printf("Invalid value for store instruction. Value must be 0-255.\n"); 
		return 1;
	}
	if (PT_PIDHasWritePerm(pid, VPN(va)) == 0) { //check if memory is writable
		printf("Error: virtual address %d does not have write permissions.\n", va);
		return 1;
	  }

	
	int offset = PAGE_OFFSET(va);
	
	pa = MMU_TranslateAddress(pid, VPN(va), offset);
	
	// Translate the virtual address into its physical address for the process
	// Hint use MMU_TranslateAddress 

	printf("Stored value %u at virtual address %d (physical address %d)\n", value_in, va, pa);

	// Finally stores the value in the physical memory address, mapped from the virtual address
	// Hint, modify a byte in physmem using a pa and value_in
	
	physmem[pa] = value_in;

	return 0;
}

/*
 * Translate the virtual address into its physical address for
 * the process. If the virutal memory is mapped to valid physical memory, 
 * return the value at the physical address. Permission checking is not needed,
 * since we assume all processes have (at least) read permissions on pages.
 */
int Instruction_Load(int pid, int va){
	int pa;
	char* physmem = Memsim_GetPhysMem(); 

	if(isMapped == 0){
		printf("Address has not been mapped to. Invalid instruction. \n");
		return 1;
	}
	
	//check for a valid value (instructions validate the value_in)
	// Hint use MMU_TranslateAddress to do a successful VA -> PA translation.
	
	int offset = PAGE_OFFSET(va);
	
	pa = MMU_TranslateAddress(pid, VPN(va), offset);
	
	int presentBit = getPresBit(pid, VPN(va));
	int rwBit = PT_GetRWBit(pid, va);
	if(presentBit == 0){
		int PFN = PT_Evict(0, 0, 1);
		Disk_ReadFrame(16, PFN);
		PT_SetPTE(pid, VPN(va), PFN, 1, rwBit, 1, 0);
		pa = MMU_TranslateAddress(pid, VPN(va), offset);
		int value = physmem[pa];
		printf("The value %u was found at virtual address %d (physical address: %d)\n", value, va, pa);
		return 0;
	}
	
	if (pa != -1) {
		uint8_t value = physmem[pa]; // And this value would be copied to the user program's register!
		printf("The value %u was found at virtual address %d (physical address: %d)\n", value, va, pa);
	} else {
		printf("Error: The virtual address %d is not valid.\n", va);
		return 1;
	}
	return 0;
}

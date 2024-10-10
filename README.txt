Maddux Berry
Grace Robinson

Memory Management Unit Project

How to run:
	Run the 'make' command.
	Input ./mmu to being the Memory Management Unit
	Input instructions as the following:

		- process_id,instruction_type,virtual_address,value_in

		When using load, value_in MUST BE 'NA'
	
		example: 0,load,7,NA
	To end the file, just do Ctrl+C.


This code simulates an MMU virtually. When run, you have 3 commands, as instructed. These 3 commands are map, load, and store. map creates a PTE entry.
If there is no page table for the process ID, it creates a page table as well. These each take up a frame of memory. Store is the command that allows
you to store values at virtual addresses into physical memory. Load then outputs these stored values as directed.

In a case where the memory is full, the program will evict pages or swap pages in/out to/from the disk. There is a known bug in our code where the values
at the virtual/physical address don't print out properly specifically after a swap happens. This bug only occurs after being swapped back into physmem from
disk. Was not able to hash out this bug before the due date. However, swaps out to the disk work, and physical/virtual address mappings are updated accordingly.

To test our code, we used the sample output given to us in the instructions for the project. If you follow these instructions, the output should match for
the most part to the expected values before the last line of instructions.

How it works:

	PTEs and PTs are initialized as bit fields, with a typedef union called PTE_Converter being defined so we can put these into physical memory,
	which is simulated as an array of chars. When you call map, the program creates a Page Table if one is needed for the process ID, and then sets
	/ maps a page table entry. This entry, as well as the page table, is stored in a spot in physical memory relative to the PFN and the VPN.

	When you call store, the function will store the value input into the physical memory array, using MMU_TranslateAddress() function. Stores a value
	relative to the virtual address into the physical memory.

	When you call load, one of two things will happen. If the virtual address no longer has a physical mapping (AKA, the frame had previously been 
	swapped out), it will swap the frame with the virtual address back into the physical memory, albeit (more than likely) in a different frame. It
	then prints this value. Other than that, as long as the PTE/address is valid, the value stored at the virtual AD will be printed to the output.

Here is a sample output:

Instruction? 0,map,0,0
Put page table for PID 0 into physical frame 0.
Mapped virtual address 0 (page 0) into physical frame 1.
Instruction? 0,store,7,255
Error: virtual address 7 does not have write permissions.
Instruction? 0,map,0,1
Updating permissions of virtual address 0
Instruction? 0,store,7,255
Stored value 255 at virtual address 7 (physical address 23)
Instruction? 0,map,10,1
Error: Virtual page already mapped into physical frame 1 with the rwBit of 1.
Instruction? 0,map,16,0,
Mapped virtual address 16 (page 1) into physical frame 2.
Instruction? 0,map,32,0
Mapped virtual address 32 (page 2) into physical frame 3.
Instruction? 1,map,0,0
Swapped Frame 1 to disk at offset 0.
Put page table for PID 1 into physical frame 1.
Swapped Frame 2 to disk at offset 16.
Mapped virtual address 0 (page 0) into physical frame 2.
Instruction? 0,load,7,NA
Swapped Frame 3 to disk at offset 32.
Swapped disk frame 16 into physical frame 3.
The value 0 was found at virtual address 7 (physical address: 55)
Instruction? ^C

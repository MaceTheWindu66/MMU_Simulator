// Starting code version 1.0

/*
 * Public Interface:
 */

#define NUM_PROCESSES 4

/*
 * Public Interface:
 */
void PT_SetPTE(int process_id, int VPN, int PFN, int valid, int protection, int present, int referenced);
int PT_GetRWBit(int pid, int va);
void PT_AddPTEToPageTable(int process_id, int VPN, int PFN, int valid, int protection, int present, int referenced);
int PT_PageTableInit(int process_id, int pageAddress);
void PT_PageTableCreate(int process_id, int pageAddress);
int PT_PageTableExists(int process_id);
int PT_GetRootPtrRegVal(int process_id);
int PT_Evict(int targPFN, int diskFrameNum, int verbose);
int getPageToEvict();
int getValFromDisk(int va);
int isMapped(int pid, int VPN);
int getPresBit(int pid, int VPN);
int Disk_ReadFrame(int diskAddr, int targPFN);
int Disk_Write(FILE* swapFile, int targPFN, int verbose);
int Disk_Flush(int fd, FILE* swapFile);
int PTNextEvictionRR();
int PT_VPNtoPA(int process_id, int VPN);
int PT_PIDHasWritePerm(int process_id, int VPN);
void PT_Init();

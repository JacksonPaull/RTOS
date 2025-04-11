#import "../RTOS_Labs_common/os.h"

uint32_t SVC_OS_Id(void);
void SVC_OS_Kill(void);
void SVC_OS_Sleep(uint32_t t);
uint32_t SVC_OS_Time(void);
int SVC_OS_AddThread(void(*t)(void), uint32_t s, uint32_t p);
void SVC_InitSemaphore(Sema4Type *semaPt, int32_t value); 
void SVC_ContextSwitch();
void SVC_InitSemaphore(Sema4Type *semaPt, int32_t value); 
void SVC_Wait(Sema4Type *semaPt);
void SVC_Signal(Sema4Type *semaPt);
void SVC_bWait(Sema4Type *semaPt);
void SVC_bSignal(Sema4Type *semaPt);
int SVC_AddPeriodicThread(void(*task)(void), 
   uint32_t period, uint32_t priority);
int SVC_AddSW1Task(void(*task)(void), uint32_t priority);
int SVC_AddSW2Task(void(*task)(void), uint32_t priority);
void SVC_Suspend(void);
unsigned long SVC_LockScheduler(void);
void SVC_UnLockScheduler(unsigned long previous);
void SVC_Fifo_Init(uint32_t size);
int SVC_Fifo_Put(uint32_t data);
uint32_t SVC_Fifo_Get(void);
int32_t SVC_Fifo_Size(void);
void SVC_MailBox_Init(void);
void SVC_MailBox_Send(uint32_t data);
uint32_t SVC_MailBox_Recv(void);
uint32_t SVC_TimeDifference(uint32_t start, uint32_t stop);
void SVC_ClearMsTime(void);
uint32_t SVC_MsTime(void);
int SVC_RedirectToFile(const char *name);
int SVC_EndRedirectToFile(void);
int SVC_RedirectToUART(void);
int SVC_RedirectToST7735(void);
int SVC_AddProcess(void(*entry)(void), void *heaps[], unsigned long stack_pri[]); 


;/*****************************************************************************/
;/* OSasm.s: low-level OS commands, written in assembly                       */
;/* derived from uCOS-II                                                      */
;/*****************************************************************************/
;Jonathan Valvano, OS Lab2/3/4/5, 1/12/20
;Students will implement these functions as part of EE445M/EE380L.12 Lab

        AREA |.text|, CODE, READONLY, ALIGN=2
        THUMB
        REQUIRE8
        PRESERVE8

        EXTERN  RunPt            ; currently running thread

        EXPORT  StartOS
        EXPORT  ContextSwitch
        EXPORT  PendSV_Handler
        EXPORT  SVC_Handler
		
		IMPORT scheduler_next

NVIC_INT_CTRL   EQU     0xE000ED04                              ; Interrupt control state register.
NVIC_SYSPRI14   EQU     0xE000ED22                              ; PendSV priority register (position 14).
NVIC_SYSPRI15   EQU     0xE000ED23                              ; Systick priority register (position 15).
NVIC_LEVEL14    EQU           0xEF                              ; Systick priority value (second lowest).
NVIC_LEVEL15    EQU           0xFF                              ; PendSV priority value (lowest).
NVIC_PENDSVSET  EQU     0x10000000                              ; Value to trigger PendSV exception.
STCURRENT 		EQU 	0xE000E018								; Systick current value (reset on context switch)



StartOS	
;CPSIE I					; Enable Interupts
	PUSH {LR}
	BL scheduler_next	; R0 <-- pointer to next thread
	POP {LR}
	
	; 2) Reset STCURRENT=0
	LDR R1, =STCURRENT
	MOV R2, #0
	STR R2, [R1]
	
	; 3) Perform the context switch
	CPSID I
	LDR R1, =RunPt
	LDR R2, [R1]		; R2 = run_pt
	STR R0, [R1]		; run_pt = next_pt
	LDR R2, [R0, #12]	; Load new stack pointer

	LDM	R2, {R4-R11}
	ADDS R2, R2, #0x20
	
	LDM	R2, {R0,R1,R3}
	ADDS R2, R2, #0xC
	LDM	R2, {R3,R12}		; pop regs from the stack
	ADDS R2, R2, #0x8
	LDM R2, {LR}					; pop dummy LR
	ADDS R2, R2, #0x4
	LDM R2, {LR}					; pop real LR	
	ADDS R2, R2, #0x4
	LDM R2, {R0}					; pop initial PSR
	ADDS R2, R2, #0x4
	MSR PSP, R2
	
	; switch from MSP to PSP
	MRS R0, CONTROL
	ORR R1, R0, #2
	MSR CONTROL, R1
	ISB
	
	CPSIE I				; enable interrupts
	
	; switch to unprivileged mode
	MRS R0, CONTROL
	ORR R1, R0, #1
	MSR CONTROL, R1
	ISB
	
	;SUBS R2, R2, #0x20
	;LDM	R2, {R0,R1,R2}
	
	;CPSIE I				; enable interrupts
	;SVC #0
	
    BX 		LR                 ; start first thread
	
	;BL ContextSwitch		; Call ContextSwitch (and return to infinite loop when done)
	B OSStartHang			; Enter an infinite loop until the interrupt is serviced
							; Alternatively, branch to OS background thread
							; e.g. replace [46] with `BX LR` and update OS_Launch() to call some (non-returning) task
							;		or replace [46] with `b func_label` to some (non-returning) task



OSStartHang
    B       OSStartHang        ; Should never get here

	

;********************************************************************************************************
;                               PERFORM A CONTEXT SWITCH (From task level)
;                                           void ContextSwitch(void)
;
; Note(s) : 1) ContextSwitch() is called when OS wants to perform a task context switch.  This function
;              triggers the PendSV exception which is where the real work is done.
;********************************************************************************************************

ContextSwitch
		LDR R0, =NVIC_INT_CTRL
		LDR R1, [R0]
		MOV R2, #1		; R2 <- 1<<28 (28th bit)
		LSL R2, R2, #28
		ORR R1, R1, R2	; R1 <- INT_CTRL | 0x10000000
		STR R1, [R0]	; INT_CTRL <- R1 = INT_CTRL | 0x10000000
		BX		LR
		
    

;********************************************************************************************************
;                                         HANDLE PendSV EXCEPTION
;                                     void OS_CPU_PendSVHandler(void)
;
; Note(s) : 1) PendSV is used to cause a context switch.  This is a recommended method for performing
;              context switches with Cortex-M.  This is because the Cortex-M3 auto-saves half of the
;              processor context on any exception, and restores same on return from exception.  So only
;              saving of R4-R11 is required and fixing up the stack pointers.  Using the PendSV exception
;              this way means that context saving and restoring is identical whether it is initiated from
;              a thread or occurs due to an interrupt or exception.
;
;           2) Pseudo-code is:
;              a) Get the process SP, if 0 then skip (goto d) the saving part (first context switch);
;              b) Save remaining regs r4-r11 on process stack;
;              c) Save the process SP in its TCB, OSTCBCur->OSTCBStkPtr = SP;
;              d) Call OSTaskSwHook();
;              e) Get current high priority, OSPrioCur = OSPrioHighRdy;
;              f) Get current ready thread TCB, OSTCBCur = OSTCBHighRdy;
;              g) Get new process SP from TCB, SP = OSTCBHighRdy->OSTCBStkPtr;
;              h) Restore R4-R11 from new process stack;
;              i) Perform exception return which will restore remaining context.
;
;           3) On entry into PendSV handler:
;              a) The following have been saved on the process stack (by processor):
;                 xPSR, PC, LR, R12, R0-R3
;              b) Processor mode is switched to Handler mode (from Thread mode)
;              c) Stack is Main stack (switched from Process stack)
;              d) OSTCBCur      points to the OS_TCB of the task to suspend
;                 OSTCBHighRdy  points to the OS_TCB of the task to resume
;
;           4) Since PendSV is set to lowest priority in the system (by OSStartHighRdy() above), we
;              know that it will only be run when no other exception or interrupt is active, and
;              therefore safe to assume that context being switched out was using the process stack (PSP).
;********************************************************************************************************

PendSV_Handler
	; 1) Call the scheduler	
	PUSH {LR}
	BL scheduler_next	; R0 <-- pointer to next thread
	POP {LR}
	
	; 2) Reset STCURRENT=0
	LDR R1, =STCURRENT
	MOV R2, #0
	STR R2, [R1]
	
	; 3) Perform the context switch
	CPSID I
	LDR R1, =RunPt
	LDR R2, [R1]		; R2 = run_pt
	
	CMP R0, R2			; Exit early if the new thread is the current thread
	;BEQ PendSV_exit
	
	MRS R3, PSP ; R=PSP, the process stack pointer
	SUBS R3, R3, #0x20 
	STM R3, {R4-R11} 
	
	;PUSH {R4-R11}		; Save registers
	STR R3, [R2, #12] 	; Save stack pointer in TCB
	STR R0, [R1]		; run_pt = next_pt
	LDR R3, [R0, #12]	; Load new stack pointer
	;POP {R4-R11}		; Restore registers
	
	LDM R3, {R4-R11}
	ADDS R3, R3, #0x20
	
	MSR PSP, R3 ; Load PSP with new process SP
	
PendSV_exit
	ORR LR, LR, #0x04 ; 0xFFFFFFFD (return to thread PSP)
	CPSIE I
    BX	LR              ; Exception return will restore remaining context   
    

;********************************************************************************************************
;                                         HANDLE SVC EXCEPTION
;                                     void OS_CPU_SVCHandler(void)
;
; Note(s) : SVC is a software-triggered exception to make OS kernel calls from user land. 
;           The function ID to call is encoded in the instruction itself, the location of which can be
;           found relative to the return address saved on the stack on exception entry.
;           Function-call paramters in R0..R3 are also auto-saved on stack on exception entry.
;********************************************************************************************************

        IMPORT    OS_Id
        IMPORT    OS_Kill
        IMPORT    OS_Sleep
        IMPORT    OS_Time
        IMPORT    OS_AddThread
		IMPORT 	  OS_InitSemaphore
		IMPORT	  OS_Wait
		IMPORT OS_Signal
		IMPORT OS_bWait
		IMPORT OS_bSignal
		IMPORT OS_AddPeriodicThread
		IMPORT OS_AddSW1Task
		IMPORT OS_AddSW2Task
		IMPORT OS_Suspend
		IMPORT OS_LockScheduler
		IMPORT OS_UnLockScheduler
		IMPORT OS_Fifo_Init
		IMPORT OS_Fifo_Get
		IMPORT OS_Fifo_Put
		IMPORT OS_Fifo_Size
		IMPORT OS_MailBox_Init
		IMPORT OS_MailBox_Send
		IMPORT OS_MailBox_Recv
		IMPORT OS_TimeDifference
		IMPORT OS_ClearMsTime
		IMPORT OS_MsTime
;		IMPORT OS_RedirectToFile
;		IMPORT OS_EndRedirectToFile
;		IMPORT OS_RedirectToUART
;		IMPORT OS_RedirectToST7735
		IMPORT OS_AddProcess
;		IMPORT TxFifo_PutSVC
;		IMPORT TxFifo_GetSVC
		IMPORT OS_get_current_TCB
			
SVC_Handler
; put your Lab 5 code here
	CPSID I
	MRS R2, PSP 
	PUSH {R4}
	MOV R4, R2

	LDR R12, [R4, #24]
	LDRH R12, [R12, #-2]
	BIC R12, #0xFF00
	LDM R4, {R0-R3}
	SUBS R4, R4, #0x4 
	STM R4, {LR} 
	LDR LR, =svc_done
	
	;
	; ADD R12, R12, PC
	; MOV PC, R12
	
	CMP R12, #0
	BEQ OS_Id
	
	CMP R12, #1
	BEQ OS_Kill
	
	CMP R12, #2
	BEQ OS_Sleep
	
	CMP R12, #3
	BEQ OS_Time
	
	CMP R12, #4
	BEQ OS_AddThread
	
	CMP R12, #5
	BEQ ContextSwitch
	
	CMP R12, #6
	BEQ OS_InitSemaphore
	
	CMP R12, #7
	BEQ OS_Wait
	
	CMP R12, #8
	BEQ OS_Signal
	
	CMP R12, #9
	BEQ OS_bWait
	
	CMP R12, #10
	BEQ OS_bSignal
	
	CMP R12, #11
	BEQ OS_AddPeriodicThread

	CMP R12, #12
	BEQ OS_AddSW1Task

	CMP R12, #13
	BEQ OS_AddSW2Task
	
	CMP R12, #14
	BEQ OS_Suspend

	CMP R12, #15
	BEQ OS_LockScheduler

	CMP R12, #16
	BEQ OS_UnLockScheduler
	
	CMP R12, #17
	BEQ OS_Fifo_Init
	
	CMP R12, #18
	BEQ OS_Fifo_Put
	
	CMP R12, #19
	BEQ OS_Fifo_Get

	CMP R12, #20
	BEQ OS_Fifo_Size
	
	CMP R12, #21
	BEQ OS_MailBox_Init
	
	CMP R12, #22
	BEQ OS_MailBox_Send
	
	CMP R12, #23
	BEQ OS_MailBox_Recv
	
	CMP R12, #24
	BEQ OS_TimeDifference
	
	CMP R12, #25
	BEQ OS_ClearMsTime
	
	CMP R12, #26
	BEQ OS_MsTime
	
;	CMP R12, #27
;	BEQ OS_RedirectToFile
;	
;	CMP R12, #28
;	BEQ OS_EndRedirectToFile
;	
;	CMP R12, #29
;	BEQ OS_RedirectToUART
;	
;	CMP R12, #30
;	BEQ OS_RedirectToST7735
	
	CMP R12, #31
	BEQ OS_AddProcess
	
;	CMP R12, #32
;	BEQ TxFifo_PutSVC
;	
;	CMP R12, #33
;	BEQ TxFifo_GetSVC
	
	CMP R12, #34
	BEQ OS_get_current_TCB
	
	
svc_done
	LDM R4, {LR}
	ADDS R4, R4, #0x4
	;POP {LR}

	STR R0,[R4] ; Store return value
	
	POP {R4}
	CPSIE I
    BX      LR                   ; Return from exception

    ALIGN
    END

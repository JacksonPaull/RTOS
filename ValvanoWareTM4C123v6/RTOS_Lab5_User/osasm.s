;/*****************************************************************************/
;/* OSasm.s: low-level OS commands, written in assembly                       */
;/*****************************************************************************/
;Jonathan Valvano/Andreas Gerstlauer, OS Lab 5 solution, 2/28/16


        AREA |.text|, CODE, READONLY, ALIGN=2
        THUMB
        REQUIRE8
        PRESERVE8

		EXPORT	SVC_OS_Id
        EXPORT  SVC_OS_Sleep
		EXPORT	SVC_OS_Kill
		EXPORT	SVC_OS_Time
		EXPORT	SVC_OS_AddThread
		EXPORT SVC_TimeDifference
			
			
SVC_OS_Id
	SVC		#0
	BX		LR

SVC_OS_Kill
	SVC		#1
	BX		LR

SVC_OS_Sleep
	SVC		#2
	BX		LR

SVC_OS_Time
	SVC		#3
	BX		LR

SVC_OS_AddThread
	SVC		#4
	BX		LR
	
SVC_TimeDifference
	SVC #24
	BX  LR

    ALIGN
    END

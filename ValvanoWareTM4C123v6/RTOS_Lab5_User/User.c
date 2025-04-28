// User.c
// Runs on LM4F120/TM4C123
// Standalone user-level process example

#include <stdio.h>
#include <stdint.h>
#include "inc/hw_types.h"
#include "inc/tm4c123gh6pm.h"

#include "OS.h"
#include "Display.h"

#define PF2     (*((volatile uint32_t *)0x40025010))
#define PF3     (*((volatile uint32_t *)0x40025020))

unsigned int line = 0;

void thread(void)
{
  unsigned int id;
	
  id = SVC_OS_Id();
  PF3 ^= 0x08;
  Display_Message(0,line++, "Thread: ", id);
  SVC_OS_Sleep(2000);
  Display_Message(0,line++, "Thread dying: ", id);
  PF3 ^= 0x08;
  SVC_OS_Kill();
}

int main(void)
{
  unsigned int id;
  unsigned long time;
	
  id = SVC_OS_Id();
  PF2 ^= 0x04;
  Display_Message(0,line++, "Hello world: ", id);
  SVC_OS_AddThread(thread, 128, 1);
  time = SVC_OS_Time();
  SVC_OS_Sleep(1000);
  time = (((SVC_TimeDifference(time, SVC_OS_Time()))/1000ul)*125ul)/10000ul;
  Display_Message(0,line++, "Sleep time: ", time);
  PF2 ^= 0x04;
  SVC_OS_Kill();
}

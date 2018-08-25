#ifndef __INTERRUPT_HANDLERS_H__
#define __INTERRUPT_HANDLERS_H__

#include "types.h"
#include "multiprocessor.h"

/**
  < Interrupt Mode in Multi-core Processor>
  - PIC mode
    : deliver interrupts only to BSP as if it's single-core processor.
      PIC -> CPU are connected directly.
      BIOS decides PIC mode or virtual wire mode.
      
  - virtual wire mode using Local APIC
    : deliver interrupts only to BSP as if it's single-core processor.
      PIC -> Local APIC -> CPU are connected.
      BIOS decides PIC mode or virtual wire mode.
      
  - virtual wire mode using IO APIC
    : deliver interrupts only to BSP as if it's single-core processor.
      PIC -> IO APIC -> Local APIC -> CPU are connected.
      BIOS decides PIC mode or virtual wire mode.
      
  - symmetric IO mode
    : general interrupt mode in multi-core processor
      IO APIC receives interrupts from devices directly.
      Interrupt load balancing can be applied using Interrupt Vector Table of IO APIC.
*/

#define INTERRUPT_MAXVECTORCOUNT       16 // interrupt vector count only for interrupt from ISA bus.
#define INTERRUPT_LOADBALANCINGDIVIDOR 10 // process load balancing when interrupt count reaches the multiple of 10.

#pragma pack(push, 1)

typedef struct k_InterruptManager {
	qword interruptCounts[MAXPROCESSORCOUNT][INTERRUPT_MAXVECTORCOUNT]; // interrupt counts by core * IRQ
	bool symmetricIoMode; // symmetric io mode flag
	bool loadBalancing;   // interrupt load balancing flag
} InterruptManager;

#pragma pack(pop)

void k_initInterruptManager(void);
void k_setSymmetricIoMode(bool symmetricIoMode);
void k_setInterruptLoadBalancing(bool loadBalancing);
void k_increaseInterruptCount(int irq);
void k_sendEoi(int irq);
InterruptManager* k_getInterruptManager(void);
void k_processLoadBalancing(int irq);

/**
  < Exception/Interrupt Message Print Position >
  1. big print in top-center                        : k_commonExceptionHandler(Exception)
  2. print in the first position of the first line  : k_keyboardHandler(INT), k_deviceNotAvailableHandler(EXC)
  3. print in the second position of the first line : k_hddHandler(INT)
  4. print in the last position of the first line   : k_timerHandler(INT), k_commonInterruptHandler(INT)
*/

/* Exception Handlers */
void k_commonExceptionHandler(int vector, qword errorCode);
void k_deviceNotAvailableHandler(int vector);

/* Interrupt Handlers */
void k_commonInterruptHandler(int vector);
void k_timerHandler(int vector);
void k_keyboardHandler(int vector);
void k_mouseHandler(int vector);
void k_hddHandler(int vector);

#endif // __INTERRUPT_HANDLERS_H__

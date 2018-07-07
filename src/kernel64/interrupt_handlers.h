#ifndef __INTERRUPT_HANDLERS_H__
#define __INTERRUPT_HANDLERS_H__

#include "Types.h"

/**
  ====================================================================================================
   < Exception/Interrupt Message Print Position >
   1. big print in top-center                        : k_commonExceptionHandler(Exception)
   2. print in the first position of the first line  : k_keyboardHandler(INT), k_deviceNotAvailableHandler(EXC)
   3. print in the second position of the first line : k_hddHandler(INT)
   4. print in the last position of the first line   : k_timerHandler(INT), k_commonInterruptHandler(INT)
  ====================================================================================================
 */

void k_commonExceptionHandler(int vectorNumber, qword errorCode);
void k_commonInterruptHandler(int vectorNumber);
void k_keyboardHandler(int vectorNumber);
void k_timerHandler(int vectorNumber);
void k_deviceNotAvailableHandler(int vectorNumber);
void k_hddHandler(int vectorNumber);

#endif // __INTERRUPT_HANDLERS_H__

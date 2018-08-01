#ifndef __ISR_H__
#define __ISR_H__

/* Exception Handling ISR (21) */
void k_isrDivideError(void);
void k_isrDebug(void);
void k_isrNmi(void);
void k_isrBreakPoint(void);
void k_isrOverflow(void);
void k_isrBoundRangeExceeded(void);
void k_isrInvalidOpcode(void);
void k_isrDeviceNotAvailable(void);
void k_isrDoubleFault(void);
void k_isrCoprocessorSegmentOverrun(void);
void k_isrInvalidTss(void);
void k_isrSegmentNotPresent(void);
void k_isrStackSegmentFault(void);
void k_isrGeneralProtection(void);
void k_isrPageFault(void);
void k_isr15(void);
void k_isrFpuError(void);
void k_isrAlignmentCheck(void);
void k_isrMachineCheck(void);
void k_isrSimdError(void);
void k_isrEtcException(void);

/* Interrupt Handling ISR (17) */
void k_isrTimer(void);
void k_isrKeyboard(void);
void k_isrSlavePic(void);
void k_isrSerialPort2(void);
void k_isrSerialPort1(void);
void k_isrParallelPort2(void);
void k_isrFloppyDisk(void);
void k_isrParallelPort1(void);
void k_isrRtc(void);
void k_isrReserved(void);
void k_isrNotUsed1(void);
void k_isrNotUsed2(void);
void k_isrMouse(void);
void k_isrCoprocessor(void);
void k_isrHdd1(void);
void k_isrHdd2(void);
void k_isrEtcInterrupt(void);

#endif // __ISR_H__

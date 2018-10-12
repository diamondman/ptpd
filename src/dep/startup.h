#ifndef STARTUP_H_
#define STARTUP_H_

#include "ptp_primitives.h"
#include "datatypes_stub.h"

void restartSubsystems(RunTimeOpts *rtOpts, PtpClock *ptpClock);
void ptpdShutdown(PtpClock*);
PtpClock * ptpClockCreate(const RunTimeOpts*, Integer16* ret);

#endif /* include guard */

#ifndef STARTUP_H_
#define STARTUP_H_

#include "dep/iniparser/dictionary.h"
#include "ptp_primitives.h"
#include "datatypes_stub.h"


void applyConfig(dictionary *baseConfig, RunTimeOpts *rtOpts, PtpClock *ptpClock);
void restartSubsystems(RunTimeOpts *rtOpts, PtpClock *ptpClock);
void checkSignals(RunTimeOpts * rtOpts, PtpClock * ptpClock);

void enable_runtime_debug(void );
void disable_runtime_debug(void );

void ptpdShutdown(PtpClock*);
PtpClock * ptpClockCreate(RunTimeOpts*, Integer16* ret);
Boolean sysPrePtpClockInit(RunTimeOpts*, Integer16* ret);

#ifdef PTPD_FEATURE_NTP
void ntpSetup(RunTimeOpts *rtOpts, PtpClock *ptpClock);
#endif

#define D_ON      do { enable_runtime_debug();  } while (0);
#define D_OFF     do { disable_runtime_debug(); } while (0);

#endif /* include guard */

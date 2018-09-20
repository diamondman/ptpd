#ifndef SERVO_H_
#define SERVO_H_

#include "ptp_primitives.h"
#include "ptp_datatypes.h"
#include "dep/datatypes_dep.h"
#include "datatypes.h"

void resetWarnings(const RunTimeOpts * rtOpts, PtpClock * ptpClock);
void initClock(const RunTimeOpts*,PtpClock*);
void updateDelay (one_way_delay_filter*, const RunTimeOpts*, PtpClock*,TimeInternal*);
void updatePeerDelay (one_way_delay_filter*, const RunTimeOpts*,PtpClock*,TimeInternal*,Boolean);
void updateOffset(TimeInternal*,TimeInternal*,
  offset_from_master_filter*,const RunTimeOpts*,PtpClock*,TimeInternal*);
void stepClock(const RunTimeOpts * rtOpts, PtpClock * ptpClock);

void adjFreq_wrapper(const RunTimeOpts * rtOpts, PtpClock * ptpClock, double adj);

void checkOffset(const RunTimeOpts*, PtpClock*);
void updateClock(const RunTimeOpts*,PtpClock*);

void setupPIservo(PIservo* servo, const RunTimeOpts* rtOpts);
void resetPIservo(PIservo* servo);
double runPIservo(PIservo* servo, const Integer32 input);

#ifdef PTPD_STATISTICS
void updatePtpEngineStats (PtpClock* ptpClock, const RunTimeOpts* rtOpts);
#endif /* PTPD_STATISTICS */

#endif

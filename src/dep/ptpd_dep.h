/**
 * @file   ptpd_dep.h
 *
 * @brief  External definitions for inclusion elsewhere.
 *
 *
 */

#ifndef PTPD_DEP_H_
#define PTPD_DEP_H_

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif /* HAVE_CONFIG_H */

#include "ptp_primitives.h"
#include "ptp_datatypes.h"
#include "dep/alarm_datatypes.h"
#include "dep/datatypes_dep.h"
#include "dep/iniparser/dictionary.h"
#include "datatypes.h"

/*
  list of per-module defines:

./dep/sys.c:#define PRINT_MAC_ADDRESSES
./dep/timer.c:#define US_TIMER_INTERVAL 125000
*/

/** \name sys.c (Unix API dependent)
 * -Manage timing system API*/
 /**\{*/

/* new debug methods to debug time variables */
char *time2st(const TimeInternal * p);
void DBG_time(const char *name, const TimeInternal  p);

int setCpuAffinity(int cpu);
void logStatistics(PtpClock *ptpClock);
int restartLog(LogFileHandler* handler, Boolean quiet);
void restartLogging(RunTimeOpts* rtOpts);
void stopLogging(RunTimeOpts* rtOpts);
void periodicUpdate(const RunTimeOpts *rtOpts, PtpClock *ptpClock);
void displayStatus(PtpClock *ptpClock, const char *prefixMessage);
void displayPortIdentity(PortIdentity *port, const char *prefixMessage);
int snprint_PortIdentity(char *s, int max_len, const PortIdentity *id);
Boolean nanoSleep(TimeInternal*);
void getTime(TimeInternal*);
void getTimeMonotonic(TimeInternal*);
void setTime(TimeInternal*);
#ifdef linux
void setRtc(TimeInternal *);
#endif /* linux */
double getRand(void);
int lockFile(int fd);
int checkLockStatus(int fd, short lockType, int *lockPid);
int checkFileLockable(const char *fileName, int *lockPid);
Boolean checkOtherLocks(RunTimeOpts *rtOpts);

void recordSync(UInteger16 sequenceId, TimeInternal * time);

Boolean adjFreq(double);
double getAdjFreq(void);

#if defined(HAVE_SYS_TIMEX_H) && defined(PTPD_FEATURE_NTP)
void informClockSource(PtpClock* ptpClock);

/* Helper function to manage ntpadjtime / adjtimex flags */
void setTimexFlags(int flags, Boolean quiet);
void unsetTimexFlags(int flags, Boolean quiet);
int getTimexFlags(void);
Boolean checkTimexFlags(int flags);

#  if defined(MOD_TAI) &&  NTP_API == 4
void setKernelUtcOffset(int utc_offset);
Boolean getKernelUtcOffset(int *utc_offset);
#  endif /* MOD_TAI */

#endif /* HAVE_SYS_TIMEX_H && PTPD_FEATURE_NTP */

/* Observed drift save / recovery functions */
void restoreDrift(PtpClock * ptpClock, const RunTimeOpts * rtOpts, Boolean quiet);
void saveDrift(PtpClock * ptpClock, const RunTimeOpts * rtOpts, Boolean quiet);

int parseLeapFile(char * path, LeapSecondInfo *info);

void writeStatusFile(PtpClock *ptpClock, const RunTimeOpts *rtOpts, Boolean quiet);
void updateXtmp (TimeInternal oldTime, TimeInternal newTime);


#endif /*PTPD_DEP_H_*/

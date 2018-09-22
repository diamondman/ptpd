#ifndef SYS_H_
#define SYS_H_

#include "ptp_primitives.h"
#include "dep/datatypes_dep_stub.h"
#include "ptp_datatypes.h"
#include "datatypes_stub.h"

char *dump_TimeInternal(const TimeInternal * p);
char *dump_TimeInternal2(const char *st1, const TimeInternal * p1, const char *st2, const TimeInternal * p2);

/* new debug methods to debug time variables */
char *time2st(const TimeInternal * p);
void DBG_time(const char *name, const TimeInternal  p);

int snprint_PortIdentity(char *s, int max_len, const PortIdentity *id);

// Also in ptpd_logging.h. TODO: Figure out a nice, single place to put this.
void logMessage(int priority, const char *format, ...);

int restartLog(LogFileHandler* handler, Boolean quiet);
void restartLogging(RunTimeOpts* rtOpts);
void stopLogging(RunTimeOpts* rtOpts);
void logStatistics(PtpClock *ptpClock);
void periodicUpdate(const RunTimeOpts *rtOpts, PtpClock *ptpClock);
void displayStatus(PtpClock *ptpClock, const char *prefixMessage);
void writeStatusFile(PtpClock *ptpClock, const RunTimeOpts *rtOpts, Boolean quiet);
void displayPortIdentity(PortIdentity *port, const char *prefixMessage);
void recordSync(UInteger16 sequenceId, TimeInternal * time);
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

void updateXtmp (TimeInternal oldTime, TimeInternal newTime);
int setCpuAffinity(int cpu);


#endif /* include guard */

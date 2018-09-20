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
#define USE_BINDTODEVICE

/** \name net.c (Unix API dependent)
 * -Init network stuff, send and receive datas*/
 /**\{*/

Boolean testInterface(char* ifaceName, const RunTimeOpts* rtOpts);
Boolean netInit(NetPath*,RunTimeOpts*,PtpClock*);
Boolean netShutdown(NetPath*);
int netSelect(TimeInternal*,NetPath*,fd_set*);
ssize_t netRecvEvent(Octet*,TimeInternal*,NetPath*,int);
ssize_t netRecvGeneral(Octet*,NetPath*);
ssize_t netSendEvent(Octet*,UInteger16,NetPath*,const RunTimeOpts*,Integer32,TimeInternal*);
ssize_t netSendGeneral(Octet*,UInteger16,NetPath*,const RunTimeOpts*,Integer32 );
ssize_t netSendPeerGeneral(Octet*,UInteger16,NetPath*,const RunTimeOpts*, Integer32);
ssize_t netSendPeerEvent(Octet*,UInteger16,NetPath*,const RunTimeOpts*,Integer32,TimeInternal*);
Boolean netRefreshIGMP(NetPath *, const RunTimeOpts *, PtpClock *);
Boolean hostLookup(const char* hostname, Integer32* addr);

/** \}*/

#if defined PTPD_SNMP
/** \name snmp.c (SNMP subsystem)
 * -Handle SNMP subsystem*/
 /**\{*/

void snmpInit(RunTimeOpts *, PtpClock *);
void snmpShutdown();
void eventHandler_snmp(AlarmEntry *alarm);
void alarmHandler_snmp(AlarmEntry *alarm);

//void sendNotif(int eventType, PtpEventData *eventData);


/** \}*/
#endif

/** \name servo.c
 * -Clock servo*/
 /**\{*/

void initClock(const RunTimeOpts*,PtpClock*);
void updatePeerDelay (one_way_delay_filter*, const RunTimeOpts*,PtpClock*,TimeInternal*,Boolean);
void updateDelay (one_way_delay_filter*, const RunTimeOpts*, PtpClock*,TimeInternal*);
void updateOffset(TimeInternal*,TimeInternal*,
  offset_from_master_filter*,const RunTimeOpts*,PtpClock*,TimeInternal*);
void checkOffset(const RunTimeOpts*, PtpClock*);
void updateClock(const RunTimeOpts*,PtpClock*);
void stepClock(const RunTimeOpts * rtOpts, PtpClock * ptpClock);

/** \}*/

/** \name startup.c (Unix API dependent)
 * -Handle with runtime options*/
 /**\{*/
int setCpuAffinity(int cpu);
int logToFile(RunTimeOpts * rtOpts);
int recordToFile(RunTimeOpts * rtOpts);
PtpClock * ptpdStartup(int,char**,Integer16*,RunTimeOpts*);

void ptpdShutdown(PtpClock * ptpClock);
void checkSignals(RunTimeOpts * rtOpts, PtpClock * ptpClock);
void restartSubsystems(RunTimeOpts *rtOpts, PtpClock *ptpClock);
void applyConfig(dictionary *baseConfig, RunTimeOpts *rtOpts, PtpClock *ptpClock);

void enable_runtime_debug(void );
void disable_runtime_debug(void );

#ifdef PTPD_FEATURE_NTP
void ntpSetup(RunTimeOpts *rtOpts, PtpClock *ptpClock);
#endif

#define D_ON      do { enable_runtime_debug();  } while (0);
#define D_OFF     do { disable_runtime_debug(); } while (0);


/** \}*/

/** \name sys.c (Unix API dependent)
 * -Manage timing system API*/
 /**\{*/

/* new debug methods to debug time variables */
char *time2st(const TimeInternal * p);
void DBG_time(const char *name, const TimeInternal  p);

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

void adjFreq_wrapper(const RunTimeOpts * rtOpts, PtpClock * ptpClock, double adj);

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

void
resetWarnings(const RunTimeOpts * rtOpts, PtpClock * ptpClock);

void setupPIservo(PIservo* servo, const RunTimeOpts* rtOpts);
void resetPIservo(PIservo* servo);
double runPIservo(PIservo* servo, const Integer32 input);

#ifdef PTPD_STATISTICS
void updatePtpEngineStats (PtpClock* ptpClock, const RunTimeOpts* rtOpts);
#endif /* PTPD_STATISTICS */

void writeStatusFile(PtpClock *ptpClock, const RunTimeOpts *rtOpts, Boolean quiet);
void updateXtmp (TimeInternal oldTime, TimeInternal newTime);


#endif /*PTPD_DEP_H_*/

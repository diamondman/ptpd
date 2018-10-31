/*-
 * Copyright (c) 2012-2015 Wojciech Owczarek,
 * Copyright (c) 2011-2012 George V. Neville-Neil,
 *                         Steven Kreuzer,
 *                         Martin Burnicki,
 *                         Jan Breuer,
 *                         Gael Mace,
 *                         Alexandre Van Kempen,
 *                         Inaqui Delgado,
 *                         Rick Ratzel,
 *                         National Instruments.
 * Copyright (c) 2009-2010 George V. Neville-Neil,
 *                         Steven Kreuzer,
 *                         Martin Burnicki,
 *                         Jan Breuer,
 *                         Gael Mace,
 *                         Alexandre Van Kempen
 *
 * Copyright (c) 2005-2008 Kendall Correll, Aidan Williams
 *
 * All Rights Reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file   sys.c
 * @date   Tue Jul 20 16:19:46 2010
 *
 * @brief  Code to call kernel time routines and also display server statistics.
 *
 *
 */

//OPTIONS
//#define PRINT_MAC_ADDRESSES

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif /* HAVE_CONFIG_H */

#if defined(linux) && !defined(_GNU_SOURCE)
#  define _GNU_SOURCE
#endif

#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <glob.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <time.h>
#include <syslog.h>

#ifdef HAVE_UTMPX_H
#  include <utmpx.h>
#else
#  ifdef HAVE_UTMP_H
#    include <utmp.h>
#  endif /* HAVE_UTMP_H */
#endif /* HAVE_UTMPX_H */

#if defined(linux)
//Alternate option instead of HAVE_STRUCT_ETHER_ADDR_OCTET
//#  define octet ether_addr_octet
#  if defined(HAVE_SCHED_H)
#    include <sched.h>
#  endif
#else
#  define adjtimex ntp_adjtime
#endif

#ifdef HAVE_NETINET_ETHER_H
#  include <netinet/ether.h>
#endif

#ifdef HAVE_NET_ETHERNET_H
#  include <net/ethernet.h>
#endif

#if defined(HAVE_SYS_SOCKET_H)
// Required by openBSD before net/if.h
#  include <sys/socket.h>
#endif

#ifdef HAVE_NET_IF_H
#  include <net/if.h>
#endif

#ifdef HAVE_NET_IF_ETHER_H
#  include <net/if_ether.h>
#endif

#if defined(HAVE_SYS_TIMEX_H) && defined(PTPD_FEATURE_NTP)
#  include <sys/timex.h>
#endif

#ifdef HAVE_SYS_CPUSET_H
#  include <sys/cpuset.h>
#endif

#ifdef HAVE_LINUX_RTC_H
#  include <sys/ioctl.h>
#  include <linux/rtc.h>
#endif

#ifdef HAVE_SYS_CPUSET_H
#  include <sys/cpuset.h>
#endif

#ifdef HAVE_UNIX_H /* setlinebuf() on QNX */
#  include <unix.h>
#endif /* HAVE_UNIX_H */

#include "constants.h"
#include "dep/constants_dep.h"
#include "ptp_primitives.h"
#include "dep/datatypes_dep.h"
#include "ptp_datatypes.h"
#include "arith.h"
#include "dep/servo.h" // For adjFreq_wrapper
#include "datatypes.h"
#include "dep/sys.h" // For getTime, getTimexFlags
#include "dep/daemonconfig.h"
#include "dep/alarms.h"
#include "protocol.h"
#include "display.h"
#include "ptpd_logging.h"

typedef struct LogFileHandler {

	Boolean logEnabled;

	LogFileConfig* config;

	FILE* logFP;
	uint32_t lastHash;
	UInteger32 fileSize;

} LogFileHandler;

static LogFileHandler logFiles[LOGFILE_MAX] = {0};

Boolean LogFileHandlerIsEnabled(LogFile_e id) {
	if(id >= LOGFILE_MAX) return FALSE;
	return logFiles[id].logEnabled;
}

static Boolean LogFileHandlerInit(LogFile_e id, LogFileConfig* config) {
	if(id >= LOGFILE_MAX) return FALSE;
	if(!config) return FALSE;

	LogFileHandler* log = &logFiles[id];
	log->config = config;
	log->logEnabled = config->logInitiallyEnabled;

	return TRUE;
}

Boolean initLogging(RunTimeOpts* rtOpts) {
	if(!LogFileHandlerInit(LOGFILE_STATISTICS,&rtOpts->sysopts.statisticsLogConfig)) return FALSE;
	if(!LogFileHandlerInit(LOGFILE_RECORD,    &rtOpts->sysopts.recordLogConfig)    ) return FALSE;
	if(!LogFileHandlerInit(LOGFILE_EVENT,     &rtOpts->sysopts.eventLogConfig)     ) return FALSE;
	if(!LogFileHandlerInit(LOGFILE_MAX,       &rtOpts->sysopts.statusLogConfig)    ) return FALSE;
	return TRUE;
}


/* only C99 has the round function built-in */
double round (double __x);

static Boolean closeLog(LogFileHandler* handler);

static Boolean maintainLogSize(LogFileHandler* handler);
static void updateLogSize(LogFileHandler* handler);
static int snprint_TimeInternal(char *s, int max_len, const TimeInternal * p);

#ifdef __QNXNTO__
typedef struct {
  _uint64 counter;		/* iteration counter */
  _uint64 prev_tsc;		/* previous clock cycles */
  _uint64 last_clock;		/* clock reading at last timer interrupt */
  _uint64 cps;			/* cycles per second */
  _uint64 prev_delta;		/* previous clock cycle delta */
  _uint64 cur_delta;		/* last clock cycle delta */
  _uint64 filtered_delta;	/* filtered delta */
  double ns_per_tick;		/* nanoseconds per cycle */
} TimerIntData;

/* do not access directly! tied to clock interrupt! */
static TimerIntData tData;
static Boolean tDataUpdated = FALSE;

#endif /* __QNXNTO__ */

/* Pointer to the current lock file */
static FILE* G_lockFilePointer;

/*
 returns a static char * for the representation of time, for debug purposes
 DO NOT call this twice in the same printf!
*/
char *dump_TimeInternal(const TimeInternal * p)
{
	static char buf[100];

	snprint_TimeInternal(buf, 100, p);
	return buf;
}


/*
 displays 2 timestamps and their strings in sequence, and the difference between then
 DO NOT call this twice in the same printf!
*/
char *dump_TimeInternal2(const char *st1, const TimeInternal * p1, const char *st2, const TimeInternal * p2)
{
	static char buf[BUF_SIZE];
	int n = 0;

	/* display Timestamps */
	if (st1) {
		n += snprintf(buf + n, BUF_SIZE - n, "%s ", st1);
	}
	n += snprint_TimeInternal(buf + n, BUF_SIZE - n, p1);
	n += snprintf(buf + n, BUF_SIZE - n, "    ");

	if (st2) {
		n += snprintf(buf + n, BUF_SIZE - n, "%s ", st2);
	}
	n += snprint_TimeInternal(buf + n, BUF_SIZE - n, p2);
	n += snprintf(buf + n, BUF_SIZE - n, " ");

	/* display difference */
	TimeInternal r;
	subTime(&r, p1, p2);
	n += snprintf(buf + n, BUF_SIZE - n, "   (diff: ");
	n += snprint_TimeInternal(buf + n, BUF_SIZE - n, &r);
	n += snprintf(buf + n, BUF_SIZE - n, ") ");

	return buf;
}



static int
snprint_TimeInternal(char *s, int max_len, const TimeInternal * p)
{
	int len = 0;

	/* always print either a space, or the leading "-". This makes the stat files columns-aligned */
	len += snprintf(&s[len], max_len - len, "%c",
		isTimeInternalNegative(p)? '-':' ');

	len += snprintf(&s[len], max_len - len, "%d.%09d",
	    abs(p->seconds), abs(p->nanoseconds));

	return len;
}


/* debug aid: convert a time variable into a static char */
char *time2st(const TimeInternal * p)
{
	static char buf[1000];

	snprint_TimeInternal(buf, sizeof(buf), p);
	return buf;
}



void DBG_time(const char *name, const TimeInternal  p)
{
	DBG("             %s:   %s\n", name, time2st(&p));
}


static char *
translatePortState(PtpClock *ptpClock)
{
	char *s;
	switch(ptpClock->portDS.portState) {
	    case PTP_INITIALIZING:  s = "init";  break;
	    case PTP_FAULTY:        s = "flt";   break;
	    case PTP_LISTENING:
		    /* seperate init-reset from real resets */
		    if(ptpClock->resetCount == 1){
		    	s = "lstn_init";
		    } else {
		    	s = "lstn_reset";
		    }
		    break;
	    case PTP_PASSIVE:       s = "pass";  break;
	    case PTP_UNCALIBRATED:  s = "uncl";  break;
	    case PTP_SLAVE:         s = "slv";   break;
	    case PTP_PRE_MASTER:    s = "pmst";  break;
	    case PTP_MASTER:        s = "mst";   break;
	    case PTP_DISABLED:      s = "dsbl";  break;
	    default:                s = "?";     break;
	}
	return s;
}


static int
snprint_ClockIdentity(char *s, int max_len, const ClockIdentity id)
{
	int len = 0;
	int i;

	for (i = 0; ;) {
		len += snprintf(&s[len], max_len - len, "%02x", (unsigned char) id[i]);

		if (++i >= CLOCK_IDENTITY_LENGTH)
			break;
	}

	return len;
}


#ifdef PRINT_MAC_ADDRESSES
/* show the mac address in an easy way */
static int
snprint_ClockIdentity_mac(char *s, int max_len, const ClockIdentity id)
{
	int len = 0;
	int i;

	for (i = 0; ;) {
		/* skip bytes 3 and 4 */
		if(!((i==3) || (i==4))){
			len += snprintf(&s[len], max_len - len, "%02x", (unsigned char) id[i]);

			if (++i >= CLOCK_IDENTITY_LENGTH)
				break;

			/* print a separator after each byte except the last one */
			len += snprintf(&s[len], max_len - len, "%s", ":");
		} else {

			i++;
		}
	}

	return len;
}
#endif


/*
 * wrapper that caches the latest value of ether_ntohost
 * this function will NOT check the last accces time of /etc/ethers,
 * so it only have different output on a failover or at restart
 *
 */
static int ether_ntohost_cache(char *hostname, struct ether_addr *addr)
{
	static int valid = 0;
	static struct ether_addr prev_addr;
	static char buf[BUF_SIZE];

#ifdef HAVE_STRUCT_ETHER_ADDR_OCTET
	if (memcmp(addr->octet, &prev_addr,
		  sizeof(struct ether_addr )) != 0) {
		valid = 0;
	}
#else
	if (memcmp(addr->ether_addr_octet, &prev_addr,
		  sizeof(struct ether_addr )) != 0) {
		valid = 0;
	}
#endif
	if (!valid) {
		if(ether_ntohost(buf, addr)){
			snprintf(buf, BUF_SIZE,"%s", "unknown");
		}

		/* clean possible commas from the string */
		while (strchr(buf, ',') != NULL) {
			*(strchr(buf, ',')) = '_';
		}

		prev_addr = *addr;
	}

	valid = 1;
	strncpy(hostname, buf, 100);
	return 0;
}


/* Show the hostname configured in /etc/ethers */
static int
snprint_ClockIdentity_ntohost(char *s, int max_len, const ClockIdentity id)
{
	int len = 0;
	int i,j;
	char  buf[100];
	struct ether_addr e;

	/* extract mac address */
	for (i = 0, j = 0; i< CLOCK_IDENTITY_LENGTH ; i++ ){
		/* skip bytes 3 and 4 */
		if(!((i==3) || (i==4))){
#ifdef HAVE_STRUCT_ETHER_ADDR_OCTET
			e.octet[j] = (uint8_t) id[i];
#else
			e.ether_addr_octet[j] = (uint8_t) id[i];
#endif
			j++;
		}
	}

	/* convert and print hostname */
	ether_ntohost_cache(buf, &e);
	len += snprintf(&s[len], max_len - len, "(%s)", buf);

	return len;
}


int
snprint_PortIdentity(char *s, int max_len, const PortIdentity *id)
{
	int len = 0;

#ifdef PRINT_MAC_ADDRESSES
	len += snprint_ClockIdentity_mac(&s[len], max_len - len, id->clockIdentity);
#else
	len += snprint_ClockIdentity(&s[len], max_len - len, id->clockIdentity);
#endif

	len += snprint_ClockIdentity_ntohost(&s[len], max_len - len, id->clockIdentity);

	len += snprintf(&s[len], max_len - len, "/%d", (unsigned) id->portNumber);
	return len;
}

/* Write a formatted string to file pointer */
static int writeMessage(FILE* destination, uint32_t *lastHash, int priority, const char * format, va_list ap)
{
	extern RunTimeOpts rtOpts;
	extern Boolean startupInProgress;
	extern PtpClock *G_ptpClock;

	int written;
	char time_str[MAXTIMESTR];
	struct timeval now;
#ifndef RUNTIME_DEBUG
	char buf[PATH_MAX +1];
	uint32_t hash;
	va_list ap1;
#endif /* RUNTIME_DEBUG */

	extern char *translatePortState(PtpClock *ptpClock);

	if(destination == NULL)
		return -1;

	/* If we're starting up as daemon, only print <= WARN */
	if ((destination == stderr) &&
		!rtOpts.sysopts.nonDaemon && startupInProgress &&
		(priority > LOG_WARNING)){
		    return 1;
		}

#ifndef RUNTIME_DEBUG
	/* check if this message produces the same hash as last */
	memset(buf, 0, sizeof(buf));
	va_copy(ap1, ap);
	vsnprintf(buf, PATH_MAX, format, ap1);
	hash = fnvHash(buf, sizeof(buf), 0);
	if(lastHash != NULL) {
	    if(format[0] != '\n' && lastHash != NULL) {
		    /* last message was the same - don't print the next one */
		    if( (lastHash != 0) && (hash == *lastHash)) {
		    return 0;
		}
	    }
	    *lastHash = hash;
	}
#endif /* RUNTIME_DEBUG */

	/* Print timestamps and prefixes only if we're running in foreground or logging to file*/
	if( rtOpts.sysopts.nonDaemon || destination != stderr) {

		/*
		 * select debug tagged with timestamps. This will slow down PTP itself if you send a lot of messages!
		 * it also can cause problems in nested debug statements (which are solved by turning the signal
		 *  handling synchronous, and not calling this function inside asycnhronous signal processing)
		 */
		gettimeofday(&now, 0);
		strftime(time_str, MAXTIMESTR, "%F %X", localtime((time_t*)&now.tv_sec));
		fprintf(destination, "%s.%06d ", time_str, (int)now.tv_usec);
		fprintf(destination,PTPD_PROGNAME"[%d].%s (%-9s ",
			(int)getpid(), startupInProgress ? "startup" :
			netPathGetInterfaceName(G_ptpClock->netPath, &rtOpts),
			priority == LOG_EMERG   ? "emergency)" :
			priority == LOG_ALERT   ? "alert)" :
			priority == LOG_CRIT    ? "critical)" :
			priority == LOG_ERR     ? "error)" :
			priority == LOG_WARNING ? "warning)" :
			priority == LOG_NOTICE  ? "notice)" :
			priority == LOG_INFO    ? "info)" :
			priority == LOG_DEBUG   ? "debug1)" :
			priority == LOG_DEBUG2  ? "debug2)" :
			priority == LOG_DEBUGV  ? "debug3)" :
			"unk)");


		fprintf(destination, " (%s) ", G_ptpClock ?
		       translatePortState(G_ptpClock) : "___");
	}
	written = vfprintf(destination, format, ap);
	return written;
}

static void
updateLogSize(LogFileHandler* handler)
{
	struct stat st;
	if(handler->logFP == NULL)
		return;
	if (fstat(fileno(handler->logFP), &st) != -1) {
		handler->fileSize = st.st_size;
	} else {
#ifdef RUNTIME_DEBUG
/* 2.3.1: do not print to stderr or log file */
//		fprintf(stderr, "fstat on %s file failed!\n", handler->config->logID);
#endif /* RUNTIME_DEBUG */
	}
}


/*
 * Prints a message, randing from critical to debug.
 * This either prints the message to syslog, or with timestamp+state to stderr
 */
void
logMessage(int priority, const char * format, ...)
{
	extern RunTimeOpts rtOpts;
	extern Boolean startupInProgress;
	va_list ap , ap1;

	va_copy(ap1, ap);
	va_start(ap1, format);
	va_start(ap, format);

#ifdef RUNTIME_DEBUG
	if ((priority >= LOG_DEBUG) && (priority > rtOpts.debug_level)) {
		goto end;
	}
#endif

	/* log level filter */
	if(priority > rtOpts.sysopts.logLevel) {
	    goto end;
	}
	/* If we're using a log file and the message has been written OK, we're done*/
	if(logFiles[LOGFILE_EVENT].logEnabled && logFiles[LOGFILE_EVENT].logFP != NULL) {
	    if(writeMessage(logFiles[LOGFILE_EVENT].logFP, &logFiles[LOGFILE_EVENT].lastHash,
			    priority, format, ap) > 0) {
		maintainLogSize(&logFiles[LOGFILE_EVENT]);
		if(!startupInProgress)
		    goto end;
		else {
		    logFiles[LOGFILE_EVENT].lastHash = 0;
		    goto std_err;
		    }
	    }
	}

	/*
	 * Otherwise we try syslog - if we're here, we didn't write to log file.
	 * If we're running in background and we're starting up, also log first
	 * messages to syslog to at least leave a trace.
	 */
	if (rtOpts.sysopts.useSysLog ||
	    (!rtOpts.sysopts.nonDaemon && startupInProgress)) {
		static Boolean syslogOpened;
#ifdef RUNTIME_DEBUG
		/*
		 *  Syslog only has 8 message levels (3 bits)
		 *  important: messages will only appear if "*.debug /var/log/debug" is on /etc/rsyslog.conf
		 */
		if(priority > LOG_DEBUG){
			priority = LOG_DEBUG;
		}
#endif

		if (!syslogOpened) {
			openlog(PTPD_PROGNAME, LOG_PID, LOG_DAEMON);
			syslogOpened = TRUE;
		}
		vsyslog(priority, format, ap);
		if (!startupInProgress) {
			goto end;
		}
		else {
			logFiles[LOGFILE_EVENT].lastHash = 0;
			goto std_err;
		}
	}
std_err:

	/* Either all else failed or we're running in foreground - or we also log to stderr */
	writeMessage(stderr, &logFiles[LOGFILE_EVENT].lastHash, priority, format, ap1);

end:
	va_end(ap1);
	va_end(ap);
	return;
}


/* Restart a file log target based on its settings  */
Boolean
restartLog(LogFileHandler* handler, Boolean quiet)
{
	closeLog(handler);

        /* FP is not open and we're not logging */
        if (!handler->logEnabled)
                return TRUE;

	/* Open the file */
        if ( (handler->logFP = fopen(handler->config->logPath, handler->config->openMode)) == NULL) {
                if(!quiet) PERROR("Could not open %s file", handler->config->logID);
		return FALSE;
        }

	if(handler->config->truncateOnReopen) {
		if(!ftruncate(fileno(handler->logFP), 0)) {
			if(!quiet) INFO("Truncated %s file\n", handler->config->logID);
		} else {
			DBG("Could not truncate % file: %s\n",
			    handler->config->logID, handler->config->logPath);
		}
	}
	/* \n flushes output for us, no need for fflush() - if you want something different, set it later */
	setlinebuf(handler->logFP);
	return TRUE;
}

/* Close a file log target */
static Boolean
closeLog(LogFileHandler* handler)
{
	if(handler->logFP != NULL) {
		//if(!quiet) INFO("Closing %s log file.\n", handler->config->logID);
		handler->lastHash=0;
		fclose(handler->logFP);
		/*
		 * fclose doesn't do this at least on Linux - changes the underlying FD to -1,
		 * but not the FP to NULL - with this we can tell if the FP is closed
		 */
		handler->logFP=NULL;
		/* If we're not logging to file (any more), call it quits */
		if (!handler->logEnabled) {
			if(handler->config->unlinkOnClose) {
				//if(!quiet) INFO("Logging to %s file disabled. Deleting file.\n", handler->config->logID);
				unlink(handler->config->logPath);
			}
		}
		return TRUE;
	}
	return FALSE;
}

/* Return TRUE only if the log file had to be rotated / truncated - FALSE does not mean error */
/* Mini-logrotate: truncate file if exceeds preset size, also rotate up to n number of files if configured */
static Boolean
maintainLogSize(LogFileHandler* handler)
{
	if(handler->config->maxSize) {
		if(handler->logFP == NULL)
		    return FALSE;
		updateLogSize(handler);
#ifdef RUNTIME_DEBUG
/* 2.3.1: do not print to stderr or log file */
//		fprintf(stderr, "%s logsize: %d\n", handler->config->logID, handler->fileSize);
#endif /* RUNTIME_DEBUG */
		if(handler->fileSize > (handler->config->maxSize * 1024)) {

		    /* Rotate the log file */
		    if (handler->config->maxFiles) {
			int i = 0;
			int logFileNumber = 0;
			time_t maxMtime = 0;
			struct stat st;
			char fname[PATH_MAX];
			/* Find the last modified file of the series */
			while(++i <= handler->config->maxFiles) {
				memset(fname, 0, PATH_MAX);
				snprintf(fname, PATH_MAX,"%s.%d", handler->config->logPath, i);
				if((stat(fname,&st) != -1) && S_ISREG(st.st_mode)) {
					if(st.st_mtime > maxMtime) {
						maxMtime = st.st_mtime;
						logFileNumber = i;
					}
				}
			}
			/* Use next file in line or first one if rolled over */
			if(++logFileNumber > handler->config->maxFiles)
				logFileNumber = 1;
			memset(fname, 0, PATH_MAX);
			snprintf(fname, PATH_MAX,"%s.%d", handler->config->logPath, logFileNumber);
			/* Move current file to new location */
			rename(handler->config->logPath, fname);
			/* Reopen to reactivate the original path */
			if(restartLog(handler,TRUE)) {
				INFO("Rotating %s file - size above %dkB\n",
					handler->config->logID, handler->config->maxSize);
			} else {
				DBG("Could not rotate %s file\n", handler->config->logPath);
			}
			return TRUE;
		    /* Just truncate - maxSize given but no maxFiles */
		    } else {

			if(!ftruncate(fileno(handler->logFP),0)) {
			INFO("Truncating %s file - size above %dkB\n",
				handler->config->logID, handler->config->maxSize);
			} else {
#ifdef RUNTIME_DEBUG
/* 2.3.1: do not print to stderr or log file */
//				fprintf(stderr, "Could not truncate %s file\n", handler->config->logPath);
#endif
			}
			return TRUE;
		    }
		}
	}

	return FALSE;
}

void
restartLogging()
{
	for(int i = 0; i < LOGFILE_MAX; i++){
		if(!restartLog(&logFiles[i], TRUE))
			NOTIFY("Failed logging to %s file\n", logFiles[i].config->logID);
	}
}

void
stopLogging(RunTimeOpts* rtOpts)
{
	for(int i = 0; i < LOGFILE_MAX; i++){
		logFiles[i].logEnabled = FALSE;
		closeLog(&logFiles[i]);
	}
}

void
logStatistics(PtpClock * ptpClock)
{
	extern RunTimeOpts rtOpts;
	static int errorMsg = 0;
	static char sbuf[SCREEN_BUFSZ * 2];
	int len = 0;
	TimeInternal now;
	time_t time_s;
	FILE* destination;
	static TimeInternal prev_now_sync, prev_now_delay;
	char time_str[MAXTIMESTR];

	if (!rtOpts.logStatistics) {
		return;
	}

	if(logFiles[LOGFILE_STATISTICS].logEnabled &&
	   logFiles[LOGFILE_STATISTICS].logFP != NULL)
	    destination = logFiles[LOGFILE_STATISTICS].logFP;
	else
	    destination = stdout;

	if (ptpClock->resetStatisticsLog) {
		ptpClock->resetStatisticsLog = FALSE;
		fprintf(destination,"# %s, State, Clock ID, One Way Delay, "
		       "Offset From Master, Slave to Master, "
		       "Master to Slave, Observed Drift, Last packet Received, Sequence ID"
#ifdef PTPD_STATISTICS
			", One Way Delay Mean, One Way Delay Std Dev, Offset From Master Mean, Offset From Master Std Dev, Observed Drift Mean, Observed Drift Std Dev, raw delayMS, raw delaySM"
#endif
			"\n", (rtOpts.sysopts.statisticsTimestamp == TIMESTAMP_BOTH) ? "Timestamp, Unix timestamp" : "Timestamp");
	}

	memset(sbuf, 0, sizeof(sbuf));

	getTime(&now);

	/*
	 * print one log entry per X seconds for Sync and DelayResp messages, to reduce disk usage.
	 */

	if ((ptpClock->portDS.portState == PTP_SLAVE) && (rtOpts.sysopts.statisticsLogInterval)) {

		switch(ptpClock->char_last_msg) {
			case 'S':
			if((now.seconds - prev_now_sync.seconds) < rtOpts.sysopts.statisticsLogInterval){
				DBGV("Suppressed Sync statistics log entry - statisticsLogInterval configured\n");
				return;
			}
			prev_now_sync = now;
			    break;
			case 'D':
			case 'P':
			if((now.seconds - prev_now_delay.seconds) < rtOpts.sysopts.statisticsLogInterval){
				DBGV("Suppressed Sync statistics log entry - statisticsLogInterval configured\n");
				return;
			}
			prev_now_delay = now;
			default:
			    break;
		}
	}

	time_s = now.seconds;

	/* output date-time timestamp if configured */
	if (rtOpts.sysopts.statisticsTimestamp == TIMESTAMP_DATETIME ||
	    rtOpts.sysopts.statisticsTimestamp == TIMESTAMP_BOTH) {
	    strftime(time_str, MAXTIMESTR, "%Y-%m-%d %X", localtime(&time_s));
	    len += snprintf(sbuf + len, sizeof(sbuf) - len, "%s.%06d, %s, ",
		       time_str, (int)now.nanoseconds/1000, /* Timestamp */
		       translatePortState(ptpClock)); /* State */
	}

	/* output unix timestamp s.ns if configured */
	if (rtOpts.sysopts.statisticsTimestamp == TIMESTAMP_UNIX ||
	    rtOpts.sysopts.statisticsTimestamp == TIMESTAMP_BOTH) {
	    len += snprintf(sbuf + len, sizeof(sbuf) - len, "%d.%06d, %s,",
		       now.seconds, now.nanoseconds, /* Timestamp */
		       translatePortState(ptpClock)); /* State */
	}

	if (ptpClock->portDS.portState == PTP_SLAVE) {
		len += snprint_PortIdentity(sbuf + len, sizeof(sbuf) - len,
			 &ptpClock->parentDS.parentPortIdentity); /* Clock ID */

		/*
		 * if grandmaster ID differs from parent port ID then
		 * also print GM ID
		 */
		if (memcmp(ptpClock->parentDS.grandmasterIdentity,
			   ptpClock->parentDS.parentPortIdentity.clockIdentity,
			   CLOCK_IDENTITY_LENGTH)) {
			len += snprint_ClockIdentity(sbuf + len,
						     sizeof(sbuf) - len,
						     ptpClock->parentDS.grandmasterIdentity);
		}

		len += snprintf(sbuf + len, sizeof(sbuf) - len, ", ");

		if(rtOpts.delayMechanism == E2E) {
			len += snprint_TimeInternal(sbuf + len, sizeof(sbuf) - len,
						    &ptpClock->currentDS.meanPathDelay);
		} else {
			len += snprint_TimeInternal(sbuf + len, sizeof(sbuf) - len,
						    &ptpClock->portDS.peerMeanPathDelay);
		}

		len += snprintf(sbuf + len, sizeof(sbuf) - len, ", ");

		len += snprint_TimeInternal(sbuf + len, sizeof(sbuf) - len,
		    &ptpClock->currentDS.offsetFromMaster);

		/* print MS and SM with sign */
		len += snprintf(sbuf + len, sizeof(sbuf) - len, ", ");

		if(rtOpts.delayMechanism == E2E) {
			len += snprint_TimeInternal(sbuf + len, sizeof(sbuf) - len,
							&(ptpClock->delaySM));
		} else {
			len += snprint_TimeInternal(sbuf + len, sizeof(sbuf) - len,
							&(ptpClock->pdelaySM));
		}

		len += snprintf(sbuf + len, sizeof(sbuf) - len, ", ");

		len += snprint_TimeInternal(sbuf + len, sizeof(sbuf) - len,
				&(ptpClock->delayMS));

		len += snprintf(sbuf + len, sizeof(sbuf) - len, ", %.09f, %c, %05d",
			       ptpClock->servo.observedDrift,
			       ptpClock->char_last_msg,
			       ptpClock->msgTmpHeader.sequenceId);

#ifdef PTPD_STATISTICS

		len += snprintf(sbuf + len, sizeof(sbuf) - len, ", %.09f, %.00f, %.09f, %.00f",
			       ptpClock->slaveStats.mpdMean,
			       ptpClock->slaveStats.mpdStdDev * 1E9,
			       ptpClock->slaveStats.ofmMean,
			       ptpClock->slaveStats.ofmStdDev * 1E9);

		len += snprintf(sbuf + len, sizeof(sbuf) - len, ", %.0f, %.0f, ",
			       ptpClock->servo.driftMean,
			       ptpClock->servo.driftStdDev);

		len += snprint_TimeInternal(sbuf + len, sizeof(sbuf) - len,
							&(ptpClock->rawDelayMS));

		len += snprintf(sbuf + len, sizeof(sbuf) - len, ", ");

		len += snprint_TimeInternal(sbuf + len, sizeof(sbuf) - len,
							&(ptpClock->rawDelaySM));

#endif /* PTPD_STATISTICS */

	} else {
		if ((ptpClock->portDS.portState == PTP_MASTER) || (ptpClock->portDS.portState == PTP_PASSIVE)) {

			len += snprint_PortIdentity(sbuf + len, sizeof(sbuf) - len,
				 &ptpClock->parentDS.parentPortIdentity);

			//len += snprintf(sbuf + len, sizeof(sbuf) - len, ")");
		}

		/* show the current reset number on the log */
		if (ptpClock->portDS.portState == PTP_LISTENING) {
			len += snprintf(sbuf + len,
						     sizeof(sbuf) - len,
						     " %d ", ptpClock->resetCount);
		}
	}

	/* add final \n in normal status lines */
	len += snprintf(sbuf + len, sizeof(sbuf) - len, "\n");

#if 0   /* NOTE: Do we want this? */
	if (rtOpts.sysopts.nonDaemon) {
		/* in -C mode, adding an extra \n makes stats more clear intermixed with debug comments */
		len += snprintf(sbuf + len, sizeof(sbuf) - len, "\n");
	}
#endif

	/* fprintf may get interrupted by a signal - silently retry once */
	if (fprintf(destination, "%s", sbuf) < len) {
	    if (fprintf(destination, "%s", sbuf) < len) {
		if(!errorMsg) {
		    PERROR("Error while writing statistics");
		}
		errorMsg = TRUE;
	    }
	}

	if(destination == logFiles[LOGFILE_STATISTICS].logFP) {
		if (maintainLogSize(&logFiles[LOGFILE_STATISTICS]))
			ptpClock->resetStatisticsLog = TRUE;
	}
}

/* periodic status update */
void
periodicUpdate(const RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
    char tmpBuf[200];
    char masterIdBuf[150];
    int len = 0;

    memset(tmpBuf, 0, sizeof(tmpBuf));
    memset(masterIdBuf, 0, sizeof(masterIdBuf));

    TimeInternal *mpd = &ptpClock->currentDS.meanPathDelay;

    len += snprint_PortIdentity(masterIdBuf + len, sizeof(masterIdBuf) - len,
	    &ptpClock->parentDS.parentPortIdentity);
    if(ptpClock->bestMaster && ptpClock->bestMaster->sourceAddr) {
	char strAddr[MAXHOSTNAMELEN];
	struct in_addr tmpAddr;
	tmpAddr.s_addr = ptpClock->bestMaster->sourceAddr;
	inet_ntop(AF_INET, (struct sockaddr_in *)(&tmpAddr), strAddr, MAXHOSTNAMELEN);
	len += snprintf(masterIdBuf + len, sizeof(masterIdBuf) - len, " (IPv4:%s)",strAddr);
    }

    if(ptpClock->portDS.delayMechanism == P2P) {
	mpd = &ptpClock->portDS.peerMeanPathDelay;
    }

    if(ptpClock->portDS.portState == PTP_SLAVE) {
	snprint_TimeInternal(tmpBuf, sizeof(tmpBuf), &ptpClock->currentDS.offsetFromMaster);
#ifdef PTPD_STATISTICS
	if(ptpClock->slaveStats.statsCalculated) {
	    INFO("Status update: state %s, best master %s, ofm %s s, ofm_mean % .09f s, ofm_dev % .09f s\n",
		portState_getName(ptpClock->portDS.portState),
		masterIdBuf,
		tmpBuf,
		ptpClock->slaveStats.ofmMean,
		ptpClock->slaveStats.ofmStdDev);
	    snprint_TimeInternal(tmpBuf, sizeof(tmpBuf), mpd);
	    if (ptpClock->portDS.delayMechanism == E2E) {
		INFO("Status update: state %s, best master %s, mpd %s s, mpd_mean % .09f s, mpd_dev % .09f s\n",
		    portState_getName(ptpClock->portDS.portState),
		    masterIdBuf,
		    tmpBuf,
		    ptpClock->slaveStats.mpdMean,
		    ptpClock->slaveStats.mpdStdDev);
	    } else if(ptpClock->portDS.delayMechanism == P2P) {
		INFO("Status update: state %s, best master %s, mpd %s s\n", portState_getName(ptpClock->portDS.portState), masterIdBuf, tmpBuf);
	    }
	} else {
	    INFO("Status update: state %s, best master %s, ofm %s s\n", portState_getName(ptpClock->portDS.portState), masterIdBuf, tmpBuf);
	    snprint_TimeInternal(tmpBuf, sizeof(tmpBuf), mpd);
	    if(ptpClock->portDS.delayMechanism != DELAY_DISABLED) {
		INFO("Status update: state %s, best master %s, mpd %s s\n", portState_getName(ptpClock->portDS.portState), masterIdBuf, tmpBuf);
	    }
	}
#else
	INFO("Status update: state %s, best master %s, ofm %s s\n", portState_getName(ptpClock->portDS.portState), masterIdBuf, tmpBuf);
	snprint_TimeInternal(tmpBuf, sizeof(tmpBuf), mpd);
	if(ptpClock->portDS.delayMechanism != DELAY_DISABLED) {
	    INFO("Status update: state %s, best master %s, mpd %s s\n", portState_getName(ptpClock->portDS.portState), masterIdBuf, tmpBuf);
	}
#endif /* PTPD_STATISTICS */
    } else if(ptpClock->portDS.portState == PTP_MASTER) {
	if(rtOpts->unicastNegotiation || ptpClock->unicastDestinationCount) {
	    INFO("Status update: state %s, %d slaves\n", portState_getName(ptpClock->portDS.portState),
	    ptpClock->unicastDestinationCount + ptpClock->slaveCount);
	} else {
	    INFO("Status update: state %s\n", portState_getName(ptpClock->portDS.portState));
	}
    } else {
	INFO("Status update: state %s\n", portState_getName(ptpClock->portDS.portState));
    }
}


void
displayStatus(PtpClock *ptpClock, const char *prefixMessage)
{
	static char sbuf[SCREEN_BUFSZ];
	char strAddr[MAXHOSTNAMELEN];
	int len = 0;

	memset(strAddr, 0, sizeof(strAddr));
	memset(sbuf, ' ', sizeof(sbuf));
	len += snprintf(sbuf + len, sizeof(sbuf) - len, "%s", prefixMessage);
	len += snprintf(sbuf + len, sizeof(sbuf) - len, "%s",
			portState_getName(ptpClock->portDS.portState));

	if (ptpClock->portDS.portState >= PTP_MASTER) {
		len += snprintf(sbuf + len, sizeof(sbuf) - len, ", Best master: ");
		len += snprint_PortIdentity(sbuf + len, sizeof(sbuf) - len,
			&ptpClock->parentDS.parentPortIdentity);
		if(ptpClock->bestMaster && ptpClock->bestMaster->sourceAddr) {
		    struct in_addr tmpAddr;
		    tmpAddr.s_addr = ptpClock->bestMaster->sourceAddr;
		    inet_ntop(AF_INET, (struct sockaddr_in *)(&tmpAddr), strAddr, MAXHOSTNAMELEN);
		    len += snprintf(sbuf + len, sizeof(sbuf) - len, " (IPv4:%s)",strAddr);
		}
        }
	if(ptpClock->portDS.portState == PTP_MASTER)
    		len += snprintf(sbuf + len, sizeof(sbuf) - len, " (self)");
        len += snprintf(sbuf + len, sizeof(sbuf) - len, "\n");
        NOTICE("%s",sbuf);
}

#define STATUSPREFIX "%-19s:"
void
writeStatusFile(PtpClock *ptpClock,const RunTimeOpts *rtOpts, Boolean quiet)
{
	char outBuf[2048];
	char tmpBuf[200];

	if(!logFiles[LOGFILE_STATUS].logEnabled)
		return;

	int n = getAlarmSummary(NULL, 0, ptpClock->alarms, ALRM_MAX);
	char alarmBuf[n];

	getAlarmSummary(alarmBuf, n, ptpClock->alarms, ALRM_MAX);

	if(logFiles[LOGFILE_STATUS].logFP == NULL)
		return;

	char timeStr[MAXTIMESTR];
	char hostName[MAXHOSTNAMELEN];

	struct timeval now;
	memset(hostName, 0, MAXHOSTNAMELEN);

	gethostname(hostName, MAXHOSTNAMELEN);
	gettimeofday(&now, 0);
	strftime(timeStr, MAXTIMESTR, "%a %b %d %X %Z %Y", localtime((time_t*)&now.tv_sec));

	FILE* out = logFiles[LOGFILE_STATUS].logFP;
	memset(outBuf, 0, sizeof(outBuf));

	setbuf(out, outBuf);
	if(ftruncate(fileno(out), 0) < 0) {
	    DBG("writeStatusFile: ftruncate() failed\n");
	}
	rewind(out);

	fprintf(out, 		STATUSPREFIX"  %s, PID %d\n","Host info", hostName, (int)getpid());
	fprintf(out, 		STATUSPREFIX"  %s\n","Local time", timeStr);
	strftime(timeStr, MAXTIMESTR, "%a %b %d %X %Z %Y", gmtime((time_t*)&now.tv_sec));
	fprintf(out, 		STATUSPREFIX"  %s\n","Kernel time", timeStr);
	fprintf(out, 		STATUSPREFIX"  %s%s\n", "Interface", netPathGetInterfaceName(ptpClock->netPath, rtOpts),
		(rtOpts->sysopts.backupIfaceEnabled && !netPathGetUsePrimaryIf(ptpClock->netPath)) ?
		" (backup)" : (rtOpts->sysopts.backupIfaceEnabled) ?
		" (primary)" : "");
	fprintf(out, 		STATUSPREFIX"  %s\n","Preset", dictionary_get(rtOpts->currentConfig, "ptpengine:preset", ""));
	fprintf(out, 		STATUSPREFIX"  %s%s","Transport", dictionary_get(rtOpts->currentConfig, "ptpengine:transport", ""),
		(rtOpts->transport==UDP_IPV4 && rtOpts->sysopts.pcap == TRUE)?" + libpcap":"");

	if(rtOpts->transport != IEEE_802_3) {
	    fprintf(out,", %s", dictionary_get(rtOpts->currentConfig, "ptpengine:ip_mode", ""));
	    fprintf(out,"%s", rtOpts->unicastNegotiation ? " negotiation":"");
	}

	fprintf(out,"\n");

	fprintf(out, 		STATUSPREFIX"  %s\n","Delay mechanism", dictionary_get(rtOpts->currentConfig, "ptpengine:delay_mechanism", ""));
	if(ptpClock->portDS.portState >= PTP_MASTER) {
	fprintf(out, 		STATUSPREFIX"  %s\n","Sync mode", ptpClock->defaultDS.twoStepFlag ? "TWO_STEP" : "ONE_STEP");
	}
	if(ptpClock->defaultDS.slaveOnly && rtOpts->anyDomain) {
		fprintf(out, 		STATUSPREFIX"  %d, preferred %d\n","PTP domain",
		ptpClock->defaultDS.domainNumber, rtOpts->domainNumber);
	} else if(ptpClock->defaultDS.slaveOnly && rtOpts->unicastNegotiation) {
		fprintf(out, 		STATUSPREFIX"  %d, default %d\n","PTP domain", ptpClock->defaultDS.domainNumber, rtOpts->domainNumber);
	} else {
		fprintf(out, 		STATUSPREFIX"  %d\n","PTP domain", ptpClock->defaultDS.domainNumber);
	}
	fprintf(out, 		STATUSPREFIX"  %s\n","Port state", portState_getName(ptpClock->portDS.portState));
	if(strlen(alarmBuf) > 0) {
	    fprintf(out, 		STATUSPREFIX"  %s\n","Alarms", alarmBuf);
	}
	    memset(tmpBuf, 0, sizeof(tmpBuf));
	    snprint_PortIdentity(tmpBuf, sizeof(tmpBuf),
	    &ptpClock->portDS.portIdentity);
	fprintf(out, 		STATUSPREFIX"  %s\n","Local port ID", tmpBuf);


	if(ptpClock->portDS.portState >= PTP_MASTER) {
	    memset(tmpBuf, 0, sizeof(tmpBuf));
	    snprint_PortIdentity(tmpBuf, sizeof(tmpBuf),
	    &ptpClock->parentDS.parentPortIdentity);
	fprintf(out, 		STATUSPREFIX"  %s","Best master ID", tmpBuf);
	if(ptpClock->portDS.portState == PTP_MASTER)
	    fprintf(out," (self)");
	    fprintf(out,"\n");
	}
	if(rtOpts->transport == UDP_IPV4 &&
	    ptpClock->portDS.portState > PTP_MASTER &&
	    ptpClock->bestMaster && ptpClock->bestMaster->sourceAddr) {
	    {
	    struct in_addr tmpAddr;
	    tmpAddr.s_addr = ptpClock->bestMaster->sourceAddr;
	    fprintf(out, 		STATUSPREFIX"  %s\n","Best master IP", inet_ntoa(tmpAddr));
	    }
	}
	if(ptpClock->portDS.portState == PTP_SLAVE) {
	fprintf(out, 		STATUSPREFIX"  Priority1 %d, Priority2 %d, clockClass %d","GM priority",
	ptpClock->parentDS.grandmasterPriority1, ptpClock->parentDS.grandmasterPriority2, ptpClock->parentDS.grandmasterClockQuality.clockClass);
	if(rtOpts->unicastNegotiation && ptpClock->parentGrants != NULL ) {
	    	fprintf(out, ", localPref %d", ptpClock->parentGrants->localPreference);
	}
	fprintf(out, "%s\n", (ptpClock->bestMaster != NULL && ptpClock->bestMaster->disqualified) ? " (timeout)" : "");
	}

	if(ptpClock->defaultDS.clockQuality.clockClass < 128 ||
		ptpClock->portDS.portState == PTP_SLAVE ||
		ptpClock->portDS.portState == PTP_PASSIVE){
	fprintf(out, 		STATUSPREFIX"  ","Time properties");
	fprintf(out, "%s timescale, ",ptpClock->timePropertiesDS.ptpTimescale ? "PTP":"ARB");
	fprintf(out, "tracbl: time %s, freq %s, src: %s(0x%02x)\n", ptpClock->timePropertiesDS.timeTraceable ? "Y" : "N",
							ptpClock->timePropertiesDS.frequencyTraceable ? "Y" : "N",
							getTimeSourceName(ptpClock->timePropertiesDS.timeSource),
							ptpClock->timePropertiesDS.timeSource);
	fprintf(out, 		STATUSPREFIX"  ","UTC properties");
	fprintf(out, "UTC valid: %s", ptpClock->timePropertiesDS.currentUtcOffsetValid ? "Y" : "N");
	fprintf(out, ", UTC offset: %d",ptpClock->timePropertiesDS.currentUtcOffset);
	fprintf(out, "%s",ptpClock->timePropertiesDS.leap61 ?
			", LEAP61 pending" : ptpClock->timePropertiesDS.leap59 ? ", LEAP59 pending" : "");
	if (ptpClock->portDS.portState == PTP_SLAVE) {
	    fprintf(out, "%s", rtOpts->preferUtcValid ? ", prefer UTC" : "");
	    fprintf(out, "%s", rtOpts->requireUtcValid ? ", require UTC" : "");
	}
	fprintf(out,"\n");
	}

	if(ptpClock->portDS.portState == PTP_SLAVE) {
	    memset(tmpBuf, 0, sizeof(tmpBuf));
	    snprint_TimeInternal(tmpBuf, sizeof(tmpBuf),
		&ptpClock->currentDS.offsetFromMaster);
	fprintf(out, 		STATUSPREFIX" %s s","Offset from Master", tmpBuf);
#ifdef PTPD_STATISTICS
	if(ptpClock->slaveStats.statsCalculated)
	fprintf(out, ", mean % .09f s, dev % .09f s",
		ptpClock->slaveStats.ofmMean,
		ptpClock->slaveStats.ofmStdDev
	);
#endif /* PTPD_STATISTICS */
	    fprintf(out,"\n");

	if(ptpClock->portDS.delayMechanism == E2E) {
	    memset(tmpBuf, 0, sizeof(tmpBuf));
	    snprint_TimeInternal(tmpBuf, sizeof(tmpBuf),
		&ptpClock->currentDS.meanPathDelay);
	fprintf(out, 		STATUSPREFIX" %s s","Mean Path Delay", tmpBuf);
#ifdef PTPD_STATISTICS
	if(ptpClock->slaveStats.statsCalculated)
	fprintf(out, ", mean % .09f s, dev % .09f s",
		ptpClock->slaveStats.mpdMean,
		ptpClock->slaveStats.mpdStdDev
	);
#endif /* PTPD_STATISTICS */
	fprintf(out,"\n");
	}
	if(ptpClock->portDS.delayMechanism == P2P) {
	    memset(tmpBuf, 0, sizeof(tmpBuf));
	    snprint_TimeInternal(tmpBuf, sizeof(tmpBuf),
		&ptpClock->portDS.peerMeanPathDelay);
	fprintf(out, 		STATUSPREFIX" %s s","Mean Path (p)Delay", tmpBuf);
	fprintf(out,"\n");
	}

	fprintf(out, 		STATUSPREFIX"  ","Clock status");
		if(rtOpts->enablePanicMode) {
	    if(ptpClock->panicMode) {
		fprintf(out,"panic mode,");

	    }
	}
	if(rtOpts->calibrationDelay) {
	    fprintf(out, "%s, ",
		ptpClock->isCalibrated ? "calibrated" : "not calibrated");
	}
	fprintf(out, "%s",
		ptpClock->clockControl.granted ? "in control" : "no control");
	if(rtOpts->noAdjust) {
	    fprintf(out, ", read-only");
	}
#ifdef PTPD_STATISTICS
	else {
	    if (rtOpts->servoStabilityDetection) {
		fprintf(out, ", %s",
		    ptpClock->servo.isStable ? "stabilised" : "not stabilised");
	    }
	}
#endif /* PTPD_STATISTICS */
	fprintf(out,"\n");


	fprintf(out, 		STATUSPREFIX" % .03f ppm","Clock correction",
			    ptpClock->servo.observedDrift / 1000.0);
if(ptpClock->servo.runningMaxOutput)
	fprintf(out, " (slewing at maximum rate)");
else {
#ifdef PTPD_STATISTICS
	if(ptpClock->slaveStats.statsCalculated)
	    fprintf(out, ", mean % .03f ppm, dev % .03f ppm",
		ptpClock->servo.driftMean / 1000.0,
		ptpClock->servo.driftStdDev / 1000.0
	    );
	if(rtOpts->servoStabilityDetection) {
	    fprintf(out, ", dev thr % .03f ppm",
		ptpClock->servo.stabilityThreshold / 1000.0
	    );
	}
#endif /* PTPD_STATISTICS */
}
	    fprintf(out,"\n");


	}



	if(ptpClock->portDS.portState == PTP_MASTER || ptpClock->portDS.portState == PTP_PASSIVE) {

	fprintf(out, 		STATUSPREFIX"  %d","Priority1 ", ptpClock->defaultDS.priority1);
	if(ptpClock->portDS.portState == PTP_PASSIVE)
		fprintf(out, " (best master: %d)", ptpClock->parentDS.grandmasterPriority1);
	fprintf(out,"\n");
	fprintf(out, 		STATUSPREFIX"  %d","Priority2 ", ptpClock->defaultDS.priority2);
	if(ptpClock->portDS.portState == PTP_PASSIVE)
		fprintf(out, " (best master: %d)", ptpClock->parentDS.grandmasterPriority2);
	fprintf(out,"\n");
	fprintf(out, 		STATUSPREFIX"  %d","ClockClass ", ptpClock->defaultDS.clockQuality.clockClass);
	if(ptpClock->portDS.portState == PTP_PASSIVE)
		fprintf(out, " (best master: %d)", ptpClock->parentDS.grandmasterClockQuality.clockClass);
	fprintf(out,"\n");
	if(ptpClock->portDS.delayMechanism == P2P) {
	    memset(tmpBuf, 0, sizeof(tmpBuf));
	    snprint_TimeInternal(tmpBuf, sizeof(tmpBuf),
		&ptpClock->portDS.peerMeanPathDelay);
	fprintf(out, 		STATUSPREFIX" %s s","Mean Path (p)Delay", tmpBuf);
	fprintf(out,"\n");
	}

	}

	if(ptpClock->portDS.portState == PTP_MASTER || ptpClock->portDS.portState == PTP_PASSIVE ||
	    ptpClock->portDS.portState == PTP_SLAVE) {

	fprintf(out,		STATUSPREFIX"  ","Message rates");

	if (ptpClock->portDS.logSyncInterval == UNICAST_MESSAGEINTERVAL)
	    fprintf(out,"[UC-unknown]");
	else if (ptpClock->portDS.logSyncInterval <= 0)
	    fprintf(out,"%.0f/s",pow(2,-ptpClock->portDS.logSyncInterval));
	else
	    fprintf(out,"1/%.0fs",pow(2,ptpClock->portDS.logSyncInterval));
	fprintf(out, " sync");


	if(ptpClock->portDS.delayMechanism == E2E) {
		if (ptpClock->portDS.logMinDelayReqInterval == UNICAST_MESSAGEINTERVAL)
		    fprintf(out,", [UC-unknown]");
		else if (ptpClock->portDS.logMinDelayReqInterval <= 0)
		    fprintf(out,", %.0f/s",pow(2,-ptpClock->portDS.logMinDelayReqInterval));
		else
		    fprintf(out,", 1/%.0fs",pow(2,ptpClock->portDS.logMinDelayReqInterval));
		fprintf(out, " delay");
	}

	if(ptpClock->portDS.delayMechanism == P2P) {
		if (ptpClock->portDS.logMinPdelayReqInterval == UNICAST_MESSAGEINTERVAL)
		    fprintf(out,", [UC-unknown]");
		else if (ptpClock->portDS.logMinPdelayReqInterval <= 0)
		    fprintf(out,", %.0f/s",pow(2,-ptpClock->portDS.logMinPdelayReqInterval));
		else
		    fprintf(out,", 1/%.0fs",pow(2,ptpClock->portDS.logMinPdelayReqInterval));
		fprintf(out, " pdelay");
	}

	if (ptpClock->portDS.logAnnounceInterval == UNICAST_MESSAGEINTERVAL)
	    fprintf(out,", [UC-unknown]");
	else if (ptpClock->portDS.logAnnounceInterval <= 0)
	    fprintf(out,", %.0f/s",pow(2,-ptpClock->portDS.logAnnounceInterval));
	else
	    fprintf(out,", 1/%.0fs",pow(2,ptpClock->portDS.logAnnounceInterval));
	fprintf(out, " announce");

	    fprintf(out,"\n");

	}

	fprintf(out, 		STATUSPREFIX"  ","TimingService");

	fprintf(out, "current %s, best %s, pref %s", (timingDomain.current != NULL) ? timingDomain.current->id : "none",
						(timingDomain.best != NULL) ? timingDomain.best->id : "none",
		    				(timingDomain.preferred != NULL) ? timingDomain.preferred->id : "none");

	if((timingDomain.current != NULL) &&
	    (timingDomain.current->holdTimeLeft > 0)) {
		fprintf(out, ", hold %d sec", timingDomain.current->holdTimeLeft);
	} else	if(timingDomain.electionLeft) {
		fprintf(out, ", elec %d sec", timingDomain.electionLeft);
	}

	fprintf(out, "\n");

	fprintf(out, 		STATUSPREFIX"  ","TimingServices");

	fprintf(out, "total %d, avail %d, oper %d, idle %d, in_ctrl %d%s\n",
				    timingDomain.serviceCount,
				    timingDomain.availableCount,
				    timingDomain.operationalCount,
				    timingDomain.idleCount,
				    timingDomain.controlCount,
				    timingDomain.controlCount > 1 ? " (!)":"");

	fprintf(out, 		STATUSPREFIX"  ","Performance");
	fprintf(out,"Message RX %d/s, TX %d/s", ptpClock->counters.messageReceiveRate,
						  ptpClock->counters.messageSendRate);
	if(ptpClock->portDS.portState == PTP_MASTER) {
		if(rtOpts->unicastNegotiation) {
		    fprintf(out,", slaves %d", ptpClock->slaveCount);
		} else if (rtOpts->ipMode == IPMODE_UNICAST) {
		    fprintf(out,", slaves %d", ptpClock->unicastDestinationCount);
		}
	}

	fprintf(out,"\n");

	if ( ptpClock->portDS.portState == PTP_SLAVE ||
	    ptpClock->defaultDS.clockQuality.clockClass == 255 ) {

	fprintf(out, 		STATUSPREFIX"  %lu\n","Announce received",
	    (unsigned long)ptpClock->counters.announceMessagesReceived);
	fprintf(out, 		STATUSPREFIX"  %lu\n","Sync received",
	    (unsigned long)ptpClock->counters.syncMessagesReceived);
	if(ptpClock->defaultDS.twoStepFlag)
	fprintf(out, 		STATUSPREFIX"  %lu\n","Follow-up received",
	    (unsigned long)ptpClock->counters.followUpMessagesReceived);
	if(ptpClock->portDS.delayMechanism == E2E) {
		fprintf(out, 		STATUSPREFIX"  %lu\n","DelayReq sent",
		    (unsigned long)ptpClock->counters.delayReqMessagesSent);
		fprintf(out, 		STATUSPREFIX"  %lu\n","DelayResp received",
		    (unsigned long)ptpClock->counters.delayRespMessagesReceived);
	}
	}

	if( ptpClock->portDS.portState == PTP_MASTER ||
	    ptpClock->defaultDS.clockQuality.clockClass < 128 ) {
	fprintf(out, 		STATUSPREFIX"  %lu received, %lu sent \n","Announce",
	    (unsigned long)ptpClock->counters.announceMessagesReceived,
	    (unsigned long)ptpClock->counters.announceMessagesSent);
	fprintf(out, 		STATUSPREFIX"  %lu\n","Sync sent",
	    (unsigned long)ptpClock->counters.syncMessagesSent);
	if(ptpClock->defaultDS.twoStepFlag)
	fprintf(out, 		STATUSPREFIX"  %lu\n","Follow-up sent",
	    (unsigned long)ptpClock->counters.followUpMessagesSent);

	if(ptpClock->portDS.delayMechanism == E2E) {
		fprintf(out, 		STATUSPREFIX"  %lu\n","DelayReq received",
		    (unsigned long)ptpClock->counters.delayReqMessagesReceived);
		fprintf(out, 		STATUSPREFIX"  %lu\n","DelayResp sent",
		    (unsigned long)ptpClock->counters.delayRespMessagesSent);
	}

	}

	if(ptpClock->portDS.delayMechanism == P2P) {

		fprintf(out, 		STATUSPREFIX"  %lu received, %lu sent\n","PdelayReq",
		    (unsigned long)ptpClock->counters.pdelayReqMessagesReceived,
		    (unsigned long)ptpClock->counters.pdelayReqMessagesSent);
		fprintf(out, 		STATUSPREFIX"  %lu received, %lu sent\n","PdelayResp",
		    (unsigned long)ptpClock->counters.pdelayRespMessagesReceived,
		    (unsigned long)ptpClock->counters.pdelayRespMessagesSent);
		fprintf(out, 		STATUSPREFIX"  %lu received, %lu sent\n","PdelayRespFollowUp",
		    (unsigned long)ptpClock->counters.pdelayRespFollowUpMessagesReceived,
		    (unsigned long)ptpClock->counters.pdelayRespFollowUpMessagesSent);

	}

	if(ptpClock->counters.domainMismatchErrors)
	fprintf(out, 		STATUSPREFIX"  %lu\n","Domain Mismatches",
		    (unsigned long)ptpClock->counters.domainMismatchErrors);

	if(ptpClock->counters.ignoredAnnounce)
	fprintf(out, 		STATUSPREFIX"  %lu\n","Ignored Announce",
		    (unsigned long)ptpClock->counters.ignoredAnnounce);

	if(ptpClock->counters.unicastGrantsDenied)
	fprintf(out, 		STATUSPREFIX"  %lu\n","Denied Unicast",
		    (unsigned long)ptpClock->counters.unicastGrantsDenied);

	fprintf(out, 		STATUSPREFIX"  %lu\n","State transitions",
		    (unsigned long)ptpClock->counters.stateTransitions);
	fprintf(out, 		STATUSPREFIX"  %lu\n","PTP Engine resets",
		    (unsigned long)ptpClock->resetCount);


	fflush(out);
}

void
displayPortIdentity(PortIdentity *port, const char *prefixMessage)
{
	static char sbuf[SCREEN_BUFSZ];
	int len = 0;

	memset(sbuf, ' ', sizeof(sbuf));
	len += snprintf(sbuf + len, sizeof(sbuf) - len, "%s ", prefixMessage);
	len += snprint_PortIdentity(sbuf + len, sizeof(sbuf) - len, port);
        len += snprintf(sbuf + len, sizeof(sbuf) - len, "\n");
        INFO("%s",sbuf);
}


void
recordSync(UInteger16 sequenceId, TimeInternal * time)
{
	extern RunTimeOpts rtOpts;
	if (logFiles[LOGFILE_RECORD].logEnabled && logFiles[LOGFILE_RECORD].logFP != NULL) {
		fprintf(logFiles[LOGFILE_RECORD].logFP, "%d %llu\n", sequenceId,
		  ((time->seconds * 1000000000ULL) + time->nanoseconds)
		);
		maintainLogSize(&logFiles[LOGFILE_RECORD]);
	}
}

Boolean
nanoSleep(TimeInternal * t)
{
	struct timespec ts, tr;

	ts.tv_sec = t->seconds;
	ts.tv_nsec = t->nanoseconds;

	if (nanosleep(&ts, &tr) < 0) {
		t->seconds = tr.tv_sec;
		t->nanoseconds = tr.tv_nsec;
		return FALSE;
	}
	return TRUE;
}

#ifdef __QNXNTO__

static const struct sigevent* timerIntHandler(void* data, int id) {
  struct timespec tp;
  TimerIntData* myData = (TimerIntData*)data;
  uint64_t new_tsc = ClockCycles();

  clock_gettime(CLOCK_REALTIME, &tp);

  if(new_tsc > myData->prev_tsc) {
    myData->cur_delta = new_tsc - myData->prev_tsc;
  /* when hell freezeth over, thy TSC shall roll over */
  } else {
    myData->cur_delta = myData->prev_delta;
  }
  /* 4/6 weighted average */
  myData->filtered_delta = (40 * myData->cur_delta + 60 * myData->prev_delta) / 100;
  myData->prev_delta = myData->cur_delta;
  myData->prev_tsc = new_tsc;

  if(myData->counter < 2) {
    myData->counter++;
  }

  myData->last_clock = timespec2nsec(&tp);
  return NULL;

}
#endif

void getTime(TimeInternal *time)
{
#ifdef __QNXNTO__
  static TimerIntData tmpData;
  int ret;
  uint64_t delta;
  double tick_delay;
  uint64_t clock_offset;
  struct timespec tp;
  if(!tDataUpdated) {
    memset(&tData, 0, sizeof(TimerIntData));
    if(ThreadCtl(_NTO_TCTL_IO, 0) == -1) {
      ERROR("QNX: could not give process I/O privileges");
      return;
    }

    tData.cps = SYSPAGE_ENTRY(qtime)->cycles_per_sec;
    tData.ns_per_tick = 1000000000.0 / tData.cps;
    tData.prev_tsc = ClockCycles();
    clock_gettime(CLOCK_REALTIME, &tp);
    tData.last_clock = timespec2nsec(&tp);
    ret = InterruptAttach(0, timerIntHandler, &tData, sizeof(TimerIntData), _NTO_INTR_FLAGS_END | _NTO_INTR_FLAGS_TRK_MSK);

    if(ret == -1) {
      ERROR("QNX: could not attach to timer interrupt");
      return ;
    }
    tDataUpdated = TRUE;
    time->seconds = tp.tv_sec;
    time->nanoseconds = tp.tv_nsec;
    return;
  }

  memcpy(&tmpData, &tData, sizeof(TimerIntData));

  delta = ClockCycles() - tmpData.prev_tsc;

  /* compute time since last clock update */
  tick_delay = (double)delta / (double)tmpData.filtered_delta;
  clock_offset = (uint64_t)(tick_delay * tmpData.ns_per_tick * (double)tmpData.filtered_delta);

  /* not filtered yet */
  if(tData.counter < 2) {
    clock_offset = 0;
  }

    DBGV("QNX getTime cps: %lld tick interval: %.09f, time since last tick: %lld\n",
    tmpData.cps, tmpData.filtered_delta * tmpData.ns_per_tick, clock_offset);

    nsec2timespec(&tp, tmpData.last_clock + clock_offset);

    time->seconds = tp.tv_sec;
    time->nanoseconds = tp.tv_nsec;
  return;
#else
#  if defined(_POSIX_TIMERS) && (_POSIX_TIMERS > 0)

	struct timespec tp;
	if (clock_gettime(CLOCK_REALTIME, &tp) < 0) {
		PERROR("clock_gettime() failed, exiting.");
		exit(0);
	}
	time->seconds = tp.tv_sec;
	time->nanoseconds = tp.tv_nsec;

#  else

	struct timeval tv;
	gettimeofday(&tv, 0);
	time->seconds = tv.tv_sec;
	time->nanoseconds = tv.tv_usec * 1000;

#  endif /* _POSIX_TIMERS */
#endif /* __QNXNTO__ */
}

void
getTimeMonotonic(TimeInternal * time)
{
#if defined(_POSIX_TIMERS) && (_POSIX_TIMERS > 0)
	struct timespec tp;
#  ifndef CLOCK_MONOTINIC
	if (clock_gettime(CLOCK_REALTIME, &tp) < 0) {
#  else
	if (clock_gettime(CLOCK_MONOTONIC, &tp) < 0) {
#  endif /* CLOCK_MONOTONIC */
		PERROR("clock_gettime() failed, exiting.");
		exit(0);
	}
	time->seconds = tp.tv_sec;
	time->nanoseconds = tp.tv_nsec;
#else
	struct timeval tv;
	gettimeofday(&tv, 0);
	time->seconds = tv.tv_sec;
	time->nanoseconds = tv.tv_usec * 1000;
#endif /* _POSIX_TIMERS */
}


void
setTime(TimeInternal * time)
{
#if defined(_POSIX_TIMERS) && (_POSIX_TIMERS > 0)

	struct timespec tp;
	tp.tv_sec = time->seconds;
	tp.tv_nsec = time->nanoseconds;

#else

	struct timeval tv;
	tv.tv_sec = time->seconds;
	tv.tv_usec = time->nanoseconds / 1000;

#endif /* _POSIX_TIMERS */

#if defined(_POSIX_TIMERS) && (_POSIX_TIMERS > 0)

	if (clock_settime(CLOCK_REALTIME, &tp) < 0) {
		PERROR("Could not set system time");
		return;
	}

#else

	settimeofday(&tv, 0);

#endif /* _POSIX_TIMERS */

	struct timespec tmpTs = { time->seconds,0 };

	char timeStr[MAXTIMESTR];
	strftime(timeStr, MAXTIMESTR, "%x %X", localtime(&tmpTs.tv_sec));
	WARNING("Stepped the system clock to: %s.%d\n",
	       timeStr, time->nanoseconds);
}

#ifdef HAVE_LINUX_RTC_H

/* Set the RTC to the desired time time */
void setRtc(TimeInternal *timeToSet)
{
	static Boolean deviceFound = FALSE;
	static char* rtcDev;
	struct tm* tmTime;
	time_t seconds;
	int rtcFd;
	struct stat statBuf;

	if (!deviceFound) {
	    if(stat("/dev/misc/rtc", &statBuf) == 0) {
            	rtcDev="/dev/misc/rtc\0";
		deviceFound = TRUE;
	    } else if(stat("/dev/rtc", &statBuf) == 0) {
            	rtcDev="/dev/rtc\0";
		deviceFound = TRUE;
	    }  else if(stat("/dev/rtc0", &statBuf) == 0) {
            	rtcDev="/dev/rtc0\0";
		deviceFound = TRUE;
	    } else {

			ERROR("Could not set RTC time - no suitable rtc device found\n");
			return;
	    }

	    if(!S_ISCHR(statBuf.st_mode)) {
			ERROR("Could not set RTC time - device %s is not a character device\n",
			rtcDev);
			deviceFound = FALSE;
			return;
	    }

	}

	DBGV("Usable RTC device: %s\n",rtcDev);

	if(timeToSet->seconds == 0 && timeToSet->nanoseconds==0) {
	    getTime(timeToSet);
	}



	if((rtcFd = open(rtcDev, O_RDONLY)) < 0) {
		PERROR("Could not set RTC time: error opening %s", rtcDev);
		return;
	}

	seconds = (time_t)timeToSet->seconds;
	if(timeToSet->nanoseconds >= 500000) seconds++;
	tmTime =  gmtime(&seconds);

	DBGV("Set RTC from %d seconds to y: %d m: %d d: %d \n",timeToSet->seconds,tmTime->tm_year,tmTime->tm_mon,tmTime->tm_mday);

	if(ioctl(rtcFd, RTC_SET_TIME, tmTime) < 0) {
		PERROR("Could not set RTC time on %s - ioctl failed", rtcDev);
		goto cleanup;
	}

	NOTIFY("Succesfully set RTC time using %s\n", rtcDev);

cleanup:

	close(rtcFd);
}

#endif /* HAVE_LINUX_RTC_H */

/* returns a double beween 0.0 and 1.0 */
double
getRand(void)
{
	return((rand() * 1.0) / RAND_MAX);
}

/* Attempt setting advisory write lock on a file descriptor*/
int
lockFile(int fd)
{
        struct flock fl;

        fl.l_type = DEFAULT_LOCKMODE;
        fl.l_start = 0;
        fl.l_whence = SEEK_SET;
        fl.l_len = 0;
        return(fcntl(fd, F_SETLK, &fl));
}

/*
 * Check if a file descriptor's lock flags match specified flags,
 * do not acquire lock - just query. If the flags match, populate
 * lockPid with the PID of the process already holding the lock(s)
 * Return values: -1 = error, 0 = locked, 1 = not locked
 */
int checkLockStatus(int fd, short lockType, int *lockPid)
{
    struct flock fl;

    memset(&fl, 0, sizeof(fl));

    if(fcntl(fd, F_GETLK, &fl) == -1)
	{
	return -1;
	}
    /* Return 0 if file is already locked, but not by us */
    if((lockType & fl.l_type) && fl.l_pid != getpid()) {
	*lockPid = fl.l_pid;
	return 0;
    }

    return 1;
}

/*
 * Check if it's possible to acquire lock on a file - if already locked,
 * populate lockPid with the PID currently holding the write lock.
 */
int
checkFileLockable(const char *fileName, int *lockPid)
{
    FILE* fileHandle;
    int ret;

    if((fileHandle = fopen(fileName,"w+")) == NULL) {
	PERROR("Could not open %s for writing\n");
	return -1;
    }

    ret = checkLockStatus(fileno(fileHandle), DEFAULT_LOCKMODE, lockPid);
    if (ret == -1) {
	PERROR("Could not check lock status of %s",fileName);
    }

    fclose(fileHandle);
    return ret;
}

/*
 * If automatic lock files are used, check for potential conflicts
 * based on already existing lock files containing our interface name
 * or clock driver name
 */
Boolean
checkOtherLocks(RunTimeOpts* rtOpts, PtpClock* ptpClock)
{
	char searchPattern[PATH_MAX];
	char * lockPath = 0;
	int lockPid = 0;
	glob_t matchedFiles;
	Boolean ret = TRUE;
	int matches = 0, counter = 0;

	/* no need to check locks */
	if(rtOpts->sysopts.ignore_daemon_lock ||
	   !rtOpts->sysopts.autoLockFile)
		return TRUE;

	/*
	 * Try to discover if we can run in desired mode
	 * based on the presence of other lock files
	 * and them being lockable
	 */

	/* Check for other ptpd running on the same interface - same for all modes */
	snprintf(searchPattern, PATH_MAX, "%s/%s_*_%s.lock",
		 rtOpts->sysopts.lockDirectory, PTPD_PROGNAME,
		 netPathGetInterfaceName(ptpClock->netPath, rtOpts));

	DBGV("SearchPattern: %s\n",searchPattern);
	switch(glob(searchPattern, 0, NULL, &matchedFiles)) {

	    case GLOB_NOSPACE:
	    case GLOB_ABORTED:
		    PERROR("Could not scan %s directory\n", rtOpts->sysopts.lockDirectory);;
		    ret = FALSE;
		    goto end;
	    default:
		    break;
	}

	counter = matchedFiles.gl_pathc;
	matches = counter;
	while (matches--) {
		lockPath=matchedFiles.gl_pathv[matches];
		DBG("matched: %s\n",lockPath);
	/* check if there is a lock file with our NIC in the name */
	    switch(checkFileLockable(lockPath, &lockPid)) {
		/* Could not check lock status */
		case -1:
		    ERROR("Looks like "USER_DESCRIPTION" may be already running on %s: %s found, but could not check lock\n",
			  netPathGetInterfaceName(ptpClock->netPath, rtOpts), lockPath);
		    ret = FALSE;
		    goto end;
		/* It was possible to acquire lock - file looks abandoned */
		case 1:
		    DBG("Lock file %s found, but is not locked for writing.\n", lockPath);
		    ret = TRUE;
		    break;
		/* file is locked */
		case 0:
		    ERROR("Looks like "USER_DESCRIPTION" is already running on %s: %s found and is locked by pid %d\n",
			  netPathGetInterfaceName(ptpClock->netPath, rtOpts), lockPath, lockPid);
		    ret = FALSE;
		    goto end;
	    }
	}

	if(matches > 0)
		globfree(&matchedFiles);

	/* Any mode that can control the clock - also check the clock driver */
	if(rtOpts->clockQuality.clockClass > 127 ) {
		snprintf(searchPattern, PATH_MAX,"%s/%s_%s_*.lock",
			 rtOpts->sysopts.lockDirectory, PTPD_PROGNAME, DEFAULT_CLOCKDRIVER);
		DBGV("SearchPattern: %s\n",searchPattern);

		switch(glob(searchPattern, 0, NULL, &matchedFiles)) {

		case GLOB_NOSPACE:
		case GLOB_ABORTED:
			PERROR("Could not scan %s directory\n", rtOpts->sysopts.lockDirectory);;
			ret = FALSE;
			goto end;
		default:
			break;
		}
		counter = matchedFiles.gl_pathc;
		matches = counter;
		while (counter--) {
			lockPath=matchedFiles.gl_pathv[counter];
			DBG("matched: %s\n",lockPath);
			/* Check if there is a lock file with our clock driver in the name */
			switch(checkFileLockable(lockPath, &lockPid)) {
				/* could not check lock status */
			case -1:
				ERROR("Looks like "USER_DESCRIPTION" may already be controlling the \""DEFAULT_CLOCKDRIVER
				      "\" clock: %s found, but could not check lock status.\n", lockPath);
				ret = FALSE;
				goto end;
				/* it was possible to acquire lock - looks abandoned */
			case 1:
				DBG("Lock file %s found, but is not locked for writing.\n", lockPath);
				ret = TRUE;
				break;
				/* file is locked */
			case 0:
				ERROR("Looks like "USER_DESCRIPTION" is already controlling the \""DEFAULT_CLOCKDRIVER
				      "\" clock: %s found and is locked by pid %d\n", lockPath, lockPid);
			default:
				ret = FALSE;
				goto end;
			}
		}
	}

	ret = TRUE;

end:
	if(matches>0)
		globfree(&matchedFiles);
	return ret;
}

int
writeLockFile(RunTimeOpts* rtOpts)
{
	int lockPid = 0;

	DBGV("Checking lock file: %s\n", rtOpts->sysopts.lockFile);

	if ( (G_lockFilePointer=fopen(rtOpts->sysopts.lockFile, "w+")) == NULL) {
		PERROR("Could not open lock file %s for writing", rtOpts->sysopts.lockFile);
		return(0);
	}
	if (lockFile(fileno(G_lockFilePointer)) < 0) {
		if ( checkLockStatus(fileno(G_lockFilePointer),
				     DEFAULT_LOCKMODE, &lockPid) == 0) {
			ERROR("Another "PTPD_PROGNAME" instance is running: %s locked by PID %d\n",
			      rtOpts->sysopts.lockFile, lockPid);
		} else {
			PERROR("Could not acquire lock on %s:", rtOpts->sysopts.lockFile);
		}
		goto failure;
	}
	if(ftruncate(fileno(G_lockFilePointer), 0) == -1) {
		PERROR("Could not truncate %s: %s",
			rtOpts->sysopts.lockFile, strerror(errno));
		goto failure;
	}
	if ( fprintf(G_lockFilePointer, "%ld\n", (long)getpid()) == -1) {
		PERROR("Could not write to lock file %s: %s",
			rtOpts->sysopts.lockFile, strerror(errno));
		goto failure;
	}
	INFO("Successfully acquired lock on %s\n", rtOpts->sysopts.lockFile);
	fflush(G_lockFilePointer);
	return(1);
	failure:
	fclose(G_lockFilePointer);
	return(0);
}

void clearLockFile(RunTimeOpts* rtOpts)
{
	/* properly clean lockfile (eventough new deaemons can acquire the lock after we die) */
	if(!rtOpts->sysopts.ignore_daemon_lock && G_lockFilePointer != NULL) {
		fclose(G_lockFilePointer);
		G_lockFilePointer = NULL;
	}
	unlink(rtOpts->sysopts.lockFile);
}


/* Whole block of adjtimex() functions starts here - only for systems with sys/timex.h */

#if defined(HAVE_SYS_TIMEX_H) && defined(PTPD_FEATURE_NTP)

/*
 * Apply a tick / frequency shift to the kernel clock
 */

Boolean
adjFreq(double adj)
{
	extern RunTimeOpts rtOpts;
	struct timex t;

#  ifdef HAVE_STRUCT_TIMEX_TICK
	Integer32 tickAdj = 0;
#    ifdef PTPD_DBG2
	double oldAdj = adj;
#    endif
#  endif /* HAVE_STRUCT_TIMEX_TICK */

	memset(&t, 0, sizeof(t));

	/* Clamp to max PPM */
	if (adj > rtOpts.servoMaxPpb){
		adj = rtOpts.servoMaxPpb;
	} else if (adj < -rtOpts.servoMaxPpb){
		adj = -rtOpts.servoMaxPpb;
	}

/* Y U NO HAVE TICK? */
#  ifdef HAVE_STRUCT_TIMEX_TICK

	/* Get the USER_HZ value */
	Integer32 userHZ = sysconf(_SC_CLK_TCK);

	/*
	 * Get the tick resolution (ppb) - offset caused by changing the tick value by 1.
	 * The ticks value is the duration of one tick in us. So with userHz = 100  ticks per second,
	 * change of ticks by 1 (us) means a 100 us frequency shift = 100 ppm = 100000 ppb.
	 * For userHZ = 1000, change by 1 is a 1ms offset (10 times more ticks per second)
	 */
	Integer32 tickRes = userHZ * 1000;

	/*
	 * If we are outside the standard +/-512ppm, switch to a tick + freq combination:
	 * Keep moving ticks from adj to tickAdj until we get back to the normal range.
	 * The offset change will not be super smooth as we flip between tick and frequency,
	 * but this in general should only be happening under extreme conditions when dragging the
	 * offset down from very large values. When maxPPM is left at the default value, behaviour
	 * is the same as previously, clamped to 512ppm, but we keep tick at the base value,
	 * preventing long stabilisation times say when  we had a non-default tick value left over
	 * from a previous NTP run.
	 */
	if (adj > ADJ_FREQ_MAX){
		while (adj > ADJ_FREQ_MAX) {
		    tickAdj++;
		    adj -= tickRes;
		}

	} else if (adj < -ADJ_FREQ_MAX){
		while (adj < -ADJ_FREQ_MAX) {
		    tickAdj--;
		    adj += tickRes;
		}
        }
	/* Base tick duration - 10000 when userHZ = 100 */
	t.tick = 1E6 / userHZ;
	/* Tick adjustment if necessary */
        t.tick += tickAdj;


	t.modes = ADJ_TICK;

#  endif /* HAVE_STRUCT_TIMEX_TICK */

	t.modes |= MOD_FREQUENCY;

	double dFreq = adj * ((1 << 16) / 1000.0);
	t.freq = (int) round(dFreq);
#  ifdef HAVE_STRUCT_TIMEX_TICK
	DBG2("adjFreq: oldadj: %.09f, newadj: %.09f, tick: %d, tickadj: %d\n", oldAdj, adj,t.tick,tickAdj);
#  endif /* HAVE_STRUCT_TIMEX_TICK */
	DBG2("        adj is %.09f;  t freq is %d       (float: %.09f)\n", adj, t.freq,  dFreq);

	return !adjtimex(&t);
}


double
getAdjFreq(void)
{
	struct timex t;
	double dFreq;

	DBGV("getAdjFreq called\n");

	memset(&t, 0, sizeof(t));
	t.modes = 0;
	adjtimex(&t);

	dFreq = (t.freq + 0.0) / ((1<<16) / 1000.0);

	DBGV("          kernel adj is: %f, kernel freq is: %d\n",
		dFreq, t.freq);

	return(dFreq);
}


/* First cut on informing the clock */
void
informClockSource(PtpClock* ptpClock)
{
	struct timex tmx;

	memset(&tmx, 0, sizeof(tmx));

	tmx.modes = MOD_MAXERROR | MOD_ESTERROR;

	tmx.maxerror = (ptpClock->currentDS.offsetFromMaster.seconds * 1E9 +
			ptpClock->currentDS.offsetFromMaster.nanoseconds) / 1000;
	tmx.esterror = tmx.maxerror;

	if (adjtimex(&tmx) < 0)
		DBG("informClockSource: could not set adjtimex flags: %s", strerror(errno));
}


void
unsetTimexFlags(int flags, Boolean quiet)
{
	struct timex tmx;
	int ret;

	memset(&tmx, 0, sizeof(tmx));

	tmx.modes = MOD_STATUS;

	tmx.status = getTimexFlags();
	if(tmx.status == -1)
		return;
	/* unset all read-only flags */
	tmx.status &= ~STA_RONLY;
	tmx.status &= ~flags;

	ret = adjtimex(&tmx);

	if (ret < 0)
		PERROR("Could not unset adjtimex flags: %s", strerror(errno));

	if(!quiet && ret > 2) {
		switch (ret) {
		case TIME_OOP:
			WARNING("Adjtimex: leap second already in progress\n");
			break;
		case TIME_WAIT:
			WARNING("Adjtimex: leap second already occurred\n");
			break;
#  if !defined(TIME_BAD)
		case TIME_ERROR:
#  else
		case TIME_BAD:
#  endif /* TIME_BAD */
		default:
			DBGV("unsetTimexFlags: adjtimex() returned TIME_BAD\n");
			break;
		}
	}
}

int getTimexFlags(void)
{
	struct timex tmx;
	int ret;

	memset(&tmx, 0, sizeof(tmx));

	tmx.modes = 0;
	ret = adjtimex(&tmx);
	if (ret < 0) {
		PERROR("Could not read adjtimex flags: %s", strerror(errno));
		return(-1);
	}
	return( tmx.status );
}

Boolean
checkTimexFlags(int flags)
{
	int tflags = getTimexFlags();

	if (tflags == -1)
		return FALSE;
	return ((tflags & flags) == flags);
}

/*
 * TODO: track NTP API changes - NTP API version check
 * is required - the method of setting the TAI offset
 * may change with next API versions
 */

#  if defined(MOD_TAI) &&  NTP_API == 4
void
setKernelUtcOffset(int utc_offset)
{
	struct timex tmx;
	int ret;

	memset(&tmx, 0, sizeof(tmx));

	tmx.modes = MOD_TAI;
	tmx.constant = utc_offset;

	DBG2("Kernel NTP API supports TAI offset. "
	     "Setting TAI offset to %d", utc_offset);

	ret = adjtimex(&tmx);

	if (ret < 0) {
		PERROR("Could not set kernel TAI offset: %s", strerror(errno));
	}
}

Boolean
getKernelUtcOffset(int *utc_offset)
{
	static Boolean warned = FALSE;
	int ret;

#    if defined(HAVE_NTP_GETTIME)
	struct ntptimeval ntpv;
	memset(&ntpv, 0, sizeof(ntpv));
	ret = ntp_gettime(&ntpv);
#    else
	struct timex tmx;
	memset(&tmx, 0, sizeof(tmx));
	tmx.modes = 0;
	ret = adjtimex(&tmx);
#    endif /* HAVE_NTP_GETTIME */

	if (ret < 0) {
		if(!warned) {
		    PERROR("Could not read adjtimex/ntp_gettime flags: %s", strerror(errno));
		}
		warned = TRUE;
		return FALSE;

	}

#    if !defined(HAVE_NTP_GETTIME) && defined(HAVE_STRUCT_TIMEX_TAI)
	*utc_offset = ( tmx.tai );
	return TRUE;
#    elif defined(HAVE_NTP_GETTIME) && defined(HAVE_STRUCT_NTPTIMEVAL_TAI)
	*utc_offset = (int)(ntpv.tai);
	return TRUE;
#    endif /* HAVE_STRUCT_TIMEX_TAI */
	if(!warned) {
	    WARNING("No OS support for kernel TAI/UTC offset information. Cannot read UTC offset.\n");
	}
	warned = TRUE;
	return FALSE;
}

#  endif /* MOD_TAI */

void
setTimexFlags(int flags, Boolean quiet)
{
	struct timex tmx;
	int ret;

	memset(&tmx, 0, sizeof(tmx));

	tmx.modes = MOD_STATUS;

	tmx.status = getTimexFlags();
	if(tmx.status == -1)
		return;
	/* unset all read-only flags */
	tmx.status &= ~STA_RONLY;
	tmx.status |= flags;

	ret = adjtimex(&tmx);

	if (ret < 0)
		PERROR("Could not set adjtimex flags: %s", strerror(errno));

	if(!quiet && ret > 2) {
		switch (ret) {
		case TIME_OOP:
			WARNING("Adjtimex: leap second already in progress\n");
			break;
		case TIME_WAIT:
			WARNING("Adjtimex: leap second already occurred\n");
			break;
#  if !defined(TIME_BAD)
		case TIME_ERROR:
#  else
		case TIME_BAD:
#  endif /* TIME_BAD */
		default:
			DBGV("unsetTimexFlags: adjtimex() returned TIME_BAD\n");
			break;
		}
	}
}

#endif /* defined(HAVE_SYS_TIMEX_H) && defined(PTPD_FEATURE_NTP) */

#define DRIFTFORMAT "%.0f"

void
restoreDrift(PtpClock * ptpClock, const RunTimeOpts * rtOpts, Boolean quiet)
{
	FILE *driftFP;
	Boolean reset_offset = FALSE;
	double recovered_drift;

	DBGV("restoreDrift called\n");

	if (ptpClock->drift_saved && rtOpts->sysopts.drift_recovery_method > 0 ) {
		ptpClock->servo.observedDrift = ptpClock->last_saved_drift;
		if (!rtOpts->noAdjust && ptpClock->clockControl.granted) {
			adjFreq_wrapper(rtOpts, ptpClock, -ptpClock->last_saved_drift);
		}
		DBG("loaded cached drift\n");
		return;
	}

	switch (rtOpts->sysopts.drift_recovery_method) {

		case DRIFT_FILE:

			if( (driftFP = fopen(rtOpts->sysopts.driftFile,"r")) == NULL) {
			    if(errno!=ENOENT) {
				    PERROR("Could not open drift file: %s - using current kernel frequency offset. Ignore this error if ",
				    rtOpts->sysopts.driftFile);
			    } else {
				    NOTICE("Drift file %s not found - will be initialised on write\n",rtOpts->sysopts.driftFile);
			    }
			} else if (fscanf(driftFP, "%lf", &recovered_drift) != 1) {
				PERROR("Could not load saved offset from drift file - using current kernel frequency offset");
				fclose(driftFP);
			} else {

			if(recovered_drift == 0)
				recovered_drift = 0;

			fclose(driftFP);
			if(quiet)
				DBGV("Observed drift loaded from %s: "DRIFTFORMAT" ppb\n",
					rtOpts->sysopts.driftFile,
					recovered_drift);
			else
				INFO("Observed drift loaded from %s: "DRIFTFORMAT" ppb\n",
					rtOpts->sysopts.driftFile,
					recovered_drift);
				break;
			}

		case DRIFT_KERNEL:
#if defined(HAVE_SYS_TIMEX_H) && defined(PTPD_FEATURE_NTP)
			recovered_drift = -getAdjFreq();
#else
			recovered_drift = 0;
#endif /* defined(HAVE_SYS_TIMEX_H) && defined(PTPD_FEATURE_NTP) */
			if(recovered_drift == 0)
				recovered_drift = 0;

			if (quiet)
				DBGV("Observed_drift loaded from kernel: "DRIFTFORMAT" ppb\n",
					recovered_drift);
			else
				INFO("Observed_drift loaded from kernel: "DRIFTFORMAT" ppb\n",
					recovered_drift);

		break;


		default:

			reset_offset = TRUE;

	}

	if (reset_offset) {
		if (!rtOpts->noAdjust && ptpClock->clockControl.granted)
		  adjFreq_wrapper(rtOpts, ptpClock, 0);
		ptpClock->servo.observedDrift = 0;
		return;
	}

	ptpClock->servo.observedDrift = recovered_drift;

	ptpClock->drift_saved = TRUE;
	ptpClock->last_saved_drift = recovered_drift;

	if (!rtOpts->noAdjust)
		adjFreq_wrapper(rtOpts, ptpClock, -recovered_drift);
}



void
saveDrift(PtpClock * ptpClock, const RunTimeOpts * rtOpts, Boolean quiet)
{
	FILE *driftFP;

	DBGV("saveDrift called\n");

       if(ptpClock->portDS.portState == PTP_PASSIVE ||
              ptpClock->portDS.portState == PTP_MASTER ||
                ptpClock->defaultDS.clockQuality.clockClass < 128) {
                    DBGV("We're not slave - not saving drift\n");
                    return;
            }

        if(ptpClock->servo.observedDrift == 0.0 &&
            ptpClock->portDS.portState == PTP_LISTENING )
                return;

	if (rtOpts->sysopts.drift_recovery_method > 0) {
		ptpClock->last_saved_drift = ptpClock->servo.observedDrift;
		ptpClock->drift_saved = TRUE;
	}

	if (rtOpts->sysopts.drift_recovery_method != DRIFT_FILE)
		return;

	if(ptpClock->servo.runningMaxOutput) {
	    DBG("Servo running at maximum shift - not saving drift file");
	    return;
	}

	if( (driftFP = fopen(rtOpts->sysopts.driftFile,"w")) == NULL) {
		PERROR("Could not open drift file %s for writing", rtOpts->sysopts.driftFile);
		return;
	}

	/* The fractional part really won't make a difference here */
	fprintf(driftFP, "%d\n", (int)round(ptpClock->servo.observedDrift));

	if (quiet) {
		DBGV("Wrote observed drift ("DRIFTFORMAT" ppb) to %s\n",
		    ptpClock->servo.observedDrift, rtOpts->sysopts.driftFile);
	} else {
		INFO("Wrote observed drift ("DRIFTFORMAT" ppb) to %s\n",
		    ptpClock->servo.observedDrift, rtOpts->sysopts.driftFile);
	}
	fclose(driftFP);
}

#undef DRIFTFORMAT

//int parseLeapFile(char *path, LeapSecondInfo *info)
Boolean updateLeapInfo(const RunTimeOpts* rtOpts, LeapSecondInfo *info)
{
    FILE *leapFP;
    TimeInternal now;
    char lineBuf[PATH_MAX];

    unsigned long ntpSeconds = 0;
    Integer32 utcSeconds = 0;
    Integer32 utcExpiry = 0;
    int ntpOffset = 0;
    int res;

    if(!rtOpts || !info) return FALSE;

    // Check that the leap file was provided. Abort if not.
    if(strcmp(rtOpts->sysopts.leapFile,"") == 0) return FALSE;

    memset(info, 0, sizeof(LeapSecondInfo));

    getTime(&now);

    info->valid = FALSE;

    if( (leapFP = fopen(rtOpts->sysopts.leapFile,"r")) == NULL) {
	PERROR("Could not open leap second list file %s", rtOpts->sysopts.leapFile);
	return FALSE;
    } else

    memset(info, 0, sizeof(LeapSecondInfo));

    while (fgets(lineBuf, PATH_MAX - 1, leapFP) != NULL) {

	/* capture file expiry time */
	res = sscanf(lineBuf, "#@ %lu", &ntpSeconds);
	if(res == 1) {
	    utcExpiry = ntpSeconds - NTP_EPOCH;
	    DBG("leapfile expiry %d\n", utcExpiry);
	}
	/* capture leap seconds information */
	res = sscanf(lineBuf, "%lu %d", &ntpSeconds, &ntpOffset);
	if(res ==2) {
	    utcSeconds = ntpSeconds - NTP_EPOCH;
	    DBG("leapfile date %d offset %d\n", utcSeconds, ntpOffset);

	    /* next leap second date found */

	    if((now.seconds ) < utcSeconds) {
		info->nextOffset = ntpOffset;
		info->endTime = utcSeconds;
		info->startTime = utcSeconds - 86400;
		break;
	    } else
	    /* current leap second value found */
		if(now.seconds >= utcSeconds) {
		info->currentOffset = ntpOffset;
	    }

	}

    }

    fclose(leapFP);

    /* leap file past expiry date */
    if(utcExpiry && utcExpiry < now.seconds) {
	WARNING("Leap seconds file is expired. Please download the current version\n");
	return FALSE;
    }

    /* we have the current offset - the rest can be invalid but at least we have this */
    if(info->currentOffset != 0) {
	info->offsetValid = TRUE;
    }

    /* if anything failed, return FALSE so we know we cannot use leap file information */
    if((info->startTime == 0) || (info->endTime == 0) ||
	(info->currentOffset == 0) || (info->nextOffset == 0)) {
	return FALSE;
	INFO("Leap seconds file %s loaded (incomplete): now %d, current %d next %d from %d to %d, type %s\n", rtOpts->sysopts.leapFile,
	now.seconds,
	info->currentOffset, info->nextOffset,
	info->startTime, info->endTime, info->leapType > 0 ? "positive" : info->leapType < 0 ? "negative" : "unknown");
    }

    if(info->nextOffset > info->currentOffset) {
	info->leapType = 1;
    }

    if(info->nextOffset < info->currentOffset) {
	info->leapType = -1;
    }

    INFO("Leap seconds file %s loaded: now %d, current %d next %d from %d to %d, type %s\n", rtOpts->sysopts.leapFile,
	now.seconds,
	info->currentOffset, info->nextOffset,
	info->startTime, info->endTime, info->leapType > 0 ? "positive" : info->leapType < 0 ? "negative" : "unknown");
    info->valid = TRUE;
    return TRUE;
}

void
updateXtmp (TimeInternal oldTime, TimeInternal newTime)
{
/* Add the old time entry to utmp/wtmp */

/* About as long as the ntpd implementation, but not any less ugly */

#ifdef HAVE_UTMPX_H
		struct utmpx utx;
	memset(&utx, 0, sizeof(utx));
		strncpy(utx.ut_user, "date", sizeof(utx.ut_user));
#  ifndef OTIME_MSG
		strncpy(utx.ut_line, "|", sizeof(utx.ut_line));
#  else
		strncpy(utx.ut_line, OTIME_MSG, sizeof(utx.ut_line));
#  endif /* OTIME_MSG */
#  ifdef OLD_TIME
		utx.ut_tv.tv_sec = oldTime.seconds;
		utx.ut_tv.tv_usec = oldTime.nanoseconds / 1000;
		utx.ut_type = OLD_TIME;
#  else /* no ut_type */
		utx.ut_time = oldTime.seconds;
#  endif /* OLD_TIME */

/* ======== BEGIN  OLD TIME EVENT - UTMPX / WTMPX =========== */
#  ifdef HAVE_UTMPXNAME
		utmpxname("/var/log/utmp");
#  endif /* HAVE_UTMPXNAME */
		setutxent();
		pututxline(&utx);
		endutxent();
#  ifdef HAVE_UPDWTMPX
		updwtmpx("/var/log/wtmp", &utx);
#  endif /* HAVE_IPDWTMPX */
/* ======== END    OLD TIME EVENT - UTMPX / WTMPX =========== */

#else /* NO HAVE_UTMPX_H */

#  ifdef HAVE_UTMP_H
		struct utmp ut;
		memset(&ut, 0, sizeof(ut));
		strncpy(ut.ut_name, "date", sizeof(ut.ut_name));
#    ifndef OTIME_MSG
		strncpy(ut.ut_line, "|", sizeof(ut.ut_line));
#    else
		strncpy(ut.ut_line, OTIME_MSG, sizeof(ut.ut_line));
#    endif /* OTIME_MSG */

#    ifdef OLD_TIME

#      ifdef HAVE_STRUCT_UTMP_UT_TIME
		ut.ut_time = oldTime.seconds;
#      else
		ut.ut_tv.tv_sec = oldTime.seconds;
		ut.ut_tv.tv_usec = oldTime.nanoseconds / 1000;
#      endif /* HAVE_STRUCT_UTMP_UT_TIME */

		ut.ut_type = OLD_TIME;
#    else /* no ut_type */
		ut.ut_time = oldTime.seconds;
#    endif /* OLD_TIME */

/* ======== BEGIN  OLD TIME EVENT - UTMP / WTMP =========== */
#    ifdef HAVE_UTMPNAME
		utmpname(UTMP_FILE);
#    endif /* HAVE_UTMPNAME */
#    ifdef HAVE_SETUTENT
		setutent();
#    endif /* HAVE_SETUTENT */
#    ifdef HAVE_PUTUTLINE
		pututline(&ut);
#    endif /* HAVE_PUTUTLINE */
#    ifdef HAVE_ENDUTENT
		endutent();
#    endif /* HAVE_ENDUTENT */
#    ifdef HAVE_UTMPNAME
		utmpname(WTMP_FILE);
#    endif /* HAVE_UTMPNAME */
#    ifdef HAVE_SETUTENT
		setutent();
#    endif /* HAVE_SETUTENT */
#    ifdef HAVE_PUTUTLINE
		pututline(&ut);
#    endif /* HAVE_PUTUTLINE */
#    ifdef HAVE_ENDUTENT
		endutent();
#    endif /* HAVE_ENDUTENT */
/* ======== END    OLD TIME EVENT - UTMP / WTMP =========== */

#  endif /* HAVE_UTMP_H */
#endif /* HAVE_UTMPX_H */

/* Add the new time entry to utmp/wtmp */

#ifdef HAVE_UTMPX_H
		memset(&utx, 0, sizeof(utx));
		strncpy(utx.ut_user, "date", sizeof(utx.ut_user));
#  ifndef NTIME_MSG
		strncpy(utx.ut_line, "{", sizeof(utx.ut_line));
#  else
		strncpy(utx.ut_line, NTIME_MSG, sizeof(utx.ut_line));
#  endif /* NTIME_MSG */
#  ifdef NEW_TIME
		utx.ut_tv.tv_sec = newTime.seconds;
		utx.ut_tv.tv_usec = newTime.nanoseconds / 1000;
		utx.ut_type = NEW_TIME;
#  else /* no ut_type */
		utx.ut_time = newTime.seconds;
#  endif /* NEW_TIME */

/* ======== BEGIN  NEW TIME EVENT - UTMPX / WTMPX =========== */
#  ifdef HAVE_UTMPXNAME
		utmpxname("/var/log/utmp");
#  endif /* HAVE_UTMPXNAME */
		setutxent();
		pututxline(&utx);
		endutxent();
#  ifdef HAVE_UPDWTMPX
		updwtmpx("/var/log/wtmp", &utx);
#  endif /* HAVE_UPDWTMPX */
/* ======== END    NEW TIME EVENT - UTMPX / WTMPX =========== */

#else /* NO HAVE_UTMPX_H */

#  ifdef HAVE_UTMP_H
		memset(&ut, 0, sizeof(ut));
		strncpy(ut.ut_name, "date", sizeof(ut.ut_name));
#    ifndef NTIME_MSG
		strncpy(ut.ut_line, "{", sizeof(ut.ut_line));
#    else
		strncpy(ut.ut_line, NTIME_MSG, sizeof(ut.ut_line));
#    endif /* NTIME_MSG */
#    ifdef NEW_TIME

#      ifdef HAVE_STRUCT_UTMP_UT_TIME
		ut.ut_time = newTime.seconds;
#      else
		ut.ut_tv.tv_sec = newTime.seconds;
		ut.ut_tv.tv_usec = newTime.nanoseconds / 1000;
#      endif /* HAVE_STRUCT_UTMP_UT_TIME */
		ut.ut_type = NEW_TIME;
#    else /* no ut_type */
		ut.ut_time = newTime.seconds;
#    endif /* NEW_TIME */

/* ======== BEGIN  NEW TIME EVENT - UTMP / WTMP =========== */
#    ifdef HAVE_UTMPNAME
		utmpname(UTMP_FILE);
#    endif /* HAVE_UTMPNAME */
#    ifdef HAVE_SETUTENT
		setutent();
#    endif /* HAVE_SETUTENT */
#    ifdef HAVE_PUTUTLINE
		pututline(&ut);
#    endif /* HAVE_PUTUTLINE */
#    ifdef HAVE_ENDUTENT
		endutent();
#    endif /* HAVE_ENDUTENT */
#    ifdef HAVE_UTMPNAME
		utmpname(WTMP_FILE);
#    endif /* HAVE_UTMPNAME */
#    ifdef HAVE_SETUTENT
		setutent();
#    endif /* HAVE_SETUTENT */
#    ifdef HAVE_PUTUTLINE
		pututline(&ut);
#    endif /* HAVE_PUTUTLINE */
#    ifdef HAVE_ENDUTENT
		endutent();
#    endif /* HAVE_ENDUTENT */
/* ======== END    NEW TIME EVENT - UTMP / WTMP =========== */

#  endif /* HAVE_UTMP_H */
#endif /* HAVE_UTMPX_H */
}

int setCpuAffinity(int cpu)
{
#ifdef __QNXNTO__
    unsigned    num_elements = 0;
    int         *rsizep, masksize_bytes, size;
    int    *rmaskp, *imaskp;
    void        *my_data;
    uint32_t cpun;
    num_elements = RMSK_SIZE(_syspage_ptr->num_cpu);

    masksize_bytes = num_elements * sizeof(unsigned);

    size = sizeof(int) + 2 * masksize_bytes;
    if ((my_data = malloc(size)) == NULL) {
        return -1;
    } else {
        memset(my_data, 0x00, size);

        rsizep = (int *)my_data;
        rmaskp = rsizep + 1;
        imaskp = rmaskp + num_elements;

        *rsizep = num_elements;

	if(cpu > _syspage_ptr->num_cpu) {
	    return -1;
	}

	if(cpu >= 0) {
	    cpun = (uint32_t)cpu;
	    RMSK_SET(cpun, rmaskp);
    	    RMSK_SET(cpun, imaskp);
	} else {
		for(cpun = 0;  cpun < num_elements; cpun++) {
		    RMSK_SET(cpun, rmaskp);
		    RMSK_SET(cpun, imaskp);
		}
	}
	int ret = ThreadCtl( _NTO_TCTL_RUNMASK_GET_AND_SET_INHERIT, my_data);
	free(my_data);
	return ret;
    }

#endif

#ifdef HAVE_SYS_CPUSET_H
	cpuset_t mask;
    	CPU_ZERO(&mask);
	if(cpu >= 0) {
    	    CPU_SET(cpu,&mask);
	} else {
		int i;
		for(i = 0;  i < CPU_SETSIZE; i++) {
			CPU_SET(i, &mask);
		}
	}
    	return(cpuset_setaffinity(CPU_LEVEL_WHICH, CPU_WHICH_PID,
			      -1, sizeof(mask), &mask));
#endif /* HAVE_SYS_CPUSET_H */

#if defined(linux) && defined(HAVE_SCHED_H)
	cpu_set_t mask;
	CPU_ZERO(&mask);
	if(cpu >= 0) {
	    CPU_SET(cpu,&mask);
	} else {
		int i;
		for(i = 0;  i < CPU_SETSIZE; i++) {
			CPU_SET(i, &mask);
		}
	}
	return sched_setaffinity(0, sizeof(mask), &mask);
#endif /* linux && HAVE_SCHED_H */

    return -1;
}

 /*
 * Synchronous signal processing:
 * original idea: http://www.openbsd.org/cgi-bin/cvsweb/src/usr.sbin/ntpd/ntpd.c?rev=1.68;content-type=text%2Fplain
 */
volatile sig_atomic_t	 sigint_received  = 0;
volatile sig_atomic_t	 sigterm_received = 0;
volatile sig_atomic_t	 sighup_received  = 0;
volatile sig_atomic_t	 sigusr1_received = 0;
volatile sig_atomic_t	 sigusr2_received = 0;

/*
 * Function to catch signals asynchronously.
 * Assuming that the daemon periodically calls checkSignals(), then all operations are safely done synchrously at a later opportunity.
 *
 * Please do NOT call any functions inside this handler - especially DBG() and its friends, or any glibc.
 */
static void catchSignals(int sig)
{
	switch (sig) {
	case SIGINT:
		sigint_received = 1;
		break;
	case SIGTERM:
		sigterm_received = 1;
		break;
	case SIGHUP:
		sighup_received = 1;
		break;
	case SIGUSR1:
		sigusr1_received = 1;
		break;
	case SIGUSR2:
		sigusr2_received = 1;
		break;
	default:
		/*
		 * TODO: should all other signals be catched, and handled as SIGINT?
		 *
		 * Reason: currently, all other signals are just uncatched, and the OS kills us.
		 * The difference is that we could then close the open files properly.
		 */
		break;
	}
}

/*
 * exit the program cleanly
 */
static void
do_signal_close(PtpClock * ptpClock)
{
	timingDomain.shutdown(&timingDomain);

	NOTIFY("Shutdown on close signal\n");
	exit(0);
}


/**
 * Signal handler for HUP which tells us to swap the log file
 * and reload configuration file if specified
 *
 * @param sig
 */
static void
do_signal_sighup(RunTimeOpts * rtOpts, PtpClock * ptpClock)
{
	NOTIFY("SIGHUP received\n");

#ifdef RUNTIME_DEBUG
	if(rtOpts->transport == UDP_IPV4 && rtOpts->ipMode != IPMODE_UNICAST) {
		DBG("SIGHUP - running an ipv4 multicast based mode, re-sending IGMP joins\n");
		netRefreshIGMP(ptpClock->netPath, rtOpts, ptpClock);
	}
#endif /* RUNTIME_DEBUG */

	reloadConfigFile(rtOpts, ptpClock);

	/* tell the service it can perform any HUP-triggered actions */
	ptpClock->timingService.reloadRequested = TRUE;

	if(logFiles[LOGFILE_RECORD].logEnabled ||
	   logFiles[LOGFILE_EVENT].logEnabled ||
	   logFiles[LOGFILE_STATISTICS].logEnabled)
		INFO("Reopening log files\n");

	restartLogging();

	if(logFiles[LOGFILE_STATISTICS].logEnabled)
		ptpClock->resetStatisticsLog = TRUE;
}

/*
 * Synchronous signal processing:
 * This function should be called regularly from the main loop
 */
void
checkSignals(RunTimeOpts * rtOpts, PtpClock * ptpClock)
{
	/*
	 * note:
	 * alarm signals are handled in a similar way in dep/timer.c
	 */

	if(sigint_received || sigterm_received){
		do_signal_close(ptpClock);
	}

	if(sighup_received){
		do_signal_sighup(rtOpts, ptpClock);
		sighup_received=0;
	}

	if(sigusr1_received){
		if(ptpClock->portDS.portState == PTP_SLAVE){
			WARNING("SIGUSR1 received, stepping clock to current known OFM\n");
			stepClock(rtOpts, ptpClock);
			//ptpClock->clockControl.stepRequired = TRUE;
		} else {
			ERROR("SIGUSR1 received - will not step clock, not in PTP_SLAVE state\n");
		}
		sigusr1_received = 0;
	}

	if(sigusr2_received){

/* testing only: testing step detection */
#if 0
		{
		ptpClock->addOffset ^= 1;
		INFO("a: %d\n", ptpClock->addOffset);
		sigusr2_received = 0;
		return;
		}
#endif
		displayCounters(ptpClock);
		displayAlarms(ptpClock->alarms, ALRM_MAX);
		if(netPathGetTimingACL(ptpClock->netPath)) {
			INFO("\n\n");
			INFO("** Timing message ACL:\n");
			dumpIpv4AccessList(netPathGetTimingACL(ptpClock->netPath));
		}
		if(netPathGetManagementACL(ptpClock->netPath)) {
			INFO("\n\n");
			INFO("** Management message ACL:\n");
			dumpIpv4AccessList(netPathGetManagementACL(ptpClock->netPath));
		}
		if(rtOpts->sysopts.clearCounters) {
			clearCounters(ptpClock);
			NOTIFY("PTP engine counters cleared\n");
		}
#ifdef PTPD_STATISTICS
		if(rtOpts->oFilterSMConfig.enabled) {
			ptpClock->oFilterSM.display(&ptpClock->oFilterSM);
		}
		if(rtOpts->oFilterMSConfig.enabled) {
			ptpClock->oFilterMS.display(&ptpClock->oFilterMS);
		}
#endif /* PTPD_STATISTICS */
		sigusr2_received = 0;
	}
}

Boolean
sysPrePtpClockInit(RunTimeOpts* rtOpts, Integer16* ret)
{
	TimeInternal tmpTime;

	/*
	 * Set the default mode for all newly created files - previously
	 * this was not the case for log files. This adds consistency
	 * and allows to use FILE* vs. fds everywhere
	 */
	umask(~DEFAULT_FILE_PERMS);

	/* get some entropy in... */
	getTime(&tmpTime);
	srand(tmpTime.seconds ^ tmpTime.nanoseconds);

	/*
	 * we try to catch as many error conditions as possible, but before we call daemon().
	 * the exception is the lock file, as we get a new pid when we call daemon(),
	 * so this is checked twice: once to read, second to read/write
	 */
	if(geteuid() != 0)
	{
		printf("Error: "PTPD_PROGNAME" daemon can only be run as root\n");
		*ret = 1;
		return FALSE;
	}

	/* DAEMON */
	if(!rtOpts->sysopts.nonDaemon){
		/*
		 * fork to daemon - nochdir non-zero to preserve the working directory:
		 * allows relative paths to be used for log files, config files etc.
		 * Always redirect stdout/err to /dev/null
		 */
		if (daemon(1,0) == -1) {
			PERROR("Failed to start as daemon");
			*ret = 3;
			return FALSE;
		}
		INFO("  Info:    Now running as a daemon\n");
		/*
		 * Wait for the parent process to terminate, but not forever.
		 * On some systems this happened after we tried re-acquiring
		 * the lock, so the lock would fail. Hence, we wait.
		 */
		for (int i = 0; i < 1000000; i++) {
			/* Once we've been reaped by init, parent PID will be 1 */
			if(getppid() == 1)
				break;
			usleep(1);
		}
	}

#if (defined(linux) && defined(HAVE_SCHED_H)) || defined(HAVE_SYS_CPUSET_H) || defined(__QNXNTO__)
	/* Try binding to a single CPU core if configured to do so */
	if(rtOpts->sysopts.cpuNumber > -1) {
		if(setCpuAffinity(rtOpts->sysopts.cpuNumber) < 0) {
			ERROR("Could not bind to CPU core %d\n", rtOpts->sysopts.cpuNumber);
		} else {
			INFO("Successfully bound "PTPD_PROGNAME" to CPU core %d\n", rtOpts->sysopts.cpuNumber);
		}
	}
#endif

	/* establish signal handlers */
	signal(SIGINT,  catchSignals);
	signal(SIGTERM, catchSignals);
	signal(SIGHUP,  catchSignals);
	signal(SIGUSR1, catchSignals);
	signal(SIGUSR2, catchSignals);

	*ret = 0;
	return TRUE;
}

Boolean
sysPostPtpClockInit(RunTimeOpts* rtOpts, PtpClock* ptpClock, Integer16* ret)
{
	/* First lock check, just to be user-friendly to the operator */
	if(!rtOpts->sysopts.ignore_daemon_lock) {
		if(!writeLockFile(rtOpts)){
			/* check and create Lock */
			ERROR("Error: file lock failed (use -L or global:ignore_lock to ignore lock file)\n");
			*ret = 3;
			return FALSE;
		}
		/* check for potential conflicts when automatic lock files are used */
		if(!checkOtherLocks(rtOpts, ptpClock)) {
			*ret = 3;
			return FALSE;
		}
	}

	*ret = 0;
	return TRUE;
}

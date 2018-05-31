/**
 * @file   ptpd_logging.h
 *
 * @brief  Declare logging functions and platform independent logging defines.
 *
 */

#ifndef PTPD_LOGGING_H_
#define PTPD_LOGGING_H_

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif /* HAVE_CONFIG_H */

#ifdef RUNTIME_DEBUG
#  undef PTPD_DBGV
#  define PTPD_DBGV
#endif

 /** \name System messages*/
 /**\{*/

#if defined(HAVE_SYSLOG_H) && HAVE_SYSLOG_H
#  include <sys/syslog.h>
#else
#  define LOG_EMERG       0       /* system is unusable */
#  define LOG_ALERT       1       /* action must be taken immediately */
#  define LOG_CRIT        2       /* critical conditions */
#  define LOG_ERR         3       /* error conditions */
#  define LOG_WARNING     4       /* warning conditions */
#  define LOG_NOTICE      5       /* normal but significant condition */
#  define LOG_INFO        6       /* informational */
#  define LOG_DEBUG       7       /* debug-level messages */
#endif

// Syslog ordering. We define extra debug levels above LOG_DEBUG for internal use - but message() doesn't pass these to SysLog
#define LOG_DEBUG1   7
#define LOG_DEBUG2   8
#define LOG_DEBUG3   9
#define LOG_DEBUGV   9

#define EMERGENCY(x, ...) logMessage(LOG_EMERG, x, ##__VA_ARGS__)
#define ALERT(x, ...)     logMessage(LOG_ALERT, x, ##__VA_ARGS__)
#define CRITICAL(x, ...)  logMessage(LOG_CRIT, x, ##__VA_ARGS__)
#define ERROR(x, ...)     logMessage(LOG_ERR, x, ##__VA_ARGS__)
#define PERROR(x, ...)    logMessage(LOG_ERR, x "      (strerror: %m)\n", ##__VA_ARGS__)
#define WARNING(x, ...)   logMessage(LOG_WARNING, x, ##__VA_ARGS__)
#define NOTIFY(x, ...)    logMessage(LOG_NOTICE, x, ##__VA_ARGS__)
#define NOTICE(x, ...)    logMessage(LOG_NOTICE, x, ##__VA_ARGS__)
#define INFO(x, ...)      logMessage(LOG_INFO, x, ##__VA_ARGS__)

#define EMERGENCY_LOCAL(x, ...)	EMERGENCY(LOCAL_PREFIX": " x,##__VA_ARGS__)
#define ALERT_LOCAL(x, ...)	ALERT(LOCAL_PREFIX": " x, ##__VA_ARGS__)
#define CRITICAL_LOCAL(x, ...)	CRITICAL(LOCAL_PREFIX": " x, ##__VA_ARGS__)
#define ERROR_LOCAL(x, ...)	ERROR(LOCAL_PREFIX": " x, ##__VA_ARGS__)
#define PERROR_LOCAL(x, ...)	PERROR(LOCAL_PREFIX": " x, ##__VA_ARGS__)
#define WARNING_LOCAL(x, ...)	WARNING(LOCAL_PREFIX": " x, ##__VA_ARGS__)
#define NOTIFY_LOCAL(x, ...)	NOTIFY(LOCAL_PREFIX": " x, ##__VA_ARGS__)
#define NOTIC_LOCALE(x, ...)	NOTICE(LOCAL_PREFIX": " x, ##__VA_ARGS__)
#define INFO_LOCAL(x, ...)	INFO(LOCAL_PREFIX": " x, ##__VA_ARGS__)

#define EMERGENCY_LOCAL_ID(o,x, ...)	EMERGENCY(LOCAL_PREFIX".%s: "x,o->id,##__VA_ARGS__)
#define ALERT_LOCAL_ID(o,x, ...)	ALERT(LOCAL_PREFIX".%s: "x,o->id, ##__VA_ARGS__)
#define CRITICAL_LOCAL_ID(o,x, ...)	CRITICAL(LOCAL_PREFIX".%s: "x,o->id, ##__VA_ARGS__)
#define ERROR_LOCAL_ID(o,x, ...)	ERROR(LOCAL_PREFIX".%s: "x,o->id, ##__VA_ARGS__)
#define PERROR_LOCAL_ID(o,x, ...)	PERROR(LOCAL_PREFIX".%s: "x,o->id, ##__VA_ARGS__)
#define WARNING_LOCAL_ID(o,x, ...)	WARNING(LOCAL_PREFIX".%s: "x,o->id, ##__VA_ARGS__)
#define NOTIFY_LOCAL_ID(o,x, ...)	NOTIFY(LOCAL_PREFIX".%s: "x,o->id, ##__VA_ARGS__)
#define NOTICE_LOCAL_ID(o,x, ...)	NOTICE(LOCAL_PREFIX".%s: "x,o->id, ##__VA_ARGS__)
#define INFO_LOCAL_ID(o,x, ...)		INFO(LOCAL_PREFIX".%s: "x,o->id, ##__VA_ARGS__)

#if defined(__FILE__) && defined(__LINE__)
#  define MARKER INFO("Marker: %s:%d\n", __FILE__, __LINE__)
#else
#  define MARKER INFO("Marker\n")
#endif

/** \}*/


/** \name Debug messages*/
 /**\{*/

#ifdef PTPD_DBGV
#  undef PTPD_DBG
#  undef PTPD_DBG2
#  define PTPD_DBG
#  define PTPD_DBG2
#  define DBGV(x, ...)            logMessage(LOG_DEBUGV, x, ##__VA_ARGS__)
#  define DBGV_LOCAL(x, ...)      DBGV(LOCAL_PREFIX": " x,##__VA_ARGS__)
#  define DBGV_LOCAL_ID(o,x, ...) DBGV(LOCAL_PREFIX".%s:"x,o->id,##__VA_ARGS__)
#else
#  define DBGV(x, ...)
#  define DBGV_LOCAL(x, ...)
#  define DBGV_LOCAL_ID(x, ...)
#endif

/*
 * new debug level DBG2:
 * this is above DBG(), but below DBGV() (to avoid changing hundreds of lines)
 */

#ifdef PTPD_DBG2
#  undef PTPD_DBG
#  define PTPD_DBG
#  define DBG2(x, ...)            logMessage(LOG_DEBUG2, x, ##__VA_ARGS__)
#  define DBG2_LOCAL(x, ...)      DBG2(LOCAL_PREFIX": " x,##__VA_ARGS__)
#  define DBG2_LOCAL_ID(o,x, ...) DBG2(LOCAL_PREFIX".%s:"x,o->id,##__VA_ARGS__)
#else
#  define DBG2(x, ...)
#  define DBG2_LOCAL(x, ...)
#  define DBG2_LOCAL_ID(x, ...)
#endif

#ifdef PTPD_DBG
#  define DBG(x, ...)            logMessage(LOG_DEBUG, x, ##__VA_ARGS__)
#  define DBG_LOCAL(x, ...)      DBG(LOCAL_PREFIX": " x,##__VA_ARGS__)
#  define DBG_LOCAL_ID(o,x, ...) DBG(LOCAL_PREFIX".%s:"x,o->id,##__VA_ARGS__)
#else
#  define DBG(x, ...)
#  define DBG_LOCAL(x, ...)
#  define DBG_LOCAL_ID(x, ...)
#endif

/** \}*/

void logMessage(int priority, const char *format, ...);


#endif

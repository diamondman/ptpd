
/* constants_dep.h */

#ifndef CONSTANTS_DEP_H
#define CONSTANTS_DEP_H

/**
*\file
* \brief Plateform-dependent constants definition
*
* This header defines all includes and constants which are plateform-dependent
*
* ptpdv2 is only implemented for linux, NetBSD and FreeBSD
 */

/* platform dependent */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif /* HAVE_CONFIG_H */

#if !defined(linux) && !defined(__NetBSD__) && !defined(__FreeBSD__) && \
  !defined(__APPLE__) && !defined(__OpenBSD__) && !defined(__sun) && !defined(__QNXNTO__)
#  error PTPD hasn't been ported to this OS - should be possible \
if it's POSIX compatible, if you succeed, report it to ptpd-devel@sourceforge.net
#endif

#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <fcntl.h>

#ifdef HAVE_SYS_ISA_DEFS_H
#  include <sys/isa_defs.h> // sun related
#endif /* HAVE_SYS_ISA_DEFS_H */

#ifdef __QNXNTO__
#  include <sys/neutrino.h>
#  include <sys/syspage.h>
#  define BSD_INTERFACE_FUNCTIONS
#endif /* __QNXNTO __ */

#define IFACE_NAME_LENGTH         IF_NAMESIZE
#define NET_ADDRESS_LENGTH        INET_ADDRSTRLEN

#define CLOCK_IDENTITY_LENGTH 8
#define ADJ_FREQ_MAX 500000

#define SUBDOMAIN_ADDRESS_LENGTH  4
#define PORT_ADDRESS_LENGTH       2
#define PTP_UUID_LENGTH           6
#define CLOCK_IDENTITY_LENGTH     8
#define FLAG_FIELD_LENGTH         2

#define PACKET_SIZE  300
#define PACKET_BEGIN_UDP (ETHER_HDR_LEN + sizeof(struct ip) + sizeof(struct udphdr))
#define PACKET_BEGIN_ETHER (ETHER_HDR_LEN)

#define PTP_EVENT_PORT    319
#define PTP_GENERAL_PORT  320

#define DEFAULT_PTP_DOMAIN_ADDRESS     "224.0.1.129"
#define PEER_PTP_DOMAIN_ADDRESS        "224.0.0.107"

/* 802.3 Support */

#define PTP_ETHER_DST "01:1b:19:00:00:00"
#define PTP_ETHER_TYPE 0x88f7
#define PTP_ETHER_PEER "01:80:c2:00:00:0E"

#ifdef PTPD_UNICAST_MAX
#  define UNICAST_MAX_DESTINATIONS PTPD_UNICAST_MAX
#else
#  define UNICAST_MAX_DESTINATIONS 16
#endif /* PTPD_UNICAST_MAX */

/* dummy clock driver designation in preparation for generic clock driver API */
#define DEFAULT_CLOCKDRIVER "kernelclock"
/* default lock file location and mode */
#define DEFAULT_LOCKMODE F_WRLCK
#define DEFAULT_LOCKDIR "/var/run"
#define DEFAULT_LOCKFILE_NAME PTPD_PROGNAME".lock"
//define DEFAULT_LOCKFILE_PATH  DEFAULT_LOCKDIR"/"DEFAULT_LOCKFILE_NAME
#define DEFAULT_FILE_PERMS (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)

/* default drift file location */
#define DEFAULT_DRIFTFILE "/etc/"PTPD_PROGNAME"_"DEFAULT_CLOCKDRIVER".drift"

/* default status file location */
#define DEFAULT_STATUSFILE DEFAULT_LOCKDIR"/"PTPD_PROGNAME".status"

/* Highest log level (default) catches all */
#define LOG_ALL LOG_DEBUGV

/* Difference between Unix time / UTC and NTP time */
#define NTP_EPOCH 2208988800ULL

/* wait a maximum of 10 ms for a late TX timestamp */
#define LATE_TXTIMESTAMP_US 10000

/* drift recovery metod for use with -F */
enum {
	DRIFT_RESET = 0,
	DRIFT_KERNEL,
	DRIFT_FILE
};
/* IP transmission mode */
enum {
	IPMODE_MULTICAST = 0,
	IPMODE_UNICAST,
	IPMODE_HYBRID,
#if 0
	IPMODE_UNICAST_SIGNALING
#endif
};

/* log timestamp mode */
enum {
	TIMESTAMP_DATETIME,
	TIMESTAMP_UNIX,
	TIMESTAMP_BOTH
};

/* servo dT calculation mode */
enum {
	DT_NONE,
	DT_CONSTANT,
	DT_MEASURED
};

/* StatFilter op type */
enum {
	FILTER_NONE,
	FILTER_MEAN,
	FILTER_MIN,
	FILTER_MAX,
	FILTER_ABSMIN,
	FILTER_ABSMAX,
	FILTER_MEDIAN,
	FILTER_MAXVALUE
};

/* StatFilter window type */
enum {
	WINDOW_INTERVAL,
	WINDOW_SLIDING,
//	WINDOW_OVERLAPPING
};

/* Leap second handling */
enum {
	LEAP_ACCEPT,
	LEAP_IGNORE,
	LEAP_STEP,
	LEAP_SMEAR
};

/* others */

/* bigger screen size constants */
#define SCREEN_BUFSZ  228
#define SCREEN_MAXSZ  180

/* default size for string buffers */
#define BUF_SIZE  1000

#define NANOSECONDS_MAX 999999999

// limit operator messages to once every X seconds
#define OPERATOR_MESSAGES_INTERVAL 300.0

#define MAX_SEQ_ERRORS 50

#define MAXTIMESTR 32

#endif /*CONSTANTS_DEP_H_*/

/**
 * \file
 * \brief Implementation specific datatype
 */

#ifndef DATATYPES_DEP_H_
#define DATATYPES_DEP_H_

#include <limits.h> // For PATH_MAX

#include "ptp_primitives.h"
#include "net.h"

typedef struct LogFileConfig {
	Boolean logInitiallyEnabled;
	char* logID;
	char* openMode;
	char logPath[PATH_MAX+1];

	Boolean truncateOnReopen;
	Boolean unlinkOnClose;
	int maxFiles;
	UInteger32 maxSize;
} LogFileConfig;

#define PTPD_HAS_SYSOPTS
typedef struct sysopts_s {

	/* Receive and send packets using libpcap, bypassing
	   the network stack. */
	Boolean pcap;
	Boolean ignore_daemon_lock;

	// NET
	char unicastPeerDestination[MAXHOSTNAMELEN];

	/* Access list settings */
	Boolean timingAclEnabled;
	Boolean managementAclEnabled;
	char timingAclPermitText[PATH_MAX+1];
	char timingAclDenyText[PATH_MAX+1];
	char managementAclPermitText[PATH_MAX+1];
	char managementAclDenyText[PATH_MAX+1];
	Enumeration8 timingAclOrder;
	Enumeration8 managementAclOrder;

	int ttl;
	int dscpValue;

	/* disable UDP checksum validation where supported */
	Boolean disableUdpChecksums;

	/* list of unicast destinations for use with unicast
	   with or without signaling */
	char unicastDestinations[MAXHOSTNAMELEN * UNICAST_MAX_DESTINATIONS];
	char unicastDomains[MAXHOSTNAMELEN * UNICAST_MAX_DESTINATIONS];
	char unicastLocalPreference[MAXHOSTNAMELEN * UNICAST_MAX_DESTINATIONS];

	Octet primaryIfaceName[IFACE_NAME_LENGTH];
	Octet backupIfaceName[IFACE_NAME_LENGTH];
	Boolean backupIfaceEnabled;


	// SYS
#ifdef RUNTIME_DEBUG
	int debug_level;
#endif
#if (defined(linux) && defined(HAVE_SCHED_H)) || defined(HAVE_SYS_CPUSET_H) || defined (__QNXNTO__)
	int cpuNumber;
#endif /* linux && HAVE_SCHED_H || HAVE_SYS_CPUSET_H*/

	Boolean clearCounters;
	Enumeration8 statisticsTimestamp;
	Enumeration8 logLevel;
	int statisticsLogInterval;
	Boolean useSysLog;
	LogFileConfig statisticsLogConfig;
	LogFileConfig recordLogConfig;
	LogFileConfig eventLogConfig;
	LogFileConfig statusLogConfig;
	Boolean  nonDaemon;
	char lockFile[PATH_MAX+1]; /* lock file location */
	char driftFile[PATH_MAX+1]; /* drift file location */
	Enumeration8 drift_recovery_method; /* how the observed drift is managed
				      between restarts */
	char lockDirectory[PATH_MAX+1]; /* Directory to store lock files
				       * When automatic lock files used */
	Boolean autoLockFile; /* mode and interface specific lock files are used
				    * when set to TRUE */
	char leapFile[PATH_MAX+1]; /* leap seconds file location */

} SysOpts;


#endif /*DATATYPES_DEP_H_*/

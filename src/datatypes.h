#ifndef DATATYPES_H_
#define DATATYPES_H_

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif /* HAVE_CONFIG_H */

#include <limits.h>
#include <sys/param.h>
#include <stdio.h>
#include <stdint.h>

#include "constants.h" // For USER_DESCRIPTION_MAX
#include "dep/constants_dep.h" // For UNICAST_MAX_DESTINATIONS, PACKET_SIZE
#include "ptp_primitives.h"
#include "dep/datatypes_dep.h"
#include "timingdomain.h"
#include "ptp_datatypes.h"
#include "signaling.h"
#include "ptp_timers.h"
#include "datatypes_stub.h"
#include "dep/iniparser/dictionary.h"
#ifdef PTPD_FEATURE_NTP
#  include "dep/ntpengine/ntpdcontrol.h"
#endif
#ifdef PTPD_STATISTICS
#  include "dep/statistics.h"
#  include "dep/outlierfilter.h"
#endif /* PTPD_STATISTICS */
#include "dep/alarm_datatypes.h"
#include "dep/net.h"
#include "dep/servo.h"

/**
 * \struct PtpdCounters
 * \brief Ptpd engine counters per port
 */
typedef struct
{

	/*
	 * message sent/received counters:
	 * - sent only incremented on success,
	 * - received only incremented when message valid and accepted,
	 * - looped messages to self don't increment received,
	 */
	uint32_t announceMessagesSent;
	uint32_t announceMessagesReceived;
	uint32_t syncMessagesSent;
	uint32_t syncMessagesReceived;
	uint32_t followUpMessagesSent;
	uint32_t followUpMessagesReceived;
	uint32_t delayReqMessagesSent;
	uint32_t delayReqMessagesReceived;
	uint32_t delayRespMessagesSent;
	uint32_t delayRespMessagesReceived;
	uint32_t pdelayReqMessagesSent;
	uint32_t pdelayReqMessagesReceived;
	uint32_t pdelayRespMessagesSent;
	uint32_t pdelayRespMessagesReceived;
	uint32_t pdelayRespFollowUpMessagesSent;
	uint32_t pdelayRespFollowUpMessagesReceived;
	uint32_t signalingMessagesSent;
	uint32_t signalingMessagesReceived;
	uint32_t managementMessagesSent;
	uint32_t managementMessagesReceived;

/* not implemented yet */

	/* FMR counters */
	uint32_t foreignAdded; // implement me! /* number of insertions to FMR */
	uint32_t foreignCount; // implement me! /* number of foreign masters seen */
	uint32_t foreignRemoved; // implement me! /* number of FMR records deleted */
	uint32_t foreignOverflows; // implement me! /* how many times the FMR was full */

	/* protocol engine counters */

	uint32_t stateTransitions;	  /* number of state changes */
	uint32_t bestMasterChanges;		  /* number of BM changes as result of BMC */
	uint32_t announceTimeouts;	  /* number of announce receipt timeouts */

	/* discarded / uknown / ignored */
	uint32_t discardedMessages;	  /* only messages we shouldn't be receiving - ignored from self don't count */
	uint32_t unknownMessages;	  /* unknown type - also increments discarded */
	uint32_t ignoredAnnounce;	  /* ignored Announce messages: acl / security / preference */
	uint32_t aclTimingMessagesDiscarded;	  /* Timing messages discarded by access lists */
	uint32_t aclManagementMessagesDiscarded;	  /* Timing messages discarded by access lists */

	/* error counters */
	uint32_t messageRecvErrors;	  /* message receive errors */
	uint32_t messageSendErrors;	  /* message send errors */
	uint32_t messageFormatErrors;	  /* headers or messages too short etc. */
	uint32_t protocolErrors;	  /* conditions that shouldn't happen */
	uint32_t versionMismatchErrors;	  /* V1 received, V2 expected - also increments discarded */
	uint32_t domainMismatchErrors;	  /* different domain than configured - also increments discarded */
	uint32_t sequenceMismatchErrors;  /* mismatched sequence IDs - also increments discarded */
	uint32_t delayMechanismMismatchErrors; /* P2P received, E2E expected or vice versa - incremets discarded */
	uint32_t consecutiveSequenceErrors;    /* number of consecutive sequence mismatch errors */

	/* unicast sgnaling counters */
	uint32_t unicastGrantsRequested;  /* slave: how many we requested, master: how many requests we received */
	uint32_t unicastGrantsGranted;	  /* slave: how many we got granted, master: how many we granted */
	uint32_t unicastGrantsDenied;	 /* slave: how many we got denied, master: how many we denied */
	uint32_t unicastGrantsCancelSent;   /* how many we canceled */
	uint32_t unicastGrantsCancelReceived; /* how many cancels we received */
	uint32_t unicastGrantsCancelAckSent; /* how many cancel ack we sent */
	uint32_t unicastGrantsCancelAckReceived; /* how many cancel ack we received */

#ifdef PTPD_STATISTICS
	uint32_t delayMSOutliersFound;	  /* Number of outliers found by the delayMS filter */
	uint32_t delaySMOutliersFound;	  /* Number of outliers found by the delaySM filter */
#endif /* PTPD_STATISTICS */
	uint32_t maxDelayDrops; /* number of samples dropped due to maxDelay threshold */

	uint32_t messageSendRate;	/* RX message rate per sec */
	uint32_t messageReceiveRate;	/* TX message rate per sec */

} PtpdCounters;

typedef struct {
	Boolean activity; 		/* periodic check, updateClock sets this to let the watchdog know we're holding clock control */
	Boolean	available; 	/* flags that we can control the clock */
	Boolean granted; 	/* upstream watchdog will grant this when we're the best provider */
	Boolean updateOK;		/* if not, updateClock() will not run */
	Boolean stepRequired;		/* if set, we need to step the clock */
	Boolean offsetOK;		/* if set, updateOffset accepted oFm */
} ClockControlInfo;

typedef struct {
	Boolean inSync;
	Boolean leapInsert;
	Boolean leapDelete;
	Boolean majorChange;
	Boolean update;
	Boolean override; /* this tells the client that this info takes priority
			   * over whatever the client itself would like to set
			   * prime example: leap second file
			   */
	int utcOffset;
	long clockOffset;
} ClockStatusInfo;

typedef struct {
	Integer32 transportAddress;
} SyncDestEntry;


/**
 * \struct RunTimeOpts
 * \brief Program options set at run-time
 */
/* program options set at run-time */
typedef struct RunTimeOpts {
	Integer8 logAnnounceInterval;
	Integer8 announceReceiptTimeout;
	Integer8 logSyncInterval;
	Integer8 logMinDelayReqInterval;
	Integer8 logMinPdelayReqInterval;

	Boolean slaveOnly;

	ClockQuality clockQuality;
	TimePropertiesDS timeProperties;

	UInteger8 priority1;
	UInteger8 priority2;
	UInteger8 domainNumber;
	UInteger16 portNumber;

	/* Max intervals for unicast negotiation */
	Integer8 logMaxPdelayReqInterval;
	Integer8 logMaxDelayReqInterval;
	Integer8 logMaxSyncInterval;
	Integer8 logMaxAnnounceInterval;

	/* optional BMC extension: accept any domain, prefer configured domain, prefer lower domain */
	Boolean anyDomain;

	/*
	 * For slave state, grace period of n * announceReceiptTimeout
	 * before going into LISTENING again - we disqualify the best GM
	 * and keep waiting for BMC to act - if a new GM picks up,
	 * we don't lose the calculated offsets etc. Allows seamless GM
	 * failover with little time variation
	 */

	int announceTimeoutGracePeriod;

	Octet primaryIfaceName[IFACE_NAME_LENGTH];
	Octet backupIfaceName[IFACE_NAME_LENGTH];
	Boolean backupIfaceEnabled;

	Boolean	noResetClock; // don't step the clock if offset > 1s
	Boolean stepForce; // force clock step on first sync after startup
	Boolean stepOnce; // only step clock on first sync after startup
#ifdef linux
	Boolean setRtc;
#endif /* linux */

	Integer8 masterRefreshInterval;

	Integer32 maxOffset; /* Maximum number of nanoseconds of offset */
	Integer32 maxDelay; /* Maximum number of nanoseconds of delay */

	Boolean	noAdjust;

	Boolean displayPackets;
	Integer16 s;
	TimeInternal inboundLatency, outboundLatency, ofmShift;
	Integer16 max_foreign_records;
	Enumeration8 delayMechanism;

	Boolean portDisabled;

	Boolean alwaysRespectUtcOffset;
	Boolean preferUtcValid;
	Boolean requireUtcValid;
	Boolean checkConfigOnly;
	Boolean printLockFile;

	int leapSecondPausePeriod;
	Enumeration8 leapSecondHandling;
	Integer32 leapSecondSmearPeriod;
	int leapSecondNoticePeriod;

	Boolean periodicUpdates;
	Boolean logStatistics;

	int statusFileUpdateInterval;

	Boolean do_IGMP_refresh;

	int initial_delayreq;

	Boolean ignore_delayreq_interval_master;
	Boolean autoDelayReqInterval;

	char leapFile[PATH_MAX+1]; /* leap seconds file location */

	LeapSecondInfo	leapInfo;

#ifdef PTPD_SNMP
	Boolean snmpEnabled;		/* SNMP subsystem enabled / disabled even if compiled in */
	Boolean snmpTrapsEnabled; 	/* enable sending of SNMP traps (requires alarms enabled) */
#endif

	Boolean alarmsEnabled; 		/* enable support for alarms */
	int	alarmMinAge;		/* minimal alarm age in seconds (from set to clear notification) */
	int	alarmInitialDelay;	/* initial delay before we start processing alarms; example:  */
					/* we don't need a port state alarm just before the port starts to sync */

	Enumeration8 transport; /* transport type */
	Enumeration8 ipMode; /* IP transmission mode */
	Boolean dot1AS; /* 801.2AS support -> transportSpecific field */

	char productDescription[65];
	char portDescription[65];

	Boolean		unicastDestinationsSet;
	Boolean		unicastPeerDestinationSet;

	UInteger32	unicastGrantDuration;

	Boolean unicastNegotiation; /* Enable unicast negotiation support */
	Boolean	unicastNegotiationListening; /* Master: Reply to signaling messages when in LISTENING */
	Boolean disableBMCA; /* used to achieve master-only for unicast */
	Boolean unicastAcceptAny; /* Slave: accept messages from all GMs, regardless of grants */
	/*
	 * port mask to apply to portNumber when using negotiation:
	 * treats different port numbers as the same port ID for clocks which
	 * transmit signaling using one port ID, and rest of messages with another
	 */
	UInteger16  unicastPortMask; /* port mask to apply to portNumber when using negotiation */

	Boolean pidAsClockId;

	/**
	 * This field holds the flags denoting which subsystems
	 * have to be re-initialised as a result of config reload.
	 * Flags are defined in daemonconfig.h
	 * 0 = no change
	 */
	UInteger32 restartSubsystems;
	/* config dictionary containers - current and from CLI */
	dictionary *currentConfig, *cliConfig;

	Enumeration8 selectedPreset;

	int servoMaxPpb;
	double servoKP;
	double servoKI;
	Enumeration8 servoDtMethod;
	double servoMaxdT;

	/**
	 *  When enabled, ptpd ensures that Sync message sequence numbers
	 *  are increasing (consecutive sync is not lower than last).
	 *  This can prevent reordered sequences, but can also lock the slave
	 *  up if, say, GM restarted and reset sequencing.
	 */
	Boolean syncSequenceChecking;

	/**
	  * (seconds) - if set to non-zero, slave will reset if no clock updates
	  * after this amount of time. Used to "unclog" slave stuck on offset filter
	  * threshold or after GM reset the Sync sequence number
	  */
	int clockUpdateTimeout;

#ifdef	PTPD_STATISTICS
	OutlierFilterConfig oFilterMSConfig;
	OutlierFilterConfig oFilterSMConfig;

	StatFilterOptions filterMSOpts;
	StatFilterOptions filterSMOpts;


	Boolean servoStabilityDetection;
	double servoStabilityThreshold;
	int servoStabilityTimeout;
	int servoStabilityPeriod;

	Boolean maxDelayStableOnly;
#endif
	/* also used by the periodic message ticker */
	int statsUpdateInterval;

	int calibrationDelay;
	Boolean enablePanicMode;
	Boolean panicModeReleaseClock;
	int panicModeDuration;
	UInteger32 panicModeExitThreshold;
	int idleTimeout;

	UInteger32 ofmAlarmThreshold;

#ifdef PTPD_FEATURE_NTP
	NTPoptions ntpOptions;
	Boolean preferNTP;
#endif

	int electionDelay; /* timing domain election delay to prevent flapping */

	int maxDelayMaxRejected;

	/* max reset cycles in LISTENING before full network restart */
	int maxListen;

	Boolean managementEnabled;
	Boolean managementSetEnable;


	/***************************************************/
	/* Variables for port implementation (net.c/sys.c) */
	/***************************************************/
	struct sysopts_s {

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
		LogFileHandler statisticsLog;
		LogFileHandler recordLog;
		LogFileHandler eventLog;
		LogFileHandler statusLog;
		Boolean  nonDaemon;
		char lockFile[PATH_MAX+1]; /* lock file location */
		char driftFile[PATH_MAX+1]; /* drift file location */
		Enumeration8 drift_recovery_method; /* how the observed drift is managed
					      between restarts */
		char lockDirectory[PATH_MAX+1]; /* Directory to store lock files
					       * When automatic lock files used */
		Boolean autoLockFile; /* mode and interface specific lock files are used
					    * when set to TRUE */

	} sysopts;

} RunTimeOpts;


/**
 * \struct PtpClock
 * \brief Main program data structure
 */
/* main program data structure */
typedef struct PtpClock {

	/* PTP datsets */
	DefaultDS defaultDS; 			/* Default data set */
	CurrentDS currentDS; 			/* Current data set */
	TimePropertiesDS timePropertiesDS; 	/* Global time properties data set */
	PortDS portDS; 				/* Port data set */
	ParentDS parentDS; 			/* Parent data set */

	/* Leap second related flags */
	Boolean leapSecondInProgress;
	Boolean leapSecondPending;

	/* Foreign master data set */
	ForeignMasterRecord *foreign;
	/* Current best master (unless it's us) */
	ForeignMasterRecord *bestMaster;

	/* Other things we need for the protocol */
	UInteger16 number_foreign_records;
	Integer16  max_foreign_records;
	Integer16  foreign_record_i;
	Integer16  foreign_record_best;
	UInteger32 random_seed;
	Boolean  record_update;    /* should we run bmc() after receiving an announce message? */

	Boolean disabled;	/* port is permanently disabled */

	/* unicast grant table - our own grants or our slaves' grants or grants to peers */
	UnicastGrantTable unicastGrants[UNICAST_MAX_DESTINATIONS];
	/* our trivial index table to speed up lookups */
	UnicastGrantIndex grantIndex;
	/* current parent from the above table */
	UnicastGrantTable *parentGrants;
	/* previous parent's grants when changing parents: if not null, this is what should be canceled */
	UnicastGrantTable *previousGrants;
	/* another index to match unicast Sync with FollowUp when we can't capture the destination address of Sync */
	SyncDestEntry syncDestIndex[UNICAST_MAX_DESTINATIONS];

	/* unicast destinations parsed from config */
	UnicastDestination unicastDestinations[UNICAST_MAX_DESTINATIONS];
	int unicastDestinationCount;

	/* number of slaves we have granted Announce to */
	int slaveCount;

	/* unicast destination for pdelay */
	UnicastDestination  unicastPeerDestination;

	/* support for unicast negotiation in P2P mode */
	UnicastGrantTable peerGrants;

	MsgHeader msgTmpHeader;

	union {
		MsgSync  sync;
		MsgFollowUp  follow;
		MsgDelayReq  req;
		MsgDelayResp resp;
		MsgPdelayReq  preq;
		MsgPdelayResp  presp;
		MsgPdelayRespFollowUp  prespfollow;
		MsgManagement  manage;
		MsgAnnounce  announce;
		MsgSignaling signaling;
	} msgTmp;

	MsgManagement outgoingManageTmp;
	MsgSignaling outgoingSignalingTmp;

	Octet msgObuf[PACKET_SIZE];
	Octet msgIbuf[PACKET_SIZE];

	int followUpGap;

	/*
	20110630: These variables were deprecated in favor of the ones that appear in the stats log (delayMS and delaySM)

	TimeInternal  master_to_slave_delay;
	TimeInternal  slave_to_master_delay;
	*/

	TimeInternal	pdelay_req_receive_time;
	TimeInternal	pdelay_req_send_time;
	TimeInternal	pdelay_resp_receive_time;
	TimeInternal	pdelay_resp_send_time;
	TimeInternal	sync_receive_time;
	TimeInternal	delay_req_send_time;
	TimeInternal	delay_req_receive_time;
	MsgHeader	PdelayReqHeader;
	MsgHeader	delayReqHeader;
	TimeInternal	pdelayMS;
	TimeInternal	pdelaySM;
	TimeInternal	delayMS;
	TimeInternal	delaySM;
	TimeInternal	lastSyncCorrectionField;
	TimeInternal	lastPdelayRespCorrectionField;

	Boolean		sentPdelayReq;
	UInteger16	sentPdelayReqSequenceId;
	UInteger16	sentDelayReqSequenceId;
	UInteger16	sentSyncSequenceId;
	UInteger16	sentAnnounceSequenceId;
	UInteger16	sentSignalingSequenceId;
	UInteger16	recvPdelayReqSequenceId;
	UInteger16	recvSyncSequenceId;
	UInteger16	recvPdelayRespSequenceId;
	Boolean		waitingForFollow;
	Boolean		waitingForDelayResp;

	offset_from_master_filter  ofm_filt;
	one_way_delay_filter  mpd_filt;

	Boolean message_activity;

	IntervalTimer   timers[PTP_MAX_TIMER];
	AlarmEntry	alarms[ALRM_MAX];
	int alarmDelay;

	NetPath* netPath;

	/*Usefull to init network stuff*/
	UInteger8 port_communication_technology;

	/*Stats header will be re-printed when set to true*/
	Boolean resetStatisticsLog;

	int listenCount; // number of consecutive resets to listening
	int resetCount;
	int announceTimeouts;
	int current_init_clock;
	int can_step_clock;
	int warned_operator_slow_slewing;
	int warned_operator_fast_slewing;
	Boolean warnedUnicastCapacity;
	int maxDelayRejected;

	Boolean runningBackupInterface;


	char char_last_msg;                             /* representation of last message processed by servo */

	int syncWaiting;                     /* we'll only start the delayReq timer after the first sync */
	int delayRespWaiting;                /* Just for information purposes */
	Boolean startup_in_progress;

	Boolean pastStartup;				/* we've set the clock already, at least once */

	Boolean	offsetFirstUpdated;

	uint32_t init_timestamp;                        /* When the clock was last initialised */
	Integer32 stabilisation_time;                   /* How long (seconds) it took to stabilise the clock */
	double last_saved_drift;                     /* Last observed drift value written to file */
	Boolean drift_saved;                            /* Did we save a drift value already? */

	/* user description is max size + 1 to leave space for a null terminator */
	Octet userDescription[USER_DESCRIPTION_MAX + 1];
	Octet profileIdentity[6];

	Integer32	lastSyncDst;		/* destination address for last sync, so we know where to send the followUp - last resort: we should capture the dst address ourselves */
	Integer32	lastPdelayRespDst;	/* captures the destination address of last pdelayResp so we know where to send the pdelayRespfollowUp */

	/*
	 * counters - useful for debugging and monitoring,
	 * should be exposed through management messages
	 * and SNMP eventually
	 */
	PtpdCounters counters;

	/* PI servo model */
	PIservo servo;

	/* "panic mode" support */
	Boolean panicMode; /* in panic mode - do not update clock or calculate offsets */
	Boolean panicOver; /* panic mode is over, we can reset the clock */
	int panicModeTimeLeft; /* How many 30-second periods left in panic mode */

	/* used to wait on failure while allowing timers to tick */
	Boolean initFailure;
	Integer32 initFailureTimeout;

	/*
	 * used to inform TimingService about our status, but so
	 * that PTP code is independent from LibCCK and glue code can poll this
	 */
	ClockControlInfo clockControl;
	ClockStatusInfo clockStatus;


	TimeInternal	rawDelayMS;
	TimeInternal	rawDelaySM;
	TimeInternal	rawPdelayMS;
	TimeInternal	rawPdelaySM;

#ifdef	PTPD_STATISTICS
	/*
	 * basic clock statistics information, useful
	 * for monitoring servo performance and estimating
	 * clock stability - should be exposed through
	 * management messages and SNMP eventually
	*/
	PtpEngineSlaveStats slaveStats;

	OutlierFilter 	oFilterMS;
	OutlierFilter	oFilterSM;

	DoubleMovingStatFilter *filterMS;
	DoubleMovingStatFilter *filterSM;

#endif

	Integer32 acceptedUpdates;
	Integer32 offsetUpdates;

	Boolean isCalibrated;

#ifdef PTPD_FEATURE_NTP
	NTPcontrol ntpControl;
#endif

	/* the interface to TimingDomain */
	TimingService timingService;

	/* accumulating offset correction added when smearing leap second */
	double leapSmearFudge;

	/* testing only - used to add a 1ms offset */
#if 0
	Boolean addOffset;
#endif

	const RunTimeOpts *rtOpts;

} PtpClock;


#endif /*DATATYPES_H_*/

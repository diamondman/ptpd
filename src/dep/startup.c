/*-
 * Copyright (c) 2014-2015 Wojciech Owczarek,
 * Copyright (c) 2012-2013 George V. Neville-Neil,
 *                         Wojciech Owczarek
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
 * @file   startup.c
 * @date   Wed Jun 23 09:33:27 2010
 *
 * @brief  Code to handle daemon startup, including command line args
 *
 * The function in this file are called when the daemon starts up
 * and include the getopt() command line argument parsing.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <signal.h>

#include "constants.h"
#include "dep/constants_dep.h"
#include "dep/ipv4_acl.h"
#include "ptp_primitives.h"
#include "timingdomain.h"
#if defined(PTPD_FEATURE_NTP)
#  include "dep/ntpengine/ntpdcontrol.h"
#endif
#include "ptp_datatypes.h"
#include "ptp_timers.h"
#include "datatypes.h"
#include "dep/sys.h" // Used for lots of stuff
#ifdef PTPD_SNMP
#  include "dep/snmp.h"
#endif
#include "dep/net.h"
#include "dep/startup.h"
#include "dep/servo.h"
#include "dep/msg.h"
#include "dep/alarms.h"
#include "protocol.h"
#include "display.h"
#include "dep/daemonconfig.h"
#include "dep/configdefaults.h"
#include "ptpd_logging.h"

/*
 * valgrind 3.5.0 currently reports no errors (last check: 20110512)
 * valgrind 3.4.1 lacks an adjtimex handler
 *
 * to run:   sudo valgrind --show-reachable=yes --leak-check=full --track-origins=yes -- ./ptpd2 -c ...
 */

/*
  to test daemon locking and startup sequence itself, try:

  function s()  { set -o pipefail ;  eval "$@" |  sed 's/^/\t/' ; echo $?;  }
  sudo killall ptpd2
  s ./ptpd2
  s sudo ./ptpd2
  s sudo ./ptpd2 -t -g
  s sudo ./ptpd2 -t -g -b eth0
  s sudo ./ptpd2 -t -g -b eth0
  ps -ef | grep ptpd2
*/

/*
 * Synchronous signal processing:
 * original idea: http://www.openbsd.org/cgi-bin/cvsweb/src/usr.sbin/ntpd/ntpd.c?rev=1.68;content-type=text%2Fplain
 */
volatile sig_atomic_t	 sigint_received  = 0;
volatile sig_atomic_t	 sigterm_received = 0;
volatile sig_atomic_t	 sighup_received  = 0;
volatile sig_atomic_t	 sigusr1_received = 0;
volatile sig_atomic_t	 sigusr2_received = 0;

/* Pointer to the current lock file */
FILE* G_lockFilePointer;

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

void
applyConfig(dictionary *baseConfig, RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	Boolean reloadSuccessful = TRUE;


	/* Load default config to fill in the blanks in the config file */
	RunTimeOpts tmpOpts;
	loadDefaultSettings(&tmpOpts);

	/* Check the new configuration for errors, fill in the blanks from defaults */
	if( ( rtOpts->candidateConfig = parseConfig(CFGOP_PARSE, NULL, baseConfig, &tmpOpts)) == NULL ) {
		WARNING("Configuration has errors, reload aborted\n");
		return;
	}

	/* Check for changes between old and new configuration */
	if(compareConfig(rtOpts->candidateConfig,rtOpts->currentConfig)) {
		INFO("Configuration unchanged\n");
		goto cleanup;
	}

	/*
	 * Mark which subsystems have to be restarted. Most of this will be picked up by doState()
	 * If there are errors past config correctness (such as non-existent NIC,
	 * or lock file clashes if automatic lock files used - abort the mission
	 */

	rtOpts->restartSubsystems =
	    checkSubsystemRestart(rtOpts->candidateConfig, rtOpts->currentConfig, rtOpts);

	/* If we're told to re-check lock files, do it: tmpOpts already has what rtOpts should */
	if( (rtOpts->restartSubsystems & PTPD_CHECK_LOCKS) &&
	    tmpOpts.autoLockFile && !checkOtherLocks(&tmpOpts)) {
		reloadSuccessful = FALSE;
	}

	/* If the network configuration has changed, check if the interface is OK */
	if(rtOpts->restartSubsystems & PTPD_RESTART_NETWORK) {
		INFO("Network configuration changed - checking interface(s)\n");
		if(!testInterface(tmpOpts.primaryIfaceName, &tmpOpts)) {
			reloadSuccessful = FALSE;
			ERROR("Error: Cannot use %s interface\n",tmpOpts.primaryIfaceName);
		}
		if(rtOpts->backupIfaceEnabled && !testInterface(tmpOpts.backupIfaceName, &tmpOpts)) {
			rtOpts->restartSubsystems = -1;
			ERROR("Error: Cannot use %s interface as backup\n",tmpOpts.backupIfaceName);
		}
	}
#if (defined(linux) && defined(HAVE_SCHED_H)) || defined(HAVE_SYS_CPUSET_H) || defined(__QNXNTO__)
	/* Changing the CPU affinity mask */
	if(rtOpts->restartSubsystems & PTPD_CHANGE_CPUAFFINITY) {
		NOTIFY("Applying CPU binding configuration: changing selected CPU core\n");

		if(setCpuAffinity(tmpOpts.cpuNumber) < 0) {
			if(tmpOpts.cpuNumber == -1) {
				ERROR("Could not unbind from CPU core %d\n", rtOpts->cpuNumber);
			} else {
				ERROR("Could bind to CPU core %d\n", tmpOpts.cpuNumber);
			}
			reloadSuccessful = FALSE;
		} else {
			if(tmpOpts.cpuNumber > -1)
				INFO("Successfully bound "PTPD_PROGNAME" to CPU core %d\n", tmpOpts.cpuNumber);
			else
				INFO("Successfully unbound "PTPD_PROGNAME" from cpu core CPU core %d\n", rtOpts->cpuNumber);
		}
	}
#endif

	if(!reloadSuccessful) {
		ERROR("New configuration cannot be applied - aborting reload\n");
		rtOpts->restartSubsystems = 0;
		goto cleanup;
	}


		/**
		 * Commit changes to rtOpts and currentConfig
		 * (this should never fail as the config has already been checked if we're here)
		 * However if this DOES fail, some default has been specified out of range -
		 * this is the only situation where parse will succeed but commit not:
		 * disable quiet mode to show what went wrong, then die.
		 */
		if (rtOpts->currentConfig) {
			dictionary_del(&rtOpts->currentConfig);
		}
		if ( (rtOpts->currentConfig = parseConfig(CFGOP_PARSE_QUIET, NULL, rtOpts->candidateConfig,rtOpts)) == NULL) {
			CRITICAL("************ "PTPD_PROGNAME": parseConfig returned NULL during config commit"
				 "  - this is a BUG - report the following: \n");

			if ((rtOpts->currentConfig = parseConfig(CFGOP_PARSE, NULL, rtOpts->candidateConfig,rtOpts)) == NULL)
				CRITICAL("*****************" PTPD_PROGNAME" shutting down **********************\n");
			/*
			 * Could be assert(), but this should be done any time this happens regardless of
			 * compile options. Anyhow, if we're here, the daemon will no doubt segfault soon anyway
			 */
			abort();
		}

	/* clean up */
	cleanup:

		dictionary_del(&rtOpts->candidateConfig);
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


	/* if we don't have a config file specified, we're done - just reopen log files*/
	if(strlen(rtOpts->configFile) !=  0) {

		dictionary* tmpConfig = dictionary_new(0);

		/* Try reloading the config file */
		NOTIFY("Reloading configuration file: %s\n",rtOpts->configFile);

		if(!loadConfigFile(&tmpConfig, rtOpts)) {

			dictionary_del(&tmpConfig);

		} else {
			dictionary_merge(rtOpts->cliConfig, tmpConfig, 1, 1, "from command line");
			applyConfig(tmpConfig, rtOpts, ptpClock);
			dictionary_del(&tmpConfig);

		}

	}

	/* tell the service it can perform any HUP-triggered actions */
	ptpClock->timingService.reloadRequested = TRUE;

	if(rtOpts->recordLog.logEnabled ||
	    rtOpts->eventLog.logEnabled ||
	    (rtOpts->statisticsLog.logEnabled))
		INFO("Reopening log files\n");

	restartLogging(rtOpts);

	if(rtOpts->statisticsLog.logEnabled)
		ptpClock->resetStatisticsLog = TRUE;
}

void
restartSubsystems(RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	DBG("RestartSubsystems: %d\n",rtOpts->restartSubsystems);
	/* So far, PTP_INITIALIZING is required for both network and protocol restart */
	if((rtOpts->restartSubsystems & PTPD_RESTART_PROTOCOL) ||
	   (rtOpts->restartSubsystems & PTPD_RESTART_NETWORK)) {

		if(rtOpts->restartSubsystems & PTPD_RESTART_NETWORK) {
			NOTIFY("Applying network configuration: going into PTP_INITIALIZING\n");
		}

		/* These parameters have to be passed to ptpClock before re-init */
		ptpClock->defaultDS.clockQuality.clockClass = rtOpts->clockQuality.clockClass;
		ptpClock->defaultDS.slaveOnly = rtOpts->slaveOnly;
		ptpClock->disabled = rtOpts->portDisabled;

		if(rtOpts->restartSubsystems & PTPD_RESTART_PROTOCOL) {
			INFO("Applying protocol configuration: going into %s\n",
			     ptpClock->disabled ? "PTP_DISABLED" : "PTP_INITIALIZING");
		}

		/* Move back to primary interface only during configuration changes. */
		ptpClock->runningBackupInterface = FALSE;
		toState(ptpClock->disabled ? PTP_DISABLED : PTP_INITIALIZING, rtOpts, ptpClock);

	} else {
		/* Nothing happens here for now - SIGHUP handler does this anyway */
		if(rtOpts->restartSubsystems & PTPD_UPDATE_DATASETS) {
			NOTIFY("Applying PTP engine configuration: updating datasets\n");
			updateDatasets(ptpClock, rtOpts);
		}
	}
	/* Nothing happens here for now - SIGHUP handler does this anyway */
	if(rtOpts->restartSubsystems & PTPD_RESTART_LOGGING) {
		NOTIFY("Applying logging configuration: restarting logging\n");
	}


	if(rtOpts->restartSubsystems & PTPD_RESTART_ACLS) {
		NOTIFY("Applying access control list configuration\n");
		/* re-compile ACLs */
		netInitializeACLs(ptpClock->netPath, rtOpts);
	}

	if(rtOpts->restartSubsystems & PTPD_RESTART_ALARMS) {
		NOTIFY("Applying alarm configuration\n");
		configureAlarms(ptpClock->alarms, ALRM_MAX, (void*)ptpClock);
	}

#ifdef PTPD_STATISTICS
	/* Reinitialising the outlier filter containers */
	if(rtOpts->restartSubsystems & PTPD_RESTART_FILTERS) {

		NOTIFY("Applying filter configuration: re-initialising filters\n");

		freeDoubleMovingStatFilter(&ptpClock->filterMS);
		freeDoubleMovingStatFilter(&ptpClock->filterSM);

		ptpClock->oFilterMS.shutdown(&ptpClock->oFilterMS);
		ptpClock->oFilterSM.shutdown(&ptpClock->oFilterSM);

		outlierFilterSetup(&ptpClock->oFilterMS);
		outlierFilterSetup(&ptpClock->oFilterSM);

		ptpClock->oFilterMS.init(&ptpClock->oFilterMS,&rtOpts->oFilterMSConfig, "delayMS");
		ptpClock->oFilterSM.init(&ptpClock->oFilterSM,&rtOpts->oFilterSMConfig, "delaySM");


		if(rtOpts->filterMSOpts.enabled) {
			ptpClock->filterMS = createDoubleMovingStatFilter(&rtOpts->filterMSOpts,"delayMS");
		}

		if(rtOpts->filterSMOpts.enabled) {
			ptpClock->filterSM = createDoubleMovingStatFilter(&rtOpts->filterSMOpts, "delaySM");
		}

	}
#endif /* PTPD_STATISTICS */


	ptpClock->timingService.reloadRequested = TRUE;

#ifdef PTPD_FEATURE_NTP
	if(rtOpts->restartSubsystems & PTPD_RESTART_NTPENGINE && timingDomain.serviceCount > 1) {
		ptpClock->ntpControl.timingService.shutdown(&ptpClock->ntpControl.timingService);
	}

	if((rtOpts->restartSubsystems & PTPD_RESTART_NTPENGINE) ||
	   (rtOpts->restartSubsystems & PTPD_RESTART_NTPCONFIG)) {
		ntpSetup(rtOpts, ptpClock);
	}
	if((rtOpts->restartSubsystems & PTPD_RESTART_NTPENGINE) && rtOpts->ntpOptions.enableEngine) {
		timingServiceSetup(&ptpClock->ntpControl.timingService);
		ptpClock->ntpControl.timingService.init(&ptpClock->ntpControl.timingService);
	}

	//TODO: Check that disabling this with NTP doesn't break stuff.
	ptpClock->timingService.dataSet.priority1 = rtOpts->preferNTP;

	timingDomain.services[0]->holdTime = rtOpts->ntpOptions.failoverTimeout;

	if(timingDomain.services[0]->holdTimeLeft >
	   timingDomain.services[0]->holdTime) {
		timingDomain.services[0]->holdTimeLeft = rtOpts->ntpOptions.failoverTimeout;
	}
#else
	// When ntp is disabled, set priority to 0 (the value of preferNTP).
	ptpClock->timingService.dataSet.priority1 = 0;
#endif

	timingDomain.electionDelay = rtOpts->electionDelay;
	if(timingDomain.electionLeft > timingDomain.electionDelay) {
		timingDomain.electionLeft = timingDomain.electionDelay;
	}

	ptpClock->timingService.timeout = rtOpts->idleTimeout;

	/* Update PI servo parameters */
	setupPIservo(&ptpClock->servo, rtOpts);
	/* Config changes don't require subsystem restarts - acknowledge it */
	if(rtOpts->restartSubsystems == PTPD_RESTART_NONE) {
		NOTIFY("Applying configuration\n");
	}

	if(rtOpts->restartSubsystems != -1)
		rtOpts->restartSubsystems = 0;
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
		if(rtOpts->timingAclEnabled) {
			INFO("\n\n");
			INFO("** Timing message ACL:\n");
			dumpIpv4AccessList(netPathGetTimingACL(ptpClock->netPath));
		}
		if(rtOpts->managementAclEnabled) {
			INFO("\n\n");
			INFO("** Management message ACL:\n");
			dumpIpv4AccessList(netPathGetManagementACL(ptpClock->netPath));
		}
		if(rtOpts->clearCounters) {
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

#ifdef RUNTIME_DEBUG
/* These functions are useful to temporarily enable Debug around parts of code, similar to bash's "set -x" */
void enable_runtime_debug(void )
{
	extern RunTimeOpts rtOpts;

	rtOpts.debug_level = max(LOG_DEBUGV, rtOpts.debug_level);
}

void disable_runtime_debug(void )
{
	extern RunTimeOpts rtOpts;

	rtOpts.debug_level = LOG_INFO;
}
#endif

static int
writeLockFile(RunTimeOpts * rtOpts)
{
	int lockPid = 0;

	DBGV("Checking lock file: %s\n", rtOpts->lockFile);

	if ( (G_lockFilePointer=fopen(rtOpts->lockFile, "w+")) == NULL) {
		PERROR("Could not open lock file %s for writing", rtOpts->lockFile);
		return(0);
	}
	if (lockFile(fileno(G_lockFilePointer)) < 0) {
		if ( checkLockStatus(fileno(G_lockFilePointer),
				     DEFAULT_LOCKMODE, &lockPid) == 0) {
			ERROR("Another "PTPD_PROGNAME" instance is running: %s locked by PID %d\n",
			      rtOpts->lockFile, lockPid);
		} else {
			PERROR("Could not acquire lock on %s:", rtOpts->lockFile);
		}
		goto failure;
	}
	if(ftruncate(fileno(G_lockFilePointer), 0) == -1) {
		PERROR("Could not truncate %s: %s",
			rtOpts->lockFile, strerror(errno));
		goto failure;
	}
	if ( fprintf(G_lockFilePointer, "%ld\n", (long)getpid()) == -1) {
		PERROR("Could not write to lock file %s: %s",
			rtOpts->lockFile, strerror(errno));
		goto failure;
	}
	INFO("Successfully acquired lock on %s\n", rtOpts->lockFile);
	fflush(G_lockFilePointer);
	return(1);
	failure:
	fclose(G_lockFilePointer);
	return(0);
}

void
ptpdShutdown(PtpClock * ptpClock)
{
	extern RunTimeOpts rtOpts;

	/*
	 * go into DISABLED state so the FSM can call any PTP-specific shutdown actions,
	 * such as canceling unicast transmission
	 */
	toState(PTP_DISABLED, &rtOpts, ptpClock);
	/* process any outstanding events before exit */
	updateAlarms(ptpClock->alarms, ALRM_MAX);
	netShutdown(ptpClock->netPath);
	netPathFree(&ptpClock->netPath);
	free(ptpClock->foreign);

	/* free management and signaling messages, they can have dynamic memory allocated */
	if(ptpClock->msgTmpHeader.messageType == MANAGEMENT)
		freeManagementTLV(&ptpClock->msgTmp.manage);
	freeManagementTLV(&ptpClock->outgoingManageTmp);
	if(ptpClock->msgTmpHeader.messageType == SIGNALING)
		freeSignalingTLV(&ptpClock->msgTmp.signaling);
	freeSignalingTLV(&ptpClock->outgoingSignalingTmp);

#ifdef PTPD_SNMP
	snmpShutdown();
#endif /* PTPD_SNMP */

#ifndef PTPD_STATISTICS
	/* Not running statistics code - write observed drift to driftfile if enabled, inform user */
	if(ptpClock->defaultDS.slaveOnly && !ptpClock->servo.runningMaxOutput)
		saveDrift(ptpClock, &rtOpts, FALSE);
#else
	ptpClock->oFilterMS.shutdown(&ptpClock->oFilterMS);
	ptpClock->oFilterSM.shutdown(&ptpClock->oFilterSM);
	freeDoubleMovingStatFilter(&ptpClock->filterMS);
	freeDoubleMovingStatFilter(&ptpClock->filterSM);

	/* We are running statistics code - save drift on exit only if we're not monitoring servo stability */
	if(!rtOpts.servoStabilityDetection && !ptpClock->servo.runningMaxOutput)
		saveDrift(ptpClock, &rtOpts, FALSE);
#endif /* PTPD_STATISTICS */

	if (rtOpts.currentConfig != NULL)
		dictionary_del(&rtOpts.currentConfig);
	if(rtOpts.cliConfig != NULL)
		dictionary_del(&rtOpts.cliConfig);

	timerShutdown(ptpClock->timers);

	free(ptpClock);
	ptpClock = NULL;

	extern PtpClock* G_ptpClock;
	G_ptpClock = NULL;



	/* properly clean lockfile (eventough new deaemons can acquire the lock after we die) */
	if(!rtOpts.ignore_daemon_lock && G_lockFilePointer != NULL) {
		fclose(G_lockFilePointer);
		G_lockFilePointer = NULL;
	}
	unlink(rtOpts.lockFile);

	if(rtOpts.statusLog.logEnabled) {
		/* close and remove the status file */
		if(rtOpts.statusLog.logFP != NULL) {
			fclose(rtOpts.statusLog.logFP);
			rtOpts.statusLog.logFP = NULL;
		}
		unlink(rtOpts.statusLog.logPath);
	}

	stopLogging(&rtOpts);
}

PtpClock *
ptpClockCreate(RunTimeOpts* rtOpts, Integer16* ret) {
	PtpClock* ptpClock;
	/* Allocate memory after we're done with other checks but before going into daemon */
	ptpClock = (PtpClock *) calloc(1, sizeof(PtpClock));
	if (!ptpClock) {
		PERROR("Error: Failed to allocate memory for protocol engine data");
		*ret = 2;
		goto fail;
	}
	DBG("allocated %d bytes for protocol engine data\n", (int)sizeof(PtpClock));

	ptpClock->foreign = (ForeignMasterRecord *)
		calloc(rtOpts->max_foreign_records, sizeof(ForeignMasterRecord));
	if (!ptpClock->foreign) {
		PERROR("failed to allocate memory for foreign master data");
		*ret = 2;
	        goto fail;
	}
	DBG("allocated %d bytes for foreign master data\n",
	    (int)(rtOpts->max_foreign_records * sizeof(ForeignMasterRecord)));

	if (!(ptpClock->netPath = netPathCreate())) {
		PERROR("Error: Failed to allocate memory for protocol engine NetPath data");
		*ret = 2;
		goto fail;
	}
	DBG("allocated %d bytes for protocol engine NetPath data\n", (int)sizeof(NetPath));

	ptpClock->resetStatisticsLog = TRUE;

	/* Init to 0 net buffer */
	memset(ptpClock->msgIbuf, 0, PACKET_SIZE);
	memset(ptpClock->msgObuf, 0, PACKET_SIZE);

	/* Init outgoing management message */
	ptpClock->outgoingManageTmp.tlv = NULL;

	ptpClock->rtOpts = rtOpts;

#ifdef PTPD_STATISTICS
	outlierFilterSetup(&ptpClock->oFilterMS);
	outlierFilterSetup(&ptpClock->oFilterSM);

	ptpClock->oFilterMS.init(&ptpClock->oFilterMS, &rtOpts->oFilterMSConfig, "delayMS");
	ptpClock->oFilterSM.init(&ptpClock->oFilterSM, &rtOpts->oFilterSMConfig, "delaySM");


	if(rtOpts->filterMSOpts.enabled) {
		ptpClock->filterMS = createDoubleMovingStatFilter(&rtOpts->filterMSOpts, "delayMS");
	}

	if(rtOpts->filterSMOpts.enabled) {
		ptpClock->filterSM = createDoubleMovingStatFilter(&rtOpts->filterSMOpts, "delaySM");
	}

#endif

	/* set up timers */
	if(!timerSetup(ptpClock->timers)) {
		PERROR("failed to set up event timers");
		*ret = 2;
		goto fail;
	}

	/* init alarms */
	initAlarms(ptpClock->alarms, ALRM_MAX, (void*)ptpClock);
	configureAlarms(ptpClock->alarms, ALRM_MAX, (void*)ptpClock);
	ptpClock->alarmDelay = rtOpts->alarmInitialDelay;
	/* we're delaying alarm processing - disable alarms for now */
	if(ptpClock->alarmDelay) {
		enableAlarms(ptpClock->alarms, ALRM_MAX, FALSE);
	}

	netPathClearSockets(ptpClock->netPath);

#if defined PTPD_SNMP
	/* Start SNMP subsystem */
	if (rtOpts->snmpEnabled)
		snmpInit(rtOpts, ptpClock);
#endif

	return ptpClock;

 fail:
	if(ptpClock) {
		if(ptpClock->foreign)
			free(ptpClock->foreign);

		if(ptpClock->netPath)
			netPathFree(&ptpClock->netPath);

#ifdef PTPD_STATISTICS
		if(ptpClock->oFilterMS.shutdown != NULL)
			ptpClock->oFilterMS.shutdown(&ptpClock->oFilterMS);
		if(ptpClock->oFilterSM.shutdown != NULL)
			ptpClock->oFilterSM.shutdown(&ptpClock->oFilterSM);

		if(ptpClock->filterMS)
			freeDoubleMovingStatFilter(&ptpClock->filterMS);
		if(ptpClock->filterSM)
			freeDoubleMovingStatFilter(&ptpClock->filterSM);
#endif
		free(ptpClock);
	}
	return NULL;
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
	if(!rtOpts->nonDaemon){
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

	/* First lock check, just to be user-friendly to the operator */
	if(!rtOpts->ignore_daemon_lock) {
		if(!writeLockFile(rtOpts)){
			/* check and create Lock */
			ERROR("Error: file lock failed (use -L or global:ignore_lock to ignore lock file)\n");
			*ret = 3;
			return FALSE;
		}
		/* check for potential conflicts when automatic lock files are used */
		if(!checkOtherLocks(rtOpts)) {
			*ret = 3;
			return FALSE;
		}
	}

#if (defined(linux) && defined(HAVE_SCHED_H)) || defined(HAVE_SYS_CPUSET_H) || defined(__QNXNTO__)
	/* Try binding to a single CPU core if configured to do so */
	if(rtOpts->cpuNumber > -1) {
		if(setCpuAffinity(rtOpts->cpuNumber) < 0) {
			ERROR("Could not bind to CPU core %d\n", rtOpts->cpuNumber);
		} else {
			INFO("Successfully bound "PTPD_PROGNAME" to CPU core %d\n", rtOpts->cpuNumber);
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

#ifdef PTPD_FEATURE_NTP
void
ntpSetup (RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	TimingService *ts = &ptpClock->ntpControl.timingService;

	if (rtOpts->ntpOptions.enableEngine) {
		timingDomain.services[1] = ts;
		strncpy(ts->id, "NTP0", TIMINGSERVICE_MAX_DESC);
		ts->dataSet.priority1 = 0;
		ts->dataSet.type = TIMINGSERVICE_NTP;
		ts->config = &rtOpts->ntpOptions;
		ts->controller = &ptpClock->ntpControl;
		/* for now, NTP is considered always active, so will never go idle */
		ts->timeout = 60;
		ts->updateInterval = rtOpts->ntpOptions.checkInterval;
		timingDomain.serviceCount = 2;
	} else {
		timingDomain.serviceCount = 1;
		timingDomain.services[1] = NULL;
		if(timingDomain.best == ts || timingDomain.current == ts || timingDomain.preferred == ts) {
			timingDomain.best = timingDomain.current = timingDomain.preferred = NULL;
		}
	}
}
#endif

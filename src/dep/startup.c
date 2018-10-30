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

#include "constants.h"
#include "dep/constants_dep.h"
#include "ptp_primitives.h"
#include "timingdomain.h"
#include "ptp_datatypes.h"
#include "ptp_timers.h"
#include "dep/daemonconfig.h"
#if defined(PTPD_FEATURE_NTP)
#  include "dep/ntpengine/ntpdcontrol.h"
#endif
#include "dep/sys.h" // Used for lots of stuff
#ifdef PTPD_SNMP
#  include "dep/snmp.h"
#endif
#include "datatypes.h"
#include "dep/net.h"
#include "dep/startup.h"
#include "dep/servo.h"
#include "dep/msg.h" // For freeManagementTLV, freeSignalingTLV
#include "dep/alarms.h"
#include "protocol.h"
#include "display.h"
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
		netPathSetUsePrimaryIf(ptpClock->netPath, TRUE);
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
	ntpReset(rtOpts, ptpClock);
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

	extern PtpClock* G_ptpClock;
	G_ptpClock = NULL;

	clearLockFile(&rtOpts);

	stopLogging(&rtOpts);
}

PtpClock *
ptpClockCreate(const RunTimeOpts* rtOpts, Integer16* ret) {
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

	if (!(ptpClock->netPath = netPathCreate(rtOpts))) {
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

#ifdef PTPD_FEATURE_NTP
#endif

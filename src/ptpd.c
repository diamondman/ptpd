/*-
 * Copyright (c) 2012-2013 Wojciech Owczarek,
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
 * @file   ptpd.c
 * @date   Wed Jun 23 10:13:38 2010
 *
 * @brief  The main() function for the PTP daemon
 *
 * This file contains very little code, as should be obvious,
 * and only serves to tie together the rest of the daemon.
 * All of the default options are set here, but command line
 * arguments and configuration file is processed in the
 * ptpdStartup() routine called
 * below.
 */

#include <stddef.h>
#include <string.h>

#include "constants.h" // For USER_DESCRIPTION
#include "ptp_primitives.h"
#include "timingdomain.h"
#if defined PTPD_SNMP
#  include "dep/snmp.h"
#endif
#include "datatypes.h"
#include "dep/configdefaults.h"
#include "dep/startup.h" // For ptpdClockCreate
#include "dep/daemonconfig.h"
#if defined(PTPD_FEATURE_NTP)
#  include "dep/ntpengine/ntpdcontrol.h"
#endif
#include "dep/sys.h"
#include "protocol.h"
#include "ptpd_logging.h"

RunTimeOpts rtOpts;			/* statically allocated run-time
					 * configuration data */

Boolean startupInProgress;

/*
 * Global variable with the main PTP port. This is used to show the current state in DBG()/message()
 * without having to pass the pointer everytime.
 *
 * if ptpd is extended to handle multiple ports (eg, to instantiate a Boundary Clock),
 * then DBG()/message() needs a per-port pointer argument
 */
PtpClock *G_ptpClock = NULL;

TimingDomain timingDomain;

int
main(int argc, char **argv)
{
	PtpClock *ptpClock = NULL;
	Integer16 ret;
	TimingService *ts;

	startupInProgress = TRUE;


	/* Initialize run time options with command line arguments */
	if (!runTimeOptsInit(argc, argv, &ret, &rtOpts) ||
	    !sysPrePtpClockInit(&rtOpts, &ret) ||
	    !(ptpClock = ptpClockCreate(&rtOpts, &ret, NULL)) ||
	    !sysPostPtpClockInit(&rtOpts, ptpClock,  &ret)
	    ) {
		if (ret != 0 && !rtOpts.checkConfigOnly)
			ERROR(USER_DESCRIPTION" startup failed\n");
		return ret;
	}

	/* Manage log files: stats, log, status and quality file */
	initLogging(&rtOpts);
	restartLogging();

	NOTICE(USER_DESCRIPTION" started successfully on %s using \"%s\" preset (PID %d)\n",
	       netPathGetInterfaceName(ptpClock->netPath, &rtOpts),
	       (getPtpPreset(rtOpts.selectedPreset, &rtOpts)).presetName,
	       getpid());

	timingDomainSetup(&timingDomain);
	timingDomain.electionLeft = 10;
	timingDomain.electionDelay = rtOpts.electionDelay;
	timingDomain.updateInterval = 1;

	/* configure PTP TimeService */

	timingDomain.services[0] = &ptpClock->timingService;
	timingDomain.serviceCount = 1;

	ts = timingDomain.services[0];
	strncpy(ts->id, "PTP0", TIMINGSERVICE_MAX_DESC);
	ts->dataSet.type = TIMINGSERVICE_PTP;
	ts->config = &rtOpts;
	ts->controller = ptpClock;
	ts->timeout = rtOpts.idleTimeout;
	ts->updateInterval = 1;

#if defined(PTPD_FEATURE_NTP)
	// TODO: Check that removing ntp doesn't break something here.
	ts->dataSet.priority1 = rtOpts.preferNTP;
	ts->holdTime = rtOpts.ntpOptions.failoverTimeout;

	if (rtOpts.ntpOptions.enableEngine) {
		ntpSetup(&rtOpts, ptpClock);
	}
#else
	// When ntp is disabled, set priority to 0 (the value of preferNTP).
	ts->dataSet.priority1 = 0; // preferNTP would be 0.
#endif

	timingDomain.init(&timingDomain);

#if defined PTPD_SNMP
	/* Start SNMP subsystem */
	if (rtOpts.snmpEnabled)
		snmpInit(&rtOpts, ptpClock);
#endif

	startupInProgress = FALSE;

	/* global variable for message(), please see comment on top of this file */
	G_ptpClock = ptpClock;

	/* do the protocol engine */
	protocol(&rtOpts, ptpClock);
	/* forever loop.. */

	/* this also calls ptpd shutdown */
	timingDomain.shutdown(&timingDomain);

	NOTIFY("Self shutdown\n");

	return 1;
}

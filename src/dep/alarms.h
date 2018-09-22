#ifndef PTPDALARMS_H_
#define PTPDALARMS_H_

/*-
 * Copyright (c) 2015      Wojciech Owczarek,
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
 * @file    alarms.h
 * @authors Wojciech Owczarek
 * @date   Wed Dec 9 19:13:10 2015
 * Data type and function definitions related to
 * handling raising and clearing alarms.
 */

#include "ptp_primitives.h"
#include "ptp_datatypes.h"
#include "dep/alarm_datatypes.h"
#include "datatypes_stub.h"

void initAlarms(AlarmEntry* alarms, int count, void* userData); 			/* fill an array with initial alarm data */
void configureAlarms(AlarmEntry* alarms, int count, void* userData); 			/* fill an array with initial alarm data */
void enableAlarms(AlarmEntry* alarms, int count, Boolean enabled); 			/* enable/disable all alarms */
void setAlarmCondition(AlarmEntry *alarm, Boolean condition, PtpClock *ptpClock);	/* set alarm condition and capture data */
void updateAlarms(AlarmEntry *alarms, int count);					/* dispatch alarms: age, set, clear alarms etc. */
void displayAlarms(AlarmEntry *alarms, int count);					/* display a formatted alarm summary table */
int  getAlarmSummary(char * output, int size, AlarmEntry *alarms, int count);		/* produce a one-line alarm summary string */
void handleAlarm(AlarmEntry *alarms, void *userData);

#define SET_ALARM(alarm, val) \
	setAlarmCondition(&ptpClock->alarms[alarm], val, ptpClock)

#endif /*PTPDALARMS_H_*/

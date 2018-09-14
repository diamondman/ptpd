/**
 * @file   ptpd.h
 * @mainpage Ptpd v2 Documentation
 * @authors Martin Burnicki, Alexandre van Kempen, Steven Kreuzer,
 *          George Neville-Neil
 * @version 2.0
 * @date   Fri Aug 27 10:22:19 2010
 *
 * @section implementation Implementation
 * PTPdV2 is not a full implementation of 1588 - 2008 standard.
 * It is implemented only with use of Transparent Clock and Peer delay
 * mechanism, according to 802.1AS requierements.
 *
 * This header file includes all others headers.
 * It defines functions which are not dependant of the operating system.
 */

#ifndef PTPD_H_
#define PTPD_H_

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif /* HAVE_CONFIG_H */

/* Disable SO_TIMESTAMPING if configured to do so */
#ifdef PTPD_DISABLE_SOTIMESTAMPING
#  ifdef SO_TIMESTAMPING
#    undef SO_TIMESTAMPING
#  endif /* SO_TIMESTAMPING */
#endif /* PTPD_DISABLE_SOTIMESTAMPING */

#include <stdint.h>
#include <stddef.h>

#include "ptp_primitives.h"
#include "ptp_datatypes.h"
#include "dep/alarms.h"
#include "datatypes.h"


#define SET_ALARM(alarm, val) \
	setAlarmCondition(&ptpClock->alarms[alarm], val, ptpClock)

/** \name management.c
 * -Management message support*/
 /**\{*/
/* management.c */
/**
 * \brief Management message support
 */
void handleManagement(MsgHeader *header,
		 Boolean isFromSelf, Integer32 sourceAddress, RunTimeOpts *rtOpts, PtpClock *ptpClock);

/** \}*/

char *dump_TimeInternal(const TimeInternal * p);
char *dump_TimeInternal2(const char *st1, const TimeInternal * p1, const char *st2, const TimeInternal * p2);

int snprint_TimeInternal(char *s, int max_len, const TimeInternal * p);

/* alarms.c - this will be moved */
void capturePtpEventData(PtpEventData *data, PtpClock *ptpClock, RunTimeOpts *rtOpts); 	/* capture data from an alarm event */
void setAlarmCondition(AlarmEntry *alarm, Boolean condition, PtpClock *ptpClock); /* set alarm condition and capture data */

#endif /*PTPD_H_*/

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
#include "ptp_timers.h"
#include "dep/datatypes_dep.h"
#include "dep/alarms.h"
#include "datatypes.h"


#define SET_ALARM(alarm, val) \
	setAlarmCondition(&ptpClock->alarms[alarm], val, ptpClock)

/** \}*/

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

/** \name signaling.c
 * -Signaling message support*/
 /**\{*/
/* signaling.c */
/**
 * \brief Signaling message support
 */
UnicastGrantTable* findUnicastGrants(const PortIdentity* portIdentity, Integer32 TransportAddress, UnicastGrantTable *grantTable, UnicastGrantIndex *index, int nodeCount, Boolean update);
void 	initUnicastGrantTable(UnicastGrantTable *grantTable, Enumeration8 delayMechanism, int nodeCount, UnicastDestination *destinations, const RunTimeOpts *rtOpts, PtpClock *ptpClock);

void 	cancelUnicastTransmission(UnicastGrantData*, const RunTimeOpts*, PtpClock*);
void 	cancelAllGrants(UnicastGrantTable *grantTable, int nodeCount, const RunTimeOpts *rtOpts, PtpClock *ptpClock);

void 	handleSignaling(MsgHeader*, Boolean, Integer32, const RunTimeOpts*,PtpClock*);

void 	refreshUnicastGrants(UnicastGrantTable *grantTable, int nodeCount, const RunTimeOpts *rtOpts, PtpClock *ptpClock);
void 	updateUnicastGrantTable(UnicastGrantTable *grantTable, int nodeCount, const RunTimeOpts *rtOpts);

/*
 * \brief Packing and Unpacking macros
 */
#define DECLARE_PACK( type ) void pack##type( void*, void* );

DECLARE_PACK( NibbleUpper )
DECLARE_PACK( Enumeration4Lower )
DECLARE_PACK( UInteger4Lower )
DECLARE_PACK( UInteger4Upper )
DECLARE_PACK( UInteger16 )
DECLARE_PACK( UInteger8 )
DECLARE_PACK( Octet )
DECLARE_PACK( Integer8 )
DECLARE_PACK( UInteger48 )
DECLARE_PACK( Integer64 )

#define DECLARE_UNPACK( type ) void unpack##type( void*, void*, PtpClock *ptpClock );

DECLARE_UNPACK( Boolean )
DECLARE_UNPACK( Enumeration4Lower )
DECLARE_UNPACK( Enumeration4Upper )
DECLARE_UNPACK( Octet )
DECLARE_UNPACK( UInteger48 )
DECLARE_UNPACK( Integer64 )

char *dump_TimeInternal(const TimeInternal * p);
char *dump_TimeInternal2(const char *st1, const TimeInternal * p1, const char *st2, const TimeInternal * p2);

int snprint_TimeInternal(char *s, int max_len, const TimeInternal * p);

/* alarms.c - this will be moved */
void capturePtpEventData(PtpEventData *data, PtpClock *ptpClock, RunTimeOpts *rtOpts); 	/* capture data from an alarm event */
void setAlarmCondition(AlarmEntry *alarm, Boolean condition, PtpClock *ptpClock); /* set alarm condition and capture data */

#endif /*PTPD_H_*/

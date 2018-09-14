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

/** \name bmc.c
 * -Best Master Clock Algorithm functions*/
 /**\{*/
/* bmc.c */
/**
 * \brief Compare data set of foreign masters and local data set
 * \return The recommended state for the port
 */

UInteger8 bmc(ForeignMasterRecord*, const RunTimeOpts*,PtpClock*);

/* compare two portIdentTitties */
int cmpPortIdentity(const PortIdentity *a, const PortIdentity *b);
/* check if portIdentity is all zero */
Boolean portIdentityEmpty(PortIdentity *portIdentity);
/* check if portIdentity is all ones */
Boolean portIdentityAllOnes(PortIdentity *portIdentity);

/**
 * \brief When recommended state is Master, copy local data into parent and grandmaster dataset
 */
void m1(const RunTimeOpts *, PtpClock*);

/**
 * \brief When recommended state is Slave, copy dataset of master into parent and grandmaster dataset
 */
void s1(MsgHeader*,MsgAnnounce*,PtpClock*, const RunTimeOpts *);


void p1(PtpClock *ptpClock, const RunTimeOpts *rtOpts);

/**
 * \brief Initialize datas
 */
void initData(RunTimeOpts*,PtpClock*);
/** \}*/


/** \name protocol.c
 * -Execute the protocol engine*/
 /**\{*/
/**
 * \brief Protocol engine
 */
/* protocol.c */
void protocol(RunTimeOpts*,PtpClock*);
void updateDatasets(PtpClock* ptpClock, const RunTimeOpts* rtOpts);
void setPortState(PtpClock *ptpClock, Enumeration8 state);

Boolean acceptPortIdentity(PortIdentity thisPort, PortIdentity targetPort);

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

/* display.c */
void displayRunTimeOpts(const RunTimeOpts*);
void displayDefault (const PtpClock*);
void displayCurrent (const PtpClock*);
void displayParent (const PtpClock*);
void displayGlobal (const PtpClock*);
void displayPort (const PtpClock*);
void displayForeignMaster (const PtpClock*);
void displayOthers (const PtpClock*);
void displayBuffer (const PtpClock*);
void displayPtpClock (const PtpClock*);
void timeInternal_display(const TimeInternal*);
void clockIdentity_display(const ClockIdentity);
void netPath_display(const NetPath*);
void intervalTimer_display(const IntervalTimer*);
void integer64_display (const Integer64*);
void timeInterval_display(const TimeInterval*);
void portIdentity_display(const PortIdentity*);
void clockQuality_display (const ClockQuality*);
void PTPText_display(const PTPText*, const PtpClock*);
void iFaceName_display(const Octet*);
void unicast_display(const Octet*);
const char *portState_getName(Enumeration8 portState);
const char *getMessageTypeName(Enumeration8 messageType);
const char* accToString(uint8_t acc);
const char* delayMechToString(uint8_t mech);
void timestamp_display(const Timestamp * timestamp);

void displayCounters(const PtpClock*);
void displayStatistics(const PtpClock*);
void clearCounters(PtpClock *);

void msgHeader_display(const MsgHeader*);
void msgAnnounce_display(const MsgAnnounce*);
void msgSync_display(const MsgSync *sync);
void msgFollowUp_display(const MsgFollowUp*);
void msgPdelayReq_display(const MsgPdelayReq*);
void msgDelayReq_display(const MsgDelayReq * req);
void msgDelayResp_display(const MsgDelayResp * resp);
void msgPdelayResp_display(const MsgPdelayResp * presp);
void msgPdelayRespFollowUp_display(const MsgPdelayRespFollowUp * prespfollow);
void msgManagement_display(const MsgManagement * manage);
void msgSignaling_display(const MsgSignaling * signaling);

void mMSlaveOnly_display(const MMSlaveOnly*, const PtpClock*);
void mMClockDescription_display(const MMClockDescription*, const PtpClock*);
void mMUserDescription_display(const MMUserDescription*, const PtpClock*);
void mMInitialize_display(const MMInitialize*, const PtpClock*);
void mMDefaultDataSet_display(const MMDefaultDataSet*, const PtpClock*);
void mMCurrentDataSet_display(const MMCurrentDataSet*, const PtpClock*);
void mMParentDataSet_display(const MMParentDataSet*, const PtpClock*);
void mMTimePropertiesDataSet_display(const MMTimePropertiesDataSet*, const PtpClock*);
void mMPortDataSet_display(const MMPortDataSet*, const PtpClock*);
void mMPriority1_display(const MMPriority1*, const PtpClock*);
void mMPriority2_display(const MMPriority2*, const PtpClock*);
void mMDomain_display(const MMDomain*, const PtpClock*);
void mMLogAnnounceInterval_display(const MMLogAnnounceInterval*, const PtpClock*);
void mMAnnounceReceiptTimeout_display(const MMAnnounceReceiptTimeout*, const PtpClock*);
void mMLogSyncInterval_display(const MMLogSyncInterval*, const PtpClock*);
void mMVersionNumber_display(const MMVersionNumber*, const PtpClock*);
void mMTime_display(const MMTime*, const PtpClock*);
void mMClockAccuracy_display(const MMClockAccuracy*, const PtpClock*);
void mMUtcProperties_display(const MMUtcProperties*, const PtpClock*);
void mMTraceabilityProperties_display(const MMTraceabilityProperties*, const PtpClock*);
void mMTimescaleProperties_display(const MMTimescaleProperties*, const PtpClock*);
void mMUnicastNegotiationEnable_display(const MMUnicastNegotiationEnable*, const PtpClock*);
void mMDelayMechanism_display(const MMDelayMechanism*, const PtpClock*);
void mMLogMinPdelayReqInterval_display(const MMLogMinPdelayReqInterval*, const PtpClock*);
void mMErrorStatus_display(const MMErrorStatus*, const PtpClock*);

void sMRequestUnicastTransmission_display(const SMRequestUnicastTransmission*, const PtpClock*);
void sMGrantUnicastTransmission_display(const SMGrantUnicastTransmission*, const PtpClock*);
void sMCancelUnicastTransmission_display(const SMCancelUnicastTransmission*, const PtpClock*);
void sMAcknowledgeCancelUnicastTransmission_display(const SMAcknowledgeCancelUnicastTransmission*, const PtpClock*);

char *dump_TimeInternal(const TimeInternal * p);
char *dump_TimeInternal2(const char *st1, const TimeInternal * p1, const char *st2, const TimeInternal * p2);
const char * getTimeSourceName(Enumeration8 timeSource);

int snprint_TimeInternal(char *s, int max_len, const TimeInternal * p);

void toState(UInteger8,const RunTimeOpts*,PtpClock*);

Boolean respectUtcOffset(const RunTimeOpts * rtOpts, PtpClock * ptpClock);

/* alarms.c - this will be moved */
void capturePtpEventData(PtpEventData *data, PtpClock *ptpClock, RunTimeOpts *rtOpts); 	/* capture data from an alarm event */
void setAlarmCondition(AlarmEntry *alarm, Boolean condition, PtpClock *ptpClock); /* set alarm condition and capture data */

#endif /*PTPD_H_*/

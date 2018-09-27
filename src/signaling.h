#pragma once
#ifndef SIGNALING_H_
#define SIGNALING_H_

#include "ptp_primitives.h"
#include "ptp_datatypes.h"
#include "datatypes_stub.h"

typedef struct UnicastGrantTable UnicastGrantTable;

typedef struct UnicastGrantData {
	UInteger32      duration;		/* grant duration */
	Boolean		requestable;		/* is this mesage type even requestable? */
	Boolean		requested;		/* slave: we have requested this */
	Boolean		canceled;		/* this has been canceled (awaiting ack) */
	UInteger8	cancelCount;		/* how many times we sent the cancel message while waiting for ack */
	Integer8	logInterval;		/* interval we granted or got granted */
	Integer8	logMinInterval;		/* minimum interval we're going to request */
	Integer8	logMaxInterval;		/* maximum interval we're going to request */
	UInteger16	sentSeqId;		/* used by masters: last sent sequence id */
	UInteger32	intervalCounter;	/* used as a modulo counter to allow different message rates for different slaves */
	Boolean		expired;		/* TRUE -> grant has expired */
	Boolean         granted;		/* master: we have granted this, slave: we have been granted this */
	UInteger32      timeLeft;		/* countdown timer for aging out grants */
	UInteger16      messageType;		/* message type this grant is for */
	UnicastGrantTable *parent;		/* parent entry (that has transportAddress and portIdentity */
	Boolean		receiving;		/* keepalive: used to detect if message of this type is being received */
} UnicastGrantData;

struct UnicastGrantTable {
	Integer32		transportAddress;	/* IP address of slave (or master) */
	UInteger8		domainNumber;		/* domain of the master - as used by Telecom Profile */
	UInteger8		localPreference;		/* local preference - as used by Telecom profile */
	PortIdentity    	portIdentity;		/* master: port ID of grantee, slave: portID of grantor */
	UnicastGrantData	grantData[PTP_MAX_MESSAGE_INDEXED];/* master: grantee's grants, slave: grantor's grant status */
	UInteger32		timeLeft;		/* time until expiry of last grant (max[grants.timeLeft]. when runs out and no renewal, entry can be re-used */
	Boolean			isPeer;			/* this entry is peer only */
	TimeInternal		lastSyncTimestamp;		/* last Sync message timestamp sent */
};

/* Unicast index holder: data + port mask */
typedef struct UnicastGrantIndex {
	UnicastGrantTable* data[UNICAST_MAX_DESTINATIONS];
	UInteger16 portMask;
} UnicastGrantIndex;

/* Unicast destination configuration: Address, domain, preference, last Sync timestamp sent */
typedef struct UnicastDestination {
	Integer32		transportAddress;		/* destination address */
	UInteger8		domainNumber;			/* domain number - for slaves with masters in multiple domains */
	UInteger8		localPreference;		/* local preference to influence BMC */
	TimeInternal		lastSyncTimestamp;		/* last Sync timestamp sent */
} UnicastDestination;


UnicastGrantTable* findUnicastGrants
(
 const PortIdentity* portIdentity,
 Integer32 TransportAddress,
 UnicastGrantTable* grantTable,
 UnicastGrantIndex* index,
 int nodeCount,
 Boolean update
 );

void initUnicastGrantTable
(
 UnicastGrantTable* grantTable,
 Enumeration8 delayMechanism,
 int nodeCount,
 UnicastDestination* destinations,
 const RunTimeOpts* rtOpts,
 PtpClock* ptpClock
 );

void updateUnicastGrantTable(UnicastGrantTable* grantTable, int nodeCount, const RunTimeOpts *rtOpts);
void cancelUnicastTransmission(UnicastGrantData*, const RunTimeOpts*, PtpClock*);
void cancelAllGrants(UnicastGrantTable* grantTable, int nodeCount, const RunTimeOpts* rtOpts, PtpClock* ptpClock);
void handleSignaling(MsgHeader*, Boolean, Integer32, const RunTimeOpts*,PtpClock*);
void refreshUnicastGrants(UnicastGrantTable* grantTable, int nodeCount, const RunTimeOpts* rtOpts, PtpClock* ptpClock);

#endif /* include guard */

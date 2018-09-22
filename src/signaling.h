#include "ptp_primitives.h"
#include "ptp_datatypes.h"
#include "datatypes_stub.h"

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

#include "ptp_primitives.h"
#include "ptp_datatypes.h"
#include "datatypes_stub.h" // Just for PtpClock


void msgUnpackHeader(Octet * buf,MsgHeader*);
void msgUnpackSync(Octet * buf,MsgSync*);
void msgUnpackAnnounce (Octet * buf,MsgAnnounce*);
void msgUnpackFollowUp(Octet * buf,MsgFollowUp*);
void msgUnpackDelayReq(Octet * buf, MsgDelayReq * delayreq);
void msgUnpackDelayResp(Octet * buf,MsgDelayResp *);
void msgUnpackPdelayReq(Octet * buf,MsgPdelayReq*);
void msgUnpackPdelayResp(Octet * buf,MsgPdelayResp*);
void msgUnpackPdelayRespFollowUp(Octet * buf,MsgPdelayRespFollowUp*);
void msgPackManagementTLV(Octet *,MsgManagement*, PtpClock*);
Boolean msgUnpackManagement(Octet * buf,MsgManagement*, MsgHeader*, PtpClock *ptpClock, const int tlvOffset);
Boolean msgUnpackSignaling(Octet * buf,MsgSignaling*, MsgHeader*, PtpClock *ptpClock, const int tlvOffset);
void msgPackManagementErrorStatusTLV(Octet *,MsgManagement*,PtpClock*);
void msgPackSignalingTLV(Octet *,MsgSignaling*, PtpClock*);

void msgPackHeader(Octet * buf,PtpClock*);
#ifndef PTPD_SLAVE_ONLY
void msgPackSync(Octet * buf, UInteger16, Timestamp*, PtpClock*);
void msgPackAnnounce(Octet * buf, UInteger16, Timestamp*, PtpClock*);
#endif /* PTPD_SLAVE_ONLY */
void msgPackFollowUp(Octet * buf,Timestamp*,PtpClock*, const UInteger16);
void msgPackDelayReq(Octet * buf,Timestamp *,PtpClock *);
void msgPackDelayResp(Octet * buf,MsgHeader *,Timestamp *,PtpClock *);
void msgPackPdelayReq(Octet * buf,Timestamp*,PtpClock*);
void msgPackPdelayResp(Octet * buf,MsgHeader*,Timestamp*,PtpClock*);
void msgPackPdelayRespFollowUp(Octet * buf,MsgHeader*,Timestamp*,PtpClock*, const UInteger16);
void msgPackManagement(Octet * buf,MsgManagement*,PtpClock*);
void msgPackSignaling(Octet * buf,MsgSignaling*,PtpClock*);


void msgDebugFollowUp(MsgFollowUp *follow);
void msgDebugDelayReq(MsgDelayReq *req);
void msgDebugDelayResp(MsgDelayResp *resp);


void msgDump(PtpClock *ptpClock);
void copyClockIdentity( ClockIdentity dest, ClockIdentity src);
void copyPortIdentity( PortIdentity * dest, PortIdentity * src);

void freeSignalingTLV(MsgSignaling*);
void freeManagementTLV(MsgManagement*);


int unpackMMClockDescription( Octet* buf, int, MsgManagement*, PtpClock* );
UInteger16 packMMClockDescription( MsgManagement*, Octet*);
void freeMMClockDescription( MMClockDescription*);
int unpackMMUserDescription( Octet* buf, int, MsgManagement*, PtpClock* );
UInteger16 packMMUserDescription( MsgManagement*, Octet*);
void freeMMUserDescription( MMUserDescription*);
int unpackMMErrorStatus( Octet* buf, int, MsgManagement*, PtpClock* );
UInteger16 packMMErrorStatus( MsgManagement*, Octet*);
void freeMMErrorStatus( MMErrorStatus*);
int unpackMMInitialize( Octet* buf, int, MsgManagement*, PtpClock* );
UInteger16 packMMInitialize( MsgManagement*, Octet*);
int unpackMMDefaultDataSet( Octet* buf, int, MsgManagement*, PtpClock* );
UInteger16 packMMDefaultDataSet( MsgManagement*, Octet*);
int unpackMMCurrentDataSet( Octet* buf, int, MsgManagement*, PtpClock* );
UInteger16 packMMCurrentDataSet( MsgManagement*, Octet*);
int unpackMMParentDataSet( Octet* buf, int, MsgManagement*, PtpClock* );
UInteger16 packMMParentDataSet( MsgManagement*, Octet*);
int unpackMMTimePropertiesDataSet( Octet* buf, int, MsgManagement*, PtpClock* );
UInteger16 packMMTimePropertiesDataSet( MsgManagement*, Octet*);
int unpackMMPortDataSet( Octet* buf, int, MsgManagement*, PtpClock* );
UInteger16 packMMPortDataSet( MsgManagement*, Octet*);
int unpackMMPriority1( Octet* buf, int, MsgManagement*, PtpClock* );
UInteger16 packMMPriority1( MsgManagement*, Octet*);
int unpackMMPriority2( Octet* buf, int, MsgManagement*, PtpClock* );
UInteger16 packMMPriority2( MsgManagement*, Octet*);
int unpackMMDomain( Octet* buf, int, MsgManagement*, PtpClock* );
UInteger16 packMMDomain( MsgManagement*, Octet*);
int unpackMMSlaveOnly( Octet* buf, int, MsgManagement*, PtpClock* );
UInteger16 packMMSlaveOnly( MsgManagement*, Octet* );
int unpackMMLogAnnounceInterval( Octet* buf, int, MsgManagement*, PtpClock* );
UInteger16 packMMLogAnnounceInterval( MsgManagement*, Octet*);
int unpackMMAnnounceReceiptTimeout( Octet* buf, int, MsgManagement*, PtpClock* );
UInteger16 packMMAnnounceReceiptTimeout( MsgManagement*, Octet*);
int unpackMMLogSyncInterval( Octet* buf, int, MsgManagement*, PtpClock* );
UInteger16 packMMLogSyncInterval( MsgManagement*, Octet*);
int unpackMMVersionNumber( Octet* buf, int, MsgManagement*, PtpClock* );
UInteger16 packMMVersionNumber( MsgManagement*, Octet*);
int unpackMMTime( Octet* buf, int, MsgManagement*, PtpClock * );
UInteger16 packMMTime( MsgManagement*, Octet*);
int unpackMMClockAccuracy( Octet* buf, int, MsgManagement*, PtpClock* );
UInteger16 packMMClockAccuracy( MsgManagement*, Octet*);
int unpackMMUtcProperties( Octet* buf, int, MsgManagement*, PtpClock* );
UInteger16 packMMUtcProperties( MsgManagement*, Octet*);
int unpackMMTraceabilityProperties( Octet* buf, int, MsgManagement*, PtpClock* );
UInteger16 packMMTraceabilityProperties( MsgManagement*, Octet*);
int unpackMMTimescaleProperties( Octet* buf, int, MsgManagement*, PtpClock* );
UInteger16 packMMTimescaleProperties( MsgManagement*, Octet*);
int unpackMMUnicastNegotiationEnable( Octet* buf, int, MsgManagement*, PtpClock* );
UInteger16 packMMUnicastNegotiationEnable( MsgManagement*, Octet*);
int unpackMMDelayMechanism( Octet* buf, int, MsgManagement*, PtpClock* );
UInteger16 packMMDelayMechanism( MsgManagement*, Octet*);
int unpackMMLogMinPdelayReqInterval( Octet* buf, int, MsgManagement*, PtpClock* );
UInteger16 packMMLogMinPdelayReqInterval( MsgManagement*, Octet*);

/* Signaling TLV packing / unpacking functions */
void unpackSMRequestUnicastTransmission( Octet* buf, MsgSignaling*, PtpClock* );
UInteger16 packSMRequestUnicastTransmission( MsgSignaling*, Octet*);
void unpackSMGrantUnicastTransmission( Octet* buf, MsgSignaling*, PtpClock* );
UInteger16 packSMGrantUnicastTransmission( MsgSignaling*, Octet*);
void unpackSMCancelUnicastTransmission( Octet* buf, MsgSignaling*, PtpClock* );
UInteger16 packSMCancelUnicastTransmission( MsgSignaling*, Octet*);
void unpackSMAcknowledgeCancelUnicastTransmission( Octet* buf, MsgSignaling*, PtpClock* );
UInteger16 packSMAcknowledgeCancelUnicastTransmission( MsgSignaling*, Octet*);

void unpackPortAddress( Octet* buf, PortAddress*, PtpClock*);
void packPortAddress( PortAddress*, Octet*);
void freePortAddress( PortAddress*);
void unpackPTPText( Octet* buf, PTPText*, PtpClock*);
void packPTPText( PTPText*, Octet*);
void freePTPText( PTPText*);
void unpackPhysicalAddress( Octet* buf, PhysicalAddress*, PtpClock*);
void packPhysicalAddress( PhysicalAddress*, Octet*);
void freePhysicalAddress( PhysicalAddress*);
void unpackClockIdentity( Octet* buf, ClockIdentity *c, PtpClock*);
void packClockIdentity( ClockIdentity *c, Octet* buf);
void freeClockIdentity( ClockIdentity *c);
void unpackClockQuality( Octet* buf, ClockQuality *c, PtpClock*);
void packClockQuality( ClockQuality *c, Octet* buf);
void freeClockQuality( ClockQuality *c);
void unpackTimeInterval( Octet* buf, TimeInterval *t, PtpClock*);
void packTimeInterval( TimeInterval *t, Octet* buf);
void freeTimeInterval( TimeInterval *t);
void unpackPortIdentity( Octet* buf, PortIdentity *p, PtpClock*);
void packPortIdentity( PortIdentity *p, Octet*  buf);
void freePortIdentity( PortIdentity *p);
void unpackTimestamp( Octet* buf, Timestamp *t, PtpClock*);
void packTimestamp( Timestamp *t, Octet* buf);
void freeTimestamp( Timestamp *t);
UInteger16 msgPackManagementResponse(Octet * buf,MsgHeader*,MsgManagement*,PtpClock*);

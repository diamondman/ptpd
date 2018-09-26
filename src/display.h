#include "ptp_primitives.h"
#include "ptp_datatypes.h"
#include "ptp_timers.h"
#include "datatypes_stub.h"
#include "dep/net.h"

void integer64_display (const Integer64*);
void uInteger48_display(const UInteger48*);
void timeInternal_display(const TimeInternal*);
void timestamp_display(const Timestamp* timestamp);
void clockIdentity_display(const ClockIdentity);
void clockUUID_display(const Octet*);
void netPath_display(const NetPath*);
void intervalTimer_display(const IntervalTimer*);
void timeInterval_display(const TimeInterval*);
void portIdentity_display(const PortIdentity*);
void clockQuality_display (const ClockQuality*);
void PTPText_display(const PTPText*, const PtpClock*);
void iFaceName_display(const Octet*);
void unicast_display(const Octet*);

void msgSync_display(const MsgSync *sync);
void msgHeader_display(const MsgHeader*);
void msgAnnounce_display(const MsgAnnounce*);
void msgFollowUp_display(const MsgFollowUp*);
void msgDelayReq_display(const MsgDelayReq* req);
void msgDelayResp_display(const MsgDelayResp* resp);
void msgPdelayReq_display(const MsgPdelayReq*);
void msgPdelayResp_display(const MsgPdelayResp* presp);
void msgPdelayRespFollowUp_display(const MsgPdelayRespFollowUp* prespfollow);
void msgManagement_display(const MsgManagement* manage);
void msgSignaling_display(const MsgSignaling* signaling);

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

void displayRunTimeOpts(const RunTimeOpts*);
void displayDefault(const PtpClock*);
void displayCurrent(const PtpClock*);
void displayParent(const PtpClock*);
void displayGlobal(const PtpClock*);
void displayPort(const PtpClock*);
void displayForeignMaster(const PtpClock*);
void displayOthers(const PtpClock*);
void displayBuffer(const PtpClock*);
void displayCounters(const PtpClock*);

const char* portState_getName(Enumeration8 portState);
const char* getTimeSourceName(Enumeration8 timeSource);
const char* getMessageTypeName(Enumeration8 messageType);
const char* accToString(uint8_t acc);
const char* delayMechToString(uint8_t mech);

void displayStatistics(const PtpClock*);
void displayPtpClock(const PtpClock*);

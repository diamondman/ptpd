#include "ptp_primitives.h"
#include "ptp_datatypes.h"
#include "datatypes.h"

void protocol(RunTimeOpts*, PtpClock*);
void setPortState(PtpClock* ptpClock, Enumeration8 state);
void toState(UInteger8, const RunTimeOpts*, PtpClock*);
Boolean acceptPortIdentity(PortIdentity thisPort, PortIdentity targetPort);
void updateDatasets(PtpClock* ptpClock, const RunTimeOpts* rtOpts);
void clearCounters(PtpClock*);
Boolean respectUtcOffset(const RunTimeOpts* rtOpts, PtpClock* ptpClock);

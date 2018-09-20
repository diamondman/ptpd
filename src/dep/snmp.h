#ifndef SNMP_H_
#define SNMP_H_

#include "dep/alarm_datatypes.h"
#include "datatypes.h"

void snmpInit(RunTimeOpts *, PtpClock *);
void snmpShutdown();
void alarmHandler_snmp(AlarmEntry *alarm);

#endif /* include guard */

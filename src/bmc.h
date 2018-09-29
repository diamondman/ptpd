#pragma once
#ifndef BMC_H_
#define BMC_H_

#include "ptp_primitives.h"
#include "ptp_datatypes.h"
#include "datatypes_stub.h"

void initData(RunTimeOpts*, PtpClock*);
int cmpPortIdentity(const PortIdentity* a, const PortIdentity* b);
Boolean portIdentityEmpty(PortIdentity* portIdentity);
Boolean portIdentityAllOnes(PortIdentity* portIdentity);

/**
 * \brief When recommended state is Master, copy local data into parent and grandmaster dataset
 */
void m1(const RunTimeOpts*, PtpClock*);

void p1(PtpClock* ptpClock, const RunTimeOpts* rtOpts);

/**
 * \brief When recommended state is Slave, copy dataset of master into parent and grandmaster dataset
 */
void s1(MsgHeader*, MsgAnnounce*, PtpClock*, const RunTimeOpts*);

UInteger8 bmc(ForeignMasterRecord*, const RunTimeOpts*, PtpClock*);

#endif /* include guard */

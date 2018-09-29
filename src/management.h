#pragma once
#ifndef MANAGEMENT_H_
#define MANAGEMENT_H_

#include "ptp_primitives.h"
#include "ptp_datatypes.h"
#include "datatypes_stub.h"

void handleManagement(MsgHeader *header,
                      Boolean isFromSelf, Integer32 sourceAddress, RunTimeOpts *rtOpts, PtpClock *ptpClock);

#endif /* include guard */

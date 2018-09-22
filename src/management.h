#pragma once

#include "ptp_primitives.h"
#include "ptp_datatypes.h"
#include "datatypes_stub.h"

void handleManagement(MsgHeader *header,
                      Boolean isFromSelf, Integer32 sourceAddress, RunTimeOpts *rtOpts, PtpClock *ptpClock);

#pragma once
#ifndef ARITH_H_
#define ARITH_H_

/** \name arith.c
 * -Timing management and arithmetic*/
 /**\{*/
/* arith.c */

#include <sys/time.h>

#include "ptp_primitives.h"
#include "ptp_datatypes.h"

/**
 * \brief Convert TimeInternal structure to Integer64
 */
void internalTime_to_integer64(TimeInternal, Integer64*);
/**
 * \brief Convert Integer64 into TimeInternal structure
 */
void integer64_to_internalTime(Integer64, TimeInternal*);
/**
 * \brief Convert TimeInternal into Timestamp structure (defined by the spec)
 */
void fromInternalTime(const TimeInternal*, Timestamp*);

/**
 * \brief Convert Timestamp to TimeInternal structure (defined by the spec)
 */
void toInternalTime(TimeInternal*, const Timestamp*);

void ts_to_InternalTime(const struct timespec *, TimeInternal *);
void tv_to_InternalTime(const struct timeval  *, TimeInternal *);


/**
 * \brief Use to normalize a TimeInternal structure
 *
 * The nanosecondsField member must always be less than 10⁹
 * This function is used after adding or subtracting TimeInternal
 */
void normalizeTime(TimeInternal*);

/**
 * \brief Add two InternalTime structure and normalize
 */
void addTime(TimeInternal*, const TimeInternal*, const TimeInternal*);

/**
 * \brief Substract two InternalTime structure and normalize
 */
void subTime(TimeInternal*, const TimeInternal*, const TimeInternal*);
/** \}*/

/**
 * \brief Divied an InternalTime by 2
 */
void div2Time(TimeInternal *);

void clearTime(TimeInternal *time);

void nano_to_Time(TimeInternal *time, int nano);
int gtTime(const TimeInternal *x, const TimeInternal *b);
void absTime(TimeInternal *time);
int is_Time_close(const TimeInternal *x, const TimeInternal *b, int nanos);

int check_timestamp_is_fresh2(const TimeInternal * timeA, const TimeInternal * timeB);
int check_timestamp_is_fresh(const TimeInternal * timeA);

int isTimeInternalNegative(const TimeInternal * p);

/* helper functions for leap second handling */
double secondsToMidnight(void);
double getPauseAfterMidnight(Integer8 announceInterval, int pausePeriod);

double timeInternalToDouble(const TimeInternal * p);
TimeInternal doubleToTimeInternal(const double d);

uint32_t fnvHash(void *input, size_t len, int modulo);

#endif /* include guard */

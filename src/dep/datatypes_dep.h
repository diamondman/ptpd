/**
 * \file
 * \brief Implementation specific datatype
 */

#ifndef DATATYPES_DEP_H_
#define DATATYPES_DEP_H_

#include <stdio.h> // For FILE
#include <limits.h> // For PATH_MAX

#include "ptp_primitives.h"

typedef struct LogFileHandler {

	char* logID;
	char* openMode;
	char logPath[PATH_MAX+1];
	FILE* logFP;

	Boolean logEnabled;
	Boolean truncateOnReopen;
	Boolean unlinkOnClose;

	uint32_t lastHash;
	UInteger32 maxSize;
	UInteger32 fileSize;
	int maxFiles;

} LogFileHandler;

typedef struct LeapSecondInfo {
    int currentOffset;
    int nextOffset;
    int leapType;
    Integer32 startTime;
    Integer32 endTime;
    Boolean valid;
    Boolean offsetValid;
} LeapSecondInfo;

#endif /*DATATYPES_DEP_H_*/

/**
 * \file
 * \brief Implementation specific datatype
 */

#ifndef DATATYPES_DEP_H_
#define DATATYPES_DEP_H_

#include <stdio.h> // For FILE
#include <limits.h> // For PATH_MAX

#include "ptp_primitives.h"

typedef struct LogFileConfig {
	Boolean logInitiallyEnabled;
	char* logID;
	char* openMode;
	char logPath[PATH_MAX+1];

	Boolean truncateOnReopen;
	Boolean unlinkOnClose;
	int maxFiles;
	UInteger32 maxSize;
} LogFileConfig;

#endif /*DATATYPES_DEP_H_*/

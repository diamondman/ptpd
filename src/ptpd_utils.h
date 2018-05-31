/**
 * @file   ptpd_utils.h
 *
 * @brief  Declare common math/helper macros.
 *
 */

#ifndef PTPD_UTILS_H_
#define PTPD_UTILS_H_

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "ptpd_logging.h"

//TODO: Forward Declare ptpdShutdown or something.

/* NOTE: this macro can be refactored into a function */
#define XMALLOC(ptr,size) \
	if(!((ptr)=malloc(size))) { \
		PERROR("failed to allocate memory"); \
		ptpdShutdown(ptpClock); \
		exit(1); \
	}

#define SAFE_FREE(pointer) \
	if(pointer != NULL) { \
		free(pointer); \
		pointer = NULL; \
	}

#define IS_SET(data, bitpos) \
	((data & ( 0x1 << bitpos )) == (0x1 << bitpos))

#define SET_FIELD(data, bitpos) \
	data << bitpos

/** \name Bit array manipulations*/
 /**\{*/

#define getFlag(x,y)  !!( *(uint8_t*)((x)+((y)<8?1:0)) &   (1<<((y)<8?(y):(y)-8)) )
#define setFlag(x,y)    ( *(uint8_t*)((x)+((y)<8?1:0)) |=   1<<((y)<8?(y):(y)-8)  )
#define clearFlag(x,y)  ( *(uint8_t*)((x)+((y)<8?1:0)) &= ~(1<<((y)<8?(y):(y)-8)) )
/** \}*/


#ifndef min
#  define min(a,b)     (((a)<(b))?(a):(b))
#endif /* min */

#ifndef max
#  define max(a,b)     (((a)>(b))?(a):(b))
#endif /* max */


/* quick shortcut to defining a temporary char array for the purpose of snprintf to it */
#define tmpsnprintf(var,len, ...) \
	char var[len+1]; \
	memset(var, 0, len+1); \
	snprintf(var, len, __VA_ARGS__);


/** \name Endian corrections*/
 /**\{*/

#if defined(HAVE_ARPA_INET_H) && Y == 1
#  include <arpa/inet.h>
#  define flip16(x) htons(x)
#  define flip32(x) htonl(x)
#else
/* i don't know any target platforms that do not have htons and htonl,
   but here are generic funtions just in case */
#  if BYTE_ORDER == LITTLE_ENDIAN || defined(_LITTLE_ENDIAN) ||\
      (defined(__BYTE_ORDER) && __BYTE_ORDER == __LITTLE_ENDIAN)
static inline int16_t flip16(Integer16 x)
{
   return (((x) >> 8) & 0x00ff) | (((x) << 8) & 0xff00);
}

static inline int32_t flip32(x)
{
  return (((x) >> 24) & 0x000000ff) | (((x) >> 8 ) & 0x0000ff00) |
         (((x) << 8 ) & 0x00ff0000) | (((x) << 24) & 0xff000000);
}
#  elif BYTE_ORDER == BIG_ENDIAN || defined(_BIG_ENDIAN) ||\
      (defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN)
#    define flip16(x) (x)
#    define flip32(x) (x)
#  else
#    error "htons/htonl not defined, and processor endianess not determinable!"
#  endif
#endif

/** \}*/

#endif /*PTPD_UTILS_H_*/

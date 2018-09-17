/**
 * \file
 * \brief Implementation specific datatype
 */

#ifndef DATATYPES_DEP_H_
#define DATATYPES_DEP_H_

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <limits.h>
#include <sys/param.h>
#include <sys/socket.h> //Auto included by netinet/in.h
#include <netinet/in.h>

#if defined(HAVE_NET_IF_ARP_H)
// Required by OpenBSD for netinet/if_ethers.h
#  include <net/if_arp.h>
#endif

#if defined(HAVE_NET_IF_H)
#  include <net/if.h>
#endif

#if defined(HAVE_NETINET_IF_ETHER_H)
// Many BSD systems have this file, but OpenBSD defines ether_Addr here.
#  include <netinet/if_ether.h>
#endif

#ifdef PTPD_PCAP
#  ifdef HAVE_PCAP_PCAP_H
#    include <pcap/pcap.h>
#  else
/* Cases like RHEL5 and others where only pcap.h exists */
#    ifdef HAVE_PCAP_H
#      include <pcap.h>
#    endif /* HAVE_PCAP_H */
#  endif
#endif

#if defined(HAVE_NET_ETHERNET_H)
#  include <net/ethernet.h>
#endif

#if !defined(ETHER_ADDR_LEN) && defined(ETHERADDRL)
// Solaris doesn't have ETHER_ADDR_LEN defined in net/ethernet.h
#  define ETHER_ADDR_LEN ETHERADDRL
#endif

#include "ptp_primitives.h"
#include "dep/ipv4_acl.h" // Only used for opaque type Ipv4AccessList

/**
 * \brief Struct used to average the offset from master
 *
 * The FIR filtering of the offset from master input is a simple, two-sample average
 */
typedef struct {
    Integer32  nsec_prev, y;
} offset_from_master_filter;

/**
* \brief Struct used to average the one way delay
*
* It is a variable cutoff/delay low-pass, infinite impulse response (IIR) filter.
*
*  The one-way delay filter has the difference equation: s*y[n] - (s-1)*y[n-1] = x[n]/2 + x[n-1]/2, where increasing the stiffness (s) lowers the cutoff and increases the delay.
 */
typedef struct {
    Integer32  nsec_prev, y;
    Integer32  s_exp;
} one_way_delay_filter;

/**
 * \brief Struct containing interface information and capabilities
 */
typedef struct {
        struct sockaddr afAddress;
        unsigned char hwAddress[14];
        Boolean hasHwAddress;
        Boolean hasAfAddress;
        int addressFamily;
        unsigned int flags;
	int ifIndex;
} InterfaceInfo;

/**
 * \brief Struct describing network transport data
 */
typedef struct {
	Integer32 eventSock, generalSock;
	Integer32 multicastAddr, peerMulticastAddr;

	/* Interface address and capability descriptor */
	InterfaceInfo interfaceInfo;

	/* used by IGMP refresh */
	struct in_addr interfaceAddr;
	/* Typically MAC address - outer 6 octers of ClockIdendity */
	Octet interfaceID[ETHER_ADDR_LEN];
	/* source address of last received packet - used for unicast replies to Delay Requests */
	Integer32 lastSourceAddr;
	/* destination address of last received packet - used for unicast FollowUp for multiple slaves*/
	Integer32 lastDestAddr;

	uint64_t sentPackets;
	uint64_t receivedPackets;

	uint64_t sentPacketsTotal;
	uint64_t receivedPacketsTotal;

#ifdef PTPD_PCAP
	pcap_t *pcapEvent;
	pcap_t *pcapGeneral;
	Integer32 pcapEventSock;
	Integer32 pcapGeneralSock;
#endif
	Integer32 headerOffset;

	/* used for tracking the last TTL set */
	int ttlGeneral;
	int ttlEvent;
	Boolean joinedPeer;
	Boolean joinedGeneral;
	struct ether_addr etherDest;
	struct ether_addr peerEtherDest;
	Boolean txTimestampFailure;

	Ipv4AccessList* timingAcl;
	Ipv4AccessList* managementAcl;

} NetPath;

typedef struct {

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

typedef struct{

    UInteger8 minValue;
    UInteger8 maxValue;
    UInteger8 defaultValue;

} UInteger8_option;

typedef struct{

    Integer32  minValue;
    Integer32  maxValue;
    Integer32  defaultValue;

} Integer32_option;

typedef struct{

    UInteger32  minValue;
    UInteger32  maxValue;
    UInteger32  defaultValue;

} UInteger32_option;

typedef struct{

    Integer16  minValue;
    Integer16  maxValue;
    Integer16  defaultValue;

} Integer16_option;

typedef struct{

    UInteger16  minValue;
    UInteger16  maxValue;
    UInteger16  defaultValue;

} UInteger16_option;

typedef union { uint32_t *uintval; int32_t *intval; double *doubleval; Boolean *boolval; char *strval; } ConfigPointer;
typedef union { uint32_t uintval; int32_t intval; double doubleval; Boolean boolval; char *strval; } ConfigSetting;

typedef struct ConfigOption ConfigOption;

struct ConfigOption {
    char *key;
    enum { CO_STRING, CO_INT, CO_UINT, CO_DOUBLE, CO_BOOL, CO_SELECT } type;
    enum { CO_MIN, CO_MAX, CO_RANGE, CO_STRLEN } restriction;
    ConfigPointer target;
    ConfigPointer defvalue;
    ConfigSetting constraint1;
    ConfigSetting constraint2;
    int restartFlags;
    ConfigOption *next;
};

typedef struct {
    int currentOffset;
    int nextOffset;
    int leapType;
    Integer32 startTime;
    Integer32 endTime;
    Boolean valid;
    Boolean offsetValid;
} LeapSecondInfo;

#endif /*DATATYPES_DEP_H_*/

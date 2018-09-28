#ifndef NET_H_
#define NET_H_

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif /* HAVE_CONFIG_H */

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

#if defined(HAVE_NET_ETHERNET_H)
#  include <net/ethernet.h>
#endif

#if !defined(ETHER_ADDR_LEN) && defined(ETHERADDRL)
#  define ETHER_ADDR_LEN ETHERADDRL // sun related
#endif /* ETHER_ADDR_LEN && ETHERADDRL */

#ifdef PTPD_PCAP
#  if defined(HAVE_PCAP_PCAP_H)
#    include <pcap/pcap.h>
#  elif defined(HAVE_PCAP_H)
#    include <pcap.h> /* Cases like RHEL5 and others where only pcap.h exists */
#  endif
#endif

#include "ptp_primitives.h"
#include "ptp_datatypes.h"
#include "dep/ipv4_acl.h" // Only used for opaque type Ipv4AccessList
#include "datatypes_stub.h"

#define IFACE_NAME_LENGTH         IF_NAMESIZE
#define NET_ADDRESS_LENGTH        INET_ADDRSTRLEN

/**
 * \brief Struct containing interface information and capabilities
 */
typedef struct InterfaceInfo {
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
typedef struct NetPath {
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

Boolean netShutdown(NetPath*);
Boolean testInterface(char* ifaceName, const RunTimeOpts* rtOpts);
Boolean hostLookup(const char* hostname, Integer32* addr);
Boolean netInit(NetPath*,RunTimeOpts*,PtpClock*);
int netSelect(TimeInternal*,NetPath*,fd_set*);
ssize_t netRecvEvent(Octet*,TimeInternal*,NetPath*,int);
ssize_t netRecvGeneral(Octet*,NetPath*);
ssize_t netSendEvent(Octet*,UInteger16,NetPath*,const RunTimeOpts*,Integer32,TimeInternal*);
ssize_t netSendGeneral(Octet*,UInteger16,NetPath*,const RunTimeOpts*,Integer32 );
ssize_t netSendPeerGeneral(Octet*,UInteger16,NetPath*,const RunTimeOpts*, Integer32);
ssize_t netSendPeerEvent(Octet*,UInteger16,NetPath*,const RunTimeOpts*,Integer32,TimeInternal*);
Boolean netRefreshIGMP(NetPath *, const RunTimeOpts *, PtpClock *);

#endif

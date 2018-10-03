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

#include "ptp_primitives.h"
#include "ptp_datatypes.h"
#include "dep/ipv4_acl.h" // Only used for opaque type Ipv4AccessList
#include "datatypes_stub.h"

#define IFACE_NAME_LENGTH         IF_NAMESIZE
#define NET_ADDRESS_LENGTH        INET_ADDRSTRLEN

typedef struct NetPath NetPath;

Boolean netShutdown(NetPath*);
Boolean testInterface(char* ifaceName, const RunTimeOpts* rtOpts);
Boolean hostLookup(const char* hostname, Integer32* addr);
Boolean netInit(NetPath*,RunTimeOpts*,PtpClock*);
void netInitializeACLs(NetPath*, const RunTimeOpts*);
int netSelect(TimeInternal*,NetPath*,fd_set*);
ssize_t netRecvEvent(Octet*,TimeInternal*,NetPath*,int,Boolean*);
ssize_t netRecvGeneral(Octet*,NetPath*,Boolean*);
ssize_t netSendEvent(Octet*,UInteger16,NetPath*,const RunTimeOpts*,Integer32,TimeInternal*);
ssize_t netSendGeneral(Octet*,UInteger16,NetPath*,const RunTimeOpts*,Integer32 );
ssize_t netSendPeerGeneral(Octet*,UInteger16,NetPath*,const RunTimeOpts*, Integer32);
ssize_t netSendPeerEvent(Octet*,UInteger16,NetPath*,const RunTimeOpts*,Integer32,TimeInternal*);
Boolean netRefreshIGMP(NetPath *, const RunTimeOpts *, PtpClock *);

struct ether_addr netPathGetMacAddress(const NetPath*);
struct in_addr netPathGetInterfaceAddr(const NetPath*);
int netPathGetInterfaceIndex(const NetPath*);
Integer32 netPathGetLastSourceAddress(const NetPath*);
Integer32 netPathGetLastDestAddress(const NetPath*);
Ipv4AccessList* netPathGetManagementACL(const NetPath*);
Ipv4AccessList* netPathGetTimingACL(const NetPath*);
Boolean netPathCheckTxTsValid(const NetPath*);

uint64_t netPathGetSentPacketCount(const NetPath*);
void netPathResetSentPacketCount(NetPath*);

uint64_t netPathGetReceivedPacketCount(const NetPath*);
void netPathResetReceivedPacketCount(NetPath*);
void netPathIncReceivedPacketCount(NetPath*);

uint64_t netPathGetTotalSentPacketCount(const NetPath*);
uint64_t netPathGetTotalReceivedPacketsCount(const NetPath*);

void netPathClearSockets(NetPath*);

Boolean netPathEventSocketIsSet(const NetPath*, fd_set*);
Boolean netPathGeneralSocketIsSet(const NetPath*, fd_set*);

void netPathFree(NetPath**);
NetPath* netPathCreate();

#endif

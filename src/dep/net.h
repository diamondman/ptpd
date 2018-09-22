#ifndef NET_H_
#define NET_H_

#include "ptp_primitives.h"
#include "ptp_datatypes.h"
#include "dep/datatypes_dep.h"
#include "datatypes_stub.h"

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

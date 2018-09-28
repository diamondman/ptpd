/**
 * @file   ipv4_acl.h
 *
 * @brief  definitions related to IPv4 access control list handling
 *
 */

#ifndef PTPD_IPV4_ACL_H_
#define PTPD_IPV4_ACL_H_

#include <stdint.h>

enum {
	ACL_PERMIT_DENY,
	ACL_DENY_PERMIT
};

typedef struct AclEntry AclEntry;
typedef struct MaskTable MaskTable;
typedef struct Ipv4AccessList Ipv4AccessList;

/* Parse string into AclEntry array */
int maskParser(const char* input, AclEntry* output);
/* Destroy an Ipv4AccessList structure */
void freeIpv4AccessList(Ipv4AccessList** acl);
/* Initialise an Ipv4AccessList structure */
Ipv4AccessList* createIpv4AccessList(const char* permitList, const char* denyList, int processingOrder);
/* Match on an IP address */
int matchIpv4AccessList(Ipv4AccessList* acl, const uint32_t addr);
/* Display the contents and hit counters of an access list */
void dumpIpv4AccessList(Ipv4AccessList* acl);
/* Clear counters */
void clearIpv4AccessListCounters(Ipv4AccessList* acl);

#endif /* PTPD_IPV4_ACL_H_ */

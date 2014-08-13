#ifndef PINGPING_COMMON_H_
#define PINGPING_COMMON_H_
#include <infiniband/verbs.h>
#include <rdma/rdma_cma.h>
#include <rdma/rdma_verbs.h>
#include <infiniband/arch.h>

#include <netdb.h>
#include <arpa/inet.h>

#include <cstdio>
#include <stdexcept>
#include <cstdlib>
#include <stdint.h>
#include <map>
#include "Arguments.h"

#define RESOLVE_TIMEOUT_MS	5000
#define MAXBUFFERSIZE	(1024*1024*256) // 1GByte

struct pdata {
	uint64_t buf_va;
	uint32_t buf_rkey;
};

std::map<uint8_t, const char*> active_speed = { { 1, "2.5 Gbps" }, { 2,
		"5.0 Gbps" }, { 4, "10.0 Gbps" }, { 8, "10.0 Gbps" },
		{ 16, "14.0 Gbps" }, { 32, "25.0 Gbps" } };

std::map<int, const char*> mtu = { { 1, "IBV_MTU_256" }, { 2, "IBV_MTU_512" }, {
		3, "IBV_MTU_1024" }, { 4, "IBV_MTU_2048" }, { 5, "IBV_MTU_4096" } };
std::map<uint8_t, const char*> link_layer = { { IBV_LINK_LAYER_UNSPECIFIED,
		"IBV_LINK_LAYER_UNSPECIFIED" }, { IBV_LINK_LAYER_INFINIBAND,
		"IBV_LINK_LAYER_INFINIBAND" }, { IBV_LINK_LAYER_ETHERNET,
		"IBV_LINK_LAYER_ETHERNET" } };
void query_device(Arguments* args);

#endif /* PINGPING_COMMON_H_ */

#ifndef PINGPING_COMMON_H_
#define PINGPING_COMMON_H_
#include <infiniband/verbs.h>
#include <rdma/rdma_cma.h>
#include <infiniband/arch.h>

#include <netdb.h>
#include <arpa/inet.h>

#include <cstdio>
#include <stdexcept>
#include <cstdlib>
#include <stdint.h>

#define RESOLVE_TIMEOUT_MS	5000
#define MAXBUFFERSIZE	1024*1024*1024 // 1G

struct pdata {
	uint64_t buf_va;
	uint32_t buf_rkey;
};

void query_device();

#endif /* PINGPING_COMMON_H_ */

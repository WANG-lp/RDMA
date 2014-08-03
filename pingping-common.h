#ifndef PINGPING_COMMON_H_
#define PINGPING_COMMON_H_
#include <infiniband/verbs.h>
#include <rdma/rdma_cma.h>
#include <infiniband/arch.h>

#include <arpa/inet.h>

#include <cstdio>
#include <stdexcept>
#include <cstdlib>
#include <stdint.h>

#define MAXBUFFERSIZE	1024*1024*1024 // 1G

void query_device();

#endif /* PINGPING_COMMON_H_ */

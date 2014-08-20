/*
 *
 * Usage: ./pingpong-client [-s 0/1 ] [-t num_threads] server-ip port MaxPacketSize(MByte)
 *
 */

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <sys/time.h>
#include <thread>

#include <infiniband/verbs.h>

#include "Arguments.h"
#include "pingpong-common.h"

using std::cin;
using std::cout;
using std::cerr;
using std::endl;

pdata server_pdata[MAXTHREADS];

rdma_event_channel *cm_channel;
rdma_cm_id *cm_id[MAXTHREADS];
rdma_cm_event *event;
rdma_conn_param conn_param[MAXTHREADS] = { };

ibv_pd *pd[MAXTHREADS];
ibv_comp_channel *comp_chan[MAXTHREADS];
ibv_cq *cq[MAXTHREADS];
ibv_cq *evt_cq[MAXTHREADS];
ibv_mr *mr[MAXTHREADS];
ibv_qp_init_attr qp_attr[MAXTHREADS] = { };
ibv_sge sge[MAXTHREADS];
ibv_send_wr send_wr[MAXTHREADS] = { };
ibv_send_wr *bad_send_wr[MAXTHREADS];
ibv_recv_wr recv_wr[MAXTHREADS] = { };
ibv_recv_wr *bad_recv_wr[MAXTHREADS];
ibv_wc wc[MAXTHREADS];
void *cq_context[MAXTHREADS];

addrinfo *hints, *res;
uint32_t *buffer[MAXTHREADS];

void work(int thread_id, Arguments *args) {

	struct timeval t_start;
	struct timeval t_end;

	//prepare to receive from client ---- pong function
	for (int i = 1; i <= args->max_packet_size; i++) {
		sge[thread_id].addr = (uintptr_t) (buffer[thread_id]);
		sge[thread_id].length = sizeof(uint32_t) * args->size * i; // i MByte
		sge[thread_id].length /= 4;
		sge[thread_id].lkey = mr[thread_id]->lkey;

		recv_wr[thread_id].sg_list = &sge[thread_id];
		recv_wr[thread_id].num_sge = 1;

		if (ibv_post_recv(cm_id[thread_id]->qp, &recv_wr[thread_id],
				&bad_recv_wr[thread_id])) {
			throw std::runtime_error("ibv_post_recv failed!");
		}
	}

	memset(buffer[thread_id], 0xef, sizeof(buffer[thread_id]));

	int id = 0;

	for (int i = 1; i <= args->max_packet_size; i++) {

		gettimeofday(&t_start, NULL);

		sge[thread_id].addr = (uintptr_t) (buffer[thread_id]);
		sge[thread_id].length = sizeof(uint32_t) * args->size * i; // i MByte
		sge[thread_id].length /= 4;
		sge[thread_id].lkey = mr[thread_id]->lkey;

		//send_wr.wr_id = i;
		send_wr[thread_id].opcode = IBV_WR_SEND;
		send_wr[thread_id].send_flags = IBV_SEND_SIGNALED;
		send_wr[thread_id].sg_list = &sge[thread_id];
		send_wr[thread_id].num_sge = 1;

		if (ibv_post_send(cm_id[thread_id]->qp, &send_wr[thread_id],
				&bad_send_wr[thread_id])) {
			throw std::runtime_error("ibv_post_send failed!");
		}

		if (ibv_get_cq_event(comp_chan[thread_id], &evt_cq[thread_id],
				&cq_context[thread_id])) {
			throw std::runtime_error("ibv_get_cq_event failed!");
		}

		if (ibv_req_notify_cq(cq[thread_id], 0)) {
			throw std::runtime_error("ibv_req_notify_cq failed!");
		}

		if (ibv_poll_cq(cq[thread_id], 1, &wc[thread_id]) < 1) {
			throw std::runtime_error("ibv_poll_cq failed!");
		}

		if (wc[thread_id].status != IBV_WC_SUCCESS) {
			throw std::runtime_error("IBV_WC_SUCCESS 2 failed!");
		}

//			//receive from server ---- pong function

		if (ibv_get_cq_event(comp_chan[thread_id], &evt_cq[thread_id],
				&cq_context[thread_id])) {
			throw std::runtime_error("ibv_get_cq_event failed!");
		}

		if (ibv_req_notify_cq(cq[thread_id], 0)) {
			throw std::runtime_error("ibv_req_notify_cq failed!");
		}

		if (ibv_poll_cq(cq[thread_id], 1, &wc[thread_id]) < 1) {
			throw std::runtime_error("ibv_poll_cq failed!");
		}

		if (wc[thread_id].status != IBV_WC_SUCCESS) {
			throw std::runtime_error("IBV_WC_SUCCESS failed!");
		}

		gettimeofday(&t_end, NULL);
		float time_ms = ((t_end.tv_sec - t_start.tv_sec) * 1000 * 1000
				+ t_end.tv_usec - t_start.tv_usec) / 1000.0;

		float speed = (i * 8 * 2) / (time_ms / 1000);
		if (args->size_mode == 0) {
			speed /= 1000;
		} else if (args->size_mode == 1) {
		}
		printf("%d\t%d\t%d\t        %f\t%f\n", thread_id, id, i, time_ms,
				speed);
		id++;
	}
}

int main(int argc, char* argv[]) {

	hints = (addrinfo *) malloc(sizeof(addrinfo));
	hints->ai_family = AF_INET;
	hints->ai_socktype = SOCK_STREAM;

	rdma_cm_id *cm_id_t;

	try {
		Arguments* args = new Arguments(argc, argv);

		std::thread work_threads[args->num_threads];

		query_device(args);

		cm_channel = rdma_create_event_channel();
		if (!cm_channel) {
			throw std::runtime_error("rdma_create_event_channel failed!");
		}

		printf("Thread:\tId:\tSize(%s):\tTime(ms):\tSpeed(Mbits/s):\n",
				args->size_str.c_str());
		for (int i = 0; i < args->num_threads; i++) {
			buffer[i] = (uint32_t *) malloc(sizeof(uint32_t) * MAXBUFFERSIZE);
			if (!buffer[i]) {
				throw std::runtime_error("malloc buffer failed!");
			}
			if (rdma_create_id(cm_channel, &cm_id_t, NULL, RDMA_PS_TCP)) {
				throw std::runtime_error("rdma_create_id failed!");
			}

			if (getaddrinfo(args->hostip, args->port, hints, &res)) {
				throw std::runtime_error("rdma_create_id failed!");
			}
			int err = 0;
			for (addrinfo* t = res; t; t = t->ai_next) {
				err = rdma_resolve_addr(cm_id_t, NULL, t->ai_addr,
				RESOLVE_TIMEOUT_MS);
				if (!err)
					break;
			}
			if (err) {
				throw std::runtime_error("rdma_resolve_addr failed!");
			}

			if (rdma_get_cm_event(cm_channel, &event)) {
				throw std::runtime_error("rdma_get_cm_event failed!");
			}
			if (event->event != RDMA_CM_EVENT_ADDR_RESOLVED) {
				throw std::runtime_error("RDMA_CM_EVENT_ADDR_RESOLVED failed!");
			}
			rdma_ack_cm_event(event);

			if (rdma_resolve_route(cm_id_t, RESOLVE_TIMEOUT_MS)) {
				throw std::runtime_error("rdma_resolve_route failed!");
			}
			if (rdma_get_cm_event(cm_channel, &event)) {
				throw std::runtime_error("rdma_get_cm_event failed!");
			}
			if (event->event != RDMA_CM_EVENT_ROUTE_RESOLVED) {
				throw std::runtime_error(
						"RDMA_CM_EVENT_ROUTE_RESOLVED failed!");
			}
			rdma_ack_cm_event(event);
			cm_id[i] = cm_id_t;
			pd[i] = ibv_alloc_pd(cm_id[i]->verbs);
			if (!pd[i]) {
				throw std::runtime_error("ibv_alloc_pd failed!");
			}

			comp_chan[i] = ibv_create_comp_channel(cm_id[i]->verbs);
			if (!comp_chan[i]) {
				throw std::runtime_error("ibv_create_comp_channel failed!");
			}
			cq[i] = ibv_create_cq(cm_id[i]->verbs, 2, NULL, comp_chan[i], 0);
			if (!cq[i]) {
				throw std::runtime_error("ibv_create_cq failed!");
			}
			if (ibv_req_notify_cq(cq[i], 0)) {
				throw std::runtime_error("ibv_req_notify_cq failed!");
			}

			mr[i] = ibv_reg_mr(pd[i], buffer[i],
			MAXBUFFERSIZE * sizeof(uint32_t), IBV_ACCESS_LOCAL_WRITE);
			if (!mr[i]) {
				throw std::runtime_error("ibv_reg_mr failed!");
			}

			qp_attr[i].cap.max_send_wr = args->max_qp_wr;
			qp_attr[i].cap.max_send_sge = 1;
			qp_attr[i].cap.max_recv_wr = args->max_qp_wr;
			qp_attr[i].cap.max_recv_sge = 1;

			qp_attr[i].send_cq = cq[i];
			qp_attr[i].recv_cq = cq[i];

			qp_attr[i].qp_type = IBV_QPT_RC;

			if (rdma_create_qp(cm_id[i], pd[i], &qp_attr[i])) {
				throw std::runtime_error("rdma_create_qp failed!");
			}

			conn_param[i].initiator_depth = 1;
			conn_param[i].retry_count = 7;

			if (rdma_connect(cm_id[i], &conn_param[i])) {
				throw std::runtime_error("rdma_connect failed!");
			}

			if (rdma_get_cm_event(cm_channel, &event)) {
				throw std::runtime_error("rdma_get_cm_event failed!");
			}
			if (event->event != RDMA_CM_EVENT_ESTABLISHED) {
				throw std::runtime_error("RDMA_CM_EVENT_ESTABLISHED failed!");
			}

			memcpy(&server_pdata[i], event->param.conn.private_data,
					sizeof server_pdata[i]);
			rdma_ack_cm_event(event);
			work_threads[i] = std::thread(work, i, args);
		}
		for (int i = 0; i < args->num_threads; i++) {
			work_threads[i].join();
			free(buffer[i]);
			rdma_dereg_mr(mr[i]);
			ibv_destroy_comp_channel(comp_chan[i]);
			ibv_destroy_cq(cq[i]);

		}

		rdma_destroy_event_channel(cm_channel);
	} catch (std::exception &e) {
		cerr << "Exception: " << e.what() << endl;
	}

	return 0;
}


/*
 *
 * Usage: ./pingpong-server [-s 0/1 ] ip port MaxPacketSize(MByte)
 *
 */

#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <stdexcept>
#include <thread>

#include "Arguments.h"
#include "pingpong-common.h"

using std::cin;
using std::cout;
using std::cerr;
using std::endl;

pdata rep_pdata[MAXTHREADS];

rdma_event_channel *cm_channel;
rdma_cm_id *listen_id;
rdma_cm_event *event;
rdma_cm_id *cm_id[MAXTHREADS];
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

sockaddr_in sin;

uint32_t *buffer[MAXTHREADS];

void work(int thread_id, Arguments *args) {

	int id = 0;
	printf("Thread:\tId:\tSize(%s):\n", args->size_str.c_str());
	for (int i = 1; i <= args->max_packet_size; i++) {
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

		//cerr << "Receive: " << (int) ntohl(buffer[1]) << endl;
		printf("%d\t%d\t%d\n", thread_id, id, i);
		id++;

		//sent to client ---- pong function
		//buffer[0] = htonl(ntohl(buffer[1]));
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

		/* Wait for send completion */

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
		ibv_ack_cq_events(cq[thread_id], 2);
	}
}

int main(int argc, char* argv[]) {

	int thread_id = 0;

	try {
		Arguments* args = new Arguments(argc, argv);

		std::thread work_threads[args->num_threads];

		query_device(args);

		cm_channel = rdma_create_event_channel();
		if (!cm_channel) {
			throw std::runtime_error("rdma_create_event_channel failed!");
		}

		if (rdma_create_id(cm_channel, &listen_id, NULL, RDMA_PS_TCP)) {
			throw std::runtime_error("rdma_create_id failed!");
		}

		sin.sin_family = AF_INET;
		sin.sin_port = htons(atoi(args->port));
		sin.sin_addr.s_addr = INADDR_ANY;

		if (rdma_bind_addr(listen_id, (struct sockaddr *) &sin)) {
			throw std::runtime_error("rdma_bind_addr failed!");
		}

		if (rdma_listen(listen_id, 1)) {
			throw std::runtime_error("rdma_listen failed!");
		}

		while (thread_id < args->num_threads) {
			if (rdma_get_cm_event(cm_channel, &event)) {
				throw std::runtime_error("rdma_get_cm_event failed!");
			}

			if (event->event == RDMA_CM_EVENT_CONNECT_REQUEST) {

				buffer[thread_id] = (uint32_t *) malloc(
						sizeof(uint32_t) * MAXBUFFERSIZE);
				if (!buffer[thread_id]) {
					throw std::runtime_error("malloc buffer failed!");
				}

				cm_id[thread_id] = event->id;

				rdma_ack_cm_event(event);

				pd[thread_id] = ibv_alloc_pd(cm_id[thread_id]->verbs);
				if (!pd[thread_id]) {
					throw std::runtime_error("ibv_alloc_pd failed!");
				}

				comp_chan[thread_id] = ibv_create_comp_channel(
						cm_id[thread_id]->verbs);
				if (!comp_chan[thread_id]) {
					throw std::runtime_error("ibv_create_comp_channel failed!");
				}

				cq[thread_id] = ibv_create_cq(cm_id[thread_id]->verbs, 2, NULL,
						comp_chan[thread_id], 0);
				if (!cq[thread_id]) {
					throw std::runtime_error("ibv_create_cq failed!");
				}
				if (ibv_req_notify_cq(cq[thread_id], 0)) {
					throw std::runtime_error("ibv_req_notify_cq failed!");
				}

				mr[thread_id] = ibv_reg_mr(pd[thread_id], buffer[thread_id],
				MAXBUFFERSIZE * sizeof(uint32_t),
						IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ
								| IBV_ACCESS_REMOTE_WRITE);
				if (!mr[thread_id]) {
					throw std::runtime_error("ibv_reg_mr failed!");
				}

				qp_attr[thread_id].cap.max_send_wr = args->max_qp_wr;
				qp_attr[thread_id].cap.max_send_sge = 1;
				qp_attr[thread_id].cap.max_recv_wr = args->max_qp_wr;
				qp_attr[thread_id].cap.max_recv_sge = 1;

				qp_attr[thread_id].send_cq = cq[thread_id];
				qp_attr[thread_id].recv_cq = cq[thread_id];

				qp_attr[thread_id].qp_type = IBV_QPT_RC;

				if (rdma_create_qp(cm_id[thread_id], pd[thread_id],
						&qp_attr[thread_id])) {
					throw std::runtime_error("rdma_create_qp failed!");
				}

				//TODO: why here?
				//prepare to receive from client ---- ping function
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

				rep_pdata[thread_id].buf_va = htonll(
						(uintptr_t) buffer[thread_id]);
				rep_pdata[thread_id].buf_rkey = htonl(mr[thread_id]->rkey);

				conn_param[thread_id].responder_resources = 1;
				conn_param[thread_id].private_data = &rep_pdata[thread_id];
				conn_param[thread_id].private_data_len =
						sizeof rep_pdata[thread_id];

				if (rdma_accept(cm_id[thread_id], &conn_param[thread_id])) {
					throw std::runtime_error("rdma_accept failed!");
				}
				if (rdma_get_cm_event(cm_channel, &event)) {
					throw std::runtime_error("rdma_get_cm_event failed!");
				}
				if (event->event != RDMA_CM_EVENT_ESTABLISHED) {
					throw std::runtime_error(
							"RDMA_CM_EVENT_ESTABLISHED failed!");
				}
				rdma_ack_cm_event(event);

				work_threads[thread_id] = std::thread(work, thread_id, args);

				thread_id++;
			}

		}
		for (int i = 0; i < thread_id; i++) {
			work_threads[i].join();

			free(buffer[i]);
			rdma_dereg_mr(mr[i]);
			ibv_destroy_comp_channel(comp_chan[i]);
			ibv_destroy_cq(cq[i]);
		}

		rdma_destroy_id(listen_id);
		rdma_destroy_event_channel(cm_channel);
	} catch (std::exception &e) {
		cerr << "Exception: " << e.what() << endl;
	}

	return 0;
}


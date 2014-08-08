/*
 *
 * Usage: ./pingpong-client server-ip port MaxPacketSize(KByte)
 *
 */

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <sys/time.h>

#include <infiniband/verbs.h>

#include "Arguments.h"
#include "pingpong-common.h"

using std::cin;
using std::cout;
using std::cerr;
using std::endl;

#ifdef RDMASOCKET
#include "RDMAClientSocket.h"
int main(int argc, char* argv[]) {

	char* hostip = argv[1];
	char* port = argv[2];
	int maxPacketNum = atoi(argv[3]);
	int packetNum = 1;

	timeval t_start;
	timeval t_end;

	int id_num = 0;

	int t_usec;

	cout << "ID" << '\t' << "Size(k)" << '\t' << "Time(ms)" << endl;

	rdma::ClientSocket *clientSocket = new rdma::ClientSocket(hostip, port,
			1024, 1024);
	rdma::Buffer sendPacket = clientSocket->getWriteBuffer();
	while (packetNum <= maxPacketNum) {
		try {
			gettimeofday(&t_start, NULL);
			for (int i = 0; i < packetNum; i++) {
				memset(sendPacket.get(), 0xfe, sendPacket.size);
				clientSocket->write(sendPacket);
				rdma::Buffer readPacket = clientSocket->read();
				clientSocket->returnReadBuffer(readPacket);
			}
			gettimeofday(&t_end, NULL);

			t_usec = (t_end.tv_sec - t_start.tv_sec) * 1000 * 1000
			+ (t_end.tv_usec - t_start.tv_usec);
			cout << id_num << '\t' << packetNum << '\t' << t_usec / 1000.0
			<< endl;
			usleep(10000);
			packetNum++;
			id_num++;
		} catch (std::exception& e) {
			cerr << "Exception: " << e.what() << endl;
		}
	}

	delete clientSocket;

	return 0;
}
#else
int main(int argc, char* argv[]) {
	pdata server_pdata;

	rdma_event_channel *cm_channel;
	rdma_cm_id *cm_id;
	rdma_cm_event *event;
	rdma_conn_param conn_param = { };

	ibv_pd *pd;
	ibv_comp_channel *comp_chan;
	ibv_cq *cq;
	ibv_cq *evt_cq;
	ibv_mr *mr;
	ibv_qp_init_attr qp_attr = { };
	ibv_sge sge;
	ibv_send_wr send_wr = { };
	ibv_send_wr *bad_send_wr;
	ibv_recv_wr recv_wr = { };
	ibv_recv_wr *bad_recv_wr;
	ibv_wc wc;
	void *cq_context;

	struct timeval t_start;
	struct timeval t_end;

	Arguments* args = new Arguments(argc, argv);

	addrinfo *hints, *res;
	hints = (addrinfo *) malloc(sizeof(addrinfo));
	hints->ai_family = AF_INET;
	hints->ai_socktype = SOCK_STREAM;

	try {
		//MAXBUFFERSIZE Byte buffer
		uint32_t *buffer = (uint32_t *) malloc(
				sizeof(uint32_t) * MAXBUFFERSIZE);
		if (!buffer) {
			throw std::runtime_error("malloc buffer failed!");
		}

		query_device(args);

		cm_channel = rdma_create_event_channel();
		if (!cm_channel) {
			throw std::runtime_error("rdma_create_event_channel failed!");
		}

		if (rdma_create_id(cm_channel, &cm_id, NULL, RDMA_PS_TCP)) {
			throw std::runtime_error("rdma_create_id failed!");
		}

		if (getaddrinfo(argv[1], args->port, hints, &res)) {
			throw std::runtime_error("rdma_create_id failed!");
		}
		int err;
		for (addrinfo* t = res; t; t = t->ai_next) {
			err = rdma_resolve_addr(cm_id, NULL, t->ai_addr,
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

		if (rdma_resolve_route(cm_id, RESOLVE_TIMEOUT_MS)) {
			throw std::runtime_error("rdma_resolve_route failed!");
		}
		if (rdma_get_cm_event(cm_channel, &event)) {
			throw std::runtime_error("rdma_get_cm_event failed!");
		}
		if (event->event != RDMA_CM_EVENT_ROUTE_RESOLVED) {
			throw std::runtime_error("RDMA_CM_EVENT_ROUTE_RESOLVED failed!");
		}
		rdma_ack_cm_event(event);

		pd = ibv_alloc_pd(cm_id->verbs);
		if (!pd) {
			throw std::runtime_error("ibv_alloc_pd failed!");
		}

		comp_chan = ibv_create_comp_channel(cm_id->verbs);
		if (!comp_chan) {
			throw std::runtime_error("ibv_create_comp_channel failed!");
		}
		cq = ibv_create_cq(cm_id->verbs, 2, NULL, comp_chan, 0);
		if (!cq) {
			throw std::runtime_error("ibv_create_cq failed!");
		}
		if (ibv_req_notify_cq(cq, 0)) {
			throw std::runtime_error("ibv_req_notify_cq failed!");
		}

		mr = ibv_reg_mr(pd, buffer, MAXBUFFERSIZE * sizeof(uint32_t),
				IBV_ACCESS_LOCAL_WRITE);
		if (!mr) {
			throw std::runtime_error("ibv_reg_mr failed!");
		}

		qp_attr.cap.max_send_wr = MAXPINGPONGTIME;
		qp_attr.cap.max_send_sge = 1;
		qp_attr.cap.max_recv_wr = MAXPINGPONGTIME;
		qp_attr.cap.max_recv_sge = 1;

		qp_attr.send_cq = cq;
		qp_attr.recv_cq = cq;

		qp_attr.qp_type = IBV_QPT_RC;

		if (rdma_create_qp(cm_id, pd, &qp_attr)) {
			throw std::runtime_error("rdma_create_qp failed!");
		}

		conn_param.initiator_depth = 1;
		conn_param.retry_count = 7;

		if (rdma_connect(cm_id, &conn_param)) {
			throw std::runtime_error("rdma_connect failed!");
		}

		if (rdma_get_cm_event(cm_channel, &event)) {
			throw std::runtime_error("rdma_get_cm_event failed!");
		}
		if (event->event != RDMA_CM_EVENT_ESTABLISHED) {
			throw std::runtime_error("RDMA_CM_EVENT_ESTABLISHED failed!");
		}

		memcpy(&server_pdata, event->param.conn.private_data,
				sizeof server_pdata);
		rdma_ack_cm_event(event);

		//prepare to receive from client ---- pong function
		for (int i = 1; i <= args->max_packet_size; i++) {
			sge.addr = (uintptr_t) (buffer);
			sge.length = sizeof(uint32_t) * 1 * 1024 * i; // i MByte
			sge.length /= 4;
			sge.lkey = mr->lkey;

			recv_wr.sg_list = &sge;
			recv_wr.num_sge = 1;

			if (ibv_post_recv(cm_id->qp, &recv_wr, &bad_recv_wr)) {
				throw std::runtime_error("ibv_post_recv failed!");
			}
		}

		memset(buffer, 0x01, sizeof(buffer));

		printf("Id:\tSize(KByte):\tTime(ms):\n");
		for (int i = 1; i <= args->max_packet_size; i++) {

			gettimeofday(&t_start, NULL);

			sge.addr = (uintptr_t) (buffer);
			sge.length = sizeof(uint32_t) * 1 * 1024 * i; // i MByte
			sge.length /= 4;
			sge.lkey = mr->lkey;

			send_wr.wr_id = i;
			send_wr.opcode = IBV_WR_SEND;
			send_wr.send_flags = IBV_SEND_SIGNALED;
			send_wr.sg_list = &sge;
			send_wr.num_sge = 1;

			if (ibv_post_send(cm_id->qp, &send_wr, &bad_send_wr)) {
				throw std::runtime_error("ibv_post_send failed!");
			}

			if (ibv_get_cq_event(comp_chan, &evt_cq, &cq_context)) {
				throw std::runtime_error("ibv_get_cq_event failed!");
			}

			if (ibv_req_notify_cq(cq, 0)) {
				throw std::runtime_error("ibv_req_notify_cq failed!");
			}

			if (ibv_poll_cq(cq, 1, &wc) < 1) {
				throw std::runtime_error("ibv_poll_cq failed!");
			}

			if (wc.status != IBV_WC_SUCCESS) {
				throw std::runtime_error("IBV_WC_SUCCESS 2 failed!");
			}

//			//receive from server ---- pong function

			if (ibv_get_cq_event(comp_chan, &evt_cq, &cq_context)) {
				throw std::runtime_error("ibv_get_cq_event failed!");
			}

			if (ibv_req_notify_cq(cq, 0)) {
				throw std::runtime_error("ibv_req_notify_cq failed!");
			}

			if (ibv_poll_cq(cq, 1, &wc) < 1) {
				throw std::runtime_error("ibv_poll_cq failed!");
			}

			if (wc.status != IBV_WC_SUCCESS) {
				throw std::runtime_error("IBV_WC_SUCCESS failed!");
			}

			gettimeofday(&t_end, NULL);
			printf("%d\t%d\t        %f\n", i, i,
					((t_end.tv_sec - t_start.tv_sec) * 1000 * 1000
							+ t_end.tv_usec - t_start.tv_usec) / 1000.0);

		}
		free(buffer);
	} catch (std::exception &e) {
		cerr << "Exception: " << e.what() << endl;
	}

#endif
	return 0;
}


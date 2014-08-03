/*
 *
 * Usage: ./pingpong-server ip port
 *
 */

#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <stdexcept>

#include "pingping-common.h"

using std::cin;
using std::cout;
using std::cerr;
using std::endl;

#ifdef RDMASOCKET
#include "RDMAServerSocket.h"
int main(int argc, char* argv[]) {

	char* hostip = argv[1];
	char* port = argv[2];

	int id_num = 0;

	rdma::ServerSocket serverSocket(hostip, port, 1024, 1024);
	rdma::ClientSocket* clientSocket = serverSocket.accept();

	while (1) {
		try {
			rdma::Buffer readPacket = clientSocket->read();
			rdma::Buffer sendPacket = clientSocket->getWriteBuffer();
			memcpy(sendPacket.get(), readPacket.get(), readPacket.size);
			clientSocket->write(sendPacket);
			clientSocket->returnReadBuffer(readPacket);
		} catch (std::exception &e) {
			cerr << "Exception: " << e.what() << endl;
		}
	}

	delete clientSocket;

	return 0;
}
#else
int main(int argc, char* argv[]) {

	char* hostip = argv[1];
	char* port = argv[2];

	rdma_event_channel *cm_channel;
	rdma_cm_id *listen_id;
	rdma_cm_event *event;
	rdma_cm_id *cm_id;

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

	sockaddr_in sin;

	try {
		//MAXBUFFERSIZE Byte buffer
		char *buffer = (char *) malloc(sizeof(char) * MAXBUFFERSIZE);
		if (!buffer) {
			throw std::runtime_error("malloc buffer failed!");
		}

		query_device();

		cm_channel = rdma_create_event_channel();
		if (!cm_channel) {
			throw std::runtime_error("rdma_create_event_channel failed!");
		}

		if (!rdma_create_id(cm_channel, &listen_id, NULL, RDMA_PS_TCP)) {
			throw std::runtime_error("rdma_create_id failed!");
		}

		sin.sin_family = AF_INET;
		sin.sin_port = htons(atoi(port));
		sin.sin_addr.s_addr = INADDR_ANY;

		if (!rdma_bind_addr(listen_id, (struct sockaddr *) &sin)) {
			throw std::runtime_error("rdma_bind_addr failed!");
		}

		if (!rdma_listen(listen_id, 1)) {
			throw std::runtime_error("rdma_listen failed!");
		}

		if (!rdma_get_cm_event(cm_channel, &event)) {
			throw std::runtime_error("rdma_get_cm_event failed!");
		}

		if (event->event != RDMA_CM_EVENT_CONNECT_REQUEST) {
			throw std::runtime_error("event failed!");
		}

		rdma_ack_cm_event(event);

		cm_id = event->id;

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

		mr = ibv_reg_mr(pd, buffer, MAXBUFFERSIZE * sizeof(char),
				IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ
						| IBV_ACCESS_REMOTE_WRITE);
		if (!mr) {
			throw std::runtime_error("ibv_reg_mr failed!");
		}

		qp_attr.cap.max_send_wr = 1;
		qp_attr.cap.max_send_sge = 1;
		qp_attr.cap.max_recv_wr = 1;
		qp_attr.cap.max_recv_sge = 1;

		qp_attr.send_cq = cq;
		qp_attr.recv_cq = cq;

		qp_attr.qp_type = IBV_QPT_RC;

		if(rdma_create_qp(cm_id, pd, &qp_attr)){
			throw std::runtime_error("rdma_create_qp failed!");
		}




	} catch (std::exception &e) {
		cerr << "Exception: " << e.what() << endl;
	}

#endif
	return 0;
}


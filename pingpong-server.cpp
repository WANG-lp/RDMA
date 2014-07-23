/*
 *
 * Usage: ./pingpong-server ip port MaxPacketSize
 *
 */

#include <iostream>
#include <cstdlib>
#include <cstdio>

#include "RDMAServerSocket.h"

using std::cin;
using std::cout;
using std::cerr;
using std::endl;

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

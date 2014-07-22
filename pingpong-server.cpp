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
	int maxPacketSize = atoi(argv[3]);
	int packetSize = 1;

	int id_num = 0;

	cout << "ID" << '\t' << "Size" << endl;
	while (packetSize < maxPacketSize) {
		try {
			rdma::ServerSocket serverSocket(hostip, port, packetSize, 1);
			rdma::ClientSocket* clientSocket = serverSocket.accept();
			rdma::Buffer readPacket = clientSocket->read();
			rdma::Buffer sendPacket = clientSocket->getWriteBuffer();
			cout << id_num << '\t' << readPacket.size << endl;
			memcpy(sendPacket.get(), readPacket.get(), readPacket.size);
			clientSocket->write(sendPacket);

			delete clientSocket;

			packetSize *= 2;
			id_num++;
		} catch (std::exception &e) {
			cerr << "Exception: " << e.what() << endl;
		}
	}

	return 0;
}

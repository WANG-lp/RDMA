/*
 *
 * Usage: ./pingpong-client server-ip port MaxPacketSize
 *
 */

#include "RDMAClientSocket.h"
#include <iostream>
#include <cstdlib>
#include <sys/time.h>

using std::cin;
using std::cout;
using std::cerr;
using std::endl;

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


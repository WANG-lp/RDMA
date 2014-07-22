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
	int maxPacketSize = atoi(argv[3]);
	int packetSize = 1;

	timeval t_start;
	timeval t_end;

	int id_num = 0;

	int t_usec;

	cout << "ID" << '\t' << "Size" << '\t' << "Latency(ms)" << endl;
	while (packetSize < maxPacketSize) {
		try {

			gettimeofday(&t_start, NULL);
			rdma::ClientSocket clientSocket(hostip, port, packetSize, 1);
			rdma::Buffer sendPacket = clientSocket.getWriteBuffer();
			memset(sendPacket.get(), 0xfe, sendPacket.size);
			clientSocket.write(sendPacket);

			rdma::Buffer readPacket = clientSocket.read();

			gettimeofday(&t_end, NULL);

			t_usec = (t_end.tv_sec - t_start.tv_sec) * 1000 * 1000
					+ (t_end.tv_usec - t_start.tv_usec);

			cout << id_num << '\t' << readPacket.size << '\t' << t_usec / 1000
					<< '.' << t_usec % 1000 << endl;

			usleep(10000);

			packetSize *= 2;
			id_num++;
		} catch (std::exception& e) {
			cerr << "Exception: " << e.what() << endl;
		}

	}
	return 0;
}


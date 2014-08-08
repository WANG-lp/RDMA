#ifndef ARGUMENTS_H_
#define ARGUMENTS_H_
#include <stdexcept>
#include <cstdint>
#include <cstdlib>

class Arguments {
public:
	Arguments(int argc, char* argv[]);
	virtual ~Arguments();
	uint64_t max_qp_wr;
	char* port;
	char* hostip;
	int max_mr_size;
	int max_packet_size;

};

#endif /* ARGUMENTS_H_ */

#ifndef ARGUMENTS_H_
#define ARGUMENTS_H_
#include <stdexcept>
#include <cstdint>
#include <cstdlib>
#include <string>

class Arguments {
public:
	Arguments(int argc, char* argv[]);
	virtual ~Arguments();
	uint64_t max_qp_wr;
	char* port;
	char* hostip;
	int max_mr_size;
	int min_packet_size;
	int max_packet_size;
	int size_mode;//0->Kbyte, 1-> Mbyte
	std::string size_str;
	int size;
	int num_threads;
	int loop;

private:
	void printUsage();
};

#endif /* ARGUMENTS_H_ */

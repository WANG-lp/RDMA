#include "Arguments.h"

Arguments::Arguments(int argc, char* argv[]) {
	// TODO Auto-generated constructor stub
	if(argc < 4){
		throw std::runtime_error("args error");
	}
	this->hostip = argv[1];
	this->port = argv[2];
	this->max_packet_size =atoi(argv[3]);
	this->max_mr_size = -1;
	this->max_qp_wr = -1;
}

Arguments::~Arguments() {
	// TODO Auto-generated destructor stub
}


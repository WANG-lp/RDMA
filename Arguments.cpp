#include "Arguments.h"
#include "pingpong-common.h"

Arguments::Arguments(int argc, char* argv[]) {
	// TODO Auto-generated constructor stub
	if (argc < 3) {
		printUsage();
		throw std::runtime_error("args error");
	}
	this->max_mr_size = -1;
	this->max_qp_wr = -1;
	this->size_mode = 1;
	this->size_str = "KByte";
	this->size = 1024;
	this->num_threads = 1;
	this->min_packet_size = this->max_packet_size = 64;
	this->loop = 1;

	int i = 1;
	while (i < argc) {
		if (i + 2 == argc) {
			this->hostip = argv[i];

		} else if (i + 1 == argc) {
			this->port = argv[i];

		} else {
			/* process arguements */
			switch (argv[i][1]) {
			case 's':
				this->size_mode = atoi(argv[++i]);
				if (this->size_mode > 1)
					throw std::runtime_error("-s error");
				break;
			case 't':
				this->num_threads = atoi(argv[++i]);
				if (this->num_threads > MAXTHREADS)
					this->num_threads = MAXTHREADS;
				break;
			case 'n':
				this->min_packet_size = atoi(argv[++i]);
				break;
			case 'x':
				this->max_packet_size = atoi(argv[++i]);
				break;
			case 'l':
				this->loop = atoi(argv[++i]);
				break;
			case 'h':
				printUsage();
				throw std::runtime_error("args error");
				break;
			default:
				printUsage();
				fprintf(stderr, "Invalid option %s\n", argv[i]);
				throw std::runtime_error("args error");
			}
		}
		++i;
	}

	if (size_mode == 1) {
		this->size_str = "MByte";
		this->size = 1024 * 1024;
	}
}

Arguments::~Arguments() {
	// TODO Auto-generated destructor stub
}

void Arguments::printUsage() {
	printf("Usage: pingpong-[server/client] [-h] [-s integer] [-t integer] [-n integer] [-x integer] [-l integer]"
			" host_ip port\n");
	printf("    -h          : this help message\n");
	printf("    -s integer  : 0 -> KByte, 1-> MByte  (default 1)\n");
	printf("    -t integer  : number of threads  (default 1)\n");
	printf("    -n integer  : min_packet_size in [KByte/MByte] (default 64)\n");
	printf("    -x integer  : max_packet_size in [KByte/MByte] (default 64)\n");
	printf("    -l integer  : number of loops (default 1)\n");
	printf("    host_ip     : IP Address of Host\n");
	printf("    port        : Port Address of Host\n");

}

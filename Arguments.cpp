#include "Arguments.h"
#include "pingpong-common.h"

Arguments::Arguments(int argc, char* argv[]) {
	// TODO Auto-generated constructor stub
	if (argc < 4) {
		throw std::runtime_error("args error");
	}
	this->max_mr_size = -1;
	this->max_qp_wr = -1;
	this->size_mode = 0;
	this->size_str = "KByte";
	this->size = 1024;
	this->num_threads = 1;

	int i = 1;
	while (i < argc) {
		if (i + 3 == argc) {
			this->hostip = argv[i];

		} else if (i + 2 == argc) {
			this->port = argv[i];

		} else if (i + 1 == argc) {
			this->max_packet_size = atoi(argv[i]);

		} else {
			/* process arguements */
			switch (argv[i][1]) {
			case 's':
				this->size_mode = atoi(argv[++i]);
				if(this->size_mode > 1)
					throw std::runtime_error("-s error");
				break;
			case 't':
				this->num_threads = atoi(argv[++i]);
				if(this->num_threads > MAXTHREADS)
					this->num_threads = MAXTHREADS;
				break;
			default:
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


#include "pingpong-common.h"

void query_device(Arguments* args) {
	struct ibv_context *ctx;
	struct ibv_device **device_list;
	struct ibv_device_attr device_attr;
	int device_num;

	device_list = ibv_get_device_list(&device_num);
	if (!device_list) {
		throw std::runtime_error("ibv_get_device_list failed!");
	}

	//We open the first device
	ctx = ibv_open_device(device_list[0]);
	if (!ctx) {
		throw std::runtime_error("ibv_open_device failed!");
	}

	if (ibv_query_device(ctx, &device_attr)) {
		throw std::runtime_error("ibv_query_device failed!");
	}

	printf("There are %d devices!\n", device_num);
	printf("The device '%s' max mr size is %u bytes.\n",
			ibv_get_device_name(ctx->device), device_attr.max_mr_size);
	printf("The device '%s' max_qp_wr is %u.\n",
			ibv_get_device_name(ctx->device), device_attr.max_qp_wr);

	args->max_mr_size = device_attr.max_mr_size;
	args->max_qp_wr = device_attr.max_qp_wr;

	ibv_close_device(ctx);
	ibv_free_device_list(device_list);
}

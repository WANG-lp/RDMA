#include "pingpong-common.h"

void query_device(Arguments* args) {
	struct ibv_context *ctx;
	struct ibv_device **device_list;
	struct ibv_device_attr device_attr;
	struct ibv_port_attr port_attr;
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
	if(ibv_query_port(ctx, 1, &port_attr)){
		throw std::runtime_error("ibv_query_port failed!");
	}

	printf("There are %d devices!\n", device_num);
	printf("The device '%s' max mr size is %llu bytes.\n",
			ibv_get_device_name(ctx->device), device_attr.max_mr_size);
	printf("The device '%s' max_qp_wr is %u.\n",
			ibv_get_device_name(ctx->device), device_attr.max_qp_wr);

	printf("The device '%s' phys_port_cnt is %u.\n",
				ibv_get_device_name(ctx->device), device_attr.phys_port_cnt);

	printf("The device '%s' active_speed is %s.\n",
				ibv_get_device_name(ctx->device), active_speed[port_attr.active_speed]);
	printf("The device '%s' max_mtu is %s.\n",
				ibv_get_device_name(ctx->device), mtu[port_attr.max_mtu]);
	printf("The device '%s' active_mtu is %s.\n",
					ibv_get_device_name(ctx->device), mtu[port_attr.active_mtu]);

	printf("The device '%s' link_layer is %s.\n",
						ibv_get_device_name(ctx->device), link_layer[port_attr.link_layer]);



	args->max_mr_size = device_attr.max_mr_size;
	args->max_qp_wr = device_attr.max_qp_wr;

	ibv_close_device(ctx);
	ibv_free_device_list(device_list);
}

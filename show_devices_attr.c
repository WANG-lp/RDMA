/*
 * Copyright (c) 2012 Dotan Barak - RDMAmojo. All rights reserved.
 *
 * The source code in this file can be freely used, adapted,
 * and redistributed in source or binary form, for any purpose,
 * including, but not limited, to commercial educationa, as
 * long as an acknowledgment appears in derived source files.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * show_devices_attr.c -- program that open all of the RDMA devices in the system,
 *                        print their names and the number of ports that they have
 *                        and close the devices context.
 *
 * Compilation:
 *    gcc show_devices_attr.c -o show_devices_attr -libverbs
 */

#include <stdio.h>
#include <infiniband/verbs.h>

int main(void) {
	struct ibv_context *ctx;
	struct ibv_device **device_list;
	int num_devices;
	int i;
	int rc;

	device_list = ibv_get_device_list(&num_devices);
	if (!device_list) {
		fprintf(stderr, "Error, ibv_get_device_list() failed\n");
		return -1;
	}

	printf("%d RDMA device(s) found:\n\n", num_devices);

	for (i = 0; i < num_devices; ++i) {
		struct ibv_device_attr device_attr;

		ctx = ibv_open_device(device_list[i]);
		if (!ctx) {
			fprintf(stderr, "Error, failed to open the device '%s'\n",
					ibv_get_device_name(device_list[i]));
			rc = -1;
			goto out;
		}

		rc = ibv_query_device(ctx, &device_attr);
		if (rc) {
			fprintf(stderr,
					"Error, failed to query the device '%s' attributes\n",
					ibv_get_device_name(device_list[i]));
			rc = -1;
			goto out_device;
		}

		printf("The device '%s' has %d port(s)\n",
				ibv_get_device_name(ctx->device), device_attr.phys_port_cnt);

		printf("The device '%s' max mr size is %u bytes\n",
						ibv_get_device_name(ctx->device), device_attr.max_mr_size);
		printf("The device '%s' max qp_wr size is %d\n",
								ibv_get_device_name(ctx->device), device_attr.max_qp_wr);

		rc = ibv_close_device(ctx);
		if (rc) {
			fprintf(stderr, "Error, failed to close the device '%s'\n",
					ibv_get_device_name(ctx->device));
			rc = -1;
			goto out;
		}
	}

	ibv_free_device_list(device_list);

	return 0;

	out_device: ibv_close_device(ctx);

	out: ibv_free_device_list(device_list);
	return rc;
}

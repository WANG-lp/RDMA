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
 * open_devices.c -- program that open and close all of the RDMA devices in the system.
 *
 * Compilation:
 *    gcc open_devices.c -o open_devices -libverbs
 */

#include <stdio.h>
#include <infiniband/verbs.h>

int main(void)
{
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

	for (i = 0; i < num_devices; ++ i) {
		struct ibv_context *ctx;

		ctx = ibv_open_device(device_list[i]);
		if (!ctx) {
			fprintf(stderr, "Error, failed to open the device '%s'\n",
				ibv_get_device_name(device_list[i]));
			rc = -1;
			goto out;
		}

		printf("The device '%s' was opened\n", ibv_get_device_name(ctx->device));

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

out:
	ibv_free_device_list(device_list);
	return rc;
}

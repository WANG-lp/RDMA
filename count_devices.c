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
 * count_devices.c -- program that get the number of RDMA devices in 2 ways:
 *    1) Using count
 *    2) As returned parameter from ibv_get_device_list
 *
 * Compilation:
 *    gcc count_devices.c -o count_devices -libverbs
 */

#include <stdio.h>
#include <infiniband/verbs.h>

static int get_num_rdma_devices_count(void)
{
	struct ibv_device **device_list;
	int num_devices = 0;

	device_list = ibv_get_device_list(NULL);
	if (!device_list) {
		fprintf(stderr, "Error, ibv_get_device_list() failed\n");
		return -1;
	}

	while (device_list[num_devices])
		num_devices ++;

	ibv_free_device_list(device_list);

	return num_devices;
}

static int get_num_rdma_devices_param(void)
{
	struct ibv_device **device_list;
	int num_devices;

	device_list = ibv_get_device_list(&num_devices);
	if (!device_list) {
		fprintf(stderr, "Error, ibv_get_device_list() failed\n");
		return -1;
	}

	ibv_free_device_list(device_list);

	return num_devices;
}

int main(void)
{
	int num_devices;

	/* print the number of RDMA devices by counting them */
	num_devices = get_num_rdma_devices_count();
	if (num_devices < 0) {
		fprintf(stderr, "Error, failed ot get number of RDMA devices by count\n");
		return 1;
	}

	printf("%d RDMA device(s) found by count\n", num_devices);

	/* print the number of RDMa devices using parameter */
	num_devices = get_num_rdma_devices_param();
	if (num_devices < 0) {
		fprintf(stderr, "Error, failed ot get number of RDMA devices by function parameter\n");
		return 1;
	}

	printf("%d RDMA device(s) found by function parameter\n", num_devices);

	return 0;
}

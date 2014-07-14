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
 * show_devices_guid -- program that print the GUIDs of the RDMA devices that
 *                      exists in the local machine.
 *
 * Compilation:
 *    gcc show_devices_guid.c -o show_devices_guid -libverbs
 */

#include <stdio.h>
#include <infiniband/verbs.h>
//#include "utils.h"

int main(void)
{
	struct ibv_device **device_list;
	int num_devices;
	int i;

	device_list = ibv_get_device_list(&num_devices);
	if (!device_list) {
		fprintf(stderr, "Error, ibv_get_device_list() failed\n");
		return -1;
	}

	printf("%d RDMA device(s) found:\n\n", num_devices);

	for (i = 0; i < num_devices; ++ i) {
		printf("RDMA device[%d]: name=%s, GUID=0x%016Lx\n", i,
		       ibv_get_device_name(device_list[i]),
		       ntohl(ibv_get_device_guid(device_list[i])));
	}

	ibv_free_device_list(device_list);

	return 0;
}

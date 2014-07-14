/*
 * Copyright (c) 2012 Dotan Barak - RDMAmojo. All rights reserved.
 *
 * The source code in this file can be freely used, adapted,
 * and redistributed in source or binary form, for any purpose,
 * including, but not limited, to commercial and educational, as
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
 * async_event.c -- program that reads the async events and prints them.
 *
 * Compilation:
 *    gcc async_event.c -o async_event -libverbs
 *
 * Running:
 *    ./async_event -d <RDMA device>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <infiniband/verbs.h>

static struct ibv_context *get_device_context(const char *device_name)
{
	struct ibv_device **device_list;
	struct ibv_context *ctx = NULL;
	int num_devices;
	int i;

	device_list = ibv_get_device_list(&num_devices);
	if (!device_list) {
		fprintf(stderr, "Error, ibv_get_device_list() failed\n");
		return NULL;
	}

	for (i = 0; i < num_devices; ++ i) {
		/* if this isn't the requested device */
		if (strcmp(ibv_get_device_name(device_list[i]), device_name))
			continue;

		ctx = ibv_open_device(device_list[i]);
		if (!ctx) {
			fprintf(stderr, "Error, failed to open the device '%s'\n",
				ibv_get_device_name(device_list[i]));
			goto out;
		}

		printf("The device '%s' was detected\n", device_name);
		break;
	}

out:
	ibv_free_device_list(device_list);

	return ctx;
}

static void print_async_event(struct ibv_context *ctx, struct ibv_async_event *event)
{
	switch (event->event_type) {
	/* QP events */
	case IBV_EVENT_QP_FATAL:
		printf("QP fatal event for QP with handle %p\n", event->element.qp);
		break;
	case IBV_EVENT_QP_REQ_ERR:
		printf("QP Requestor error for QP with handle %p\n", event->element.qp);
		break;
	case IBV_EVENT_QP_ACCESS_ERR:
		printf("QP access error event for QP with handle %p\n", event->element.qp);
		break;
	case IBV_EVENT_COMM_EST:
		printf("QP communication established event for QP with handle %p\n", event->element.qp);
		break;
	case IBV_EVENT_SQ_DRAINED:
		printf("QP Send Queue drained event for QP with handle %p\n", event->element.qp);
		break;
	case IBV_EVENT_PATH_MIG:
		printf("QP Path migration loaded event for QP with handle %p\n", event->element.qp);
		break;
	case IBV_EVENT_PATH_MIG_ERR:
		printf("QP Path migration error event for QP with handle %p\n", event->element.qp);
		break;
	case IBV_EVENT_QP_LAST_WQE_REACHED:
		printf("QP last WQE reached event for QP with handle %p\n", event->element.qp);
		break;

	/* CQ events */
	case IBV_EVENT_CQ_ERR:
		printf("CQ error for CQ with handle %p\n", event->element.cq);
		break;

	/* SRQ events */
	case IBV_EVENT_SRQ_ERR:
		printf("SRQ error for SRQ with handle %p\n", event->element.srq);
		break;
	case IBV_EVENT_SRQ_LIMIT_REACHED:
		printf("SRQ limit reached event for SRQ with handle %p\n", event->element.srq);
		break;

	/* Port events */
	case IBV_EVENT_PORT_ACTIVE:
		printf("Port active event for port number %d\n", event->element.port_num);
		break;
	case IBV_EVENT_PORT_ERR:
		printf("Port error event for port number %d\n", event->element.port_num);
		break;
	case IBV_EVENT_LID_CHANGE:
		printf("LID change event for port number %d\n", event->element.port_num);
		break;
	case IBV_EVENT_PKEY_CHANGE:
		printf("P_Key table change event for port number %d\n", event->element.port_num);
		break;
	case IBV_EVENT_GID_CHANGE:
		printf("GID table change event for port number %d\n", event->element.port_num);
		break;
	case IBV_EVENT_SM_CHANGE:
		printf("SM change event for port number %d\n", event->element.port_num);
		break;
	case IBV_EVENT_CLIENT_REREGISTER:
		printf("Client reregister event for port number %d\n", event->element.port_num);
		break;

	/* RDMA device events */
	case IBV_EVENT_DEVICE_FATAL:
		printf("Fatal error event for device %s\n", ibv_get_device_name(ctx->device));
		break;

	default:
		printf("Unknown event (%d)\n", event->event_type);
	}
}

static void usage(const char *argv0)
{
	printf("Usage: %s             print asynchronous events\n", argv0);
	printf("\n");
	printf("Options:\n");
	printf("  -h           print this help screen and exit\n");
	printf("  -d <dev>     use RDMA device <dev>\n");
}

int main(int argc, char *argv[])
{
	char *device_name = NULL;
	struct ibv_async_event event;
	struct ibv_context *ctx;
	int ret = 0;

	/* parse command line options */
	while (1) {
		int c;

		c = getopt(argc, argv, "d:h");
		if (c == -1)
			break;

		switch (c) {
		case 'd':
			device_name = strdup(optarg);
			if (!device_name) {
				fprintf(stderr, "Error, failed to allocate memory for the device name\n");
				return -1;
			}
			break;

		case 'h':
			usage(argv[0]);
			exit(1);

		default:
			fprintf(stderr, "Bad command line was used\n\n");
			usage(argv[0]);
			exit(1);
		}
	}

	if (!device_name) {
		fprintf(stderr, "Error, the device name is mandatory\n");
		return -1;
	}

	ctx = get_device_context(device_name);
	if (!ctx) {
		fprintf(stderr, "Error, the context of the device name '%s' could not be opened\n", device_name);
		free(device_name);
		return -1;
	}

	printf("Listening on events for the device '%s'\n", device_name);

	while (1) {
		/* wait for the next async event */
		ret = ibv_get_async_event(ctx, &event);
		if (ret) {
			fprintf(stderr, "Error, ibv_get_async_event() failed\n");
			goto out;
		}

		/* print the event */
		print_async_event(ctx, &event);

		/* ack the event */
		ibv_ack_async_event(&event);
	}

	ret = 0;

out:
	if (ibv_close_device(ctx)) {
		fprintf(stderr, "Error, failed to close the context of the device '%s'\n", device_name);
		return -1;
	}

	printf("The context of the device name '%s' was closed\n", device_name);
	free(device_name);

	return ret;
}

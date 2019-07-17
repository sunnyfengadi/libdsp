/*
 * =====================================================================================
 *
 *       Filename:  dsp_msg.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2019年06月05日 11时43分56秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdio.h>
#include <string.h> /* for memcpy */
#include <fcntl.h>
#include <sys/mman.h>
#include <stdlib.h> /* for malloc */
#include "dsp_communication.h"

#define CHECK_STATUS(ops, status, line) check_status(ops, status, line);
#define CHECK_SIZE(max, check, line) check_size(max, check, line);
#define BUFF_SIZE 64
#define TIMEOUT 5*1000
#define MAX_BUF_SIZE BUFF_SIZE*10
#define CMD_VIR_TO_PHYS    9999 

void WRONG(unsigned line)
{
	fprintf(stderr,"WRONG: line=%u\n", line);
	fflush(stdout);
	exit(1);
}

void check_status(char* ops, mcapi_status_t status, unsigned line)
{
	char status_message[32];
	char print_buf[BUFF_SIZE];
	mcapi_display_status(status, status_message, 32);
	if (NULL != ops) {
		snprintf(print_buf, sizeof(print_buf), "%s:%s", ops, status_message);
		printf("CHECK_STATUS---%s\n", print_buf);
	}
	if ((status != MCAPI_PENDING) && (status != MCAPI_SUCCESS)) {
		WRONG(line);
	}
}

void check_size(size_t max_size, size_t check_size, unsigned line)
{
	if (check_size > max_size || check_size < 0) {
		printf("check_size:%d is out of range [0,%d]!\n",check_size, max_size);
		WRONG(line);
	}
}

#if 0
static void dsp_com_init(DSP_COM *master, DSP_COM *slave, int block_mode, int timeout)
{
    mcapi_status_t statu;
    mcapi_request_t request;
	mcapi_param_t parms;
	mcapi_info_t version;
	size_t size;

	/* Map an area in L2 to share data with DSP */
	/* Open the device */

//	printf("\nhfeng %s %d open sram_mmap start\n",__func__, __LINE__);
	master->sram_handle = open("/dev/sram_mmap", O_RDWR);
//	printf("sram_handle = 0x%x\n", master->sram_handle);

    /* create a node */
//	printf("\nhfeng %s %d create a node\n",__func__, __LINE__);
    mcapi_initialize(master->domain,master->node,NULL,&parms,&version,&status);
    CHECK_STATUS("initialize", status, __LINE__); 
    
    /* create endpoints */ 
//	printf("\nhfeng %s %d create endpoint\n",__func__, __LINE__);
	master->endpoint = mcapi_endpoint_create(master->port,&status);
	CHECK_STATUS("create_ep", status, __LINE__);

    if (block_mode) {
		slave->endpoint = mcapi_endpoint_get(slave->domain,slave->node,slave->port, TIMEOUT,&status);
		CHECK_STATUS("get_ep", status, __LINE__);
	} else {
		mcapi_endpoint_get_i(slave->domain, slave->node, slave->port,&slave->endpoint,&request,&status);
		CHECK_STATUS("get_ep_i", status,   __LINE__);
        mcapi_wait(&request, &size, timeout, &status);
		CHECK_STATUS("wait", status, __LINE__);
    }
	printf("Init dsp communication successfully!\
			\nmaster endpoint:0x%x = (%d, %d, %d), \
			\nslave endpoint:0x%x = (%d, %d, %d)\n",
			master->endpoint, master->domain, master->node, master->port,
			slave->endpoint, slave->domain, slave->node, slave->port);
	return;
}
#endif
static void dsp_com_finalize(DSP_COM_HANDLE *handle)
{
#if 0
	mcapi_status_t status;
	mcapi_endpoint_delete(handle->master->endpoint, &status);
	CHECK_STATUS("del_ep", status, __LINE__);


	printf("hfeng %s %d\n",__func__, __LINE__);
	mcapi_finalize(&status);
	CHECK_STATUS("finalize", status, __LINE__);

	close(handle->sram_handle);
#endif 
}


static void alloc_dsp_com_mem(DSP_COM_HANDLE *handle, char *addr)
{
	int ret;
	unsigned long paddr;
#if 1

	/* malloc buffer in SRAM */
	addr = mmap(0, handle->buf.size, PROT_READ | PROT_WRITE, 
				MAP_FILE | MAP_SHARED, handle->sram_handle, 0);
	if (!addr)
	{
		fprintf(stderr,"mmap failed\n");
		close(handle->sram_handle);
		exit(1) ;
	}
	printf("hfeng %s %d vaddr=0x%x\n",__func__,__LINE__,addr);

//	printf("hfeng vaddr =0x%x\n", vaddr);
	ret = ioctl(handle->sram_handle, CMD_VIR_TO_PHYS, &paddr);
	if (ret < 0) { 
		printf("Failed to get phys address\n");
		close(handle->sram_handle);
		exit(1);
	}

	handle->buf.paddr = paddr;
	printf("hfeng %s paddr =0x%x\n",__func__, handle->buf.paddr);
	
#endif
}

static void free_dsp_com_mem(DSP_COM_BUF *buffer, unsigned long *vaddr)
{
	munmap(vaddr, buffer->size);

}
static void send_msg_to_dsp(DSP_COM *master, DSP_COM *slave, DSP_COM_BUF *buffer, 
			int block_mode, int timeout)
{
	int priority = 1;
	size_t size;
	mcapi_status_t status;
	mcapi_request_t request;
	size_t send_size = sizeof(DSP_COM_BUF);
	char * vaddr;

	printf("msg-l2addr=0x%x, osc_buffer_size=%d,mcapi_size=%d\n", buffer->paddr, buffer->size,send_size);
	switch (block_mode) {
		case 0:
			mcapi_msg_send_i(master->endpoint,slave->endpoint,buffer,send_size,priority,&request,&status);
			CHECK_STATUS("send_i", status, __LINE__);
			break;
		case 1:
			mcapi_msg_send(master->endpoint, slave->endpoint, buffer, send_size, priority, &status);
			CHECK_STATUS("send", status, __LINE__);
			break;
		default:
			printf("Invalid block mode value: %d\
					\nIt should be in the range of 0 or 1\n", block_mode);
			WRONG(__LINE__);
	}
}

static void recv_msg_from_dsp(mcapi_endpoint_t recv, char *buffer, size_t buffer_size, mcapi_status_t *status,
	mcapi_request_t *request, int mode, int timeout)
{
#if 1
return;
#else
	size_t size;
	mcapi_uint_t avail;
	if (buffer == NULL) {
		printf("recv() Invalid buffer ptr\n");
		WRONG(__LINE__);
	}
	printf("recv() start......\n");
	switch (mode) {
		case 0:
			do {
				avail = mcapi_msg_available(recv, status);
			}while(avail <= 0);
			CHECK_STATUS("available", *status, __LINE__);

			mcapi_msg_recv_i(recv, buffer, buffer_size, request, status);
			CHECK_STATUS("recv_i", *status, __LINE__);
			/* use mcapi_wait to release request */
			mcapi_wait(request, &size, timeout, status);
			CHECK_STATUS("wait", *status, __LINE__);
			CHECK_SIZE(buffer_size, size, __LINE__);
			break;
		case 1:
			mcapi_msg_recv_i(recv, buffer, buffer_size, request, status);
			CHECK_STATUS("recv_i", *status, __LINE__);
			do{
				mcapi_test(request, &size, status);
			}while(*status == MCAPI_PENDING);
			CHECK_STATUS("test", *status, __LINE__);
			CHECK_SIZE(buffer_size, size, __LINE__);

			/* use mcapi_wait to release request */
			mcapi_wait(request, &size, timeout, status);
			CHECK_STATUS("wait", *status, __LINE__);
			CHECK_SIZE(buffer_size, size, __LINE__);
			break;
		case 2:
			mcapi_msg_recv_i(recv, buffer, buffer_size, request, status);
			CHECK_STATUS("recv_i", *status, __LINE__);
			mcapi_wait(request, &size, timeout, status);
			CHECK_STATUS("wait", *status, __LINE__);
			CHECK_SIZE(buffer_size, size, __LINE__);
			break;
		case 3:
			mcapi_msg_recv(recv, buffer, buffer_size, &size, status);
			CHECK_STATUS("recv", *status, __LINE__);
			CHECK_SIZE(buffer_size, size, __LINE__);
			break;
		default:
			printf("Invalid mode value: %d\
					\nIt should be in the range of 0 to 3\n", mode);
			WRONG(__LINE__);
	}
	buffer[size] = '\0';
	printf("end of recv() - endpoint=%i size 0x%x has received: [%s]\n", (int)recv, size, buffer);
#endif
}
static void setup_dsp_inode(DSP_COM *inode, mcapi_domain_t mdomain,
						mcapi_node_t mnode, mcapi_endpoint_t mport)
{
	inode->domain = mdomain;
	inode->node = mnode;
	inode->port = mport;
}

int dsp_com_open(int connection_id, size_t sram_size, int block_mode)
{
	mcapi_status_t status;
    mcapi_request_t request;
	mcapi_param_t parms;
	mcapi_info_t version;
	size_t size;
	int ret = 0;
	unsigned long paddr;
	int fd;
	DSP_COM_HANDLE *handle;

	handle = malloc(sizeof(DSP_COM_HANDLE));
	if (!handle) {
		fprintf(stderr, "Failed to alloc memory for dsp communication\n");
		return -1;
	}

	fd = (unsigned int)handle;
	printf("hfeng %s %d fdfdfd =0x%x,handle=0x%x\n",__func__,__LINE__,fd,(unsigned int)handle);

	/* Map an area in L2 to share data with DSP */
	/* Open the device */
	handle->sram_size = sram_size;
	handle->sram_handle = open("/dev/sram_mmap", O_RDWR);
	if (!handle->sram_handle)
	{
		fprintf(stderr,"Failed to open sram mmap\n");
		ret = -1;
		goto free_dsp_com_mem;
	}
	printf("\nhfeng %s %d- open sram_mmap fd=%d\n",__func__,__LINE__, handle->sram_handle);

	/* Malloc buffer in SRAM */
	handle->sram_vaddr = mmap(0, handle->sram_size, PROT_READ | PROT_WRITE, 
				MAP_FILE | MAP_SHARED, handle->sram_handle, 0);
	if (!handle->sram_vaddr)
	{
		fprintf(stderr,"mmap failed\n");
		ret = -1;
		goto free_sram_mem;
	}
	printf("hfeng %s %d vaddr=0x%x\n",__func__,__LINE__,handle->sram_vaddr);

	/* Get physical address through ioctl */
	ret = ioctl(handle->sram_handle, CMD_VIR_TO_PHYS, &paddr);
	if (ret < 0) { 
		printf("Failed to get phys address\n");
		goto free_sram_mem;
	}

	handle->buf.paddr = paddr;
	printf("hfeng %s %d paddr =0x%x\n",__func__, __LINE__, handle->buf.paddr);

	/* Create MCAPI communication socket */
	DSP_COM *master = &handle->master; 
	DSP_COM *slave = &handle->slave;

switch(connection_id){
	case 1:
		setup_dsp_inode(master, 0, 0, 1);
		setup_dsp_inode(slave, 0, 1, 1);
		break;
	case 2:
		setup_dsp_inode(master, 0, 0, 2);
		setup_dsp_inode(slave, 0, 1, 2);
		break;
	case 3:
		setup_dsp_inode(master, 0, 0, 101);
		setup_dsp_inode(slave, 0, 1, 5);
		break;
	default:
		ret = -1;
		printf("Invalid connction id\n");	
		goto free_sram_mem;
}

    /* create a node */
    mcapi_initialize(master->domain, master->node,
				NULL,&parms, &version, &status);
    CHECK_STATUS("initialize", status, __LINE__); 
    
    /* create endpoints */ 
	master->endpoint = mcapi_endpoint_create(master->port, &status);
	CHECK_STATUS("create_ep", status, __LINE__);

    if (block_mode) {
		slave->endpoint = mcapi_endpoint_get(slave->domain, slave->node,
				slave->port, TIMEOUT, &status);
		CHECK_STATUS("get_ep", status, __LINE__);
	} else {
		mcapi_endpoint_get_i(slave->domain, slave->node, 
					slave->port, &slave->endpoint, &request, &status);
		CHECK_STATUS("get_ep_i", status,   __LINE__);
     //   mcapi_wait(&request, &size, TIMEOUT, &status);
	//	CHECK_STATUS("wait", status, __LINE__);
    }
	printf("    Init dsp communication successfully!\
			\n    master endpoint:0x%x = (%d, %d, %d), \
			\n    slave endpoint:0x%x = (%d, %d, %d)\n",
			master->endpoint, master->domain,
			master->node, master->port,
			slave->endpoint, slave->domain,
			slave->node, slave->port);
	printf("hfeng %s %d fdfdfd =0x%x\n",__func__,__LINE__,fd);

	goto exit;

free_mcapi_node:
	mcapi_finalize(&status);
free_sram_mem:
	close(handle->sram_handle);
free_dsp_com_mem:
	free(handle);
exit:	
	return fd;
}

void dsp_com_close(int fd, int connection_id)
{
	DSP_COM_HANDLE *handle = (DSP_COM_HANDLE *)fd;
	printf("hfeng %s %d handle=0x%x",__func__,__LINE__,handle);

	printf("%s fdfdfd =0x%x\n",__func__,fd);
	printf("hfeng %s %d,vaddr=0x%x, size=%d\n",__func__,__LINE__,handle->sram_vaddr,handle->sram_size);
	munmap(handle->sram_vaddr, handle->sram_size);
	close(handle->sram_handle);
	free(fd);
}
void dsp_com_read(DSP_COM_HANDLE *handle, void *value, size_t data_size, int block_mode)
{

}

void dsp_com_write(int fd, void *value, size_t data_size, int block_mode)
{
	int priority = 1;
	mcapi_status_t status;
	mcapi_request_t request;
//char *vaddr;
	int ret;
	char recv_buf[BUFF_SIZE];
	size_t size;
	DSP_COM_HANDLE *handle = (DSP_COM_HANDLE *)fd;;
	/* mcapi_size is the size of message that send to DSP_COM_BUF
	 * including the physic address of SRAM and the allocated size in SRAM */
	size_t mcapi_data_size = sizeof(DSP_COM_BUF);

	printf("%s fdfdfd =0x%x\n",__func__,fd);
	printf("hfeng %s %d handle=0x%x",__func__,__LINE__,handle);

	/*  Data size is the size of data stored in SRAM */
	handle->buf.size = data_size;

	printf("hfeng %s %d vaddr = 0x%x, paddr=0x%x,\n",__func__,__LINE__,handle->sram_vaddr, handle->buf.paddr);
	/* Copy the data into allocated SRAM */
	printf("hfeng %s %d vaddr = 0x%x, value=%d\n",__func__,__LINE__,handle->sram_vaddr,*(int *)value);
	printf("hfeng %s %d vaddr = 0x%x, value=%f\n",__func__,__LINE__,handle->sram_vaddr,*(float *)value);
	memcpy(handle->sram_vaddr, value, data_size);
//	vaddr[0] = 5;
#if 0
	int i;
	for (i = 1; i < handle->buf.size ; i++)
	{
		vaddr[i] = i;
		printf("hfeng %s- write %d to memory\n",__func__,i);
	}
#endif
	/* Send msg to DSP */
	switch (block_mode) {
		case 0:
			mcapi_msg_send_i(handle->master.endpoint, handle->slave.endpoint,
						&handle->buf, mcapi_data_size, priority, &request, &status);
			CHECK_STATUS("send_i", status, __LINE__);
			break;
		case 1:
			mcapi_msg_send(handle->master.endpoint, handle->slave.endpoint,
						&handle->buf, mcapi_data_size, priority, &status);
			CHECK_STATUS("send", status, __LINE__);
			break;
		default:
			close(handle->sram_handle);
			printf("Invalid block mode value: %d\
					\nIt should be in the range of 0 or 1\n", block_mode);
			WRONG(__LINE__);
	}
	printf("hfeng %s %d mcapi_msg_recv start\n",__func__,__LINE__);
	mcapi_msg_recv(handle->master.endpoint, recv_buf, sizeof(recv_buf)-1, &size, &status);
	recv_buf[size] = '\0';
	printf("end of recv() - endpoint=%d size 0x%x has received: [%s]\n", (int)handle->slave.endpoint, size, recv_buf);
}

/*
 * =====================================================================================
 *
 *       Filename:  dsp_msg.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2019年06月05日 11时35分59秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef DSP_COMMUNICATION_H
#define DSP_COMMUNICATION_H

#include <mcapi-2.0/mcapi.h>

typedef struct dsp_comminication{
	unsigned int domain;
	unsigned int node;
	unsigned int port;
	mcapi_endpoint_t endpoint;
} DSP_COM;

/* Send this structure to DSP */
typedef struct com_buffer{
	unsigned long paddr;	/* allocated sram memory physic address */ 
	size_t size;			/* allocated buffer size in sram */
} DSP_COM_BUF;

typedef union{
    int32_t  i;
    float    f;
    char     c;
    uint32_t nl;
} DSP_COM_BUF_TYPE32;

typedef union {
    int64_t    i;
    double     f;
    uint64_t   nl;
} DSP_COM_BUF_TYPE64; 

typedef struct dsp_com_handle {
	int sram_fd;		/* sram fd to open/mmap/munmap/close memory in L2 */
	char *sram_vaddr;	/* virtual address mapped in sram */
	size_t sram_size;	/* mmaped size in sram */
	DSP_COM master;		/* MCAPI communication master point */
	DSP_COM slave;		/* MCAPI communication slave point */
	DSP_COM_BUF buf;	/* msg to send to DSP through MCAPI */
} DSP_COM_HANDLE;

int dsp_com_open(int connection_id, size_t sram_size, int block_mode);
void dsp_com_close(int fd);
void dsp_com_write(int fd, void *value, size_t data_size, int offset);
//void dsp_com_read(DSP_COM_HANDLE *handle, void *value, size_t data_size, int block_mode);

#endif


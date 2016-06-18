/*********************************************************************
 *                     openNetVM
 *              https://sdnfv.github.io
 *
 *   BSD LICENSE
 *
 *   Copyright(c)
 *            2015-2016 George Washington University
 *            2015-2016 University of California Riverside
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * monitor.c - an example using onvm. Print a message each p package received
 ********************************************************************/

#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/queue.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <sys/time.h>

#include <rte_common.h>
#include <rte_mbuf.h>
#include <rte_ip.h>

#include "onvm_nflib.h"
#include "onvm_pkt_helper.h"

#include "rules.h"
#include "parser.h"
#include "gpu-match.h"

#include <pthread.h>
#include <time.h>

#define NF_TAG "nids"

#define HEADER 54
#define MAX_LEN 1600
#define PKTS_NUM 10
#define ITERATION 10

/* Struct that contains information about this NF */
struct onvm_nf_info *nf_info;

/* number of package between each print */
static uint32_t print_delay = 1000000;

//static double time_cpu = 0.0;
float time_gpu = 0.0;

static uint32_t destination;

int blockNum, threadNum, batchSize;
RuleSetRoot *rsr;
/*char *packets;
uint16_t *packets_len;
char *packets2;
uint16_t *packets_len2;*/
char **packets;
uint16_t **packets_len;

uint16_t *acGPU;
char *pkt_cuda;
uint16_t *pkt_len;
uint16_t *res;
int *batch;

struct timespec ts1, ts2;

uint16_t pkt_calculate;
int flag_counter;
int flag_packets;
int flag_start;

/*
 * Print a usage message
 */
static void
usage(const char *progname) {
        printf("Usage: %s [EAL args] -- [NF_LIB args] -- -d <destination> -p <print_delay>\n\n", progname);
}

/*
 * Parse the application arguments.
 */
static int
parse_app_args(int argc, char *argv[], const char *progname) {
        int c, dst_flag = 0;

        while ((c = getopt(argc, argv, "d:p:")) != -1) {
                switch (c) {
                case 'd':
                        destination = strtoul(optarg, NULL, 10);
                        dst_flag = 1;
                        break;
                case 'p':
                        print_delay = strtoul(optarg, NULL, 10);
                        break;
                case '?':
                        usage(progname);
                        if (optopt == 'd')
                                RTE_LOG(INFO, APP, "Option -%c requires an argument.\n", optopt);
                        else if (optopt == 'p')
                                RTE_LOG(INFO, APP, "Option -%c requires an argument.\n", optopt);
                        else if (isprint(optopt))
                                RTE_LOG(INFO, APP, "Unknown option `-%c'.\n", optopt);
                        else
                                RTE_LOG(INFO, APP, "Unknown option character `\\x%x'.\n", optopt);
                        return -1;
                default:
                        usage(progname);
                        return -1;
                }
        }

        if (!dst_flag) {
                RTE_LOG(INFO, APP, "Simple Forward NF requires destination flag -d.\n");
                return -1;
        }

        return optind;
}

static struct timespec diff(struct timespec start, struct timespec end)
{
	struct timespec temp;
	if((end.tv_nsec - start.tv_nsec) < 0)
	{
		temp.tv_sec = end.tv_sec - start.tv_sec - 1;
		temp.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
	}
	else
	{
		temp.tv_sec = end.tv_sec - start.tv_sec;
		temp.tv_nsec = end.tv_nsec - start.tv_nsec;
	}
	return temp;
}

static void 
pkt_to_nids(struct rte_mbuf* pkt)
{
	// parser packet for NIDS
	//printf("######PKT\n");
	//char *content;
	char *ptr;
	//uint16_t i;
	if(flag_counter == batchSize)
	{
	//	printf("pkt_to_nids: flag_packet: %d\n", flag_packets);
		flag_start = 1;
		flag_counter = 0;
		flag_packets++;
	}
	if(flag_packets == PKTS_NUM) flag_packets = 0;
	ptr = (char *)pkt->buf_addr + pkt->data_off + HEADER;
/*	for(i = 0; i < pkt->data_len - HEADER; i++)
	{
		packets[flag_packets][flag_counter * MAX_LEN + i] = *(ptr);
		ptr++;
	}
*/	memcpy((packets[flag_packets] + flag_counter * MAX_LEN), ptr, pkt->data_len - HEADER);
	packets_len[flag_packets][flag_counter] = pkt->data_len - HEADER;
}

/*
 * This function displays stats. It uses ANSI terminal codes to clear
 * screen when called. It is called from a single non-master
 * thread in the server process, when the process is run with more
 * than one lcore enabled.
 */
/*static void
do_stats_display(struct rte_mbuf* pkt) {
        const char clr[] = { 27, '[', '2', 'J', '\0' };
        const char topLeft[] = { 27, '[', '1', ';', '1', 'H', '\0' };
        static int pkt_process = 0;
        struct ipv4_hdr* ip;

        pkt_process += print_delay;

        // Clear screen and move to top left 
        printf("%s%s", clr, topLeft);

        printf("PACKETS\n");
        printf("-----\n");
        printf("Port : %d\n", pkt->port);
        printf("Size : %d\n", pkt->pkt_len);
        printf("N°   : %d\n", pkt_process);
        printf("\n\n");

        ip = onvm_pkt_ipv4_hdr(pkt);
        if (ip != NULL) {
                onvm_pkt_print(pkt);
        } else {
                printf("No IP4 header found\n");
        }
}
*/
/*
static void
printresults(uint16_t *res)
{
		printf("\n\n------Results\n");
		int i;
		for(i = 0; i < blockNum * threadNum; i++)
		{
//			if(res[i] != 0) printf("---thread: %d -> pattern: %d\n", i, res[i]);
			//printf("%d -- %d\n", i, res[i]);
		}
}
*/
static int
packet_handler(struct rte_mbuf* pkt, struct onvm_pkt_meta* meta) {
		pkt_to_nids(pkt);

		meta->action = ONVM_NF_ACTION_DROP;
        meta->destination = destination;
		
		flag_counter++;
        return 0;
}

static int nids_match(int tmp)
{
	int t;
	int last_flag_packets = 0;
	if(tmp == 0)
	{
		; // nonsense
	}
	sleep(8);
	//int flag = 0;
	while(flag_start == 0)
	{
		//printf("-------------flag_start %d\n", flag_start);
		usleep(10);
	}
	clock_gettime(CLOCK_MONOTONIC, &ts1);
	while(last_flag_packets == flag_packets)
	{
		usleep(10);
		clock_gettime(CLOCK_MONOTONIC, &ts1);
	}
	//while(pkt_calculate != ITERATION)
	while(1)
	{
		if(last_flag_packets != flag_packets)
		{
			last_flag_packets = flag_packets;
			if(flag_packets == 0) t = PKTS_NUM;
			else t = flag_packets;

			//uint16_t *results;
			
			//printf("------------batch num: %d\n", pkt_calculate);	
			//results = gpumatch(rsr, packets[t], packets_len[t], blockNum, threadNum, batchSize, acGPU, pkt_cuda, pkt_len, res);
			gpumatch(rsr, packets[t], packets_len[t], blockNum, threadNum, batchSize, acGPU, pkt_cuda, pkt_len, res);
			//printresults(results);
			pkt_calculate++;
			//free(results);
			//memset(packets[t], 0, batchSize * MAX_LEN * sizeof(char));
			//memset(packets_len[t], 0, batchSize * sizeof(uint16_t));

		}
		usleep(10);
	}
	// ...
	return 0;
}

int main(int argc, char *argv[]) {
        int arg_offset;

        const char *progname = argv[0];

        if ((arg_offset = onvm_nf_init(argc, argv, NF_TAG)) < 0)
                return -1;
        argc -= arg_offset;
        argv += arg_offset;

        if (parse_app_args(argc, argv, progname) < 0)
                rte_exit(EXIT_FAILURE, "Invalid command-line arguments\n");

		// NIDS
		// Init ...
		flag_start = 0;
		flag_counter = 0;
		flag_packets = 0;
		blockNum = 11;
		threadNum = 1024;
		batchSize = 983040;
		pkt_calculate = 0;

/*		packets = (char *)malloc(batchSize * blockNum * threadNum * MAX_LEN * sizeof(char));
		packets_len = (uint16_t *)malloc(batchSize * blockNum * threadNum * sizeof(uint16_t));
		memset(packets, 0, batchSize * blockNum * threadNum * MAX_LEN * sizeof(char));
		memset(packets_len, 0, batchSize * blockNum * threadNum * sizeof(uint16_t));
		packets2 = (char *)malloc(batchSize * blockNum * threadNum * MAX_LEN * sizeof(char));
		packets_len2 = (uint16_t *)malloc(batchSize * blockNum * threadNum * sizeof(uint16_t));
		memset(packets2, 0, batchSize * blockNum * threadNum * MAX_LEN * sizeof(char));
		memset(packets_len2, 0, batchSize * blockNum * threadNum * sizeof(uint16_t));
*/
		int i = 0;
		packets = (char **)malloc(PKTS_NUM * sizeof(char *));
		for(i = 0; i < PKTS_NUM;i ++)
		{
			packets[i] = (char *)malloc(batchSize * MAX_LEN * sizeof(char));
		}
		packets_len = (uint16_t **)malloc(PKTS_NUM * sizeof(uint16_t *));
		for(i = 0; i < PKTS_NUM;i ++)
		{
			packets_len[i] = (uint16_t *)malloc(batchSize * sizeof(uint16_t));
		}

		char rules_path[20]; // "community.rules";
		rules_path[0] = 't';
		rules_path[1] = 'r';
		rules_path[2] = '\0';

		ListRoot *listroot;
		listroot = configrules((char *)rules_path);
		precreatearray(listroot);
		test(listroot);

		rsr = listroot->TcpListRoot->prmGeneric->rsr;
		gpuinit(rsr, blockNum, threadNum, batchSize, &acGPU, &pkt_cuda, &pkt_len, &res);

        // Create matching thread
		int ret;
		pthread_t pid;
		ret = pthread_create(&pid, NULL, (void *)&nids_match, NULL);
		if(ret != 0)
		{
			printf("!!!!!!Error: create pthread error!\n");
			return 0;
		}


		// Run...
		onvm_nf_run(nf_info, &packet_handler);
        printf("If we reach here, program is ending");
		
	clock_gettime(CLOCK_MONOTONIC, &ts2);
//	printf("stop!!\n");
	struct timespec temp = diff(ts1, ts2);
	//double mpps = (double)(batchSize * ITERATION) / (double)(temp.tv_sec * 1000000 + temp.tv_nsec / 1000);
	double mpps = (double)(batchSize * pkt_calculate) / (double)(temp.tv_sec * 1000000 + temp.tv_nsec / 1000);
	double gbps = (mpps * 1000 * 8) / 1000;
	printf("\n");
	printf("BatchSize is %d, batchNum is %d\n", batchSize, pkt_calculate);
	printf("Throughput is %lf Gbps\n", gbps);
		// Free...
		gpufree(0, acGPU, pkt_cuda, pkt_len, res);
		freeall(listroot);
		printf("End.\n");
        return 0;
}

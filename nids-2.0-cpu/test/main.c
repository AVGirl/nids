#include <stdio.h>
#include <stdlib.h>
#include <rules.h>
#include <parser.h>
#include <gpu-match.h>
#include <time.h>
#include <assert.h>

#define MAX_LEN 1600
#define ITERATION 10
#define PKTS_LEN 1518

int blockNum, threadNum, batchSize;
char *packets;
uint16_t *len;

void gen_pkts()
{
	packets = (char *)malloc(batchSize * MAX_LEN * sizeof(char));
	len = (uint16_t *)malloc(batchSize * sizeof(uint16_t));
	
	int i, j, k;
	for(i = 0; i < batchSize; i++)
	{
		k = 0;
		for(j = 0; j < PKTS_LEN; j++)
		{
			packets[i * MAX_LEN + j] = (char)(97 + k);
			//printf("%c", (char)(97 + k));
			if(k == 25) k = -1;
			k++;
		}
		len[i] = PKTS_LEN;
	}
}

struct timespec diff(struct timespec start, struct timespec end)
{
	struct timespec temp;
	if ((end.tv_nsec - start.tv_nsec) < 0) {
		temp.tv_sec = end.tv_sec - start.tv_sec - 1;
		temp.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
	} else {
		temp.tv_sec = end.tv_sec - start.tv_sec;
		temp.tv_nsec = end.tv_nsec - start.tv_nsec;
	}
	return temp;
}


int main(int argc, char *argv[])
{
	if (argc == 1) {
		blockNum = 15;
		threadNum = 1024;
		batchSize = 2 * 15 * 1024;
	} else {
		assert(argc == 4);
		blockNum = atoi(argv[1]);
		threadNum = atoi(argv[2]);
		batchSize = atoi(argv[3]);
	}

	printf("blockNum = %d, threadNum = %d, batchSize = %d\n",
		blockNum, threadNum, batchSize);

	uint16_t *ptr_acGPU;
	char *ptr_pkt;
	uint16_t *ptr_pkt_len;
	uint16_t *ptr_res;

	ListRoot *listroot;
//	listroot = configrules("community.rules");
	listroot = configrules("test-rules");
	precreatearray(listroot);
	test(listroot);

	gen_pkts();
	
	RuleSetRoot *rsr = listroot->TcpListRoot->prmGeneric->rsr;
	gpuinit(rsr, blockNum, threadNum, batchSize, &ptr_acGPU, &ptr_pkt, &ptr_pkt_len, &ptr_res);
/*	char packets[10];
	packets[0] = 'h';
	packets[1] = 'e';
	packets[2] = 'r';
	packets[3] = 's';
	packets[4] = 't';
	packets[5] = '\0';
	uint16_t len[20];
	len[0] = 5;
*/	gpumatch(rsr, packets, len, blockNum, threadNum, batchSize, ptr_acGPU, ptr_pkt, ptr_pkt_len, ptr_res);
	int i = 0;
	struct timespec ts1, ts2;

	clock_gettime(CLOCK_MONOTONIC, &ts1);
	while(i < ITERATION)
	{
		gpumatch(rsr, packets, len, blockNum, threadNum, batchSize, ptr_acGPU, ptr_pkt, ptr_pkt_len, ptr_res);
		i++;
	}
	clock_gettime(CLOCK_MONOTONIC, &ts2);

	printf("%lld.%.9ld\n", (long long)ts1.tv_sec, ts1.tv_nsec);
	printf("%lld.%.9ld\n", (long long)ts2.tv_sec, ts2.tv_nsec);
	struct timespec temp = diff(ts1, ts2);
	printf("batchSize = %d. sec = %d, nsec = %d\n", batchSize, temp.tv_sec, temp.tv_nsec);
	double mpps = (double)(batchSize * ITERATION)/ (double)(temp.tv_sec * 1000000 + temp.tv_nsec/1000);
	double gbps = (mpps * PKTS_LEN * 8)/1000;
	printf("Throughput is %lf Mpps, %lf Gbps\n", mpps, gbps); 

	gpufree(0, ptr_acGPU, ptr_pkt, ptr_pkt_len, ptr_res);
	freeall(listroot);
	printf("End.\n");
	return 0;
}

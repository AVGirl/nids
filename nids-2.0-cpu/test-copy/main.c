#include <stdio.h>
#include <stdlib.h>
#include <rules.h>
#include <parser.h>
#include <gpu-match.h>

#define MAX_LEN 1500

int blockNum, threadNum, batchSize;
char *packets;
uint16_t *len;

void gen_pkts()
{
	packets = (char *)malloc(blockNum * threadNum * batchSize * MAX_LEN * sizeof(char));
	len = (uint16_t *)malloc(blockNum * threadNum * batchSize * sizeof(uint16_t));
	
	int i, j, k;
	for(i = 0; i < blockNum * threadNum * batchSize; i++)
	{
		k = 0;
		for(j = 0; j < MAX_LEN; j++)
		{
			packets[i * MAX_LEN + j] = (char)(97 + k);
			//printf("%c", (char)(97 + k));
			if(k == 25) k = -1;
			k++;
		}
		len[i] = MAX_LEN;
	}
}

int main()
{
	uint16_t *ptr_acGPU;
	AcNodeGPU *ptr_contPatt;
	char *ptr_pkt;
	uint16_t *ptr_pkt_len;
	uint16_t *ptr_res;
	int *batch;

	blockNum = 15;
	threadNum = 1024;
	batchSize = 2;

	ListRoot *listroot;
//	listroot = configrules("community.rules");
	listroot = configrules("test-rules");
	precreatearray(listroot);
	test(listroot);

	gen_pkts();
	
	RuleSetRoot *rsr = listroot->TcpListRoot->prmGeneric->rsr;
	gpuinit(rsr, blockNum, threadNum, batchSize, &ptr_acGPU, &ptr_contPatt, &ptr_pkt, &ptr_pkt_len, &ptr_res, &batch);
/*	char packets[10];
	packets[0] = 'h';
	packets[1] = 'e';
	packets[2] = 'r';
	packets[3] = 's';
	packets[4] = 't';
	packets[5] = '\0';
	uint16_t len[20];
	len[0] = 5;
*/	gpumatch(rsr, packets, len, blockNum, threadNum, batchSize, ptr_acGPU, ptr_contPatt, ptr_pkt, ptr_pkt_len, ptr_res, batch);
	int i = 0;
	while(i < 10)
	{
		gpumatch(rsr, packets, len, blockNum, threadNum, batchSize, ptr_acGPU, ptr_contPatt, ptr_pkt, ptr_pkt_len, ptr_res, batch);
		i++;
	}
	gpufree(0, ptr_acGPU, ptr_contPatt, ptr_pkt, ptr_pkt_len, ptr_res, batch);
	freeall(listroot);
	printf("End.\n");
	return 0;
}

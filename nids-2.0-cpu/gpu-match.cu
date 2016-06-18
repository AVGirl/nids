#include <stdio.h>
#include <stdlib.h>
#include <rules.h>

#define MAX_LEN 1600

__global__ static void match(uint16_t *acArray, char *packets, uint16_t *packets_len, uint16_t *result, int batchSize)
{
	//printf("GPU match\n");
	uint16_t len;
	uint16_t contId[20] = {0};
	int tmp;
	int state = 0;
	int i = 0, j = 0, k = 0, batchNum = 0;
	char content;

	// Multi-threads for many packets. One packet each thread.
	int bid = blockIdx.x;
	int tid = threadIdx.x;
	i = tid + bid * blockDim.x;

	for(batchNum = 0; i + batchNum * blockDim.x * gridDim.x < batchSize; batchNum++)
	{
	i = i + batchNum * blockDim.x * gridDim.x;
	len = packets_len[i];
	for(k = 0; k < len; k++)
	{
		content = packets[MAX_LEN * i + k];
		while(1)
		{
			tmp = acArray[257 * state + ((int)content - 0)];
			if(tmp != 0)
			{
				if(acArray[257 * tmp + 0] != 0)
				{
					contId[j] = acArray[257 * tmp + 0];
					j = 1;
				}
				state = tmp;
				break;
			}
			else
			{
				if(state == 0) break;
				else state = acArray[257 * state + 256];
			}
		}
		if(content == '\0')	break;
	}
	
	
	//__syncthreads();

//	printf("Finish...");
	if(j == 0)
	{	//printf("!!!!!!j==0\n");
		result[i] = 0;
	}
	else
	{	//printf("!!!!!!j!=0 contId[0]: %d\n", contId[0]);
		result[i] = contId[0];
	}
	}
}

extern "C"
void gpuinit(RuleSetRoot *rsr, int blockNum, int threadNum, int batchSize, uint16_t **acGPU, char **pkt, uint16_t **pkt_len, uint16_t **res)
{
	int n;
	n = batchSize;
	
	cudaMalloc((void **)acGPU, MAX_STATE * 257 * sizeof(uint16_t));
	cudaMalloc((void **)pkt, n * MAX_LEN * sizeof(char));
	cudaMalloc((void **)pkt_len, n * sizeof(uint16_t));
	cudaMalloc((void **)res, n * sizeof(uint16_t));

	cudaMemcpyAsync(*(acGPU), rsr->acGPU, MAX_STATE * 257 * sizeof(uint16_t), cudaMemcpyHostToDevice);

	cudaDeviceProp deviceProp;
	int devID = 0;
	cudaSetDevice(devID);
	cudaGetDeviceProperties(&deviceProp, devID);
	printf("\n\n******GPU Device %s\n", deviceProp.name);
}

extern "C"
void gpufree(int k, uint16_t *acGPU, char *pkt, uint16_t *pkt_len, uint16_t *res)
{
/*	uint16_t *acGPU;
	AcNodeGPU *contPatt;
	char *pkt;
	uint16_t *pkt_len;
	uint16_t *res;

	acGPU = *ptr_acGPU;
	contPatt = *ptr_contPatt;
	pkt = *ptr_pkt;
	pkt_len = *ptr_pkt_len;
	res = *ptr_res;
*/
	if(k == 0) 
		printf("######GPU-Free\n");
	cudaFree(acGPU);
	cudaFree(pkt);
	cudaFree(pkt_len);
	cudaFree(res);
}

extern "C"
uint16_t *gpumatch(RuleSetRoot *rsr, char *packets, int *packets_len, int blockNum, int threadNum, int batchSize, uint16_t *acGPU, char *pkt, uint16_t *pkt_len, uint16_t *res)
{
	int n;
	n = batchSize;

	cudaMemcpyAsync(pkt, packets, n * MAX_LEN * sizeof(char), cudaMemcpyHostToDevice);
	cudaMemcpyAsync(pkt_len, packets_len, n * sizeof(uint16_t), cudaMemcpyHostToDevice);

	uint16_t *results;
	results = (uint16_t *)malloc(n * sizeof(uint16_t));
	memset(results, 0, n * sizeof(uint16_t));

	match<<<blockNum, threadNum>>>(acGPU, pkt, pkt_len, res, batchSize);

	cudaMemcpyAsync(results, res, n * sizeof(uint16_t), cudaMemcpyDeviceToHost);
	
	/*cudaError_t err = cudaGetLastError();
	if(cudaSuccess != err)
	{
		printf("!!!!!!cudaGetLastError, %s\n", cudaGetErrorString(err));
		//exit(EXIT_FAILURE);
	}*/
//	printf("\n\n\n######B * T %d\n#####gpu time %f(ms)\n", blockNum * threadNum, time_gpu);
/*	printf("######Matching Results:\n");
	int i;
	for(i = 0; i < n; i++)
	{
		if(results[i] != 0) printf("%4d\t%d\n", i, results[i]);
	}
	printf("\n");
*/
	return results;
}

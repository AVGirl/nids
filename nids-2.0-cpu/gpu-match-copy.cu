#include <stdio.h>
#include <stdlib.h>
#include <rules.h>

#define MAX_LEN 1500

__global__ static void match(uint16_t *acArray, AcNodeGPU *contPatt, char *packets, uint16_t *packets_len, uint16_t *result)
{
	printf("GPU match\n");
	uint16_t len;
	uint16_t contId[20] = {0};
	int tmp;
	int state = 0;
	int i = 0, j = 0, k = 0;
	char content;

	// Multi-threads for many packets. One packet each thread.
	int bid = blockIdx.x;
	int tid = threadIdx.x;
	i = tid + bid * blockDim.x;

	len = packets_len[i];
	for(k = 0; k < len + 1; k++)
	{
		content = packets[MAX_LEN * i + k];
		while(1)
		{
			tmp = acArray[257 * state + ((int)content - 0)];
			if(tmp != 0)
			{
				if(acArray[257 * tmp + 0] != 0)
				{
					contId[j++] = acArray[257 * tmp + 0];
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
	
//	__syncthreads();

	printf("Finish...");
	if(j == 0)
	{	printf("!!!!!!j==0\n");
		result[i] = 0;
	}
	else
	{	printf("!!!!!!j!=0 contId[0]: %d\n", contId[0]);
		result[i] = contId[0];
	}
}

/*__global__ static void match(uint16_t *acArray, AcNodeGPU *contPatt, char *packets, int n, uint16_t *result)
{
	uint16_t contId[10] = {0};
	int tmp;
	int state = 0;
	int i = 0, j = 0, k = 0;

	// Single thread for many packets 
	for(i = 0; i < n; i++)
	{
		for(k = 0; k < LEN; k++)
		{
			char content = packets[LEN * i + k];
			while(1)
			{
				tmp = acArray[257 * state + ((int)content - 0)];
				if(tmp != 0)
				{
					if(acArray[257 * tmp + 0] != 0)
					{
						contId[j++] = acArray[257 * tmp + 0];
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
	}

	__syncthreads();
	for(i = 0; i < 10; i++) result[i] = contId[i];
}*/


/*__device__ uint16_t *acGPU;
__device__ AcNodeGPU *contPatt;
__device__ char *pkt;
__device__ uint16_t *pkt_len;
__device__ uint16_t *res;
__device__ int n;
*/

extern "C"
void gpuinit(RuleSetRoot *rsr, int blockNum, int threadNum, uint16_t **acGPU, AcNodeGPU **contPatt, char **pkt, uint16_t **pkt_len, uint16_t **res)
{
/*	uint16_t *acGPU;
	AcNodeGPU *contPatt;
	char *pkt;
	uint16_t *pkt_len;
	uint16_t *res;*/
	int n = blockNum * threadNum;
	
/*	uint16_t *tmp_acGPU;
	AcNodeGPU *tmp_contPatt;
	char *tmp_pkt;
	uint16_t *tmp_pkt_len;
	uint16_t *tmp_res;
	int tmp_n = blockNum * threadNum;
*/	
	cudaMalloc((void **)acGPU, MAX_STATE * 257 * sizeof(uint16_t));
	cudaMalloc((void **)contPatt, rsr->nodeNum * sizeof(acGPU));
	cudaMalloc((void **)pkt, n * MAX_LEN * sizeof(char));
	cudaMalloc((void **)pkt_len, n * sizeof(uint16_t));
	cudaMalloc((void **)res, n * sizeof(uint16_t));

	/*cudaMalloc((void **)&tmp_acGPU, MAX_STATE * 257 * sizeof(uint16_t));
	cudaMalloc((void **)&tmp_contPatt, rsr->nodeNum * sizeof(acGPU));
	cudaMalloc((void **)&tmp_pkt, tmp_n * MAX_LEN * sizeof(char));
	cudaMalloc((void **)&tmp_pkt_len, tmp_n * sizeof(uint16_t));
	cudaMalloc((void **)&tmp_res, tmp_n * sizeof(uint16_t));
	
	cudaMemcpyToSymbol(acGPU, &tmp_acGPU, MAX_STATE * 257 * sizeof(uint16_t));
	cudaMemcpyToSymbol(contPatt, &tmp_contPatt, rsr->nodeNum *sizeof(acGPU));
	cudaMemcpyToSymbol(pkt, &tmp_pkt, tmp_n * MAX_LEN * sizeof(char));
	cudaMemcpyToSymbol(pkt_len, &tmp_pkt_len, tmp_n * sizeof(uint16_t));
	cudaMemcpyToSymbol(res, &tmp_res, tmp_n * sizeof(uint16_t));
	cudaMemcpyToSymbol(n, &tmp_n, sizeof(int));
*/
	cudaDeviceProp deviceProp;
	int devID = 0;
	cudaSetDevice(devID);
	cudaGetDeviceProperties(&deviceProp, devID);
	printf("\n\n******GPU Device %s\n", deviceProp.name);
}

extern "C"
void gpufree(int k, uint16_t *acGPU, AcNodeGPU *contPatt, char *pkt, uint16_t *pkt_len, uint16_t *res)
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
	cudaFree(contPatt);
	cudaFree(pkt);
	cudaFree(pkt_len);
	cudaFree(res);
}

extern "C"
uint16_t *gpumatch(RuleSetRoot *rsr, char *packets, int *packets_len, int blockNum, int threadNum, uint16_t *acGPU, AcNodeGPU *contPatt, char *pkt, uint16_t *pkt_len, uint16_t *res)
{
	int n;
	n = blockNum * threadNum;

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
	cudaMemcpy(acGPU, rsr->acGPU, MAX_STATE * 257 * sizeof(uint16_t), cudaMemcpyHostToDevice);
	cudaMemcpy(contPatt, rsr->contPattMatch, rsr->nodeNum * sizeof(AcNodeGPU), cudaMemcpyHostToDevice);
	cudaMemcpy(pkt, packets, n * MAX_LEN * sizeof(char), cudaMemcpyHostToDevice);
	cudaMemcpy(pkt_len, packets_len, n * sizeof(uint16_t), cudaMemcpyHostToDevice);

	uint16_t *results;
	results = (uint16_t *)malloc(n * sizeof(uint16_t));
	memset(results, 0, n * sizeof(uint16_t));

	float time_gpu = 0.0;
	cudaEvent_t start_gpu, stop_gpu;
	cudaEventCreate(&stop_gpu);
	cudaEventCreate(&start_gpu);
	cudaEventRecord(start_gpu, 0);

	match<<<blockNum, threadNum>>>(acGPU, contPatt, pkt, pkt_len, res);

	cudaMemcpy(results, res, n * sizeof(uint16_t), cudaMemcpyDeviceToHost);
	
	cudaEventRecord(stop_gpu, 0);
	cudaEventSynchronize(start_gpu);
	cudaEventSynchronize(stop_gpu);
	cudaEventElapsedTime(&time_gpu, start_gpu, stop_gpu);

	cudaEventDestroy(start_gpu);
	cudaEventDestroy(stop_gpu);

	printf("\n\n\n#####gpu time %f(ms)\n", time_gpu);
	printf("######Matching Results:\n");
	int i;
	for(i = 0; i < n; i++)//n; i++)
	{
		if(results[i] != 0) printf("%4d\t%d\n", i, results[i]);
	}
	printf("\n");

	return results;
}

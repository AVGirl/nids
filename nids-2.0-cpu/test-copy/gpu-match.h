extern void gpuinit(RuleSetRoot *rsr, int blockNum, int threadNum, int batchSize, uint16_t **acGPU, AcNodeGPU **contPatt, char **pkt, uint16_t **pkt_len, uint16_t **res, int **batch);

extern void gpufree(int k, uint16_t *acGPU, AcNodeGPU *contPatt, char *pkt, uint16_t *pkt_len, uint16_t *res, int *batch);

extern uint16_t *gpumatch(RuleSetRoot *rsr, char *packets, uint16_t *packets_len, int blockNum, int threadNum, int batchSize, uint16_t *acGPU, AcNodeGPU *contPatt, char *pkt, uint16_t *pkt_len, uint16_t *res, int *batch);

extern void gpuinit(RuleSetRoot *rsr, int blockNum, int threadNum, int batchSize, uint16_t **acGPU, char **pkt, uint16_t **pkt_len, uint16_t **res);

extern void gpufree(int k, uint16_t *acGPU, char *pkt, uint16_t *pkt_len, uint16_t *res);

extern uint16_t *gpumatch(RuleSetRoot *rsr, char *packets, uint16_t *packets_len, int blockNum, int threadNum, int batchSize, uint16_t *acGPU, char *pkt, uint16_t *pkt_len, uint16_t *res);

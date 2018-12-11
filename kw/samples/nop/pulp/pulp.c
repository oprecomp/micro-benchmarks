// Copyright (c) 2017 OPRECOMP Project
// Fabian Schuiki <fschuiki@iis.ee.ethz.ch>

#include "rt/rt_api.h"
#include <stdio.h>
#include <stdint.h>


int main(uint64_t wed) {
	printf("[%d, %d] Hello from the loaded PULP binary!\n", rt_cluster_id(), rt_core_id());
	printf("[%d, %d] WED is 0x%08lx%08lx\n", rt_cluster_id(), rt_core_id(), (uint32_t)(wed >> 32), (uint32_t)wed);
	return 0;
}

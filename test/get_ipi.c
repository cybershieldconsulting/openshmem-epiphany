/*
 * Copyright (c) 2016 U.S. Army Research laboratory. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 * 
 * This software was developed by Brown Deer Technology, LLC. with Copyright
 * assigned to the US Army Research laboratory as required by contract.
 */

/*
 * Performance test for shmem_get (latency and bandwidth)
 *
 */

#define SHMEM_USE_IPI_GET
#include <shmem.h>
#include "ctimer.h"

#define NELEMENT 8192
#define NLOOP 1//0000
#define INV_GHZ 1.66666667f // 1/0.6 GHz

long pSync[SHMEM_REDUCE_SYNC_SIZE] = { SHMEM_SYNC_VALUE };
int pWrk[SHMEM_REDUCE_MIN_WRKDATA_SIZE];

int main (void)
{
	ctimer_start();
	shmem_init();
	int me = shmem_my_pe();
	int npes = shmem_n_pes();

	int nxtpe = (me + 1) % npes;
	char* source = (char*)shmem_malloc(NELEMENT);
	char* target = (char*)shmem_malloc(NELEMENT);
	for (int i = 0; i < NELEMENT; i++) {
		source[i] = i + 1;
		target[i] = -90;
	}

	if (me == 0) {
		printf("# SHMEM GetMem times for variable message size\n" \
			"# Bytes\tLatency (nanoseconds)\n");
	}

	/* For int get we take average of all the times realized by a pair of PEs,
	thus reducing effects of physical location of PEs */
	for (int nelement = 1; nelement <= NELEMENT; nelement <<= 1)
	{
		shmem_barrier_all();

		unsigned int t = ctimer();
		for (int j = 0; j < NLOOP; j++) {
			shmem_getmem(target, source, nelement, nxtpe);
		}
		t -= ctimer();

		shmem_barrier_all();

		shmem_int_sum_to_all(&t, &t, 1, 0, 0, npes, pWrk, pSync);
		t = t / (npes); // Average time across all PEs for one get

		if (me == 0) {
			int bytes = nelement * sizeof(*source);
			int cycles = t / NLOOP;
			float fcycles = (float)cycles;
			int nsec = (int)(fcycles * INV_GHZ);
			printf ("%6d %7d\n", bytes, nsec);
		}
	}

	shmem_free(target);
	shmem_free(source);

	shmem_finalize();

	return 0;
}

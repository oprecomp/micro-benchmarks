// Copyright (c) 2017 OPRECOMP Project
// Fabian Schuiki <fschuiki@iis.ee.ethz.ch>
//
// This is the OPRECOMP bootloader. It runs as the initial binary on the PULP
// accelerator and waits for a work element descriptor. Once it receives such,
// it copies the PULP executable to be run into L2 and directs control flow
// there.

#include "rt/rt_api.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <alloca.h>


// The main entry point of the program, which takes the WED as an argument.
typedef void (*payload_entry_fn)(uint64_t);


// Convert a pointer in host memory space to a PULP address that will be routed
// to the host.
inline uint64_t host2local(uint64_t host) {
	return (host & (((uint64_t)1 << 48)-1)) | ((uint64_t)1 << 48);
}


// Rudimentary putc/puts for output.
#define boot_putc_address (STDOUT_BASE_ADDR + STDOUT_PUTC_OFFSET + (hal_core_id()<<3) + (hal_cluster_id()<<7))

static void boot_putc(unsigned enable, unsigned int c) {
	if (enable) {
		*(volatile unsigned int *)boot_putc_address = c;
	}
}

static void boot_puts(unsigned enable, const char *s) {
	if (enable) {
		while (*s) boot_putc(1, *s++);
	}
}

static const char boot_hex[] = "0123456789abcdef";
#define boot_puth(enable, n) { \
	if (enable) { \
		boot_putc(1, '0'); \
		boot_putc(1, 'x'); \
		for (int i = sizeof(n)*2; i > 0; --i) \
			boot_putc(1, boot_hex[((n) >> (i*4-4)) & 0xF]); \
	} \
}


static const uint16_t BUFFER_SIZE = 2048;

/// The work element descriptor that offloads a PULP binary and contains the WED
/// for that binary.
struct wed {
	uint64_t sec_ptr;
	uint32_t sec_num;
	uint32_t wed_sym;
	uint64_t wed;
};

/// A section of the PULP binary to be loaded.
struct bin_sec {
	uint64_t src;
	uint32_t dst;
	uint32_t src_sz;
	uint32_t dst_sz;
};

/// A custom implementation of memset, just in case.
void *memset(void *ptr, int value, size_t sz) {
	void *end = ptr + sz;
	while (ptr + sizeof(int) - 1 < end) {
		*(int*)ptr = value;
		ptr += sizeof(int);
	}
	while (ptr < end) {
		*(char*)ptr = (char)value;
		++ptr;
	}
	return ptr;
}

// Retrieve a job from the WED FIFO. Blocks and waits for an event if no WED is
// in the FIFO.
static uint64_t get_job() {
	uint32_t job_lsb;
	while(1) {
		job_lsb = *(volatile uint32_t *)0x1a120000;
		if (job_lsb != 0) break;
		eu_evt_maskWaitAndClr(1 << 24);
	}
	uint32_t job_msb = *(volatile uint32_t *)0x1a120004;
	uint64_t job = (((uint64_t)job_msb) << 32) | (uint64_t)job_lsb;
	return job;
}

// Pop a job off the WED FIFO.
static void pop_job() {
	*(volatile uint32_t *)0x1a120000 = 0;
}


int main() {
	int id;
	unsigned int en = *(volatile unsigned int *)boot_putc_address;

	if (rt_cluster_id() != 0 || rt_core_id() != 0) {
		((payload_entry_fn)0x1c000080)(0);
		asm volatile ("jr %0" :: "r" (0x1a000080));
		return 0;
	}

	// Wait for a work element descriptor from the host. Use the DMA to fetch
	// the WED contents.
	boot_puts(en, "boot: Waiting for offload\n");
	uint64_t job = get_job();

	boot_puts(en, "boot: Fetching WED ");
	boot_puth(en, job);
	boot_putc(en, '\n');
	struct wed wed;
	id = plp_dma_memcpy(host2local(job), (uint32_t)&wed, sizeof(wed), 1);
	plp_dma_wait(id);

	// Copy over the list of sections that need to be loaded.
	size_t sec_size = wed.sec_num * sizeof(struct bin_sec);
	struct bin_sec *secs = alloca(sec_size);
	// struct bin_sec *secs = rt_alloc(RT_ALLOC_FC_DATA, sec_size);
	boot_puts(en, "boot: Copying section table from ");
	boot_puth(en, wed.sec_ptr);
	boot_puts(en, " (");
	boot_puth(en, sec_size);
	boot_puts(en, " bytes)\n");
	id = plp_dma_memcpy(host2local(wed.sec_ptr), (uint32_t)secs, sec_size, 1);
	plp_dma_wait(id);

	// Copy over each of the sections in the list. If the section targets a
	// cluster, we replicate the copy multiple times into each of the clusters.
	boot_puts(en, "boot: Loading ");
	boot_puth(en, (char)wed.sec_num);
	boot_puts(en, " sections\n");
	// void *buffer = rt_alloc(RT_ALLOC_FC_DATA, BUFFER_SIZE);
	void *buffer = alloca(BUFFER_SIZE);
	for (uint32_t i = 0; i < wed.sec_num; ++i) {
		struct bin_sec *sec = &secs[i];
		int num_copies;
		uint32_t base;

		// Decide how often this section has to be replicated.
		if (sec->dst < 0x10000000) {
			base = sec->dst + 0x10000000;
			num_copies = 1;
		} else {
			base = sec->dst;
			num_copies = 1;
		}

		for (int i = 0; i < num_copies; ++i, base += 0x400000) {
			uint64_t ptr = sec->src, end = ptr + sec->src_sz;
			uint32_t dst = base;

			boot_puts(en, "boot:   section ");
			boot_puth(en, ptr);
			boot_puts(en, " to ");
			boot_puth(en, dst);
			boot_puts(en, " (");
			boot_puth(en, (unsigned)sec->src_sz);
			boot_puts(en, " bytes)\n");

			while (ptr < end) {
				// rt_dma_flush();
				uint64_t to = (ptr + BUFFER_SIZE);
				if (to > end)
					to = end;
				uint16_t size = to - ptr;

				boot_puts(en, "boot:     [");
				boot_puth(en, ptr);
				boot_puts(en, ":");
				boot_puth(en, to);
				boot_puts(en, "] -> [");
				boot_puth(en, dst);
				boot_puts(en, "; ");
				boot_puth(en, (unsigned)size);
				boot_puts(en, "]\n");

				id = plp_dma_memcpy(host2local(ptr), (uint32_t)buffer, size, 1);
				plp_dma_wait(id);
				id = plp_dma_memcpy((uint32_t)dst, (uint32_t)buffer, size, 0);
				plp_dma_wait(id);
				ptr = to;
				dst += size;
			}

			// Pad the remaining bytes.
			if (sec->dst_sz > sec->src_sz) {
				memset(buffer, 0, BUFFER_SIZE);
				uint32_t end = base + sec->dst_sz;
				while (dst < end) {
					uint32_t to = (dst + BUFFER_SIZE);
					if (to > end)
						to = end;
					uint16_t size = to - dst;

					boot_puts(en, "boot:     zero [");
					boot_puth(en, dst);
					boot_puts(en, ":");
					boot_puth(en, to);
					boot_puts(en, "; ");
					boot_puth(en, (unsigned)size);
					boot_puts(en, "]\n");

					id = plp_dma_memcpy((uint32_t)dst, (uint32_t)buffer, size, 0);
					plp_dma_wait(id);
					dst = to;
				}
			}
		}
	}
	boot_puts(en, "boot:   loading done\n");

	// Start the binary that was just loaded.
	boot_puts(en, "boot: Launching binary\n");
	((payload_entry_fn)0x1c000080)(wed.wed);
	boot_puts(en, "boot: Binary returned\n");

	// Write "0" to the base of the CAPI space to indicate to the libcxl that we
	// are done with the job. This will later move into a dedicated register
	// next to the FIFO.
	uint64_t value = 1;
	id = plp_dma_memcpy(host2local(job + sizeof(struct wed)), (uint32_t)&value, sizeof(value), 0);
	plp_dma_wait(id);
	while (value != 2) {
		id = plp_dma_memcpy(host2local(job + sizeof(struct wed)), (uint32_t)&value, sizeof(value), 1);
		plp_dma_wait(id);
	}
	pop_job();

	// Restart the bootloader (disabled for now).
	// asm volatile ("jr %0" :: "r" (0x1a000080));
	return 0;
}

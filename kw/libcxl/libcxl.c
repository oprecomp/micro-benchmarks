// Copyright (c) 2017 OPRECOMP Project
// Fabian Schuiki <fschuiki@iis.ee.ethz.ch>

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>

#include "libcxl.h"
#include "vp/launcher.h"


struct cxl_afu_h {
	char *path;
	// A handle to the created virtual platform.
	void *gv;
	// The binding in the SoC interconnect where requests to CAPI will be routed
	// to. These requests are what we will be trying to forward into the host
	// program's user space memory.
	void *capi_binding;
	// The binding to the job FIFO where we can push WEDs for the platform to
	// process.
	void *wed_binding;
	int run;
	pthread_mutex_t run_mutex;
	pthread_cond_t run_cond;
};


/// The callback invoked by the virtual platform whenever a transaction towards
/// the CAPI memory space is made.
static void
capi_callback(void *context, void *data, void *addr, size_t size, int is_write, gv_ioreq_response_t callback, void *cb_context) {
	// printf("capi_callback: context=%p, data=%p, addr=%p, size=%lu, is_write=%d, callback=%p, cb_context=%p\n", context, data, addr, size, is_write, callback, cb_context);
	assert(context);
	struct cxl_afu_h *afu = context;
	// if (addr == (void *)0x1a600000) {
	if (addr == (void *)((uint64_t)1 << 48)) {
		printf("libcxl: PULP notified job termination\n");
		pthread_mutex_lock(&afu->run_mutex);
		--afu->run;
		pthread_cond_signal(&afu->run_cond);
		pthread_mutex_unlock(&afu->run_mutex);
	} else {
		// Unpack the address by removing the 48th bit that caused the access
		// to be routed to us. This bit is not part of the address.
		uint64_t local = (uint64_t)addr & (((uint64_t)1 << 48)-1);

		// Sign extend the address. If the topmost address bit (47th) is set,
		// set all the bits from 48 upwards to 1.
		if (local & ((uint64_t)1 << 47))
			local |= ((uint64_t)-1) << 48;

		// printf("libcxl: serving 0x%016lx (%lu bytes)\n", local, size);

		// Copy the data into the destination buffer an invoke the callback.
		if (is_write)
			memcpy((void*)local, data, size);
		else
			memcpy(data, (void*)local, size);
		gv_ioreq_t resp = {
			.state = GV_IOREQ_DONE,
			.latency = 0,
		};
		callback(cb_context, &resp);
	}
}


static struct cxl_afu_h *
afu_alloc() {
	struct cxl_afu_h *afu = calloc(1, sizeof(struct cxl_afu_h));
	pthread_mutex_init(&afu->run_mutex, NULL);
	pthread_cond_init(&afu->run_cond, NULL);
	return afu;
}


static struct cxl_afu_h *
afu_open(struct cxl_afu_h *afu) {
	assert(afu);
	assert(!afu->gv);

	// Check if an environment variable is set to override the configuration
	// file.
	char *cfg_path = getenv("OPC_PLATFORM_CONFIG");
	if (cfg_path && strlen(cfg_path) > 0) {
		cfg_path = strdup(cfg_path);
	} else {
		cfg_path = NULL;
	}

	// Fallback to the default configuration.
	if (!cfg_path) {
		cfg_path = getenv("PULP_SDK_HOME");
		if (!cfg_path || strlen(cfg_path) == 0) {
			fprintf(stderr, "error: PULP_SDK_HOME env var not set; maybe you forgot to source the SDK setup script?\n");
			exit(1);
		}
		size_t len = strlen(cfg_path);
		static const char *suffix = "/install/cfg/oprecompkw_config.json";
		char *p = calloc(len+strlen(suffix)+1, sizeof(char));
		strcpy(p, cfg_path);
		strcpy(p+len, suffix);
		cfg_path = p;
	}
	fprintf(stderr, "debug: using config file %s\n", cfg_path);

	// Initialize the virtual platform.
	gv_conf_t conf;
	gv_init(&conf);
	afu->gv = gv_create(&conf, cfg_path);
	free(cfg_path);
	if (!afu->gv) {
		fprintf(stderr, "libcxl error: unable to create virtual platform (gv_create)\n");
		goto fail;
	}

	// Configure the handles into the platform.
	afu->capi_binding = gv_ioreq_binding(afu->gv, "soc/host_injector", (void *)((uint64_t)1 << 48), (uint64_t)1 << 48, capi_callback, afu);
	// afu->capi_binding = gv_ioreq_binding(afu->gv, "soc/soc_ico", (void *)0x1a600000, 0x00100000, capi_callback, afu);
	if (afu->capi_binding == NULL) {
		fprintf(stderr, "libcxl error: unable to establish CAPI binding (gv_ioreq_binding)\n");
		goto fail;
	}

	afu->wed_binding = gv_ioreq_binding(afu->gv, "job_fifo_injector", NULL, 0, NULL, NULL);
	if (afu->wed_binding == NULL) {
		fprintf(stderr, "libcxl error: unable to establish WED FIFO binding (gv_ioreq_binding)\n");
		goto fail;
	}

	// Launch the virtual platform.
	if (gv_launch(afu->gv)) {
		fprintf(stderr, "libcxl error: unable to launch virtual platform (gv_launch)\n");
		goto fail;
	}

	return afu;

fail:
	errno = ENOSPC; // Just pretend like we did not have an AFU context available.
	cxl_afu_free(afu);
	return NULL;
}


struct cxl_afu_h *
cxl_afu_open_dev(char *path) {
	assert(path);
	struct cxl_afu_h *afu = afu_alloc();
	size_t pathlen = strlen(path)+1;
	afu->path = malloc(pathlen);
	memcpy(afu->path, path, pathlen);
	return afu_open(afu);
}


struct cxl_afu_h *
cxl_afu_open_h(struct cxl_afu_h *afu, enum cxl_views view) {
	assert(afu);
	if (view != CXL_VIEW_DEDICATED) {
		errno = ENODEV;
		return NULL;
	}
	// Just pretend like we actually opened anything.
	assert(!afu->path);
	afu->path = strdup(CXL_DEV_DIR"/afu0.0d");
	return afu_open(afu);
}


void
cxl_afu_free(struct cxl_afu_h *afu) {
	assert(afu);

	// Wait for the execution to finish.
	pthread_mutex_lock(&afu->run_mutex);
	while (afu->run > 0) {
		printf("libcxl: waiting for %d jobs to finish\n", afu->run);
		pthread_cond_wait(&afu->run_cond, &afu->run_mutex);
	}
	pthread_mutex_unlock(&afu->run_mutex);

	// Tear down the virtual platform if it was launched.
	if (afu->gv) {
		gv_destroy(afu->gv);
		afu->gv = NULL;
		afu->capi_binding = NULL;
		afu->wed_binding = NULL;
	}

	// Deallocate all the memory that is no longer needed.
	if (afu->path)
		free(afu->path);
	pthread_mutex_destroy(&afu->run_mutex);
	pthread_cond_destroy(&afu->run_cond);
	memset(afu, 0, sizeof(*afu));
	free(afu);
}


int
cxl_afu_opened(struct cxl_afu_h *afu) {
	assert(afu);
	return afu->gv == NULL;
}


// -------------------------------------------------------------------------- //
//   Enumeration functions                                                    //
// -------------------------------------------------------------------------- //


char *
cxl_afu_dev_name(struct cxl_afu_h *afu) {
	assert(afu);
	return afu->path;
}


struct cxl_afu_h *
cxl_afu_next(struct cxl_afu_h *afu) {
	if (afu) {
		cxl_afu_free(afu);
		return NULL;
	} else {
		return afu_alloc();
	}
}


// -------------------------------------------------------------------------- //
//   Attach AFU Context functions                                             //
// -------------------------------------------------------------------------- //


int
cxl_afu_attach(struct cxl_afu_h *afu, __u64 wed) {
	struct cxl_ioctl_start_work work;
	memset(&work, 0, sizeof(work));
	work.work_element_descriptor = wed;
	return cxl_afu_attach_work(afu, &work);
}


int
cxl_afu_attach_full(struct cxl_afu_h *afu, __u64 wed, __u16 num_interrupts, __u64 amr) {
	struct cxl_ioctl_start_work work;
	memset(&work, 0, sizeof(work));
	work.work_element_descriptor = wed;
	work.flags = CXL_START_WORK_NUM_IRQS | CXL_START_WORK_AMR;
	work.num_interrupts = num_interrupts;
	work.amr = amr;
	return cxl_afu_attach_work(afu, &work);
}


int
cxl_afu_attach_work(struct cxl_afu_h *afu, struct cxl_ioctl_start_work *work) {
	// Make sure we have a valid AFU and it has a binding where we can inject
	// work element descriptors.
	if (afu == NULL || afu->wed_binding == NULL) {
		errno = EINVAL;
		return -1;
	}

	// Keep track of the number of running jobs so we can block tear down until
	// everything is done.
	pthread_mutex_lock(&afu->run_mutex);
	while (afu->run > 0) {
		printf("libcxl: waiting for %d jobs to finish\n", afu->run);
		pthread_cond_wait(&afu->run_cond, &afu->run_mutex);
	}
	++afu->run;
	pthread_mutex_unlock(&afu->run_mutex);

	// Write the WED to the FIFO. We ignore pretty much all other details.
	if (gv_ioreq(afu->wed_binding, 0, (void *)&work->work_element_descriptor, (void *)0, 8, 1, NULL, NULL)) {
		errno = ENOSPC; // Just use this errno to represent the internal error.
		return -1;
	}

	return 0;
}

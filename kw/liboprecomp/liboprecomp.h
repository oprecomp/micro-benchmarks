// Copyright (c) 2017 OPRECOMP Project
// Fabian Schuiki <fschuiki@iis.ee.ethz.ch>

#ifndef __LIBOPRECOMPKW
#define __LIBOPRECOMPKW

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>


/* Type Defs */
typedef struct opc_dev *opc_dev_t;
typedef struct opc_kernel *opc_kernel_t;
typedef struct opc_kernel_section *opc_kernel_section_t;
typedef uint32_t opc_job_t;
typedef uint32_t opc_error_t;

/* Public Structs */
struct opc_kernel_section {
	void *src;
	uint32_t dst;
	uint32_t src_sz;
	uint32_t dst_sz; // destination size; >= src_sz; excess bytes are filled with 0
};

/* Errors. The lower 24 bits are reserved for error payload. */
enum opc_error {
	OPC_OK             = 0,
	OPC_ERRNO          = ((unsigned)-1) << 24,
	OPC_ALREADY_LOADED = ((unsigned)-2) << 24,
	OPC_ALREADY_OPENED = ((unsigned)-3) << 24,
	OPC_ELF_INVALID    = ((unsigned)-4) << 24,
	OPC_NOT_LOADED     = ((unsigned)-5) << 24,
	OPC_NOT_OPENED     = ((unsigned)-6) << 24,

	// OPC_ERRNO details
	OPC_ERRNO_OPEN  = OPC_ERRNO | 1,
	OPC_ERRNO_FSTAT = OPC_ERRNO | 2,
	OPC_ERRNO_MMAP  = OPC_ERRNO | 3,
	OPC_ERRNO_CLOSE = OPC_ERRNO | 4,
	OPC_ERRNO_CXL   = OPC_ERRNO | 5,

	// OPC_ELF_INVALID details
	OPC_ELF_INVALID_MAGIC             = OPC_ELF_INVALID | 1,
	OPC_ELF_INVALID_NOT_32_BIT        = OPC_ELF_INVALID | 2,
	OPC_ELF_INVALID_NOT_LITTLE_ENDIAN = OPC_ELF_INVALID | 3,
	OPC_ELF_INVALID_TOO_SHORT         = OPC_ELF_INVALID | 4,
};

opc_kernel_t opc_kernel_new();
opc_error_t opc_kernel_load_file(opc_kernel_t knl, const char *path);
opc_error_t opc_kernel_load_buffer(opc_kernel_t knl, void *buffer, size_t size, int take_ownership);
void opc_kernel_dump_info(opc_kernel_t knl, FILE *out);
size_t opc_kernel_get_num_sections(opc_kernel_t knl);
opc_kernel_section_t opc_kernel_get_section(opc_kernel_t knl, size_t index);
uint32_t opc_kernel_get_wed_symbol(opc_kernel_t knl);
void opc_kernel_free(opc_kernel_t knl);

opc_dev_t opc_dev_new();
opc_error_t opc_dev_open_any(opc_dev_t dev);
opc_error_t opc_dev_open_path(opc_dev_t dev, const char *path);
opc_error_t opc_dev_launch(opc_dev_t dev, opc_kernel_t knl, void *wed, opc_job_t *job);
void opc_dev_wait(opc_dev_t dev, opc_job_t job);
void opc_dev_wait_all(opc_dev_t dev);
void opc_dev_free(opc_dev_t dev);

const char *opc_errstr(opc_error_t err);
void opc_perror(const char *prefix, opc_error_t err);


#ifdef __cplusplus
}
#endif

#endif

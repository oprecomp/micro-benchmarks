// Copyright (c) 2017 OPRECOMP Project
// Fabian Schuiki <fschuiki@iis.ee.ethz.ch>
//
// A simple library to offload tasks to a PULP-based accelerator reachable via
// libcxl.

#include "liboprecomp.h"
#include "libcxl.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>


/* ELF Format Definitions */
struct elf32_ehdr {
	uint8_t  e_ident[16];
	uint16_t e_type;
	uint16_t e_machine;
	uint32_t e_version;
	uint32_t e_entry;
	uint32_t e_phoff;
	uint32_t e_shoff;
	uint32_t e_flags;
	uint16_t e_ehsize;
	uint16_t e_phentsize;
	uint16_t e_phnum;
	uint16_t e_shentsize;
	uint16_t e_shnum;
	uint16_t e_shstrndx;
};

struct elf32_phdr {
	uint32_t p_type;
	uint32_t p_offset;
	uint32_t p_vaddr;
	uint32_t p_paddr;
	uint32_t p_filesz;
	uint32_t p_memsz;
	uint32_t p_flags;
	uint32_t p_align;
};

struct elf32_shdr {
	uint32_t sh_name;
	uint32_t sh_type;
	uint32_t sh_flags;
	uint32_t sh_addr;
	uint32_t sh_offset;
	uint32_t sh_size;
	uint32_t sh_link;
	uint32_t sh_info;
	uint32_t sh_addralign;
	uint32_t sh_entsize;
};

struct elf32_sym {
	uint32_t st_name;
	uint32_t st_value;
	uint32_t st_size;
	uint8_t  st_info;
	uint8_t  st_other;
	uint16_t st_shndx;
};

enum elf_ident {
	EI_MAG0       = 0, // 0x7F
	EI_MAG1       = 1, // 'E'
	EI_MAG2       = 2, // 'L'
	EI_MAG3       = 3, // 'F'
	EI_CLASS      = 4, // Architecture (32/64)
	EI_DATA       = 5, // Byte Order
	EI_VERSION    = 6, // ELF Version
	EI_OSABI      = 7, // OS Specific
	EI_ABIVERSION = 8, // OS Specific
};


/* Work Element Descriptor and Binary Sections passed to the accelerator. */
struct wed {
	void *sec_ptr;
	uint32_t sec_num;
	uint32_t wed_sym; // the address of the WED in the loaded binary
	void *wed;
	volatile uint64_t result;
};

struct opc_kernel {
	void *buffer;
	size_t buffer_sz;
	enum { BUFFER_REGULAR, BUFFER_OWNED, BUFFER_MMAP } buffer_mode;
	size_t num_sections;
	struct opc_kernel_section *sections;
	uint32_t wed_sym;
};

struct opc_dev {
	struct cxl_afu_h *afu;
	pthread_mutex_t run_mutex;
	pthread_cond_t run_cond;
};


/// Create a new kernel. The kernel needs to be loaded via one of the
/// `opc_kernel_load_*` functions. Once loaded, it can be executed on a device.
/// Once execution terminates, the kernel needs to be freed.
///
/// @return Returns the allocated kernel.
opc_kernel_t
opc_kernel_new() {
	return calloc(1, sizeof(struct opc_kernel));
}


/// Deallocate the resources of a kernel. Do not deallocate a kernel that is
/// still running on a device.
///
/// @param knl The kernel to be freed.
void
opc_kernel_free(opc_kernel_t knl) {
	assert(knl);

	if (knl->buffer) {
		switch (knl->buffer_mode) {
			case BUFFER_REGULAR: break;
			case BUFFER_OWNED: free(knl->buffer); break;
			case BUFFER_MMAP: munmap(knl->buffer, knl->buffer_sz); break;
		}
	}

	if (knl->sections)
		free(knl->sections);

	// Clean up the kernel.
	memset(knl, 0, sizeof(*knl)); // makes use-after-free more obvious
	free(knl);
}


/// Parses the buffer of a kernel into its ELF sections.
static opc_error_t
parse_elf(opc_kernel_t knl) {
	assert(knl);
	assert(knl->buffer);

	// Abort if the kernel has already been loaded.
	if (knl->sections)
		return OPC_ALREADY_LOADED;

	// Parse and check the ELF header.
	struct elf32_ehdr *ehdr = knl->buffer;
	if (knl->buffer_sz < sizeof(struct elf32_ehdr))
		return OPC_ELF_INVALID_TOO_SHORT;
	if (ehdr->e_ident[EI_MAG0] != 0x7F || ehdr->e_ident[EI_MAG1] != 'E' || ehdr->e_ident[EI_MAG2] != 'L' || ehdr->e_ident[EI_MAG3] != 'F')
		return OPC_ELF_INVALID_MAGIC;
	if (ehdr->e_ident[EI_CLASS] != 1)
		return OPC_ELF_INVALID_NOT_32_BIT;
	if (ehdr->e_ident[EI_DATA] != 1)
		return OPC_ELF_INVALID_NOT_LITTLE_ENDIAN;

	// Parse and check each of the program headers.
	knl->num_sections = ehdr->e_phnum;
	knl->sections = calloc(knl->num_sections, sizeof(struct opc_kernel_section));

	for (size_t i = 0; i < knl->num_sections; ++i) {
		opc_kernel_section_t sec = &knl->sections[i];
		size_t offset = ehdr->e_phoff + i * ehdr->e_phentsize;
		struct elf32_phdr *phdr = knl->buffer + offset;
		if (knl->buffer_sz < offset + sizeof(struct elf32_phdr))
			return OPC_ELF_INVALID_TOO_SHORT;

		sec->src    = knl->buffer + phdr->p_offset;
		sec->dst    = phdr->p_vaddr;
		sec->src_sz = phdr->p_filesz;
		sec->dst_sz = phdr->p_memsz;
	}

	// Locate the section header string table.
#ifdef DEBUG
	printf("e_shstrndx: %u\n", ehdr->e_shstrndx);
	struct elf32_shdr *shstrtbl = knl->buffer + ehdr->e_shoff + ehdr->e_shstrndx * ehdr->e_shentsize;
	const char *shstr = knl->buffer + shstrtbl->sh_offset;
#endif

	// Locate the optional opc_wed symbol address within the binary.
	for (size_t i = 0; i < ehdr->e_shnum && !knl->wed_sym; ++i) {
		size_t offset = ehdr->e_shoff + i * ehdr->e_shentsize;
		struct elf32_shdr *shdr = knl->buffer + offset;
		if (knl->buffer_sz < offset + sizeof(struct elf32_shdr))
			return OPC_ELF_INVALID_TOO_SHORT;

#ifdef DEBUG
		printf("section %lu\n", i);
		printf("  sh_name: %u (%s)\n", shdr->sh_name, shstr + shdr->sh_name/*, (char *)knl->buffer + shstr[shdr->sh_name]*/);
		printf("  sh_type: %u\n", shdr->sh_type);
		printf("  sh_flags: %u\n", shdr->sh_flags);
		printf("  sh_addr: %u\n", shdr->sh_addr);
		printf("  sh_offset: %u\n", shdr->sh_offset);
		printf("  sh_size: %u\n", shdr->sh_size);
		printf("  sh_link: %u\n", shdr->sh_link);
		printf("  sh_info: %u\n", shdr->sh_info);
		printf("  sh_addralign: %u\n", shdr->sh_addralign);
		printf("  sh_entsize: %u\n", shdr->sh_entsize);
#endif

		// Ignore sections that are not symbol tables (type 2).
		if (shdr->sh_type != 2)
			continue;

		// Look up the string table for this section, the index of which is
		// storedin the sh_link field.
		struct elf32_shdr *symstrtbl = knl->buffer + ehdr->e_shoff + shdr->sh_link * ehdr->e_shentsize;
		const char *symstr = knl->buffer + symstrtbl->sh_offset;

		// Process each symbol.
		const size_t num_syms = shdr->sh_size / shdr->sh_entsize;
		for (size_t n = 0; n < num_syms && !knl->wed_sym; ++n) {
			size_t offset = shdr->sh_offset + n * shdr->sh_entsize;
			struct elf32_sym *sym = knl->buffer + offset;
			if (knl->buffer_sz < offset + sizeof(struct elf32_sym))
				return OPC_ELF_INVALID_TOO_SHORT;

#ifdef DEBUG
			printf("  symbol %lu\n", n);
			printf("    st_name: %u (%s)\n", (uint32_t)sym->st_name, symstr + sym->st_name);
			printf("    st_value: %u\n", (uint32_t)sym->st_value);
			printf("    st_size: %u\n", (uint32_t)sym->st_size);
			printf("    st_info: %u\n", (uint32_t)sym->st_info);
			printf("    st_other: %u\n", (uint32_t)sym->st_other);
			printf("    st_shndx: %u\n", (uint32_t)sym->st_shndx);
#endif

			// Ignore symbols without a name or that do not refer to objects
			// (type 1).
			if (!sym->st_name || (sym->st_info & 0xf) != 1)
				continue;

			// Check if this is the symbol we're looking for (opc_wed) and take
			// note of its address.
			if (strcmp(symstr + sym->st_name, "opc_wed") != 0)
				continue;
			knl->wed_sym = sym->st_value;
		}
	}

	return OPC_OK;
}


/// Load a kernel from an ELF binary file. Returns an error if the kernel has
/// already been loaded.
///
/// @param knl  The kernel for which a binary should be loaded.
/// @param path The path to the ELF binary file to be loaded.
/// @return Returns whether the operation failed.
opc_error_t
opc_kernel_load_file(opc_kernel_t knl, const char *path) {
	assert(knl);
	assert(path);

	// Abort if the kernel has already been loaded.
	if (knl->buffer != NULL)
		return OPC_ALREADY_LOADED;

	// Open the file for reading.
	int fd = open(path, O_RDONLY);
	if (fd == -1) {
		return OPC_ERRNO_OPEN | (errno & 0xFFFFFF);
	}

	// Determine the size of the file.
	struct stat fs;
	if (fstat(fd, &fs) == -1) {
		int err = errno;
		close(fd);
		return OPC_ERRNO_FSTAT | (err & 0xFFFFFF);
	}

	// Map the file into memory for reading.
	void *ptr = mmap(0, fs.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if (ptr == MAP_FAILED) {
		int err = errno;
		close(fd);
		return OPC_ERRNO_MMAP | (err & 0xFFFFFF);
	}

	// Close the file descriptor.
	if (close(fd) == -1) {
		return OPC_ERRNO_CLOSE | (errno & 0xFFFFFF);
	}

	// Store things in the kernel.
	knl->buffer = ptr;
	knl->buffer_sz = fs.st_size;
	knl->buffer_mode = BUFFER_MMAP;

	// Parse the buffer.
	return parse_elf(knl);
}


/// Load a kernel from a buffer already in memory. Returns an error if the
/// kernel has already been loaded.
///
/// @param knl            The kernel for which a binary should be loaded.
/// @param buffer         A pointer to the binary in memory.
/// @param size           The size of the binary in memory.
/// @param take_ownership If set to 1, the kernel assumes ownership over the
///                       buffer and will free it when the kernel itself is
///                       freed.
opc_error_t
opc_kernel_load_buffer(opc_kernel_t knl, void *buffer, size_t size, int take_ownership) {
	assert(knl);
	assert(buffer);

	// Abort if the kernel has already been loaded.
	if (knl->buffer != NULL)
		return OPC_ALREADY_LOADED;

	knl->buffer = buffer;
	knl->buffer_sz = size;
	knl->buffer_mode = take_ownership ? BUFFER_OWNED : BUFFER_REGULAR;

	// Parse the buffer.
	return parse_elf(knl);
}


/// Dump information about a kernel to a file.
///
/// @param knl The kernel for which information is to be dumped.
/// @param out The dump destination. Usually stderr or stdout.
void
opc_kernel_dump_info(opc_kernel_t knl, FILE *out) {
	assert(knl);
	assert(out);

	fprintf(out, "{\n");
	fprintf(out, "  buffer: %p\n", knl->buffer);
	fprintf(out, "  size: %lu\n", knl->buffer_sz);
	const char *mode_str = "unknown";
	switch (knl->buffer_mode) {
		case BUFFER_REGULAR: mode_str = "regular"; break;
		case BUFFER_OWNED: mode_str = "owned"; break;
		case BUFFER_MMAP: mode_str = "mmap"; break;
	}
	fprintf(out, "  mode: %s\n", mode_str);
	fprintf(out, "  wed_sym: 0x%08x\n", knl->wed_sym);
	fprintf(out, "  sections: %lu {\n", knl->num_sections);
	for (size_t i = 0; i < knl->num_sections; ++i) {
		fprintf(out, "    [%lu] {\n", i);
		fprintf(out, "      src: %p\n", knl->sections[i].src);
		fprintf(out, "      dst: 0x%08x\n", knl->sections[i].dst);
		fprintf(out, "      src_sz: %u\n", knl->sections[i].src_sz);
		fprintf(out, "      dst_sz: %u\n", knl->sections[i].dst_sz);
		fprintf(out, "    }\n");
	}
	fprintf(out, "  }\n");
	fprintf(out, "}\n");
}


/// Return the number of sections in the kernel.
///
/// @param knl The kernel for which the number of sections is to be returned.
/// @return Returns the number of sections in the kernel.
size_t
opc_kernel_get_num_sections(opc_kernel_t knl) {
	assert(knl);
	return knl->num_sections;
}


/// Access a section in the kernel.
///
/// @param knl   The kernel whose section should be returned.
/// @param index The index of the section. Must be within bounds.
/// @return Returns the section.
opc_kernel_section_t
opc_kernel_get_section(opc_kernel_t knl, size_t index) {
	assert(knl);
	assert(index < knl->num_sections);
	return &knl->sections[index];
}


/// Return the address of the WED symbol inside the kernel binary.
///
/// @param knl The kernel whose symbol table should be investigated.
/// @return Returns the address of the WED symbol within the binary.
uint32_t
opc_kernel_get_wed_symbol(opc_kernel_t knl) {
	assert(knl);
	return knl->wed_sym;
}



/// Create a new device. The device needs to be openend via one of the
/// `opc_dev_open_*` functions. Once opened, kernels can be launched on the
/// device. The caller is responsible to free the device after use.
///
/// @return Returns the allocated device.
opc_dev_t
opc_dev_new() {
	opc_dev_t dev = calloc(1, sizeof(struct opc_dev));
	pthread_mutex_init(&dev->run_mutex, NULL);
	pthread_cond_init(&dev->run_cond, NULL);
	return dev;
}


/// Deallocate the resources of a device.
///
/// @param dev The device to be freed.
void
opc_dev_free(opc_dev_t dev) {
	assert(dev);

	opc_dev_wait_all(dev);

	pthread_mutex_destroy(&dev->run_mutex);
	pthread_cond_destroy(&dev->run_cond);

	// Clean up the device.
	memset(dev, 0, sizeof(*dev)); // makes use-after-free more obvious
	free(dev);
}


/// Open any attached accelerator.
///
/// @param dev The device to be opened.
/// @return Returns an error code indicating whether the operation succeeded.
opc_error_t
opc_dev_open_any(opc_dev_t dev) {
	assert(dev);

	// Abort if the device has already been opened.
	if (dev->afu)
		return OPC_ALREADY_OPENED;

	dev->afu = cxl_afu_open_h(cxl_afu_next(NULL), CXL_VIEW_DEDICATED);
	if (!dev->afu) {
		return OPC_ERRNO_CXL;
	}
	return OPC_OK;
}


/// Open a device at a specific path.
///
/// @param dev  The device to be opened.
/// @param path The path to the device to be opened.
/// @return Returns an error code idnicating whether the operation succeeded.
opc_error_t
opc_dev_open_path(opc_dev_t dev, const char *path) {
	assert(dev);

	// Abort if the device has already been opened.
	if (dev->afu)
		return OPC_ALREADY_OPENED;

	dev->afu = cxl_afu_open_dev((char *)path); // ugly hack to cast away the const
	if (!dev->afu) {
		return OPC_ERRNO_CXL;
	}
	return OPC_OK;
}


/// Launch a kernel on the device.
///
/// @param dev The device on which to launch the kernel.
/// @param knl The kernel to launch on the device.
/// @param wed The work element descriptor to pass to the accelerator.
/// @param job Optional pointer to a job ID which will be assigned a unique
///            value that can later be passed to `opc_dev_wait`.
/// @return Returns an error code indicating whether the operation succeeded or
///         not.
opc_error_t
opc_dev_launch(opc_dev_t dev, opc_kernel_t knl, void *wed, opc_job_t *job) {
	assert(dev);
	assert(knl);

	// Abort if the device has not been opened or the kernel not loaded.
	if (!dev->afu)
		return OPC_NOT_OPENED;
	if (!knl->sections)
		return OPC_NOT_LOADED;

	// Assemble the work element descriptor for this lauch.
	struct wed *outer_wed = calloc(1, sizeof(struct wed));
	outer_wed->sec_ptr = knl->sections;
	outer_wed->sec_num = knl->num_sections;
	outer_wed->wed_sym = knl->wed_sym;
	outer_wed->wed = wed;
#ifdef DEBUG
	printf("liboprecomp: wed_sym is at 0x%08x\n", outer_wed->wed_sym);
#endif

	// For now just block until execution completes. Not elegant, but gets the
	// job done.
	pthread_mutex_lock(&dev->run_mutex);
	if (cxl_afu_attach(dev->afu, (uint64_t)outer_wed) != 0) {
		pthread_mutex_unlock(&dev->run_mutex);
		free(outer_wed);
		return OPC_ERRNO_CXL;
	}
	while (outer_wed->result == 0) sleep(0); // ugly, do better once interrupts are ready
	outer_wed->result = 2; // signal to the AFU that we've received the handshake
	pthread_mutex_unlock(&dev->run_mutex);

	// Clean up the work element descriptor. This thing should later go into a
	// queue with all pending WEDs.
	free(outer_wed);

	return OPC_OK;
}


/// Wait until the kernel with the given job ID finishes.
///
/// @param dev The device on which the kernel is running.
/// @param job The job upon whose termination shall be waited.
void
opc_dev_wait(opc_dev_t dev, opc_job_t job) {
	assert(dev);
	// nothing to do since currently opc_dev_launch blocks
}


/// Wait until all kernels currently scheduled finish.
///
/// @param dev The device running the kernels upon whose termination shall be
///            waited.
void
opc_dev_wait_all(opc_dev_t dev) {
	assert(dev);
	// nothing to do since currently opc_dev_launch blocks
}


/// Return a static description string of the given error.
///
/// @param err The error code which should be mapped to a static string.
/// @return Returns the static string that describes the error.
const char *
opc_errstr(opc_error_t err) {
	switch (err & 0xFF000000) {
		case OPC_OK: return "OK";
		case OPC_ERRNO: return strerror(err & 0xFFFFFF);
		case OPC_ALREADY_LOADED: return "Kernel already loaded";
		case OPC_ALREADY_OPENED: return "Device already opened";
		case OPC_ELF_INVALID: return "Invalid ELF binary";
		case OPC_NOT_LOADED: return "Kernel not loaded";
		case OPC_NOT_OPENED: return "Device not opened";
		default: return "Unknown error";
	}
}

/// Print an error to stderr. An optional prefix can be passed that allows
/// displaying additional information in front of the error message.
///
/// @param prefix The optional prefix to prepend the message with. May be NULL.
/// @param err    The error code for which the message should be printed.
void
opc_perror(const char *prefix, opc_error_t err) {
	if (prefix) fprintf(stderr, "%s: ", prefix);
	fprintf(stderr, "%s\n", opc_errstr(err));
}

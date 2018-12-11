# libcxl for Virtual Platform

This is a simulation implementation of [libcxl] that integrates with the PULP virtual platform. Host code can call into libcxl as usual, but in the background the implementation launches an instance of the virtual platform to simulate the PULP system.

## Supported API

The following is a list of the original [libcxl] functions that are implemented:

- [ ] `cxl_adapter_next`
- [ ] `cxl_adapter_dev_name`
- [ ] `cxl_adapter_free`
- [ ] `cxl_adapter_afu_next`
- [x] `cxl_afu_next`
- [x] `cxl_afu_dev_name`
- [x] `cxl_afu_open_dev`
- [x] `cxl_afu_open_h`
- [ ] `cxl_afu_fd_to_h`
- [x] `cxl_afu_free`
- [x] `cxl_afu_opened`
- [ ] `cxl_work_alloc`
- [ ] `cxl_work_free`
- [ ] `cxl_work_get_amr`
- [ ] `cxl_work_get_num_irqs`
- [ ] `cxl_work_get_wed`
- [ ] `cxl_work_set_amr`
- [ ] `cxl_work_set_num_irqs`
- [ ] `cxl_work_set_wed`
- [x] `cxl_afu_attach`
- [x] `cxl_afu_attach_full`
- [x] `cxl_afu_attach_work`
- [ ] `cxl_afu_get_process_element`
- [ ] `cxl_afu_fd`
- [ ] `cxl_afu_sysfs_pci`
- [ ] `cxl_get_api_version`
- [ ] `cxl_get_api_version_compatible`
- [ ] `cxl_get_cr_class`
- [ ] `cxl_get_cr_device`
- [ ] `cxl_get_cr_vendor`
- [ ] `cxl_get_irqs_max`
- [ ] `cxl_set_irqs_max`
- [ ] `cxl_get_irqs_min`
- [ ] `cxl_get_mmio_size`
- [ ] `cxl_get_mode`
- [ ] `cxl_set_mode`
- [ ] `cxl_get_modes_supported`
- [ ] `cxl_get_prefault_mode`
- [ ] `cxl_set_prefault_mode`
- [ ] `cxl_get_pp_mmio_len`
- [ ] `cxl_get_pp_mmio_off`
- [ ] `cxl_get_base_image`
- [ ] `cxl_get_caia_version`
- [ ] `cxl_get_image_loaded`
- [ ] `cxl_get_psl_revision`
- [ ] `cxl_get_psl_timebase_synced`
- [ ] `cxl_event_pending`
- [ ] `cxl_read_event`
- [ ] `cxl_read_expected_event`
- [ ] `cxl_fprint_event`
- [ ] `cxl_fprint_unknown_event`
- [ ] `cxl_mmio_map`
- [ ] `cxl_mmio_unmap`
- [ ] `cxl_mmio_ptr`
- [ ] `cxl_mmio_write64`
- [ ] `cxl_mmio_read64`
- [ ] `cxl_mmio_write32`
- [ ] `cxl_mmio_read32`
- [ ] `cxl_mmio_install_sigbus_handler`
- [ ] `cxl_errinfo_size`
- [ ] `cxl_errinfo_read`

[libcxl]: https://github.com/ibm-capi/libcxl

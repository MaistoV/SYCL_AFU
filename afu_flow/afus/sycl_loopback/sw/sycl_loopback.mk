register_map_offsets:
#	Change offset of the SYCL IP CSR space
	sed -E -i "s/0x0/0x${KERNEL_REGISTER_MAP_OFFSET_HEX}/g" ${AFU_SW_DIR}/register_map_offsets.hpp
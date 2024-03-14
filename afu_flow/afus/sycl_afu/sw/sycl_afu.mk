CFLAGS += -D$(RS_SCHEMA)
CFLAGS += -I$(shell dirname $(shell realpath rs_erasure.hpp))

register_map_offsets:
#	Change offset of the SYCL IP CSR space
	sed -E -i "s/0x0/0x40/g" ${AFU_SW_DIR}/register_map_offsets.hpp

CFLAGS += -D$(RS_SCHEMA)
CFLAGS += -I$(shell dirname $(shell realpath rs_erasure.hpp))
# For measure_latency.h
CFLAGS += -I${ROOT_DIR}/oneapi/sycl_rs_erasure/src
# For rs_rom_utils.h
CFLAGS += -I${ROOT_DIR}/oneapi/sycl_rs_erasure/src/rs_erasure/roms/src/
# Append callers flags
CFLAGS += ${AFU_HOST_DEFINES}

# Link to ISA-L
LDFLAGS += -lisal

# Add opae simple wrapper source
SRCS += opae_simple_wrapper.c
# Add rom utils source
SRCS += rs_rom_utils.c

# Override GCC, for chrono
CC = g++

register_map_offsets:
#	Change offset of the SYCL IP CSR space
	sed -E -i "s/0x.+/0x${KERNEL_REGISTER_MAP_OFFSET_HEX}/g" ${AFU_SW_DIR}/register_map_offsets.hpp

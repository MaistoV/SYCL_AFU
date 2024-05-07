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

include ${AFU_DEF_DIR}/sycl_afu_common/sw/sycl_afu_common.mk
CFLAGS += -D$(RS_SCHEMA)
CFLAGS += -I$(shell dirname $(shell realpath rs_erasure.hpp))

# Override CC, for chrono
CC = g++

include ${AFU_DEF_DIR}/sycl_afu_common/sw/sycl_afu_common.mk
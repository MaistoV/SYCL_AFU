CFLAGS += -D$(RS_SCHEMA)
CFLAGS += -I$(shell dirname $(shell realpath rs_erasure.hpp))

include ${AFU_DEF_DIR}/sycl_afu_common/sw/sycl_afu_common.mk
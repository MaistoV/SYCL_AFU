# Import these flags from common makefile
# 	LDFLAGS += -luuid
# 	FPGA_LIBS = -lopae-c
# 	ASE_LIBS = -lopae-c-ase
include ${EXAMPLES_AFU}/tutorial/afu_types/01_pim_ifc/common/sw/common_include.mk

# Utility software
SYCL_AFU_COMMON_SW = ../../sycl_afu_common/sw
SYCL_AFU_UTILS_DIR = $(SYCL_AFU_COMMON_SW)/sycl_afu_utils
OSW_DIR = $(SYCL_AFU_COMMON_SW)/opae_simple_wrapper

# Add to include path
# For sycl_afu_utils.h
CFLAGS += -I${SYCL_AFU_UTILS_DIR}
# For opae_simple_wrapper.h
CFLAGS += -I${OSW_DIR}

# Primary test name
TEST = $(AFU_NAME)

# AFU aspecific targets, if any
-include $(AFU_NAME).mk

# Build directories
OBJDIR = obj
BINDIR = bin
CFLAGS += -I. -I./$(OBJDIR) -I$(SYCL_AFU_COMMON_SW)

# Files and folders
SRCS += $(TEST).c
OBJS += $(addprefix $(OBJDIR)/,$(patsubst %.c,%.o,$(SRCS)))

all: $(BINDIR)/$(TEST)

# AFU info from JSON file, including AFU UUID
AFU_JSON_INFO = $(OBJDIR)/afu_json_info.h
$(AFU_JSON_INFO): ../hw/rtl/${TEST}.json | objdir
	afu_json_mgr json-info --afu-json=$^ --c-hdr=$@
$(OBJS): $(AFU_JSON_INFO)

# Build utility functions
# NOTE: the current software build is going to overshoot by building and linking sycl_afu_utils.o also for non-SYCL AFUs, this could be addressed easily
$(SYCL_AFU_UTILS_DIR)/sycl_afu_utils.o: $(SYCL_AFU_UTILS_DIR)/sycl_afu_utils.c $(SYCL_AFU_UTILS_DIR)/sycl_afu_utils.h
	$(CC) $(CFLAGS) -c $< -o $@

# Same for OPAE Simple Wrapper
$(OSW_DIR)/opae_simple_wrapper.o: $(OSW_DIR)/opae_simple_wrapper.c $(OSW_DIR)/opae_simple_wrapper.h
	$(CC) $(CFLAGS) -c $< -o $@

# Create a static library for OPAE Simple Wrapper
osw: $(OSW_DIR)/libopae_simple_wrapper.a
$(OSW_DIR)/libopae_simple_wrapper.a: $(OSW_DIR)/opae_simple_wrapper.o
	$(AR) rcs $@ $^
	
$(BINDIR)/$(TEST): $(OBJS) $(SYCL_AFU_UTILS_DIR)/sycl_afu_utils.o $(OSW_DIR)/libopae_simple_wrapper.a | bindir
	$(CC) -o $@ $^ -L$(OSW_DIR) $(LDFLAGS) $(FPGA_LIBS) -lrt -lopae_simple_wrapper

$(OBJDIR)/%.o: %.c | objdir
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(TEST) $(OBJDIR) $(BINDIR) 
	rm -rf $(SYCL_AFU_UTILS_DIR)/sycl_afu_utils.o 
	rm -rf $(OSW_DIR)/libopae_simple_wrapper.a $(OSW_DIR)/opae_simple_wrapper.o

objdir:
	@mkdir -p $(OBJDIR)

bindir:
	@mkdir -p $(BINDIR)

.PHONY: all clean

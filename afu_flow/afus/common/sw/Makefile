# Import these flags from common makefile
# 	LDFLAGS += -luuid
# 	FPGA_LIBS = -lopae-c
# 	ASE_LIBS = -lopae-c-ase
include ${EXAMPLES_AFU}/tutorial/afu_types/01_pim_ifc/common/sw/common_include.mk

# Utility software
SYCL_AFU_COMMON_SW := ../../sycl_afu_common/sw

# Primary test name
TEST = $(AFU_NAME)

# AFU aspecific targets
-include $(AFU_NAME).mk

# Build directory
OBJDIR = obj
BINDIR = bin
CFLAGS += -I./$(OBJDIR) -I$(SYCL_AFU_COMMON_SW)
CPPFLAGS += -I./$(OBJDIR) -I$(SYCL_AFU_COMMON_SW)

# Files and folders
SRCS = $(TEST).c
OBJS = $(addprefix $(OBJDIR)/,$(patsubst %.c,%.o,$(SRCS)))

all: bin/$(TEST)

# AFU info from JSON file, including AFU UUID
AFU_JSON_INFO = $(OBJDIR)/afu_json_info.h
$(AFU_JSON_INFO): ../hw/rtl/${TEST}.json | objdir
	afu_json_mgr json-info --afu-json=$^ --c-hdr=$@
$(OBJS): $(AFU_JSON_INFO)

# Build utility functions
# NOTE: the current software build is going to overshoot by building and linking sycl_afu_utils.o also for non-SYCL AFUs, this could be addressed easily
sycl_afu_utils.o: $(SYCL_AFU_COMMON_SW)/sycl_afu_utils.o
$(SYCL_AFU_COMMON_SW)/sycl_afu_utils.o: $(SYCL_AFU_COMMON_SW)/sycl_afu_utils.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BINDIR)/$(TEST): $(OBJS) $(SYCL_AFU_COMMON_SW)/sycl_afu_utils.o | bindir
	$(CC) -o $@ $^ $(LDFLAGS) $(FPGA_LIBS) -lrt

$(OBJDIR)/%.o: %.c | objdir
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(TEST) $(OBJDIR) $(BINDIR)

objdir:
	@mkdir -p $(OBJDIR)

bindir:
	@mkdir -p $(BINDIR)

.PHONY: all clean

include ${EXAMPLES_AFU}/tutorial/afu_types/01_pim_ifc/common/sw/common_include.mk

# Primary test name
TEST = $(AFU_NAME)

# Build directory
OBJDIR = obj
BINDIR = bin
CFLAGS += -I./$(OBJDIR)
CPPFLAGS += -I./$(OBJDIR)

# Files and folders
SRCS = $(TEST).c
OBJS = $(addprefix $(OBJDIR)/,$(patsubst %.c,%.o,$(SRCS)))

all: bin/$(TEST)

# AFU info from JSON file, including AFU UUID
AFU_JSON_INFO = $(OBJDIR)/afu_json_info.h
$(AFU_JSON_INFO): ../hw/rtl/${TEST}.json | objdir
	afu_json_mgr json-info --afu-json=$^ --c-hdr=$@
$(OBJS): $(AFU_JSON_INFO)

$(BINDIR)/$(TEST): $(OBJS) | bindir
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

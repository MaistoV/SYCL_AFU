# DEVICE   := 10AX115S2F45I1SG
DEVICE   := 10AX115N2F40E2LG # Arria 10 GX as found in PAC

TARGETS  := emu cosim cosim_waveforms qcompile
SOURCES	 := src/rs_erasure.c tb/tb_rs_erasure.c src/roms/src/rs_rom_utils.c

HLS_CXX_FLAGS := -v

# Default to RS[6:3]
RS_SCHEMA ?= RS_6_3
# Default to CCI_BYTE_WIDTH*2=128
CELL_LENGTH_DEFAULT ?= 128u
# Defalt unroll factor
UNROLL_FACTOR ?= 1

CXXFLAGS := $(CXXFLAGS) -lisal -D$(RS_SCHEMA) -DCELL_LENGTH_DEFAULT=$(CELL_LENGTH_DEFAULT)
CXXFLAGS += -DUNROLL_FACTOR=$(UNROLL_FACTOR)
ifeq ($(SIMULATOR_NONE),1)
	SIMULATOR_TYPE := --simulator none
endif
ifeq ($(DEBUG),1)
	CXXFLAGS += -DDEBUG
endif
ifeq ($(DEBUG0),1)
	CXXFLAGS += -DDEBUG0
endif
ifeq ($(TRANSPOSE_64B ),1)
	HLS_CXX_FLAGS += -DTRANSPOSE_64B 
endif
CXX := i++ 
RM  := rm -rf

# change output color
COLOR_GREEN  := tput setaf 2
COLOR_RED 	 := tput setaf 1
COLOR_NORMAL := tput setaf 7
HLS_INSTALL_DIR := $(shell which i++ | sed 's|/bin/i++||g')
CPP_STD := -std=c++11

.PHONY:help
help: 
	@echo "Available targets are: help rom_gen $(TARGETS)"
	@echo "	rom_gen generates ROMs"
	@echo "	Set RS_SCHEMA for Reed-Solomon code schema [RS_3_2, RS_6_3, RS_10_4]"
	@echo "	Set CELL_LENGTH_DEFAULT for default cell length"
	@echo "	Set SIMULATOR_NONE=1 for "--simulator none""
	@echo "	Set DEBUG=1 for "-DDEBUG""
	@echo "	Set DEBUG0=1 for "-DDEBUG0""
	@echo "	Set additional i++ flags in CXXFLAGS, e.g., --clock <clock_spec>, --x86-only, --fpga-only, for more see: i++ --help"

.PHONY: all
all: $(TARGETS)

# gpp: $(SOURCES)
# 	g++ $^ $(CXXFLAGS) -I"$(HLS_INSTALL_DIR)/include" -L"$(HLS_INSTALL_DIR)/host/linux64/lib" -lhls_emul -o gpp $(CPP_STD)

emu: $(SOURCES)
	$(CXX) $^ -march=x86-64 $(CXXFLAGS) -DEMULATION $(HLS_CXX_FLAGS) -o $@_$(RS_SCHEMA)$(PROTO)

cosim: $(SOURCES)
	$(CXX) $^ -march=$(DEVICE) $(CXXFLAGS) $(HLS_CXX_FLAGS) $(SIMULATOR_TYPE) -o $@_$(RS_SCHEMA)$(PROTO)

cosim_waveforms: $(SOURCES)
	$(CXX) $^ -march=$(DEVICE) $(CXXFLAGS) $(HLS_CXX_FLAGS) $(SIMULATOR_TYPE) -ghdl -o $@_$(RS_SCHEMA)$(PROTO)

qcompile: $(SOURCES)
	$(CXX) $^ -march=$(DEVICE) $(CXXFLAGS) $(HLS_CXX_FLAGS) $(SIMULATOR_TYPE) --quartus-compile -o $@_$(RS_SCHEMA)$(PROTO)
	# Extract Fit report statistics
	echo "Fmax: $$(cat qcompile_$(RS_SCHEMA).prj/reports/lib/json/quartus.json | grep MHz | sed -E "s/.+\[|\]|\"| clock fmax//g");" \
		"Resources: $$(cat qcompile_$(RS_SCHEMA).prj/reports/lib/json/quartus.json | grep Full | sed -E "s/.+\"data\":\[|\"| ]//g" | sed "s/ ,/, /g")" \
		>  qcompile_$(RS_SCHEMA).prj/reports/hls_freq.txt
	@$(COLOR_GREEN) 
	@cat qcompile_$(RS_SCHEMA).prj/reports/hls_freq.txt
	@$(COLOR_NORMAL)

.PHONY: test
test: $(TARGETS)
	mkdir -p tests
	echo "Result	Testname"> tests/test_results.txt
	for t in $(TARGETS); do \
		echo "Testing ./$$t"; \
		./$$t > tests/result_$$t.txt; \
		cat tests/result_$$t.txt; \
		grep -q "Failed" tests/result_$$t.txt && \
			echo "Failed	$$t " >> tests/test_results.txt || \
			echo "Passed	$$t " >> tests/test_results.txt; \
	done; 
	@# Cat colored output
	@echo "***** Test results *****"
	@echo "Result	Testname"
	@$(COLOR_GREEN); grep Passed tests/test_results.txt --color=none; echo "" > /dev/null
	@$(COLOR_RED); grep Failed tests/test_results.txt --color=none; echo "" > /dev/null
	@$(COLOR_NORMAL); echo "************************"

rom_gen: src/roms/src/rs_rom_gen.c src/roms/src/rs_rom_utils.c
	# This assumes ISA-L is installed on the build machine (e.g., in /usr/lib/)
	gcc $^ -o $@ -lisal
	@# Otherwise
	@#gcc $^ -o $@ -lisal -L/path/to/libisal.so

perms_gen: src/roms/src/perms_gen.c
	g++ $^ -o $@

regen_perms: perms_gen
	./$< 3 2	> src/roms/survival_patterns_3_2.txt
	./$< 6 3	> src/roms/survival_patterns_6_3.txt
	./$< 10 4 	> src/roms/survival_patterns_10_4.txt

regen_roms: rom_gen regen_perms
	./rom_gen -k 3 	-p 2 -l 1 -e 1 -o src/roms/
	./rom_gen -k 6 	-p 3 -l 1 -e 1 -o src/roms/
	./rom_gen -k 10 -p 4 -l 1 -e 1 -o src/roms/

# clean_gpp:
# 	$(RM) gpp
	
clean_rom_gen:
	$(RM) rom_gen

clean_emu:
	$(RM) emu_$(RS_SCHEMA)$(PROTO)

clean_cosim:
	$(RM) cosim_$(RS_SCHEMA)$(PROTO)*  transcript

clean_cosim_waveforms: 
	$(RM) cosim_waveforms_$(RS_SCHEMA)$(PROTO)* transcript

clean_qcompile:
	$(RM) qcompile_$(RS_SCHEMA)$(PROTO)* *.out *.o qcompile.prj *tmp* .q* *rpt outputfiles qdb ip* *tcl *name.txt transcript

clean_all: clean_emu clean_cosim clean_qcompile clean_cosim_waveforms clean_rom_gen #clean_gpp





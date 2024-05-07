# Default variables value
TEST_ARGS ?=

# NOTE: Most of this flow does rely on file timestamps, therefore, it is going to build most of the targets for every call

all: help

help:
	@cat "scripts/make_help.txt"
	
#######
# FIM #
#######

# For custom AFUs as FIM defaults, add AFU synthesis setup as requirement for FIM builds
FIM_BUILD_REQUIRED_TARGETS =
ifeq (${UPDATE_DEFAULT_AFU}, 1)
	FIM_BUILD_REQUIRED_TARGETS += ${AFU_SYNTH_DIR}
endif

fim_build_pr:
fim_build_flat:
fim_build_%: ${FIM_BUILD_REQUIRED_TARGETS}
#	Copy OFSS configuration files
	if [ -d ${OFSS_CONFIG_DIR} ]; then \
		cp -vr ${OFSS_CONFIG_DIR}/* ${OFS_BUILD_ROOT}/tools/ofss_config/; \
	fi
#	TODO: remove the need to source this script from HTS
	cd ${HTS_RELEASE}; ./setup_env.sh; \
	${ROOT_DIR}/fim_flow/build_fim.sh --$* ${OFSS_CONFIG}

FIM_IMAGE ?= ${FIM_IMAGE_USER1}
FIM_UPDATE_DEBUG ?= 0
ifeq (${FIM_UPDATE_DEBUG}, 1)
	FPGASUPDATE_FLAGS += --log-level debug 
endif
fim_update: 
#	Update flash images 
	sudo fpgasupdate ${FPGASUPDATE_FLAGS} ${FIM_IMAGE} ${PAC_PCIE_SBD}.0
	@echo "To configure the new FIM, powercycle the PAC with:"
	@echo "    ${MAKE} pac_powercycle_<bootpage>"


RSU_DEBUG ?= 0
ifeq (${RSU_DEBUG}, 1)
	RSU_FLAGS += --debug fpga
endif
pac_powercycle_user1:
pac_powercycle_user2:
pac_powercycle_factory:
pac_powercycle_%:
# 	Power cycle PAC
	sudo rsu ${RSU_FLAGS} fpga --page=$* ${PAC_PCIE_SBD}.0

opae.io_bind:
	${ROOT_DIR}/scripts/opae.io_bind.sh

opae.io_bind_one:
	sudo pci_device ${PAC_PCIE_SBD}.0 vf 1
	sudo opae.io -d ${PAC_PCIE_SBD}.${FIRST_AFU_VF} init ${USER}:${USER}
	opae.io ls

opae.io_release:
	${ROOT_DIR}/scripts/opae.io_release.sh
	
pac_hot_plug:
	sudo pci_device ${PAC_PCIE_SBD}.0 unplug
	sudo pci_device ${PAC_PCIE_SBD}.0 plug

#############################
# ONE API CMake Environment #
#############################
MMD_DEBUG ?= 0
ifeq (${MMD_DEBUG}, 1)
	ONEAPI_DEBUG_ENV := MMD_ENABLE_DEBUG=1  \
						MMD_PROGRAM_DEBUG=1
endif
SYCL_IP_CMAKE_SOURCES := ${SYCL_SRC_DIR}/CMakeLists.txt ${SYCL_SRC_DIR}/src/CMakeLists.txt

# Wrap these variables in a single list
SYCL_IP_ENV += RS_SCHEMA=${RS_SCHEMA} \
				SYCL_IP_NAME=${SYCL_IP_NAME} \
				SYCL_IP_BUILD_DIR=${SYCL_IP_BUILD_DIR} \
				SYCL_IP_PRJ=${SYCL_IP_PRJ}

SYCL_DEBUG ?= 0
SYCL_FAST_COMPILE ?= 0
SYCL_CMAKE_FLAGS ?=
ifeq (${SYCL_FAST_COMPILE}, 1)
	SYCL_CMAKE_FLAGS += -DUSER_HARDWARE_FLAGS=-Xsfast-compile
endif
ifeq (${SYCL_DEBUG}, 1)
	SYCL_CMAKE_FLAGS += --trace-expand
endif

# Environment setup for cmake
CMAKE_ENV = USER_HARDWARE_FLAGS=${USER_HARDWARE_FLAGS} \
			SYCL_IP_NAME=${SYCL_IP_NAME} \
			${SYCL_IP_ENV}
CMAKE = ${CMAKE_ENV} cmake .. ${SYCL_CMAKE_FLAGS}

# Build-time variables
ifeq (${MULTI_ERASURE_SIMPLE}, 1)
	SYCL_CXX_DEFINES += -DMULTI_ERASURE_SIMPLE
endif
ifeq (${SYCL_DEBUG}, 1)
	SYCL_CXX_DEFINES += -DDEBUG
endif
ifeq (${ASP_ZERO_COPY}, 1)
	SYCL_CXX_DEFINES += -DASP_ZERO_COPY
endif
SYCL_MAKE_ENV = "CXX_DEFINES=${SYCL_CXX_DEFINES}"

####################
# ONE API ASP Flow #
####################

oneapi_asp_build_aocx:
	cd ${OFS_ASP_ROOT}; \
	./scripts/build-default-aocx.sh -b ${OFS_ASP_BOARD_VARIANT}
	@echo Built AOCX for PR tree ${OPAE_PLATFORM_ROOT}

aocl_bsp_build:
	cd ${OFS_ASP_ROOT}; ./scripts/build-bsp.sh

aocl_bsp_install:
	${ONEAPI_DEBUG_ENV} aocl install ${OFS_ASP_ROOT}

aocl_bsp_uninstall:
	${ONEAPI_DEBUG_ENV} aocl uninstall ${OFS_ASP_ROOT}

ACL_DEVICE ?= acl0 # Assuming only one device connected
aocl_aocx_initalize: opae.io_bind_one 
	${ONEAPI_DEBUG_ENV} aocl initialize ${ACL_DEVICE} ${OFS_ASP_BOARD_VARIANT} 

CMAKE_ASP_FLAGS = -DFPGA_DEVICE=${OFS_ASP_FPGA_DEVICE} \
	-DIS_BSP=1 ${OFS_ASP_USM_FLAG}
oneapi_cmake_asp: ${SYCL_ASP_BUILD_DIR}
${SYCL_ASP_BUILD_DIR}: ${SYCL_IP_CMAKE_SOURCES}
	mkdir ${SYCL_ASP_BUILD_DIR};	\
	cd ${SYCL_ASP_BUILD_DIR};		\
	${CMAKE} ${CMAKE_ASP_FLAGS}

oneapi_asp_plain_c:
oneapi_asp_fpga_emu:
oneapi_asp_fpga_sim:
oneapi_asp_report:
oneapi_asp_fpga:
oneapi_asp_%: oneapi_cmake_asp
#	Force rebuild by touching host code
	touch ${SYCL_SRC_DIR}/src/host.cpp
	cd ${SYCL_ASP_BUILD_DIR}; \
	${MAKE} $* ${SYCL_MAKE_ENV}

test_asp_plain_c:
test_asp_fpga: # Make sure to make aocl_aocx_initalize first
test_asp_fpga_emu:
test_asp_fpga_sim: # TODO: CL_CONTEXT_MPSIM_DEVICE_INTELFPGA=1
test_asp_%:
	cd ${SYCL_ASP_BUILD_DIR}; \
	./${SYCL_IP_NAME}.$* ${TEST_ARGS}

#############################
# ONE API IP Authoring Flow #
#############################
CMAKE_IP_FLAGS = -DFPGA_DEVICE=${AGILEX7_PART_NUMBER} \
	-DIS_BSP=0
oneapi_cmake_ip: ${SYCL_IP_BUILD_DIR}
${SYCL_IP_BUILD_DIR}: ${SYCL_IP_CMAKE_SOURCES}
	mkdir ${SYCL_IP_BUILD_DIR};	\
	cd ${SYCL_IP_BUILD_DIR};	\
	${CMAKE} ${CMAKE_IP_FLAGS}

oneapi_ip_report:
oneapi_ip_fpga_emu:
oneapi_ip_plain_c:
oneapi_ip_%: oneapi_cmake_ip
	cd ${SYCL_IP_BUILD_DIR}; \
	${MAKE} $* ${SYCL_MAKE_ENV}

oneapi_open_ip_report:
	firefox ${SYCL_IP_PRJ}/reports/report.html &

oneapi_ip: oneapi_ip_report
	@echo "[INFO] Copying IP files to AFU workdir"
#	SYCL IP output files
	cp -r ${SYCL_IP_PRJ} ${AFU_HW_DIR}
#	SYCL IP CSR map headers to sw project
	cp -r ${SYCL_IP_PRJ}/include/* ${AFU_SW_DIR}
#	Update SYCL IP CSR offset
	make -C ${AFU_SW_DIR} register_map_offsets

test_ip_fpga_emu:
test_ip_plain_c:
test_ip_%:
	cd ${SYCL_IP_BUILD_DIR}; ./${SYCL_IP_NAME}.$* ${TEST_ARGS}

#######
# AFU #
#######

# Build host application
AFU_HOST_DEFINES ?=
ifeq (${DEBUG}, 1)
	AFU_HOST_DEFINES += -DDEBUG
endif
INTERRUPT_EVENTS ?= 0
ifeq (${INTERRUPT_EVENTS}, 1)
	AFU_HOST_DEFINES += -DINTERRUPT_EVENTS
endif
ifeq (${MULTI_ERASURE_SIMPLE}, 1)
	AFU_HOST_DEFINES += -DMULTI_ERASURE_SIMPLE
endif

afu_host:
	${MAKE} -C ${AFU_SW_DIR} clean all \
		RS_SCHEMA=${RS_SCHEMA} \
		AFU_HOST_DEFINES="${AFU_HOST_DEFINES}"

# Build and launch simulation
ase_setup: clean_ase ${OPAE_PLATFORM_ROOT}
#	Setup and launch simulator
	${SYCL_IP_ENV} VERBOSE=1 ${AFU_FLOW_DIR}/afu_ase.sh 

# Launch simulation without rebuilding it
ase_launch: ${AFU_ASE_DIR}
#	Clear previous runs' lock file
	rm -vf ${AFU_ASE_DIR}/work/.ase_ready.pid;
#	Launch simulation (make must be invoked from its own direcotry)
	cd ${AFU_ASE_DIR}; ${MAKE}
	cd ${AFU_ASE_DIR}; ${MAKE} sim

# Open Wafeform Log File
ase_waves: ${AFU_ASE_DIR}/work/vsim.wlf
# ${MAKE} -C ${AFU_ASE_DIR} wave
	vsim $<										\
		-do ${ROOT_DIR}/scripts/add_waves.do 	\
		-debugdb # ${AFU_ASE_DIR}/work/vsim.dbg

# Setup AFU sysnthesis environment
${AFU_SYNTH_DIR}: ${OPAE_PLATFORM_ROOT}
	${SYCL_IP_ENV} ${AFU_FLOW_DIR}/afu_synth_setup.sh

# Build Green Bitstream
# This takes around 40 minutes...
gbs: ${AFU_SYNTH_DIR} #oneapi_ip
	${SYCL_IP_ENV} ${AFU_FLOW_DIR}/afu_synth_build.sh

gbs_configure:
#	Configure PR slot with GBS
# sudo fpgaconf ${AFU_GBS_FILE}
	sudo fpgasupdate  ${FPGASUPDATE_FLAGS} ${AFU_GBS_FILE} ${PAC_PCIE_SBD}.0

# System Tests
AFU_ELF_NAME ?= bin/${AFU_NAME}

test_sycl_afu: test_gbs
test_gbs: afu_host #gbs_configure ${AFU_GBS_FILE}
#	Run host application
	cd ${AFU_SW_DIR}; ./${AFU_ELF_NAME} ${TEST_ARGS}

test_ase: afu_host
	cd ${AFU_SW_DIR}; with_ase ./${AFU_ELF_NAME} ${TEST_ARGS}

#########
# ISA-L #
#########
# Alias ISA-L on plain_c ASP or IP implementations
# TODO: switch to test_gbs to minimize coupling with OneAPI

test_isal: test_ip_plain_c
oneapi_isal: oneapi_ip_plain_c

############
# Measures #
############

measure_all: measure_isal measure_asp_fpga measure_asp_plain_c # measure_sycl_afu

measure_plots:
	cd ${MEASURE_LATENCY_DIR}/plots; \
	python plot_latency.py ${MEASURE_LATENCY_DATA_DIR} ${PLOT_OUT_DIR}

measure_isal:
measure_asp_fpga:
measure_asp_plain_c: # For debug
measure_sycl_afu: # not yet available
measure_%:
	${MEASURE_LATENCY_DIR}/scripts/measure_latency_top.sh \
		$* 						\
		${MEASURE_NUM_REPS} 	\
		${MEASURE_MAX_DECODE}

clean_measure:
	rm -rf ${ROOT_DIR}/measures/latency/data/*

############
# Clean up #
############
# clean_fim_pr:
# 	rm -rf ${FIM_PR_BUILD_DIR}/

# clean_fim_flat:
# 	rm -rf ${FIM_PR_BUILD_DIR}/

clean_ase:
	rm -rf ${AFU_ASE_DIR}

clean_gbs:
#	GBS build directory
	rm -rf ${AFU_SYNTH_DIR}

clean_afu_host:
	${MAKE} -C ${AFU_SW_DIR} clean

clean_oneapi_ip_report:
#	SYCL IP
	rm -rf ${SYCL_IP_PRJ}
#	Exported SYCL IP
	rm -rf ${SYCL_IP_PRJ_AFU_EXPORT}

clean_oneapi_ip: clean_oneapi_ip_report
#	Build directory
	rm -rf ${SYCL_IP_BUILD_DIR}

clean_oneapi_asp:
#	Build directory
	rm -rf ${SYCL_ASP_BUILD_DIR}

clean_all: # clean_ase clean_sw clean_gbs clean_ase clean_oneapi_ip clean_oneapi_asp 
	# TBD: clean for all flows?

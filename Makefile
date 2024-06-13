# Default variables value
TEST_ARGS ?=
RESEED_FITTER ?= 0
INTERRUPT_EVENTS ?= 0
NO_ASE_SUPPORT ?= 0
AFU_HOST_DEFINES ?=
ACL_DEVICE ?= acl0 # Assuming only one device connected
SYCL_DEBUG ?= 0
SYCL_FAST_COMPILE ?= 0
SYCL_CMAKE_FLAGS ?=
MMD_DEBUG ?= 0
FIM_IMAGE ?= ${FIM_IMAGE_USER1}
FIM_UPDATE_DEBUG ?= 0
RSU_DEBUG ?= 0

# Environment check
ifndef ROOT_DIR
$(error Setup script settings.sh has not been sourced, aborting)
endif

all: help

help:
	@cat "${ROOT_DIR}/scripts/make_help.txt"
	
#######
# FIM #
#######

# FIM config file
BASE_OFS_TOP_QSF = ${OFS_BUILD_ROOT}/syn/board/htk-nc220-agf014/syn_top/ofs_top.qsf
PR_BUILD_OFS_TOP_QSF = ${OPAE_PLATFORM_ROOT}/hw/lib/build/syn/board/htk-nc220-agf014/syn_top/ofs_top.qsf

# Copy OFSS configuration files
# NOTE: this requires th OFSS configuration to be exposed in ${OFSS_CONFIG_DIR} 
fim_ofss_config: ${OFSS_CONFIG_DIR}
	cp -vr ${OFSS_CONFIG_DIR}/* ${OFS_BUILD_ROOT}/tools/ofss_config/

# Reseed fitter
fim_reseed_fitter:
	if [ ${RESEED_FITTER} -eq 1 ]; then \
		sed -E -i "s/SEED .+/SEED $(shell bash -c 'echo $$RANDOM')/g" ${BASE_OFS_TOP_QSF}; \
		sed -E -i "s/SEED .+/SEED $(shell bash -c 'echo $$RANDOM')/g" ${PR_BUILD_OFS_TOP_QSF}; \
	fi

SEED_DEFAULT = 3
fim_restore_fitter_seed:
	sed -E -i "s/SEED .+/SEED ${SEED_DEFAULT}/g" ${BASE_OFS_TOP_QSF}
	sed -E -i "s/SEED .+/SEED ${SEED_DEFAULT}/g" ${PR_BUILD_OFS_TOP_QSF}

INCLUDE_PR_STRING =set_global_assignment -name VERILOG_MACRO \"INCLUDE_PR\"
fim_remove_pr:
	sed -i "s/${INCLUDE_PR_STRING}/#${INCLUDE_PR_STRING}/g" ${BASE_OFS_TOP_QSF}

fim_include_pr:
	sed -i "s/#${INCLUDE_PR_STRING}/${INCLUDE_PR_STRING}/g" ${BASE_OFS_TOP_QSF}

# Restore FIM default configuration
fim_restore_defaults: fim_restore_fitter_seed fim_include_pr fim_restore_pr

# Override script
TARGET_PR_ASSIGNMENTS_TCL=${OFS_ROOTDIR}/syn/board/${BOARD}/setup/pr_assignments.tcl
fim_resize_pr: 
	if [ ${RESIZE_PR} -eq 1 ]; then \
		cp -v ${ROOT_DIR}/fim_flow/resize_pr/resize_pr_assignments.tcl ${TARGET_PR_ASSIGNMENTS_TCL}; \
	fi
fim_restore_pr: 
		cp -v ${ROOT_DIR}/fim_flow/resize_pr/default_pr_assignments.tcl ${TARGET_PR_ASSIGNMENTS_TCL}

fim_build_pr: fim_resize_pr # Only for PR builds
fim_build_flat: fim_remove_pr # Only for flat builds
fim_build_%: fim_ofss_config fim_reseed_fitter
#	TODO: remove the need to source this script from HTS
	cd ${HTS_RELEASE}; ./setup_env.sh; \
		${ROOT_DIR}/fim_flow/build_fim.sh --$* ${OFSS_CONFIG}
# 	Restore FIM defaults
	${MAKE} fim_restore_defaults

FPGASUPDATE_FLAGS ?=
ifeq (${FIM_UPDATE_DEBUG}, 1)
	FPGASUPDATE_FLAGS += --log-level debug 
endif
fim_update: 
#	Update flash images 
	sudo fpgasupdate ${FPGASUPDATE_FLAGS} ${FIM_IMAGE} ${PAC_PCIE_SBD}.0
	@echo "[INFO] To configure the new FIM, powercycle the PAC with:"
	@echo "[INFO]     ${MAKE} pac_powercycle_<bootpage>"

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
# SYCL CMake sources
SYCL_IP_CMAKE_SOURCES := ${SYCL_SRC_DIR}/CMakeLists.txt ${SYCL_SRC_DIR}/src/CMakeLists.txt

MMD_DEBUG ?=
ifeq (${MMD_DEBUG}, 1)
	ONEAPI_DEBUG_ENV := MMD_ENABLE_DEBUG=1 MMD_PROGRAM_DEBUG=1
endif

MMD_DEBUG ?=
ifeq (${SYCL_DEBUG}, 1)
	SYCL_CMAKE_FLAGS += --trace-expand
endif

# SYCL CMake flags
# TODO: test
#	-⁠Xsoptimize=throughput-area-balanced, reduced throughput
#	-Xsoptimize=area, decreases fMax
USER_HARDWARE_FLAGS ?=
# USER_HARDWARE_FLAGS += "-Xsoptimize=latency -Xsno-hardware-kernel-invocation-queue"
SYCL_CMAKE_FLAGS += -DUSER_HARDWARE_FLAGS=${USER_HARDWARE_FLAGS}

# SYCL IP-related environment
SYCL_IP_ENV += RS_SCHEMA=${RS_SCHEMA} \
				SYCL_IP_NAME=${SYCL_IP_NAME} \
				SYCL_IP_BUILD_DIR=${SYCL_IP_BUILD_DIR} \
				SYCL_IP_PRJ=${SYCL_IP_PRJ}

# CMake environment
CMAKE_ENV = SYCL_IP_NAME=${SYCL_IP_NAME} \
			${SYCL_IP_ENV}
CMAKE = ${CMAKE_ENV} cmake .. ${SYCL_CMAKE_FLAGS}

# Build-time variables
SCYL_FREQ_MHZ ?= 600
ifdef SCYL_FREQ_MHZ
	SYCL_CXX_DEFINES += -DSCYL_FREQ_MHZ=${SCYL_FREQ_MHZ}
endif
ifeq (${MULTI_ERASURE_SIMPLE}, 1)
	SYCL_CXX_DEFINES += -DMULTI_ERASURE_SIMPLE
endif
ifeq (${SYCL_DEBUG}, 1)
	SYCL_CXX_DEFINES += -DDEBUG
endif
LOOP_COALESCE ?= 0
ifeq (${LOOP_COALESCE}, 1)
	SYCL_CXX_DEFINES += -DLOOP_COALESCE
endif
SYCL_MAKE_ENV = "CXX_DEFINES=${SYCL_CXX_DEFINES}"

####################
# ONE API ASP Flow #
####################

oneapi_asp_build_aocx:
	cd ${OFS_ASP_ROOT}; \
	./scripts/build-default-aocx.sh -b ${OFS_ASP_BOARD_VARIANT}
	@echo "[INFO] Built AOCX for PR tree ${OPAE_PLATFORM_ROOT}"

aocl_bsp_build:
	cd ${OFS_ASP_ROOT}; ./scripts/build-bsp.sh

aocl_bsp_install:
	${ONEAPI_DEBUG_ENV} aocl install ${OFS_ASP_ROOT}

aocl_bsp_uninstall:
	${ONEAPI_DEBUG_ENV} aocl uninstall ${OFS_ASP_ROOT}

# Make sure to make opae.io_bind_one with the correct VF number
aocl_aocx_initialize:
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

# requires CL_CONTEXT_MPSIM_DEVICE_INTELFPGA=1
test_asp_fpga_sim: 
#	Run simulation
	cd ${SYCL_ASP_BUILD_DIR}; \
	CL_CONTEXT_MPSIM_DEVICE_INTELFPGA=1 \
	./${SYCL_IP_NAME}.fpga_sim ${TEST_ARGS}

test_asp_plain_c:
test_asp_fpga: # Make sure to make aocl_aocx_initialize first
test_asp_fpga_emu:
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
ifeq (${DEBUG_OSW}, 1)
	AFU_HOST_DEFINES += -DDEBUG_OSW
endif
ifeq (${INTERRUPT_EVENTS}, 1)
	AFU_HOST_DEFINES += -DINTERRUPT_EVENTS
endif
ifeq (${NO_ASE_SUPPORT}, 1)
	AFU_HOST_DEFINES += -DNO_ASE_SUPPORT
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
	vsim $<										\
		-do ${ROOT_DIR}/scripts/add_waves.do 	\
		-debugdb # ${AFU_ASE_DIR}/work/vsim.dbg

# Setup AFU sysnthesis environment
${AFU_SYNTH_DIR}: ${OPAE_PLATFORM_ROOT}
	${SYCL_IP_ENV} ${AFU_FLOW_DIR}/afu_synth_setup.sh

# Build Green Bitstream
# This takes at least 40 minutes...
gbs: ${AFU_SYNTH_DIR}
	${SYCL_IP_ENV} ${AFU_FLOW_DIR}/afu_synth_build.sh

gbs_configure:
#	Configure PR slot with GBS
# sudo fpgaconf ${AFU_GBS_FILE}
	sudo fpgasupdate ${FPGASUPDATE_FLAGS} ${AFU_GBS_FILE} ${PAC_PCIE_SBD}.0

# System Tests
AFU_ELF_NAME ?= bin/${AFU_NAME}

test_sycl_afu: test_gbs
test_gbs: #afu_host
#	Run host application
	cd ${AFU_SW_DIR}; ./${AFU_ELF_NAME} ${TEST_ARGS}

test_ase: afu_host
	cd ${AFU_SW_DIR}; with_ase ./${AFU_ELF_NAME} ${TEST_ARGS}

#########
# ISA-L #
#########
# Alias ISA-L on SYCL AFU host code
test_isal: test_gbs

##################
# Measures Power #
##################
# TODO: test, evaluate and refine
measure_power:
	${ROOT_DIR}/measures/power/measure_power_top.sh ${MEASURE_POWER_DATA_DIR}

############################
# Measures (single-thread) #
############################
# Aliases to setup test preconditions
# NOTE: explicitly separate stup from test simplifies single/multi-threaded tests handling
setup_isal: afu_host
setup_asp_fpga: aocl_aocx_initialize oneapi_asp_fpga
setup_asp_fpga_sim: oneapi_asp_fpga_sim
setup_asp_plain_c: oneapi_asp_plain_c
setup_sycl_afu: afu_host gbs_configure

MULTI_THREAD ?= 0
SBDF ?= ${PAC_PCIE_SBD}.${FIRST_AFU_VF}

measure_all: measure_isal measure_asp_fpga measure_asp_plain_c measure_sycl_afu

measure_isal:
measure_asp_fpga:
measure_asp_fpga_sim:
measure_asp_plain_c: # For debug
measure_sycl_afu:
measure_%:
	${MEASURE_LATENCY_DIR}/scripts/measure_latency_top.sh \
		$* 						\
		${MEASURE_NUM_REPS} 	\
		${MEASURE_MAX_DECODE} 	\
		${MULTI_THREAD}			\
		${SBDF}

#########################
# Plots (single-thread) #
#########################

plot_latency:
	cd ${MEASURE_LATENCY_DIR}/plots; \
	python plot_latency.py ${MEASURE_LATENCY_DATA_DIR} ${MEASURE_LATENCY_PLOT_OUT_DIR}

plot_cycles:
	cd ${MEASURE_LATENCY_DIR}/plots; \
	python plot_cycles.py ${MEASURE_CYCLES_DATA_DIR} ${MEASURE_CYCLES_PLOT_OUT_DIR}

plot_power:
	cd ${MEASURE_LATENCY_DIR}/plots; \
	python plot_power.py ${MEASURE_POWER_DATA_DIR} ${MEASURE_POWER_PLOT_OUT_DIR}

###########################
# Measures (multi-thread) #
###########################
NUM_THREADS ?= 2

# measure_all: measure_isal measure_asp_fpga measure_asp_plain_c measure_sycl_afu

measure_multi_thread_isal:
# measure_multi_thread_asp_fpga: # Not supported
measure_multi_thread_asp_plain_c: # For debug
measure_multi_thread_sycl_afu:
measure_multi_thread_%:
	${MEASURE_LATENCY_DIR}/scripts/measure_latency_multi_thread.sh \
		measure_$* 		\
		${NUM_THREADS} 	\
		${SBDF}

########################
# Plots (multi-thread) #
########################
plot_multi_thread_latency:
	cd ${MEASURE_LATENCY_DIR}/plots; \
	python plot_multi_thread_latency.py 	\
		${MEASURE_LATENCY_DATA_DIR}		\
		${MEASURE_LATENCY_PLOT_OUT_DIR}

# plot_multi_thread_power:
# 	cd ${MEASURE_LATENCY_DIR}/plots; \
# 	python plot_power.py ${MEASURE_LATENCY_DATA_DIR} ${MEASURE_LATENCY_PLOT_OUT_DIR}


############
# Clean up #
############
clean_measure_latency:
	rm -rf ${MEASURE_LATENCY_DATA_DIR}

clean_fim_pr:
	rm -rf ${FIM_PR_BUILD_DIR}

clean_fim_flat:
	rm -rf ${FIM_FLAT_BUILD_DIR}

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
	rm -rf ${SYCL_IP_PRJ_ARCHIVE}
#	Exported SYCL IP
	rm -rf ${SYCL_IP_PRJ_AFU_EXPORT}

clean_oneapi_ip: clean_oneapi_ip_report
#	Build directory
	rm -rf ${SYCL_IP_BUILD_DIR}

clean_oneapi_asp:
#	Build directory
	rm -rf ${SYCL_ASP_BUILD_DIR}

clean_aocx:
	rm -rf ${AOCX_ROOT}

clean_all: # clean_ase clean_sw clean_gbs clean_ase clean_oneapi_ip clean_oneapi_asp 
	# TBD: clean for all flows?

# Default variables value
TEST_ARGS ?=

# NOTE: Most of this flow does rely on file timestamps, therefore, it is going to build most of the targets for every call

all: help

help:
	@cat "scripts/make_help.txt"
	
#######
# FIM #
#######
OFSS_CONFIG_DIR := ${ROOT_DIR}/fim_flow/ofss_configs/ofss_config_${OFSS_CONFIG}
fim_build_pr:
fim_build_flat:
fim_build_%:
#	Copy OFSS configuration files
	if [ -d ${OFSS_CONFIG_DIR} ]; then \
		cp -vr ${OFSS_CONFIG_DIR}/* ${OFS_BUILD_ROOT}/tools/ofss_config/; \
	fi
	cd ${HTS_RELEASE}; \
	./setup_env.sh; \
	${ROOT_DIR}/fim_flow/build_fim.sh --$* ${OFSS_CONFIG}

fim_update: 
#	Update flash images 
	sudo fpgasupdate --log-level debug ${FIM_IMAGE_USER1} ${PAC_PCIE_SBD}.0
#	No need to update also pase user2, for now
# sudo fpgasupdate --log-level debug ${FIM_IMAGE_USER2} ${PAC_PCIE_SBD}.0
	@echo "To configure the new FIM, powercycle the PAC with:"
	@echo "    ${MAKE} pac_powercycle_user1"

pac_powercycle_user1:
pac_powercycle_user2:
pac_powercycle_factory:
pac_powercycle_%:
# 	Power cycle PAC
	sudo rsu --debug fpga --page=$* ${PAC_PCIE_SBD}.0

opae.io_bind:
	${ROOT_DIR}/scripts/opae.io_bind.sh

opae.io_bind_one:
	sudo pci_device ${PAC_PCIE_SBD}.0 vf 1
	sudo opae.io -d ${PAC_PCIE_SBD}.0 init ${USER}:${USER}
	opae.io ls

pac_hot_plug:
	sudo pci_device ${PAC_PCIE_SBD}.0 unplug
	sudo pci_device ${PAC_PCIE_SBD}.0 plug

####################
# ONE API ASP Flow #
####################
ONEAPI_DEBUG_ENV := MMD_ENABLE_DEBUG=1  \
					MMD_PROGRAM_DEBUG=1

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

ACL_DEVICE = acl0 # Assuming only one device connected
aocl_aocx_initalize:
	sudo pci_device ${PAC_PCIE_SBD}.0 vf 1 ; 				\
	sudo opae.io init -d ${PAC_PCIE_SBD}.5 ${USER}:${USER};	\
	${ONEAPI_DEBUG_ENV} aocl initialize ${ACL_DEVICE} ${OFS_ASP_BOARD_VARIANT} 

oneapi_asp_cmake: 
	mkdir ${SYCL_ASP_BUILD_DIR};	\
	cd ${SYCL_ASP_BUILD_DIR};		\
	cmake .. -DFPGA_DEVICE=${OFS_ASP_FPGA_DEVICE} 

oneapi_asp_fpga_emu:
oneapi_asp_fpga_sim:
oneapi_asp_report:
oneapi_asp_fpga:
oneapi_asp_%: oneapi_asp_cmake
	cd ${SYCL_IP_DIR}/build_asp; \
	make $*

#############################
# ONE API IP Authoring Flow #
#############################
# Wrap these variables in a single list
SYCL_IP_ENV += RS_SCHEMA=${RS_SCHEMA} \
				SYCL_IP_NAME=${SYCL_IP_NAME} \
				SYCL_IP_BUILD_DIR=${SYCL_IP_BUILD_DIR} \
				SYCL_IP_PRJ=${SYCL_IP_PRJ}

SYCL_IP_DEBUG ?= 0
FAST_COMPILE ?= 0
ifeq (${FAST_COMPILE}, 1)
	CMAKE_FLAGS += "-DUSER_HARDWARE_FLAGS=-Xsfast-compile"
endif
ifeq (${SYCL_IP_DEBUG}, 1)
	CMAKE_FLAGS += "--trace-expand"
endif

# Environment setup for cmake
CMAKE_ENV = USER_HARDWARE_FLAGS=${USER_HARDWARE_FLAGS} \
			SYCL_IP_NAME=${SYCL_IP_NAME} \
			${SYCL_IP_ENV} 

CMAKE_FLAGS += -DFPGA_DEVICE=${AGILEX7_PART_NUMBER} 
oneapi_ip_cmake: ${SYCL_IP_BUILD_DIR}
${SYCL_IP_BUILD_DIR}: 
	mkdir ${SYCL_IP_BUILD_DIR};	\
	cd ${SYCL_IP_BUILD_DIR};		\
	${CMAKE_ENV} cmake .. ${CMAKE_FLAGS}

oneapi_ip_report: oneapi_ip_cmake ${SYCL_IP_PRJ}
${SYCL_IP_PRJ}: 
	cd ${SYCL_IP_BUILD_DIR}; \
	make report ${SYCL_IP_ENV}

oneapi_ip_report_open:
	firefox ${SYCL_IP_PRJ}/reports/report.html &

oneapi_ip: oneapi_ip_report
	@echo "[INFO] Copying IP files to AFU workdir"
#	SYCL IP output files
	cp -r ${SYCL_IP_PRJ} ${AFU_HW_DIR}
#	SYCL IP CSR map headers to sw project
	cp -r ${SYCL_IP_PRJ}/include/* ${AFU_SW_DIR}
#	Update SYCL IP CSR offset
	make -C ${AFU_SW_DIR} register_map_offsets

oneapi_ip_fpga_emu: oneapi_ip_cmake
	cd ${SYCL_IP_BUILD_DIR}; \
	make fpga_emu

oneapi_ip_emu:
	cd ${SYCL_IP_BUILD_DIR}; ./${SYCL_IP_NAME}.fpga_emu ${TEST_ARGS}

#######
# AFU #
#######

# Build host application
afu_host:
	${MAKE} -C ${AFU_SW_DIR} clean all RS_SCHEMA=${RS_SCHEMA};

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

# Build Green Bitstream
# This takes around 40 minutes...
gbs: ${AFU_SYNTH_DIR} #oneapi_ip
${AFU_SYNTH_DIR}: ${OPAE_PLATFORM_ROOT} clean_gbs
	${SYCL_IP_ENV} ${AFU_FLOW_DIR}/afu_synth.sh

gbs_configure:
#	Configure PR slot with GBS
# sudo fpgaconf ${AFU_GBS_FILE}
	sudo fpgasupdate ${AFU_GBS_FILE} ${PAC_PCIE_SBD}.0

# System Tests
AFU_ELF_NAME ?= bin/${AFU_NAME}
TEST_ARGS	 ?=

test_gbs: afu_host #gbs_configure ${AFU_GBS_FILE}
#	Run host application
	cd ${AFU_SW_DIR}; ./${AFU_ELF_NAME} ${TEST_ARGS}

test_ase: afu_host
	cd ${AFU_SW_DIR}; with_ase ./${AFU_ELF_NAME} ${TEST_ARGS}

############
# Clean up #
############
# clean_fim:
# 	rm -rf ${FIM_BUILD_DIR}/

clean_afu_host:
	${MAKE} -C ${AFU_SW_DIR} clean

clean_ase:
	rm -rf ${AFU_ASE_DIR}

clean_gbs:
#	GBS build directory
	rm -rf ${AFU_SYNTH_DIR}

clean_sw:
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
	rm -rf ${ONEAPI_IP_DIR}/build_asp
	# TBD

clean_all: # clean_ase clean_sw clean_gbs clean_ase clean_oneapi_ip clean_oneapi_asp 
	# TBD: clean for all flows?

# change output color
COLOR_GREEN := tput setaf 2
COLOR_RED := tput setaf 1
COLOR_NORMAL := tput setaf 7

all: help

help: # TBD
	@echo "${MAKE} targets:"
	@echo "	fim_build_pr          Build FIM and PR-tree (takes around 1h45m)"
	@echo "	fim_build_flat        Build flat FIM (takes around 1h10m)"
	@echo "	fim_update            Update flash images and powercycle the board"
	@echo "	pac_powercycle_<page> Power cycle from page <user1|user2|factory>, necessary after FIM udpate (fim_update)"
	@echo "	opae.io_bind          Bind all VFs to VFIO driver"
	@echo "	opae.io_unbind        Unbind all VFs to VFIO driver"
	@echo "	oneapi_ip             Build oneapi design in oneapi_afu/...."
	@echo "	afu_host              Build host application"
	@echo "	ase_setup             Setup and launch ASE simulation environment {locks a terminal}"
	@echo "	ase_launch            Subsequent launches of ASE simulator {locks a terminal}"
	@echo "	ase_waves             Open simulated waveforms in ${AFU_ASE_DIR}/work/vsim.wlf"
	@echo "	gbs                   Build green bitstream (takes around 40 minutes)"
	@echo "	gbs_configure         Configure GBS in PR-slot. On error, you must first unbind all VFs with opae.io_unbind"
# @echo "	sign_gbs              sign green bitstreaam with empty key"
	@echo "	test_gbs              Run host application against hadware"
	@echo "	test_ase              Run host application against ASE simulator (requires ${MAKE} ase_setup or ${MAKE} ase_launch in another terminal)"
# @echo "	clean_fim             Clean FIM build"
	@echo "	clean_oneapi          Clean OneAPI build"
	@echo "	clean_gbs             Clean gbs build"
	@echo "	clean_ase             Clean ASE simulator setup"
	@echo "	clean_sw              Clean software build"
	@echo "	clean_all             TBD"
	@echo "${MAKE} variables:TBD"
	@echo "	OFSS_CONFIG           Suffix to identify FIM build and OFSS flow"
# @echo "RS_SCHEMA            Reed-Solomon code schema [RS_3_2, RS_6_3, RS_10_4]"
# @echo "GBS_NAME             Name of the final signed bitstream"
# @echo "TEST_ARGS            Arguments to pass to the host exe in test_gbs and test_ase {try with -h}"

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
	cd ${HTS_FIM_RELEASE}; \
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
# 	Power cycle on page user1
	sudo rsu  --debug fpga --page=$* ${PAC_PCIE_SBD}.0

opae.io_bind: #gbs_configure
	${ROOT_DIR}/scripts/opae.io_bind.sh

opae.io_unbind:
	sudo pci_device ${PAC_PCIE_BD}.0 vf 0

#############################
# ONE API IP Authoring Flow #
#############################

oneapi_ip:
# TBD

#######
# AFU #
#######

# Build host application
afu_host:
	${MAKE} -C ${AFU_SW_DIR} clean all;

# Build and launch simulation
ase_setup: clean_ase
#	Setup and launch simulator
	${AFU_FLOW_DIR}/afu_ase.sh

# Launch simulation without rebuilding it
ase_launch: ${AFU_ASE_DIR}
#	Clear previous runs' lock file
	rm -vf ${AFU_ASE_DIR}/work/.ase_ready.pid;
#	Launch simulation
	${MAKE} -C ${AFU_ASE_DIR} sim

# Open Wafeform Log File
ase_waves: ${AFU_ASE_DIR}/work/vsim.wlf
# ${MAKE} -C ${AFU_ASE_DIR} wave
	vsim $<										\
		-do scripts/add_waves.do 				\
		-debugdb # ${AFU_ASE_DIR}/work/vsim.dbg

#########################
# Build Green Bitstream #
#########################

GBS_FILE ?= ${AFU_SYNTH_DIR}/${AFU_NAME}.gbs

# This takes around 40 minutes...
gbs: ${AFU_SYNTH_DIR}
${AFU_SYNTH_DIR}: ${OPAE_PLATFORM_ROOT}	clean_gbs
	${AFU_FLOW_DIR}/afu_synth.sh

# {Empty-}Sign bitstreaam
# sign_gbs: gbs
# 	mkdir -p ${BACKUP_DIR}
# 	PACSign PR -t UPDATE -H openssl_manager -i ${GBS_FILE} -o {AFU_SYNTH_DIR}/${AFU_NAME}.gbs && \
# 	${COLOR_GREEN}; echo "INFO: Signed bitstream is at {AFU_SYNTH_DIR}/${AFU_NAME}.gbs"; \
# 	${COLOR_NORMAL}

gbs_configure:
#	Configure PR with GBS
# sudo fpgaconf ${GBS_FILE}
	sudo fpgasupdate ${GBS_FILE} ${PAC_PCIE_SBD}.0

###############
# System Test #
###############

AFU_ELF_NAME ?= ${AFU_NAME}
TEST_ARGS	 ?=

test_gbs: afu_host gbs_configure ${GBS_FILE} 
#	Run host application
	cd ${AFU_SW_DIR}/bin; ./${AFU_ELF_NAME} ${TEST_ARGS}

test_ase: afu_host
	cd ${AFU_SW_DIR}/bin; with_ase ./${AFU_ELF_NAME} ${TEST_ARGS}

############
# Clean up #
############
# clean_fim:
# 	rm -rf ${FIM_BUILD_DIR}/

clean_ase:
	rm -rf ${AFU_ASE_DIR}/

clean_gbs:
	rm -rf ${AFU_SYNTH_DIR}

clean_sw:
	${MAKE} -C ${AFU_SW_DIR} clean

clean_oneapi:
	${MAKE} -C ${ONEAPI_WORK_DIR} clean_cosim RS_SCHEMA=${RS_SCHEMA}
	rm -rf ${ONEAPI_WORK_DIR}/../qsys/oneapi_outputs

clean_all: clean_sw clean_gbs clean_ase clean_oneapi
	# TBD: clean for all afus

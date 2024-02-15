# AFU_WORK_DIR=${ROOT_DIR}/oneapi_afu/

# change output color
COLOR_GREEN := tput setaf 2
COLOR_RED := tput setaf 1
COLOR_NORMAL := tput setaf 7

all: help

help: # TBD
	@echo "${MAKE} targets:"
	@echo "	fim_pr				build FIM and PR-tree (takes around 1h45m)"
	@echo "	fim_flat			build FIM (takes around ?)"
	@echo "	oneapi_ip			build oneapi design in oneapi_afu/...."
	@echo "	afu_host			build host application"
	@echo "	ase_setup			setup and launch ASE simulation environment {locks a terminal}"
	@echo "	ase_launch			subsequent launches of ASE simulator {locks a terminal}"
	@echo "	ase_waves			open simulated waveforms in ${AFU_ASE_DIR}/work/vsim.wlf"
	@echo "	gbs				generate green bitstream (takes around 40 minutes)"
	@echo "	sign_gbs			sign green bitstreaam with empty key"
	@echo "	test_gbs			run host application on hadware"
	@echo "	test_ase			run host application against ASE simulator (requires ${MAKE} ase_setup or ${MAKE} ase_launch in another terminal)"
	@echo "	clean_oneapi			clean oneapi build"
	@echo "	clean_gbs			clean gbs build"
	@echo "	clean_ase			clean ASE simulator setup"
	@echo "	clean_sw			clean software build"
	@echo "	clean_all			TBD"
# @echo "${MAKE} variables:TBD"
# @echo "RS_SCHEMA				Reed-Solomon code schema [RS_3_2, RS_6_3, RS_10_4]"
# @echo "GBS_NAME				Name of the final signed bitstream"
# @echo "TEST_ARGS				Arguments to pass to the host exe in test_gbs and test_ase {try with -h}"

#######
# FIM #
#######
fim_pr:
fim_flat:
fim_%:
	cd ${HTS_FIM_RELEASE}; \
	./setup_env.sh; \
	./build_fim.sh --$*

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
ase_setup:
#	Setup and launch simulator
	${AFU_WORK_DIR}/afu_ase.sh

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
# This takes around 40 minutes...
gbs: ${AFU_SYNTH_DIR}
${AFU_SYNTH_DIR}: ${OPAE_PLATFORM_ROOT}	
# ${AFU_WORK_DIR}/afu_synth.h;
#	Removing old directory, if any
	rm -rf ${AFU_SYNTH_DIR}
# 	Launch setup script
	cd ${AFU_WORK_DIR}; 				\
	afu_synth_setup                 	\
    	--sources ${AFU_SOURCE_LIST}    \
    	${AFU_SYNTH_DIR}
# 	Launch build script
	cd ${AFU_SYNTH_DIR}; \
	${OPAE_PLATFORM_ROOT}/bin/afu_synth

# {Empty-}Sign bitstreaam
sign_gbs: gbs
	mkdir -p ${BACKUP_DIR}
	PACSign PR -t UPDATE -H openssl_manager -i ${AFU_SYNTH_DIR}/${AFU_NAME}.gbs -o ${BACKUP_DIR}/${GBS_NAME}.gbs && \
	${COLOR_GREEN}; echo "INFO: Signed bitstream is at ${BACKUP_DIR}/${GBS_NAME}.gbs"; \
	${COLOR_NORMAL}

configure_gbs: sign_gbs
#	Configure PR with GBS
# sudo fpgaconf {AFU_SYNTH_DIR}/${AFU_NAME}.gbs
	sudo fpgasupdate ${AFU_SYNTH_DIR}/${AFU_NAME}.gbs <N6001 SKU2 PCIe b:d.f>
#	Create the Virtual Functions {VFs}:
	sudo pci_device b1:00.0 vf 3
#	Bind VFs to VFIO driver
	sudo opae.io init -d 0000:b1:00.3

###############
# System Test #
###############

test_gbs: afu_host configure_gbs ${BACKUP_DIR}/${GBS_NAME}.gbs
#	Configure FPGA
# fpgaconf ${BACKUP_DIR}/${GBS_NAME}.gbs
#	Run host application
	cd ${AFU_SW_DIR}; ./${AFU_ELF_NAME} ${TEST_ARGS}

test_ase: afu_host
	cd ${AFU_SW_DIR}; with_ase ./${AFU_ELF_NAME} ${TEST_ARGS}

############
# Clean up #
############
clean_ase:
	rm -rf ${AFU_ASE_DIR}/

clean_gbs:
	rm -rf ${AFU_SYNTH_DIR}

clean_sw:
	${MAKE} -C ${AFU_SW_DIR} clean

clean_oneapi:
	${MAKE} -C ${oneapi_WORK_DIR} clean_cosim RS_SCHEMA=${RS_SCHEMA}
	rm -rf ${oneapi_WORK_DIR}/../qsys/oneapi_outputs

clean_all: clean_sw clean_gbs clean_ase clean_oneapi

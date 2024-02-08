# AFU_WORK_DIR=$(ROOT_DIR)/oneapi_afu/
# ONEAPI_WORK_DIR=$(WORK_DIR)/TBD

# change output color
COLOR_GREEN := tput setaf 2
COLOR_RED := tput setaf 1
COLOR_NORMAL := tput setaf 7

all: help
ifndef $(HTS_FIM_RELEASE)
	$(error [ERROR] HTS_FIM_RELEASE undefined, source settings.sh)
endif


help: # TBD
	# @echo "Make targets:"
	# @echo "	oneapi					build oneapi design in oneapi_afu/hw/rtl/oneapi"
	# @echo "	afu_host		build host application"
	# @echo "	afu_host_ase:	build host application for simulation environment"
	# @echo "	sim_setup			first setup and launch of simulation environment after RTL build (locks a terminal)"
	# @echo "	sim_launch			subsequent launches of simulator (locks a terminal)"
	# @echo "	wave				open simulated waveforms in oneapi_afu/build_ase_dir_<RS_SCHEMA>/work/vsim.wlf"
	# @echo "	gbs					generate green bitstream"
	# @echo "	sign_gbs			sign green bitstreaam with empty key"
	# @echo "	test_gbs			run host application"
	# @echo "	test_ase			run host application against simulator"
	# @echo "						(requires make sim_setup or make sim_launch in another terminal)"
	# @echo "	clean_sim			clean simulator setup"
	# @echo "	clean_gbs			clean synthesis"
	# @echo "	clean_sw			clean software build"
	# @echo "	clean_oneapi			clean oneapi build"
	# @echo "	clean_all			Run all of the above clean recepies"
	# @echo "Make variables:"
	# @echo "RS_SCHEMA			Reed-Solomon code schema [RS_3_2, RS_6_3, RS_10_4]"
	# @echo "GBS_NAME				Name of the final signed bitstream"
	# @echo "TEST_ARGS			Arguments to pass to the host exe in test_gbs and test_ase (try with -h)"
	# @echo "TRANSPOSE_64B		Set to 1 to compile afu_host[_ase] variant with 64B transposition"
	
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
	make -C $(AFU_SW_DIR) clean all;

# Build and launch simulation
ase_setup: 
	@$(COLOR_GREEN); echo "Once simulator setup finishes, in another terminal: "
	@$(COLOR_GREEN); echo "\texport ASE_WORKDIR=$(AFU_ASE_DIR)/work?"
	@$(COLOR_GREEN); echo "\tRun: afu_host_ase"
	@$(COLOR_NORMAL);
	# Setup and launch simulator 
	cd $(AFU_WORK_DIR); ./afu_ase.sh

# Launch simulation without rebuilding it
ase_launch:
	@if [ ! -d "$(AFU_ASE_DIR)" ]; then \
		echo "[ERROR] Missing ASE directory $(AFU_ASE_DIR)"; exit 1; \
	fi
	cd $(AFU_ASE_DIR); rm -vf work/.ase_ready.pid; make sim

# Open Wafeform Log File
wave: $(AFU_ASE_DIR)/work/vsim.wlf
	vsim $(AFU_ASE_DIR)/work/vsim.wlf -do scripts/add_waves.do

#########################
# Build Green Bitstream # 
#########################
gbs:
	@if [ -d "$(AFU_SYNTH_DIR)" ]; then \
		$(COLOR_GREEN); echo "INFO: GBS directory $(AFU_SYNTH_DIR) exists, skipping rebuild"; \
		$(COLOR_NORMAL); \
	else \
		echo "This takes around 40 minutes..."; \
		cd ${AFU_WORK_DIR}; bash -x afu_synth.sh; \
	fi

# (Empty-)Sign bitstreaam
sign_gbs: gbs
	mkdir -p $(BACKUP_DIR)
	PACSign PR -t UPDATE -H openssl_manager -i $(AFU_SYNTH_DIR)/oneapi_afu.gbs -o $(BACKUP_DIR)/$(GBS_NAME).gbs && \
	$(COLOR_GREEN); echo "INFO: Signed bitstream is at $(BACKUP_DIR)/$(GBS_NAME).gbs"; \
	$(COLOR_NORMAL)

configure_gbs: sign_gbs
#	Configure PR with GBS
	sudo fpgasupdate $(AFU_SYNTH_DIR)/${AFU_NAME}.gbs <N6001 SKU2 PCIe b:d.f>
#	Create the Virtual Functions (VFs):
	sudo pci_device b1:00.0 vf 3
#	Bind VFs to VFIO driver
	sudo opae.io init -d 0000:b1:00.3

#######################
# System Test Recipes #
#######################

test_gbs: 
	# Configure FPGA
	fpgaconf $(BACKUP_DIR)/$(GBS_NAME).gbs
	# Build host application
	make -C $(WORK_DIR)sw RS_SCHEMA=$(RS_SCHEMA) clean all
	# Run host application
	cd $(WORK_DIR)sw; LD_LIBRARY_PATH=. ./afu_host $(TEST_ARGS)

test_ase: afu_host
	cd $(AFU_SW_DIR); with_ase ./$(AFU_NAME) $(TEST_ARGS)

####################
# Clean up recipes # 
####################
clean_sim:
	rm -rf $(AFU_ASE_DIR)/

clean_gbs: 
	rm -rf $(AFU_SYNTH_DIR)
clean_sw:
	make -C $(AFU_SW_DIR) clean

clean_oneapi:
	make -C $(oneapi_WORK_DIR) clean_cosim RS_SCHEMA=$(RS_SCHEMA)
	rm -rf $(oneapi_WORK_DIR)/../qsys/oneapi_outputs
	
clean_all: clean_sw clean_gbs clean_sim clean_oneapi

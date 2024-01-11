#################
# For synthesis #
#################

# From Hitek SFTP
#########################################
  export IOFS_BUILD_ROOT=$PWD/../build
#   cd $IOFS_BUILD_ROOT/intel-ofs-fim
  export OFS_ROOTDIR=$PWD
#   export WORKDIR=$OFS_ROOTDIR
  export VERDIR=$OFS_ROOTDIR/verification
#   export PATH=$PATH:/tools/altera/v21.4pro/quartus/bin/
#   export QUARTUS_ROOTDIR=/tools/altera/v21.4pro/quartus/
#   export QUARTUS_INSTALL_DIR=$QUARTUS_ROOTDIR
#   export IMPORT_IP_ROOTDIR=$QUARTUS_ROOTDIR/../ip
#   export IP_ROOTDIR=$QUARTUS_ROOTDIR/../ip
#   export OPAE_SDK_REPO_BRANCH=release/2.0.11
#   cd $IOFS_BUILD_ROOT/intel-ofs-fim
###############################################

# Note, OFS_ROOTDIR is the directory where you cloned the repo, e.g. /home/MyProject/intel-ofs-fim *

# Quartus Tools
# Note, QUARTUS_HOME is your Quartus installation directory, e.g. $QUARTUS_HOME/bin contains Quartus executable.

export WORKDIR=$OFS_ROOTDIR	
export QUARTUS_HOME=/home/vincenzo/intelFPGA_pro/22.1/quartus
export QUARTUS_ROOTDIR=$QUARTUS_HOME
export QUARTUS_INSTALL_DIR=$QUARTUS_ROOTDIR
export QUARTUS_ROOTDIR_OVERRIDE=$QUARTUS_ROOTDIR
export IMPORT_IP_ROOTDIR=$QUARTUS_ROOTDIR/../ip
export IP_ROOTDIR=$QUARTUS_ROOTDIR/../ip
export QSYS_ROOTDIR=$QUARTUS_ROOTDIR/../qsys/bin
export PATH=$QUARTUS_HOME/bin:$QUARTUS_HOME/qsys/bin:$QUARTUS_HOME/sopc_builder/bin/:$PATH

# Synopsys Verification Tools
# NOTE: MISSING
# export DESIGNWARE_HOME=/home/vincenzo/synopsys/vip_common/vip_Q-2020.03A
# export PATH=$DESIGNWARE_HOME/bin:$PATH
# export UVM_HOME=/home/vincenzo/synopsys/vcsmx/R-2020.12-SP2/linux64/rhel/etc/uvm
# export VCS_HOME=/home/vincenzo/synopsys/vcsmx/R-2020.12-SP2/linux64/rhel
# export PATH=$VCS_HOME/bin:$PATH
# export VERDIR=$OFS_ROOTDIR/verification/n6000/common
# export VIPDIR=$VERDIR

# OPAE-SDK release
export OPAE_SDK_REPO_BRANCH=release/2.1.1

# The following environment variables are required for compiling the AFU examples. 

# Location to clone the ofs-platform-afu-bbb repository which contains PIM files and AFU examples.
export OFS_PLATFORM_AFU_BBB=$IOFS_BUILD_ROOT/ofs-platform-afu-bbb

# Location to clone the intel-fpga-bbb repository which contain infrastructure shims, AFU samples and tutorials.
export FPGA_BBB_CCI_SRC=$IOFS_BUILD_ROOT/intel-fpga-bbb  

# OPAE_PLATFORM_ROOT points to a release tree configured with the Platform Interface Manager (PIM).  
export OPAE_PLATFORM_ROOT=$OFS_ROOTDIR/work/pr_build_template

# OPAE and MPF libraries must either be on the default linker search paths or on both LIBRARY_PATH and LD_LIBRARY_PATH.  
export OPAE_LOC=/usr
export LIBRARY_PATH=$OPAE_LOC/lib:$LIBRARY_PATH
export LD_LIBRARY_PATH=$OPAE_LOC/lib64:$LD_LIBRARY_PATH

# Header files from OPAE and MPF must either be on the default compiler search paths or on both C_INCLUDE_PATH and CPLUS_INCLUDE_PATH.
# export C_INCLUDE_PATH=/usr/src/debug/opae-2.1.1-1.el8.x86_64/tests/framework

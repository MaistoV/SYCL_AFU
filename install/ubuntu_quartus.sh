# Set working directory if not already set
# : ${WORK_DIR=/home/intelFPGA/intelFPGA_pro}
: ${WORK_DIR=/home/vincenzo/AGILEX/intel-ofs-2022.2-ubuntu-hitek}

# Move to working directory
cd $WORK_DIR

# Prerequisites
cd $WORK_DIR
sudo apt install -y gcc g++ make csh cmake libuuid1 autoconf automake bison libboost-dev \
            make libncurses5 grub2 bc flex libxml2 libxml2-dev libnsl2 \
            # glibc-locale-source ncurses-compat-libs 

# sudo localedef -f UTF-8 -i en_US en_US.UTF-8 
# sudo ln -s /usr/lib64/libncurses.so.6 /usr/lib64/libncurses.so.5 
sudo ln -s /usr/bin/python3 /usr/bin/python

# Download Quartus and device support
# TBD: wget is not going to work due to necessary prompt interaction
mkdir downloads
cd downloads
# wget Quartus-pro-22.1.0.174-linux.tar
# wget Quartus-pro-22.1.0.174-devices-4.tar
# Untar and install
mkdir quartus
cd quartus
tar xvf ../Quartus-pro-22.1.0.174-linux.tar
./
mkdir agilex
cd agilex
tar xvf ../Quartus-pro-22.1.0.174-devices-4.tar
./dev4_setup_pro.sh 

# Export Quartus binaries
export PATH=$PATH:/home/vincenzo/intelFPGA_pro/22.1/quartus/bin
export PATH=$PATH:/home/vincenzo/intelFPGA_pro/22.1/qsys/bin

# Apply patches
OTCSHARE_INTEL_OFS_DOCS_MAIN=otcshare_dumps/otcshare_22_Nov_2023/intel-ofs-docs-main
cd $OTCSHARE_INTEL_OFS_DOCS_MAIN/n6000/dev_guides/fim_dev/patch_v22_1
chmod +x quartus-22.1-0.04-linux.run 
chmod +x quartus-22.1-0.23-linux.run
chmod +x quartus-22.1-0.26-linux.run
chmod +x quartus-22.1-0.27-linux.run

sudo ./quartus-22.1-0.04-linux.run   
sudo ./quartus-22.1-0.23-linux.run
sudo ./quartus-22.1-0.26-linux.run
sudo ./quartus-22.1-0.27-linux.run

# Remove files
QUARTUS_22_1_BASE="/home/vincenzo/intelFPGA_pro/22.1"   
rm $QUARTUS_22_1_BASE/ip/altera/subsystems/pcie_ss/rtl/aldec/data_mover/pciess_dm_h2c_cpl_reordering.sv 
rm $QUARTUS_22_1_BASE/ip/altera/subsystems/pcie_ss/rtl/aldec/pciess_hip_if_adaptor.v
rm $QUARTUS_22_1_BASE/ip/altera/subsystems/pcie_ss/rtl/aldec/ptile_pciess_top.sv
rm $QUARTUS_22_1_BASE/ip/altera/subsystems/pcie_ss/rtl/cadence/data_mover/pciess_dm_h2c_cpl_reordering.sv
rm $QUARTUS_22_1_BASE/ip/altera/subsystems/pcie_ss/rtl/cadence/pciess_hip_if_adaptor.v
rm $QUARTUS_22_1_BASE/ip/altera/subsystems/pcie_ss/rtl/cadence/ptile_pciess_top.sv
rm $QUARTUS_22_1_BASE/ip/altera/subsystems/pcie_ss/rtl/intelfpga/data_mover/pciess_dm_h2c_cpl_reordering.sv
rm $QUARTUS_22_1_BASE/ip/altera/subsystems/pcie_ss/rtl/intelfpga/pciess_hip_if_adaptor.v
rm $QUARTUS_22_1_BASE/ip/altera/subsystems/pcie_ss/rtl/intelfpga/ptile_pciess_top.sv
rm $QUARTUS_22_1_BASE/ip/altera/subsystems/pcie_ss/rtl/mentor/data_mover/pciess_dm_h2c_cpl_reordering.sv
rm $QUARTUS_22_1_BASE/ip/altera/subsystems/pcie_ss/rtl/mentor/ptile_pciess_top.sv
rm $QUARTUS_22_1_BASE/ip/altera/subsystems/pcie_ss/rtl/mentor/pciess_hip_if_adaptor.v
rm $QUARTUS_22_1_BASE/ip/altera/subsystems/pcie_ss/rtl/data_mover/pciess_dm_h2c_cpl_reordering.sv
rm $QUARTUS_22_1_BASE/ip/altera/subsystems/pcie_ss/rtl/pciess_hip_if_adaptor.v
rm $QUARTUS_22_1_BASE/ip/altera/subsystems/pcie_ss/rtl/ptile_pciess_top.sv
rm $QUARTUS_22_1_BASE/ip/altera/subsystems/pcie_ss/rtl/synopsys/data_mover/pciess_dm_h2c_cpl_reordering.sv
rm $QUARTUS_22_1_BASE/ip/altera/subsystems/pcie_ss/rtl/synopsys/pciess_hip_if_adaptor.v
rm $QUARTUS_22_1_BASE/ip/altera/subsystems/pcie_ss/rtl/synopsys/ptile_pciess_top.sv

# Set working directory if not already set
: ${WORK_DIR=/home/intelFPGA}

# Prerequisites
sudo apt-get install -y python3 python3-pip python3-dev git gcc g++ make cmake uuid-dev libjson-c-dev libhwloc-dev libtbb-dev libedit-dev libudev-dev linuxptp pandoc devscripts debhelper doxygen
pip3 install jsonschema virtualenv pyyaml pybind11

# sudo apt install autoconf automake bison boost boost-dev cmake doxygen dwarves elfutils-libelf-dev \
# flex gcc gcc-c++ git hwloc-dev json-c-dev libarchive libedit libedit-dev libpcap libpng12 libuuid libuuid-dev libxml2 libxml2-dev make ncurses spdlog cli11-dev python3-yaml python3-pybind11  \
# ncurses-dev ncurses-libs openssl-dev python3-pip python3-dev python3-jsonschema rsync tbb-dev libudev-dev

python3 -m pip install --user jsonschema virtualenv pudb pyyaml

sudo pip3 uninstall setuptools
sudo pip3 install Pybind11==2.10.0
sudo pip3 install setuptools==59.6.0 --prefix=/usr

# Clone
mkdir -p $WORK_DIR/Intel_OFS/
cd $WORK_DIR/Intel_OFS/
git init
git clone https://github.com/OPAE/opae-sdk.git
cd $WORK_DIR/Intel_OFS/opae-sdk
git checkout tags/2.1.1-1 -b release/2.1.1
git describe --tags
echo "Expected 2.1.1-1"
git branch

# Install by script
cd $WORK_DIR/Intel_OFS/opae-sdk/packaging/opae/deb
./create
sudo dpkg -i opae*.deb

# Install from sources
# # NOTE: UNTESTED
# cd $WORK_DIR/Intel_OFS/opae-sdk
# mkdir build
# cd $WORK_DIR/Intel_OFS/opae-sdk/build
# cmake .. -DCPACK_GENERATOR=DEB -DOPAE_BUILD_FPGABIST=ON -DOPAE_BUILD_PYTHON_DIST=ON -DCMAKE_BUILD_PREFIX=/usr
# make -j `nproc`
# make -j `nproc` package_deb
# sudo dpkg -i opae*.deb

# Check installed
apt list opae*
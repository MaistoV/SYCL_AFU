# Install prerequisites

sudo subscription-manager release --set=8.6
sudo dnf update
sudo subscription-manager repos --enable codeready-builder-for-rhel-8-x86_64-rpms
sudo dnf install https://dl.fedoraproject.org/pub/epel/epel-release-latest-8.noarch.rpm

sudo dnf install -y python3 python3-pip python3-devel \
    gdb vim git gcc gcc-c++ make cmake libuuid-devel rpm-build systemd-devel nmap \
    python3-jsonschema json-c-devel tbb-devel rpmdevtools libcap-devel \
    python3-pyyaml hwloc-devel libedit-devel git kernel-headers kernel-devel elfutils-libelf-devel ncurses-devel openssl-devel bison flex cli11-devel spdlog-devel

# Stup python
python3 -m pip install --user jsonschema virtualenv pudb pyyaml
sudo pip3 uninstall setuptools
sudo pip3 install Pybind11==2.10.0 # --proxy http://yourproxy:xxx
sudo pip3 install setuptools==59.6.0 #--prefix=/usr --proxy http://yourproxy:xxx

# Download packages
mkdir -p $DOWNLOADS_DIR
cd $DOWNLOADS_DIR
wget http://ftp.pbone.net/mirror/archive.fedoraproject.org/epel/8.4/Everything/x86_64/Packages/p/pybind11-devel-2.4.3-2.el8.x86_64.rpm
wget http://ftp.pbone.net/mirror/archive.fedoraproject.org/epel/8.4/Everything/x86_64/Packages/p/python3-pybind11-2.4.3-2.el8.x86_64.rpm
sudo dnf localinstall ./python3-pybind11-2.4.3-2.el8.x86_64.rpm ./pybind11-devel-2.4.3-2.el8.x86_64.rpm -y
cd $ROOT_DIR

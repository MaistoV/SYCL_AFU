# Enable EPEL
dnf install -y https://dl.fedoraproject.org/pub/epel/epel-release-latest8.noarch.rpm

dnf install --enablerepo=codeready-builder-for-rhel-8-x86_64-rpms -y python3 \
    python3-pip python3-devel python3-jsonschema python3-pyyaml git gcc gcc-c++ make \
    cmake libuuid-devel json-c-devel hwloc-devel tbb-devel cli11-devel spdlog-devel \
    libedit-devel systemd-devel doxygen python3-sphinx pandoc rpm-build rpmdevtools\
    python3-virtualenv yaml-cpp-devel libudev-devel libcap-devel

pip3 install --upgrade --prefix=/usr pip setuptools pybind11

./opae-sdk/packaging/opae/rpm/create unrestricted
exit
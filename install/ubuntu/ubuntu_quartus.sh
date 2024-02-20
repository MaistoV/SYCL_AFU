#!/bin/bash

# Set working directory if not already set
DOWNLOADS_DIR=$ROOT_DIR=install/downloads

###########
# Quartus #
###########
cd $DOWNLOADS_DIR
QUARTUS_NAME=Quartus-pro-23.2.0.94-linux
if [ ! -d $QUARTUS_NAME ]; then
    if [ ! -e $QUARTUS_NAME.tar ]; then
        echo "Missing $QUARTUS_NAME, download Quartus and Agilex device support from web browser!"
    fi
    mkdir -p $QUARTUS_NAME
    cd $QUARTUS_NAME
    tar xvf ../$QUARTUS_NAME.tar
    ./setup_pro.sh \
            --mode unattended  \ 
            --accept_eula 1    \
            --installdir $QUARTUS_INSTALL_DIR
fi

##########
# Agilex #
##########
cd $DOWNLOADS_DIR
AGILEX_NAME=Quartus-pro-23.2.0.94-devices-4
if [ ! -d $AGILEX_NAME ]; then
    if [ ! -e $AGILEX_NAME.tar ]; then
        echo "Missing $AGILEX_NAME, download Quartus and Agilex device support from web browser!"
    fi
    mkdir -p $AGILEX_NAME
    cd $AGILEX_NAME
    tar xvf ../$AGILEX_NAME.tar
    ./dev4_setup_pro.sh \
            --mode unattended  \ 
            --accept_eula 1    \
            --installdir $QUARTUS_INSTALL_DIR
fi

###########
# Patches #
###########
QUARTUS_INSTALL_DIR=~/intelFPGA_pro/23.2/
declare -a PATCHES=(
    "quartus-23.2-0.02-linux.run"
    "quartus-23.2-0.11-linux.run"
    "quartus-23.2-0.19-linux.run"
)

GIT_RELEASE_URL=https://github.com/OFS/ofs-agx7-pcie-attach/releases/download/ofs-2023.2-1/
cd $DOWNLOADS_DIR

for patch in ${PATCHES[@]};
do
    # Download
    if [ ! -e $patch ]; then
        wget $GIT_RELEASE_URL/$patch
        chmod +x $patch
    fi
    # Install
    ./$patch \
        --mode unattended  \
        --accept_eula 1    \
        --installdir $QUARTUS_INSTALL_DIR

done


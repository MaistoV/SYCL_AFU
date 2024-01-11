# From Hitek SFTP
#########################################
   export IOFS_BUILD_ROOT=$PWD/../build
   cd $IOFS_BUILD_ROOT/intel-ofs-fim
   export OFS_ROOTDIR=$PWD
   export WORKDIR=$OFS_ROOTDIR
   export VERDIR=$OFS_ROOTDIR/verification
   export PATH=$PATH:/tools/altera/v21.4pro/quartus/bin/
   export QUARTUS_ROOTDIR=/tools/altera/v21.4pro/quartus/
   export QUARTUS_INSTALL_DIR=$QUARTUS_ROOTDIR
   export IMPORT_IP_ROOTDIR=$QUARTUS_ROOTDIR/../ip
   export IP_ROOTDIR=$QUARTUS_ROOTDIR/../ip
   export OPAE_SDK_REPO_BRANCH=release/2.0.11
   cd $IOFS_BUILD_ROOT/intel-ofs-fim
###############################################

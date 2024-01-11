###########
# For ASE #
###########

cd /usr/bin
export PATH=$PWD:$PATH
cd ../lib/python*/site-packages
export PYTHONPATH=$PWD
cd /usr/lib
export LIBRARY_PATH=$PWD
cd /usr/lib64
export LD_LIBRARY_PATH=$PWD
cd $IOFS_BUILD_ROOT/ofs-platform-afu-bbb
export OFS_PLATFORM_AFU_BBB=$PWD
cd $OFS_ROOTDIR/work_x16_adp/pr_build_template
export OPAE_PLATFORM_ROOT=$PWD
# For QuestaSIM, set the following:
export MTI_HOME=/home/mentor/questa_core_2020/questasim/
export PATH=$MTI_HOME/linux_x86_64/:$MTI_HOME/bin/:$PATH

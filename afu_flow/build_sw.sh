cd $OFS_PLATFORM_AFU_BBB/plat_if_tests/host_chan_mmio/sw
export OPAE_LOC=/usr
export LIBRARY_PATH=$OPAE_LOC/lib:$LIBRARY_PATH
export LD_LIBRARY_PATH=$OPAE_LOC/lib64:$LD_LIBRARY_PATH
make

echo "Run: ./host_chan_mmio"
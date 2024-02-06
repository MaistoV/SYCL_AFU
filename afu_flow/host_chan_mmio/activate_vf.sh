# Configure AFU GBS
cd $OFS_ROOTDIR/work_x16_adp/host_chan_mmio_synth
sudo fpgasupdate host_chan_mmio.gbs 

# Create the Virtual Functions (VFs)
sudo pci_device b1:00.0 vf 3
# Bind VFIO
sudo opae.io init -d 0000:b1:00.X
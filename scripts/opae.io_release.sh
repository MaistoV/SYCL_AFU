#!/bin/bash

# Exclude PF0.VF0 (B:00.0)
OPAEIO_SDBFs=$( opae.io ls | grep -v ${PAC_PCIE_SBD}.0 | awk '{print $1}' | sed -E "s/(\[|\])//g" )
for sbdf in ${OPAEIO_SDBFs}; do
    sudo opae.io release -d $sbdf
done

sudo pci_device ${PAC_PCIE_SBD}.0 vf 0
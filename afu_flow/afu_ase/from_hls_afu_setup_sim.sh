BUILD_ASE_DIR=build_ase_dir

echo "[INFO] try to remove old ${BUILD_ASE_DIR} (continue if fails)..."
rm -r ${BUILD_ASE_DIR} 

echo "[INFO] create ASE working directory..."
FILELIST=tbd.txt
afu_sim_setup --source ${FILELIST} ${BUILD_ASE_DIR}/ 2>&1 | tee setup_sim.log
if [ $? -ne 0 ]; then 
    echo "[ERROR] Something went wrong in afu_sim_setup!" ; exit 1;
fi

echo "[INFO] afu_sim_setup completed. Patching the generated Makefile..."
cd ${BUILD_ASE_DIR}

# Apply new patches
# patch Makefile ../../misc/build_ase_dir_Makefile.patch #copy mem init files if any
# patch ase.cfg ../../misc/ase.cfg.patch # Keep ASE alive between runs

# echo "[INFO] Makefile patched. Build testbench..."

make 2>&1 | tee ../make.log
if [ $? -ne 0 ]; then
    echo "[ERROR] make failed! exiting..." ; exit 1;
fi

echo "[INFO] Testbench built. Run it..."
make sim 2>&1 | tee ../make_sim.log
if [ $? -ne 0 ]; then
    # as of Acceleration Stack 1.1, there was an issue with the Makefile where 
    # `make sim` would not return an error code, even if the modelsim step failed.
    # therefore, this clause will not be triggered, even if `make sim` fails.
    echo "[ERROR] make sim failed." ; exit 1; 
fi
# #!/bin/bash
ROOT_HEM=$PWD

mkdir -p $HEM_OUT_DIR

# TBD: source the other scripts
# source tests/mem_tg/hem_mem_tg_benchmark.sh #TBD
# source tests/trput/hem_trput_benchmark.sh #TBD

cd $ROOT_HEM/tests/lpbk/
source hem_benchmark_lpbk.sh

cd $ROOT_HEM/tests/freq/
source hem_benchmark_freq.sh

cd $ROOT_HEM/tests/mem/
source hem_mem_benchmark.sh

# This breaks everything
# cd $ROOT_HEM/tests/test_all/
# echo y | source hem_test_all.sh
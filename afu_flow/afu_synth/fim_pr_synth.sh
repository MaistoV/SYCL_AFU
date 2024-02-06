#!/bin/bash

# Build FIM

# Setup
cd $HTS_FIM_RELEASE
soruce setup_env.sh

# Launch build
./build_fim.sh --pr


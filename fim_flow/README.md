# FIM OFSS flow
Set `OFSS_CONFIG` variable and create directory in `fim_flow/ofss_configs/`
``` 
fim_flow/ofss_configs/ofss_config_${OFSS_CONFIG}
    ├── pcie
    |   └── pcie_host_${OFSS_CONFIG}.ofss
    ├── memory (untested)
    ├── iopll (untested)
    ├── hssi (not used)
    └── htk-nc220-agf014_${OFSS_CONFIG}.ofss
```

Run FIM build with:
``` console
$ export OFSS_CONFIG=<config name>
$ make fim_build_<pr|flat>
``` 

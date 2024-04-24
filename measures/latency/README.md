# Latency Measures
The scripts in these subdirectories rely on `${ROOT_DIR}` flow and environment.

## Launch measures
From `${ROOT_DIR}`:
``` console
# cd $ROOT_DIR
$ source measures/latency/scripts/measure_latency_top.sh \
    <make measure_* target> \
    <num_reps> \   
    <max_decode_flag>
```
Or simply:
``` console
$ cd $ROOT_DIR
$ make measure_<target>
```
With `target` in:
* `isal`
* `asp_fpga`
* `asp_plain_c`
* `sycl_afu`

## Plots
Verified Python version is 3.10.12.
For plotting data, install Python prerequisites as:
``` console
$ python -m pip install -r py_prerequisites.txt
```

Plot with:
``` console
$ cd plots
$ python plot_latency.py
```
Or, from $ROOT_DIR

``` console
$ cd $ROOT_DIR
$ make measure_plots
```
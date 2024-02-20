# Host Excercisor Modules
Platform testing using Host Excercisor Modules (HEMs).

The whole flow here relies on the factory FIM image, including default PR AFUs. To program it,:
``` console
$ make pac_powercycle_factory 
```

Before starting, run:
``` console
$ source settings_hems.sh
```

## Plaftorm benchmark
In the `tests/` directory, the following sub-directories are available:
 * `freq/` assess user input --clock-mhz impact 
 * `lpbk/` evaluate 2 available AFUs with same GUID
 * `mem/` ?
 * `mem_tg/` ?
 * `test_all/` run with `--testall` flag
 * `trput/` measure max platform bandwidth per cache line reads and interleave patterns

Test results are available in `tests/results/` with file names composed as `<test_name>_<hostname>.csv`.

## Multithreading testing
Functional verification of thread-safety:
1. ✅ Same user, different VFs, **explicit VF**
2. ❌ Same user, different VFs, implicit VF
3. ❌ Same user, same AFU, multiple available VFs
4. ❌ Same user, same VF
5. ✅ (Parallel) Same user, different VFs, **explicit VF**, in loop

# Reed-Solomon ROMs generation

## Prerequisites
This project requires ISA-L to be installed on the system.

## Generation
For the default K:P combinations, i.e. `3:2`, `6:3`, `10:4`, (re)generate ROMs, permutations and lookup tables with:
``` console
$ make regen_roms
$ make regen_roms_multi_erasure
``` 

The output will be in the `roms/` folder:
```
├── erasure_patterns_[K]_[P].txt    # Raw possible erasure patterns
├── rs_decode_[K]_[P].c             # ?TBD
├── rs_erasure_patterns_[K]_[P].c   # Possible erasure patterns for of 1 to P erasures
├── rs_permutations_[K]_[P].c       # ?TBD
├── rs_rom_[K]_[P].c                # Actual ROM 
├── rs_rom_lookup_[K]_[P].c         # ROM lookup tables
└── survival_patterns_[K]_[P].txt   # Raw possible survival patterns
```
The `*.c` files can be imported in any project for testing and HLD.

More K:P combinations are supported, check the sources for custom generation
> TBD: add more doc on this

## Sources
This tree is organized as follows:
```
├── Makefile
└── src
    ├── README.md
    ├── erasure_gen.c                       # Generate erasure_patterns_[K]_[P].txt 
    ├── rs_rom_gen.c                        # Generate all .c files (non *_multi_erasures.c)
    ├── rs_rom_gen_multi_erasure.c          # Generate all *_multi_erasure.c files
    ├── rs_rom_utils.c -> rs_rom_utils.cpp  # Symlink for g++/gcc plain builds
    ├── rs_rom_utils.cpp                    # Utility functions implementations
    ├── rs_rom_utils.h                      # Utility functions declarations
    └── survival_gen.c                      # Generate survival_patterns_[K]_[P].txt 
```
> TODO: split utility rs_rom_utils.cpp into multiple files
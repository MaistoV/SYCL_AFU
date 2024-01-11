# Same user, same AFU, multiple available VFs
# RESULT: either
#   * [2023-11-28 17:07:18.876] [lpbk] [error] no accelerator found with id: 56E203E9-864F-49A7-B94B-12284C31E02B
#   *  what():  failed with return code FPGA_BUSY at: handle.cpp:open():57 -> opae_vfio_init() opaevfio.c:1131 -> pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE) != 0
host_exerciser lpbk    &
host_exerciser lpbk    &

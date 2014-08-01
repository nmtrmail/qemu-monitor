QEMU-Debug-Tool
===============
## User Guide ##
### Environment Setup ###
   1. Build qemu-system-aarch64, kernel and rootfs
      * copy `qemu-patch/qemu.patch` to qemu directory
      * cd to qemu directory
      * $ patch -p1 < ./qemu.patch
      * $ ./configure --target-list=aarch64-softmmu
      * $ make -j8
   2. Download pre-built gdb(aarch64) from linaro release website
   3. Open `tools/run_linux.sh`, modify QEMU and kernel's path
   4. Open `tools/run_gdb.sh`, modify gdb's path
   5. make

### Execution ###
   1. $ tools/run_linux.sh
   2. $ ./main
   3. $ tools/run_gdb.sh
   4. Enter command in debug tool and then debug with gdb

### Command Usage ###
   * `display $register_name[end_bit:start_bit]` - auto display registers along with gdb
   * `undisplay display_number` - disable auto display register which specified by display_number
   * `print /x $register_name[end_bit:start_bit]` - print value of register in format x(d, u, o)
   * `store filename` - store current display registers to filename, which could be used in load command
   * `load filename` - load a command script, like gdb -x
   * `refresh` - refresh display window
   * `help` - show help guide

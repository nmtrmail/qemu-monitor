QEMU-Debug-Tool
===============
## User Guide ##
### Environment Setup ###
   1. Build qemu-system-aarch64, kernel and rootfs
      * copy `qemu-patch/qemu.patch` to qemu directory
      * cd to qemu directory
      * $ checkout 2ee55b8351910e5dd898f52415064a4c5479baba
      * $ patch -p1 < ./qemu.patch
      * $ ./configure --target-list=aarch64-softmmu
      * $ make -j8
   2. Download pre-built gdb(aarch64) from linaro release website
   3. make
   4. make install PREFIX=`INSTALL_PATH`

### Execution ###
   1. $ qemu-monitor
   2. $ qemu-system-aarch64 -machine virt -cpu cortex-a57 -machine type=virt -nographic -smp 1 -m 512 -kernel `KERNEL_IMAGE_PATH` --append "console=ttyAMA0" -gdb tcp::1234 -S
   3. $ aarch64-linux-gnu-gdb(file vmlinux, remote target :1234)
   4. Enter command in debug tool and then debug with gdb

### Command Usage ###
   * `display $register_name[end_bit:start_bit]` - auto display registers along with gdb
   * `undisplay display_number` - disable auto display register which specified by display_number
   * `print /x $register_name[end_bit:start_bit]` - print value of register in format x(d, u, o)
   * `store filename` - store current display registers to filename, which could be used in load command
   * `load filename` - load a command script, like gdb -x
   * `refresh` - refresh display window(tui mode only)
   * `help` - show help guide

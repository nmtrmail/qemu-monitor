#!/bin/bash
echo Start qemu-system-aarch64...
echo Ctrl-a + c to control qemu.
~/workspace/qemu.git/aarch64-softmmu/qemu-system-aarch64 -machine virt -cpu cortex-a57 -machine type=virt -nographic -smp 1 -m 512 -kernel ~/workspace/linux.git/arch/arm64/boot/Image --append "console=ttyAMA0" -gdb tcp::1234 -S

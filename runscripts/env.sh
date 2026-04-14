toppath=<file_path>
gem5_dir=$toppath/gem5
exe=$gem5_dir/build/ARM/gem5.opt

fs_config=$gem5_dir/configs/deprecated/example/fs.py
fs_config=$toppath/configs-test/deprecated/example/fs.py

cpu=AtomicSimpleCPU

export M5_PATH=**Arm_FS_Binaries_path**

disk=<ubuntu_disk_path*>
kernel=<linux_kernel_path>


dtb=$gem5_dir/system/arm/dt/armv8_gem5_v1_1cpu.dtb

outdir=$toppath/output-basic

script=" --script=$gem5_dir/configs/boot/hack_back_ckpt.rcS "

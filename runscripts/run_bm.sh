source ./env.sh

cmd=$toppath/simple_testing_app/a.out

cpu=" --cpu-type=AtomicSimpleCPU "
#cpu=" --cpu-type=TimingSimpleCPU "
#cpu=" --cpu-type=ArmRCO3CPU "

cache=" --caches --l2cache "
mem=""

#debug=" --debug-flags=AddrRanges,PciBridge,PciDevice,PciHost,CxlDevice,CxlHost,DCOH,CXLProcessor,CXLXBar,CPU2,Cache2  "
#debug=" --debug-flags=AddrRanges,PciBridge,PciDevice,PciHost,CxlDevice,CxlHost,DCOH,CXLProcessor,CXLXBar,CPU2,Cache2,MemCtrl2  "
debug=" "

kernel=<file_path>/gem5.bare-metal/Simple/main.elf
 
$exe $debug --outdir $outdir -re $fs_config $cpu $cache $mem --kernel=$kernel --disk-image=$disk --bare-metal --dtb-filename=$dtb --machine-type=VExpress_GEM5_V1 --mem-size=2GB $script

# opts="
# 	--cpu-type=$cpu			\
# 	--cpu-clock=2GHz		\
# 	--num-cpus=1			\
# 	--caches				\
# 	--disk-image=$disk		\
# 	--kernel=$kernel		\
# 	--mem-size=1GB		\
# 	--machine-type=VExpress_GEM5_V1	\
# 	--switch_up_lanes=1 --lanes=1   --cacheline_size=64 --script= \
# 	$DTB
# 	"

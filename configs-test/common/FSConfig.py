# Copyright (c) 2010-2012, 2015-2019 ARM Limited
# Copyright (c) 2020 Barkhausen Institut
# All rights reserved.
#
# The license below extends only to copyright in the software and shall
# not be construed as granting a license to any other intellectual
# property including but not limited to intellectual property relating
# to a hardware implementation of the functionality of the software
# licensed hereunder.  You may use the software subject to the license
# terms below provided that you ensure that this notice is replicated
# unmodified and in its entirety in all distributions of the software,
# modified or unmodified, in source code or in binary form.
#
# Copyright (c) 2010-2011 Advanced Micro Devices, Inc.
# Copyright (c) 2006-2008 The Regents of The University of Michigan
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

from common import ObjectList
from common.Benchmarks import *

import m5
import m5.defines
from m5.objects import *
from m5.util import *

# Populate to reflect supported os types per target ISA
os_types = set()
if m5.defines.buildEnv["USE_ARM_ISA"]:
    os_types.update(
        [
            "linux",
            "android-gingerbread",
            "android-ics",
            "android-jellybean",
            "android-kitkat",
            "android-nougat",
        ]
    )
if m5.defines.buildEnv["USE_MIPS_ISA"]:
    os_types.add("linux")
if m5.defines.buildEnv["USE_POWER_ISA"]:
    os_types.add("linux")
if m5.defines.buildEnv["USE_RISCV_ISA"]:
    os_types.add("linux")  # TODO that's a lie
if m5.defines.buildEnv["USE_SPARC_ISA"]:
    os_types.add("linux")
if m5.defines.buildEnv["USE_X86_ISA"]:
    os_types.add("linux")


class CowIdeDisk(IdeDisk):
    image = CowDiskImage(child=RawDiskImage(read_only=True), read_only=False)

    def childImage(self, ci):
        self.image.child.image_file = ci


# CXL_Debug
class CXLCache(Cache):
    """Simple L2 Cache with default values"""

    # Default parameters
    size = "512kB"
    assoc = 16
    tag_latency = 10
    data_latency = 10
    response_latency = 1
    mshrs = 20
    tgts_per_mshr = 12

    def connectCPUSideBus(self, bus):
        self.cpu_side = bus.mem_side_ports

    def connectMemSideBus(self, bus):
        self.mem_side = bus.cpu_side_ports

# class MemBus(SystemXBar):
#     badaddr_responder = BadAddr()
#     #default = Self.badaddr_responder.pio

# class MemBus(SystemXBar):
#     badaddr_responder = BadAddr()
#     #default = Self.badaddr_responder.pio
        
class IOXBar_Bad(IOXBar):
    badaddr_responder = BadAddr()
    default = Self.badaddr_responder.pio


def attach_9p(parent, bus):
    viopci = PciVirtIO()
    viopci.vio = VirtIO9PDiod()
    viodir = os.path.realpath(os.path.join(m5.options.outdir, "9p"))
    viopci.vio.root = os.path.join(viodir, "share")
    viopci.vio.socketPath = os.path.join(viodir, "socket")
    os.makedirs(viopci.vio.root, exist_ok=True)
    if os.path.exists(viopci.vio.socketPath):
        os.remove(viopci.vio.socketPath)
    parent.viopci = viopci
    parent.attachPciDevice(viopci, bus)


def fillInCmdline(mdesc, template, **kwargs):
    kwargs.setdefault("rootdev", mdesc.rootdev())
    kwargs.setdefault("mem", mdesc.mem())
    kwargs.setdefault("script", mdesc.script())
    return template % kwargs


def makeCowDisks(disk_paths):
    disks = []
    for disk_path in disk_paths:
        disk = CowIdeDisk(driveID="device0")
        disk.childImage(disk_path)
        disks.append(disk)
    return disks


def makeSparcSystem(mem_mode, mdesc=None, cmdline=None):
    # Constants from iob.cc and uart8250.cc
    iob_man_addr = 0x9800000000
    uart_pio_size = 8

    class CowMmDisk(MmDisk):
        image = CowDiskImage(
            child=RawDiskImage(read_only=True), read_only=False
        )

        def childImage(self, ci):
            self.image.child.image_file = ci

    self = System()
    if not mdesc:
        # generic system
        mdesc = SysConfig()
    self.readfile = mdesc.script()
    self.iobus = IOXBar()
    self.membus = MemBus()
    self.bridge = Bridge(delay="50ns")
    self.t1000 = T1000()
    self.t1000.attachOnChipIO(self.membus)
    self.t1000.attachIO(self.iobus)
    self.mem_ranges = [
        AddrRange(Addr("1MB"), size="64MB"),
        AddrRange(Addr("2GB"), size="256MB"),
    ]
    self.bridge.mem_side_port = self.iobus.cpu_side_ports
    self.bridge.cpu_side_port = self.membus.mem_side_ports
    self.disk0 = CowMmDisk()
    self.disk0.childImage(mdesc.disks()[0])
    self.disk0.pio = self.iobus.mem_side_ports

    # The puart0 and hvuart are placed on the IO bus, so create ranges
    # for them. The remaining IO range is rather fragmented, so poke
    # holes for the iob and partition descriptors etc.
    self.bridge.ranges = [
        AddrRange(
            self.t1000.puart0.pio_addr,
            self.t1000.puart0.pio_addr + uart_pio_size - 1,
        ),
        AddrRange(
            self.disk0.pio_addr,
            self.t1000.fake_jbi.pio_addr + self.t1000.fake_jbi.pio_size - 1,
        ),
        AddrRange(self.t1000.fake_clk.pio_addr, iob_man_addr - 1),
        AddrRange(
            self.t1000.fake_l2_1.pio_addr,
            self.t1000.fake_ssi.pio_addr + self.t1000.fake_ssi.pio_size - 1,
        ),
        AddrRange(
            self.t1000.hvuart.pio_addr,
            self.t1000.hvuart.pio_addr + uart_pio_size - 1,
        ),
    ]

    workload = SparcFsWorkload()

    # ROM for OBP/Reset/Hypervisor
    self.rom = SimpleMemory(
        image_file=binary("t1000_rom.bin"),
        range=AddrRange(0xFFF0000000, size="8MB"),
    )
    # nvram
    self.nvram = SimpleMemory(
        image_file=binary("nvram1"), range=AddrRange(0x1F11000000, size="8kB")
    )
    # hypervisor description
    self.hypervisor_desc = SimpleMemory(
        image_file=binary("1up-hv.bin"),
        range=AddrRange(0x1F12080000, size="8kB"),
    )
    # partition description
    self.partition_desc = SimpleMemory(
        image_file=binary("1up-md.bin"),
        range=AddrRange(0x1F12000000, size="8kB"),
    )

    self.rom.port = self.membus.mem_side_ports
    self.nvram.port = self.membus.mem_side_ports
    self.hypervisor_desc.port = self.membus.mem_side_ports
    self.partition_desc.port = self.membus.mem_side_ports

    self.system_port = self.membus.cpu_side_ports

    self.workload = workload

    return self


def makeArmSystem(
    mem_mode, _options,
    machine_type,
    num_cpus=1,
    mdesc=None,
    dtb_filename=None,
    bare_metal=False,
    cmdline=None,
    external_memory="",
    ruby=False,
    vio_9p=None,
    bootloader=None,
):
    assert machine_type

    pci_devices = []

    self = ArmSystem()

    if not mdesc:
        # generic system
        mdesc = SysConfig()

    self.readfile = mdesc.script()
    self.iobus = IOXBar()

    # PCIE
    #self.pcie = pcie_link()
    cacheline_size = 64
    self.pcie1 = PCIELink(lanes = _options.switch_up_lanes, speed = '5Gbps', mps=cacheline_size, max_queue_size=_options.replay_buffer_size)
    #self.pcie2 = pcie_link()
    self.pcie3 = PCIELink(lanes = _options.lanes , speed = '5Gbps', mps = cacheline_size, max_queue_size=_options.replay_buffer_size)
    self.pcie4 = PCIELink(lanes = _options.lanes , speed = '5Gbps',  mps = cacheline_size,max_queue_size=_options.replay_buffer_size)
    self.pcie5 = PCIELink(lanes = _options.lanes , speed = '5Gbps', mps = cacheline_size, max_queue_size=_options.replay_buffer_size)
    self.pcie6 = PCIELink(lanes = _options.lanes , speed = '5Gbps' ,mps = cacheline_size, max_queue_size=_options.replay_buffer_size)
    self.pcie7 = PCIELink(lanes = _options.lanes , speed = '5Gbps', mps = cacheline_size, max_queue_size=_options.replay_buffer_size)
    self.iobus_dma = IOXBar_Bad()
    # End PCIE

    if not ruby:
        self.bridge = Bridge(delay="50ns")
        self.bridge.mem_side_port = self.iobus.cpu_side_ports

        #self.membus = SystemXBar()
        # CXL_Debug
        self.membus = CXLSystemXBar()
        self.bridge.cpu_side_port = self.membus.mem_side_ports

    self.mem_mode = mem_mode

    platform_class = ObjectList.platform_list.get(machine_type)
    # Resolve the real platform name, the original machine_type
    # variable might have been an alias.
    machine_type = platform_class.__name__
    self.realview = platform_class()
    self._bootmem = self.realview.bootmem

    # Start PCIE
    should_print = True
    self.Root_Complex = Root_Complex(is_transmit = should_print ,req_size = _options.switch_buffer_size, resp_size = _options.switch_buffer_size ,  delay = '150ns', BridgeID = 0)
    #self.Root_Complex.slave = self.membus.master
    self.membus.default = self.Root_Complex.slave
    self.Root_Complex.slave_dma1 = self.pcie1.upstreamMaster
    self.Root_Complex.slave_dma2 = self.pcie6.upstreamMaster
    self.Root_Complex.slave_dma3 = self.pcie7.upstreamMaster 
    self.Root_Complex.master1    = self.pcie1.upstreamSlave 
    self.Root_Complex.master2    = self.pcie6.upstreamSlave 
    self.Root_Complex.master3    = self.pcie7.upstreamSlave 
    self.Root_Complex.master_dma = self.iobus_dma.slave

    # self.iobus_dma.badaddr_responder.warn_access = "warn"
    # self.iobus_dma.default = self.iobus_dma.badaddr_responder
    
    self.switch = PciESwitch(pci_bus = 1 , delay=_options.pcie_switch_delay , req_size= _options.switch_buffer_size , resp_size = _options.switch_buffer_size, BridgeID = 1)
    self.switch.slave = self.pcie1.downstreamMaster
    self.switch.slave_dma1 = self.pcie3.upstreamMaster
    self.switch.slave_dma2 = self.pcie4.upstreamMaster 
    self.switch.slave_dma3 = self.pcie5.upstreamMaster
    self.switch.master1    = self.pcie3.upstreamSlave
    self.switch.master2    = self.pcie4.upstreamSlave 
    self.switch.master3    = self.pcie5.upstreamSlave
    self.switch.master_dma = self.pcie1.downstreamSlave
    '''self.Root_Complex = Root_Complex()
    self.Root_Complex.slave = self.membus.master
    self.Root_Complex.slave_dma1 = self.pcie1.master
    self.Root_Complex.slave_dma2 = self.pcie6.master
    self.Root_Complex.slave_dma3 = self.pcie7.master 
    self.Root_Complex.master1    = self.pcie1.slave 
    self.Root_Complex.master2    = self.pcie6.slave 
    self.Root_Complex.master3    = self.pcie7.slave  
    self.Root_Complex.master_dma = self.iobus_dma.slave 
    
    self.switch = PciESwitch(pci_bus = 1)
    self.switch.slave = self.pcie1.master 
    self.switch.slave_dma1 = self.pcie3.master
    self.switch.slave_dma2 = self.pcie4.master 
    self.switch.slave_dma3 = self.pcie5.master
    self.switch.master1    = self.pcie3.slave
    self.switch.master2    = self.pcie4.slave 
    self.switch.master3    = self.pcie5.slave
    self.switch.master_dma = self.pcie1.slave'''
    # End PCIE

    # Attach any PCI devices this platform supports


    # CXL_Debug
    self.realview.CXL = CXLplatform()
    self.realview.CXL.CXLDevice = CXLDevice(pio_addr=0x15000000, pio_size=0x40000, pci_bus = 6 , pci_dev = 0, pci_func = 0 , InterruptLine = 3, InterruptPin = 2, root_port_number = 1)
    self.realview.CXL.CXLDevice.pio = self.pcie6.downstreamMaster
    self.realview.CXL.CXLDevice.dma = self.pcie6.downstreamSlave
    # self.realview.CXL.cxlmembus = SystemXBar()
    # self.realview.CXL.cxlmembus = IOXBar()
    self.realview.CXL.cxlmembus = CXLXBar()
    self.realview.CXL.CXLDevice.in_port = self.realview.CXL.cxlmembus.cpu_side_ports
    self.realview.CXL.CXLDevice.ex_port = self.realview.CXL.cxlmembus.mem_side_ports
    self.realview.CXL.cxl_mem_ctrl = MemCtrl()
    self.realview.CXL.cxl_mem_ctrl.dram = DDR3_1600_8x8()
    # self.realview.CXL.cxl_mem_ctrl.dram.range = AddrRange(0x16000000, size=0x1000000)
    self.realview.CXL.cxl_mem_ctrl.dram.range = AddrRange(0x80000000, size=0x20000000)
    self.realview.CXL.cxl_mem_ctrl.port = self.realview.CXL.cxlmembus.mem_side_ports
    self.realview.CXL.cxl_cache = CXLCache()
    # self.realview.CXL.CXLDevice.coh_port = self.realview.CXL.cxl_cache.cpu_side
    self.realview.CXL.cxl_cache.connectMemSideBus(self.realview.CXL.cxlmembus)
    self.realview.CXL.cxl_processor = CXLProcessor()
    self.realview.CXL.cxl_cache.connectCPUSideBus(self.realview.CXL.cxl_processor)



    # self.realview.ethernet1 = IGbE_pcie(pci_bus = 6 , pci_dev = 0, pci_func = 0 , InterruptLine = 3, InterruptPin = 2, root_port_number = 1) #3
    # # #self.realview.ethernet1.pio = self.iobus3.master  #iobus3
    # # #self.realview.ethernet1.dma = self.iobus3.slave
    # self.realview.ethernet1.pio = self.pcie6.downstreamMaster
    # self.realview.ethernet1.dma = self.pcie6.downstreamSlave  
    self.realview.ethernet2 = IGbE_e1000(pci_bus = 4 , pci_dev = 0, pci_func = 0 , InterruptLine = 3, InterruptPin = 1, root_port_number = 0 , is_invisible=1 ) #4
    #self.realview.ethernet2.pio = self.iobus4.master  #iobus4
    #self.realview.ethernet2.dma = self.iobus4.slave
    self.realview.ethernet2.pio = self.pcie4.downstreamMaster
    self.realview.ethernet2.dma = self.pcie4.downstreamSlave  
    self.realview.ethernet3 = IGbE_pcie(pci_bus =5 , pci_dev = 0, pci_func = 0 , InterruptLine = 0x1E, InterruptPin = 2, root_port_number = 0 , is_invisible = 1) #5
    #self.realview.ethernet3.pio = self.iobus5.master  #iobus
    #self.realview.ethernet3.dma = self.iobus5.slaves
    self.realview.ethernet3.pio = self.pcie5.downstreamMaster
    self.realview.ethernet3.dma = self.pcie5.downstreamSlave    
    #self.realview.ethernet4 = IGbE_pcie(pci_bus = 6 , pci_dev = 0, pci_func = 0 , InterruptLine = 0x1E, InterruptPin = 3)
    #self.realview.ethernet4.pio = self.iobus6.master
    #self.realview.ethernet4.dma = self.iobus6.slave 
    self.realview.ethernet5 = IGbE_pcie(pci_bus = 7 , pci_dev = 0, pci_func = 0 , InterruptLine = 0x1E, InterruptPin = 4 , root_port_number = 2, is_invisible = 1)
    #self.realview.ethernet5.pio = self.iobus7.master
    #self.realview.ethernet5.dma = self.iobus7.slave
    self.realview.ethernet5.pio = self.pcie7.downstreamMaster 
    self.realview.ethernet5.dma = self.pcie7.downstreamSlave   

    self.realview.ide = IdeController(disks = [], pci_bus = 3, pci_dev=0, pci_func=0, InterruptLine=2, InterruptPin=1, root_port_number = 0, flag=1)
    #self.realview.ide.pio = self.iobus6.master
    #self.realview.ide.dma = self.iobus6.slave
    self.realview.ide.pio = self.pcie3.downstreamMaster
    self.realview.ide.dma = self.pcie3.downstreamSlave
    self.realview.ide.host = self.realview.pci_host

    #End PCIE

    # Attach any PCI devices this platform supports
    self.realview.attachPciDevices()

    disks = makeCowDisks(mdesc.disks())
    # Old platforms have a built-in IDE or CF controller. Default to
    # the IDE controller if both exist. New platforms expect the
    # storage controller to be added from the config script.
    if hasattr(self.realview, "ide"):
        self.realview.ide.disks = disks
    elif hasattr(self.realview, "cf_ctrl"):
        self.realview.cf_ctrl.disks = disks
    else:
        self.pci_ide = IdeController(disks=disks)
        pci_devices.append(self.pci_ide)

    self.mem_ranges = []
    size_remain = int(Addr(mdesc.mem()))
    for region in self.realview._mem_regions:
        if size_remain > int(region.size()):
            self.mem_ranges.append(region)
            size_remain = size_remain - int(region.size())
        else:
            self.mem_ranges.append(AddrRange(region.start, size=size_remain))
            size_remain = 0
            break
        warn(
            "Memory size specified spans more than one region. Creating"
            " another memory controller for that range."
        )

    if size_remain > 0:
        fatal(
            "The currently selected ARM platforms doesn't support"
            " the amount of DRAM you've selected. Please try"
            " another platform"
        )

    if bare_metal:
        # EOT character on UART will end the simulation
        self.realview.uart[0].end_on_eot = True
        self.workload = ArmFsWorkload(dtb_addr=0)
    else:
        workload = ArmFsLinux()

        if dtb_filename:
            workload.dtb_filename = binary(dtb_filename)

        workload.machine_type = (
            machine_type if machine_type in ArmMachineType.map else "DTOnly"
        )

        # Ensure that writes to the UART actually go out early in the boot
        if not cmdline:
            cmdline = (
                "earlyprintk=pl011,0x1c090000 console=ttyAMA0 "
                + "lpj=19988480 norandmaps rw loglevel=8 "
                + "mem=%(mem)s root=%(rootdev)s"
            )

        if hasattr(self.realview.gic, "cpu_addr"):
            self.gic_cpu_addr = self.realview.gic.cpu_addr

        # This check is for users who have previously put 'android' in
        # the disk image filename to tell the config scripts to
        # prepare the kernel with android-specific boot options. That
        # behavior has been replaced with a more explicit option per
        # the error message below. The disk can have any name now and
        # doesn't need to include 'android' substring.
        if mdesc.disks() and os.path.split(mdesc.disks()[0])[-1].lower().count(
            "android"
        ):
            if "android" not in mdesc.os_type():
                fatal(
                    "It looks like you are trying to boot an Android "
                    "platform.  To boot Android, you must specify "
                    "--os-type with an appropriate Android release on "
                    "the command line."
                )

        # android-specific tweaks
        if "android" in mdesc.os_type():
            # generic tweaks
            cmdline += " init=/init"

            # release-specific tweaks
            if "kitkat" in mdesc.os_type():
                cmdline += (
                    " androidboot.hardware=gem5 qemu=1 qemu.gles=0 "
                    + "android.bootanim=0 "
                )
            elif "nougat" in mdesc.os_type():
                cmdline += (
                    " androidboot.hardware=gem5 qemu=1 qemu.gles=0 "
                    + "android.bootanim=0 "
                    + "vmalloc=640MB "
                    + "android.early.fstab=/fstab.gem5 "
                    + "androidboot.selinux=permissive "
                    + "video=Virtual-1:1920x1080-16"
                )

        workload.command_line = fillInCmdline(mdesc, cmdline)

        self.workload = workload

        self.realview.setupBootLoader(self, binary, bootloader)

    if external_memory:
        # I/O traffic enters iobus
        self.external_io = ExternalMaster(
            port_data="external_io", port_type=external_memory
        )
        self.external_io.port = self.iobus.cpu_side_ports

        # Ensure iocache only receives traffic destined for (actual) memory.
        self.iocache = ExternalSlave(
            port_data="iocache",
            port_type=external_memory,
            addr_ranges=self.mem_ranges,
        )
        self.iocache.port = self.iobus.mem_side_ports

        # Let system_port get to nvmem and nothing else.
        self.bridge.ranges = [self.realview.nvmem.range]

        self.realview.attachOnChipIO(self.iobus)
        # Attach off-chip devices
        self.realview.attachIO(self.iobus)
    elif ruby:
        self._dma_ports = []
        self._mem_ports = []
        self.realview.attachOnChipIO(
            self.iobus, dma_ports=self._dma_ports, mem_ports=self._mem_ports
        )
        self.realview.attachIO(self.iobus, dma_ports=self._dma_ports)
    else:
        self.realview.attachOnChipIO(self.membus, self.bridge)
        # Attach off-chip devices
        self.realview.attachIO(self.iobus)

    for dev in pci_devices:
        self.realview.attachPciDevice(
            dev, self.iobus, dma_ports=self._dma_ports if ruby else None
        )

    self.terminal = Terminal()
    self.vncserver = VncServer()

    if vio_9p:
        attach_9p(self.realview, self.iobus)

    if not ruby:
        self.system_port = self.membus.cpu_side_ports

    if ruby:
        if buildEnv["PROTOCOL"] == "MI_example" and num_cpus > 1:
            fatal(
                "The MI_example protocol cannot implement Load/Store "
                "Exclusive operations. Multicore ARM systems configured "
                "with the MI_example protocol will not work properly."
            )

    # PCIE
    self.Root_Complex.host = self.realview.pci_host
    self.switch.host = self.realview.pci_host 
    # self.realview.ethernet1.host = self.realview.pci_host
    self.realview.ethernet2.host = self.realview.pci_host
    self.realview.ethernet3.host = self.realview.pci_host
    #self.realview.ethernet4.host = self.realview.pci_host
    self.realview.ethernet5.host = self.realview.pci_host

    return self


def makeLinuxMipsSystem(mem_mode, mdesc=None, cmdline=None):
    class BaseMalta(Malta):
        ethernet = NSGigE(pci_bus=0, pci_dev=1, pci_func=0)
        ide = IdeController(
            disks=Parent.disks, pci_func=0, pci_dev=0, pci_bus=0
        )

    self = System()
    if not mdesc:
        # generic system
        mdesc = SysConfig()
    self.readfile = mdesc.script()
    self.iobus = IOXBar()
    self.membus = MemBus()
    self.bridge = Bridge(delay="50ns")
    self.mem_ranges = [AddrRange("1GB")]
    self.bridge.mem_side_port = self.iobus.cpu_side_ports
    self.bridge.cpu_side_port = self.membus.mem_side_ports
    self.disks = makeCowDisks(mdesc.disks())
    self.malta = BaseMalta()
    self.malta.attachIO(self.iobus)
    self.malta.ide.pio = self.iobus.mem_side_ports
    self.malta.ide.dma = self.iobus.cpu_side_ports
    self.malta.ethernet.pio = self.iobus.mem_side_ports
    self.malta.ethernet.dma = self.iobus.cpu_side_ports
    self.simple_disk = SimpleDisk(
        disk=RawDiskImage(image_file=mdesc.disks()[0], read_only=True)
    )
    self.mem_mode = mem_mode
    self.terminal = Terminal()
    self.console = binary("mips/console")
    if not cmdline:
        cmdline = "root=/dev/hda1 console=ttyS0"
    self.workload = KernelWorkload(command_line=fillInCmdline(mdesc, cmdline))

    self.system_port = self.membus.cpu_side_ports

    return self


def x86IOAddress(port):
    IO_address_space_base = 0x8000000000000000
    return IO_address_space_base + port


def connectX86ClassicSystem(x86_sys, numCPUs):
    # Constants similar to x86_traits.hh
    IO_address_space_base = 0x8000000000000000
    pci_config_address_space_base = 0xC000000000000000
    interrupts_address_space_base = 0xA000000000000000
    APIC_range_size = 1 << 12

    x86_sys.membus = MemBus()

    # North Bridge
    x86_sys.iobus = IOXBar()
    x86_sys.bridge = Bridge(delay="50ns")
    x86_sys.bridge.mem_side_port = x86_sys.iobus.cpu_side_ports
    x86_sys.bridge.cpu_side_port = x86_sys.membus.mem_side_ports
    # Allow the bridge to pass through:
    #  1) kernel configured PCI device memory map address: address range
    #     [0xC0000000, 0xFFFF0000). (The upper 64kB are reserved for m5ops.)
    #  2) the bridge to pass through the IO APIC (two pages, already contained in 1),
    #  3) everything in the IO address range up to the local APIC, and
    #  4) then the entire PCI address space and beyond.
    x86_sys.bridge.ranges = [
        AddrRange(0xC0000000, 0xFFFF0000),
        AddrRange(IO_address_space_base, interrupts_address_space_base - 1),
        AddrRange(pci_config_address_space_base, Addr.max),
    ]

    # Create a bridge from the IO bus to the memory bus to allow access to
    # the local APIC (two pages)
    x86_sys.apicbridge = Bridge(delay="50ns")
    x86_sys.apicbridge.cpu_side_port = x86_sys.iobus.mem_side_ports
    x86_sys.apicbridge.mem_side_port = x86_sys.membus.cpu_side_ports
    x86_sys.apicbridge.ranges = [
        AddrRange(
            interrupts_address_space_base,
            interrupts_address_space_base + numCPUs * APIC_range_size - 1,
        )
    ]

    # connect the io bus
    x86_sys.pc.attachIO(x86_sys.iobus)

    x86_sys.system_port = x86_sys.membus.cpu_side_ports


def connectX86RubySystem(x86_sys):
    # North Bridge
    x86_sys.iobus = IOXBar()

    # add the ide to the list of dma devices that later need to attach to
    # dma controllers
    x86_sys._dma_ports = [x86_sys.pc.south_bridge.ide.dma]
    x86_sys.pc.attachIO(x86_sys.iobus, x86_sys._dma_ports)


def makeX86System(mem_mode, numCPUs=1, mdesc=None, workload=None, Ruby=False):
    self = System()

    self.m5ops_base = 0xFFFF0000

    if workload is None:
        workload = X86FsWorkload()
    self.workload = workload

    if not mdesc:
        # generic system
        mdesc = SysConfig()
    self.readfile = mdesc.script()

    self.mem_mode = mem_mode

    # Physical memory
    # On the PC platform, the memory region 0xC0000000-0xFFFFFFFF is reserved
    # for various devices.  Hence, if the physical memory size is greater than
    # 3GB, we need to split it into two parts.
    excess_mem_size = convert.toMemorySize(mdesc.mem()) - convert.toMemorySize(
        "3GB"
    )
    if excess_mem_size <= 0:
        self.mem_ranges = [AddrRange(mdesc.mem())]
    else:
        warn(
            "Physical memory size specified is %s which is greater than "
            "3GB.  Twice the number of memory controllers would be "
            "created." % (mdesc.mem())
        )

        self.mem_ranges = [
            AddrRange("3GB"),
            AddrRange(Addr("4GB"), size=excess_mem_size),
        ]

    # Platform
    self.pc = Pc()

    # Create and connect the busses required by each memory system
    if Ruby:
        connectX86RubySystem(self)
    else:
        connectX86ClassicSystem(self, numCPUs)

    # Disks
    disks = makeCowDisks(mdesc.disks())
    self.pc.south_bridge.ide.disks = disks

    # Add in a Bios information structure.
    structures = [X86SMBiosBiosInformation()]
    workload.smbios_table.structures = structures

    # Set up the Intel MP table
    base_entries = []
    ext_entries = []
    madt_records = []
    for i in range(numCPUs):
        bp = X86IntelMPProcessor(
            local_apic_id=i,
            local_apic_version=0x14,
            enable=True,
            bootstrap=(i == 0),
        )
        base_entries.append(bp)
        lapic = X86ACPIMadtLAPIC(acpi_processor_id=i, apic_id=i, flags=1)
        madt_records.append(lapic)
    io_apic = X86IntelMPIOAPIC(
        id=numCPUs, version=0x11, enable=True, address=0xFEC00000
    )
    self.pc.south_bridge.io_apic.apic_id = io_apic.id
    base_entries.append(io_apic)
    madt_records.append(
        X86ACPIMadtIOAPIC(id=io_apic.id, address=io_apic.address, int_base=0)
    )
    # In gem5 Pc::calcPciConfigAddr(), it required "assert(bus==0)",
    # but linux kernel cannot config PCI device if it was not connected to
    # PCI bus, so we fix PCI bus id to 0, and ISA bus id to 1.
    pci_bus = X86IntelMPBus(bus_id=0, bus_type="PCI   ")
    base_entries.append(pci_bus)
    isa_bus = X86IntelMPBus(bus_id=1, bus_type="ISA   ")
    base_entries.append(isa_bus)
    connect_busses = X86IntelMPBusHierarchy(
        bus_id=1, subtractive_decode=True, parent_bus=0
    )
    ext_entries.append(connect_busses)
    pci_dev4_inta = X86IntelMPIOIntAssignment(
        interrupt_type="INT",
        polarity="ConformPolarity",
        trigger="ConformTrigger",
        source_bus_id=0,
        source_bus_irq=0 + (4 << 2),
        dest_io_apic_id=io_apic.id,
        dest_io_apic_intin=16,
    )
    base_entries.append(pci_dev4_inta)
    pci_dev4_inta_madt = X86ACPIMadtIntSourceOverride(
        bus_source=pci_dev4_inta.source_bus_id,
        irq_source=pci_dev4_inta.source_bus_irq,
        sys_int=pci_dev4_inta.dest_io_apic_intin,
        flags=0,
    )
    madt_records.append(pci_dev4_inta_madt)

    def assignISAInt(irq, apicPin):
        assign_8259_to_apic = X86IntelMPIOIntAssignment(
            interrupt_type="ExtInt",
            polarity="ConformPolarity",
            trigger="ConformTrigger",
            source_bus_id=1,
            source_bus_irq=irq,
            dest_io_apic_id=io_apic.id,
            dest_io_apic_intin=0,
        )
        base_entries.append(assign_8259_to_apic)
        assign_to_apic = X86IntelMPIOIntAssignment(
            interrupt_type="INT",
            polarity="ConformPolarity",
            trigger="ConformTrigger",
            source_bus_id=1,
            source_bus_irq=irq,
            dest_io_apic_id=io_apic.id,
            dest_io_apic_intin=apicPin,
        )
        base_entries.append(assign_to_apic)
        # acpi
        assign_to_apic_acpi = X86ACPIMadtIntSourceOverride(
            bus_source=1, irq_source=irq, sys_int=apicPin, flags=0
        )
        madt_records.append(assign_to_apic_acpi)

    assignISAInt(0, 2)
    assignISAInt(1, 1)
    for i in range(3, 15):
        assignISAInt(i, i)
    workload.intel_mp_table.base_entries = base_entries
    workload.intel_mp_table.ext_entries = ext_entries

    madt = X86ACPIMadt(
        local_apic_address=0, records=madt_records, oem_id="madt"
    )
    workload.acpi_description_table_pointer.rsdt.entries.append(madt)
    workload.acpi_description_table_pointer.xsdt.entries.append(madt)
    workload.acpi_description_table_pointer.oem_id = "gem5"
    workload.acpi_description_table_pointer.rsdt.oem_id = "gem5"
    workload.acpi_description_table_pointer.xsdt.oem_id = "gem5"
    return self


def makeLinuxX86System(
    mem_mode, numCPUs=1, mdesc=None, Ruby=False, cmdline=None
):
    # Build up the x86 system and then specialize it for Linux
    self = makeX86System(mem_mode, numCPUs, mdesc, X86FsLinux(), Ruby)

    # We assume below that there's at least 1MB of memory. We'll require 2
    # just to avoid corner cases.
    phys_mem_size = sum([r.size() for r in self.mem_ranges])
    assert phys_mem_size >= 0x200000
    assert len(self.mem_ranges) <= 2

    entries = [
        # Mark the first megabyte of memory as reserved
        X86E820Entry(addr=0, size="639kB", range_type=1),
        X86E820Entry(addr=0x9FC00, size="385kB", range_type=2),
        # Mark the rest of physical memory as available
        X86E820Entry(
            addr=0x100000,
            size="%dB" % (self.mem_ranges[0].size() - 0x100000),
            range_type=1,
        ),
    ]

    # Mark [mem_size, 3GB) as reserved if memory less than 3GB, which force
    # IO devices to be mapped to [0xC0000000, 0xFFFF0000). Requests to this
    # specific range can pass though bridge to iobus.
    if len(self.mem_ranges) == 1:
        entries.append(
            X86E820Entry(
                addr=self.mem_ranges[0].size(),
                size="%dB" % (0xC0000000 - self.mem_ranges[0].size()),
                range_type=2,
            )
        )

    # Reserve the last 16kB of the 32-bit address space for the m5op interface
    entries.append(X86E820Entry(addr=0xFFFF0000, size="64kB", range_type=2))

    # In case the physical memory is greater than 3GB, we split it into two
    # parts and add a separate e820 entry for the second part.  This entry
    # starts at 0x100000000,  which is the first address after the space
    # reserved for devices.
    if len(self.mem_ranges) == 2:
        entries.append(
            X86E820Entry(
                addr=0x100000000,
                size="%dB" % (self.mem_ranges[1].size()),
                range_type=1,
            )
        )

    self.workload.e820_table.entries = entries

    # Command line
    if not cmdline:
        cmdline = "earlyprintk=ttyS0 console=ttyS0 lpj=7999923 root=/dev/hda1"
    self.workload.command_line = fillInCmdline(mdesc, cmdline)
    return self


def makeBareMetalRiscvSystem(mem_mode, mdesc=None, cmdline=None):
    self = System()
    if not mdesc:
        # generic system
        mdesc = SysConfig()
    self.mem_mode = mem_mode
    self.mem_ranges = [AddrRange(mdesc.mem())]

    self.workload = RiscvBareMetal()

    self.iobus = IOXBar()
    self.membus = MemBus()

    self.bridge = Bridge(delay="50ns")
    self.bridge.mem_side_port = self.iobus.cpu_side_ports
    self.bridge.cpu_side_port = self.membus.mem_side_ports
    # Sv39 has 56 bit physical addresses; use the upper 8 bit for the IO space
    IO_address_space_base = 0x00FF000000000000
    self.bridge.ranges = [AddrRange(IO_address_space_base, Addr.max)]

    self.system_port = self.membus.cpu_side_ports
    return self


def makeDualRoot(full_system, testSystem, driveSystem, dumpfile):
    self = Root(full_system=full_system)
    self.testsys = testSystem
    self.drivesys = driveSystem
    self.etherlink = EtherLink()

    if hasattr(testSystem, "realview"):
        self.etherlink.int0 = Parent.testsys.realview.ethernet.interface
        self.etherlink.int1 = Parent.drivesys.realview.ethernet.interface
    elif hasattr(testSystem, "tsunami"):
        self.etherlink.int0 = Parent.testsys.tsunami.ethernet.interface
        self.etherlink.int1 = Parent.drivesys.tsunami.ethernet.interface
    else:
        fatal("Don't know how to connect these system together")

    if dumpfile:
        self.etherdump = EtherDump(file=dumpfile)
        self.etherlink.dump = Parent.etherdump

    return self


def makeDistRoot(
    testSystem,
    rank,
    size,
    server_name,
    server_port,
    sync_repeat,
    sync_start,
    linkspeed,
    linkdelay,
    dumpfile,
):
    self = Root(full_system=True)
    self.testsys = testSystem

    self.etherlink = DistEtherLink(
        speed=linkspeed,
        delay=linkdelay,
        dist_rank=rank,
        dist_size=size,
        server_name=server_name,
        server_port=server_port,
        sync_start=sync_start,
        sync_repeat=sync_repeat,
    )

    if hasattr(testSystem, "realview"):
        self.etherlink.int0 = Parent.testsys.realview.ethernet.interface
    elif hasattr(testSystem, "tsunami"):
        self.etherlink.int0 = Parent.testsys.tsunami.ethernet.interface
    else:
        fatal("Don't know how to connect DistEtherLink to this system")

    if dumpfile:
        self.etherdump = EtherDump(file=dumpfile)
        self.etherlink.dump = Parent.etherdump

    return self

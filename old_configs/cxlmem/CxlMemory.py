from m5.defines import buildEnv
from m5.objects import *
import m5.objects
from common import ObjectList
from common import HMC

class CxlMemCache(Cache):
    assoc = 8
    tag_latency = 20
    data_latency = 20
    response_latency = 20
    mshrs = 20
    tgts_per_mshr = 12
    write_buffers = 8

# class CxlDRAM(DDR4_2400_16x4):
# A single DDR4-2400 x64 channel (one command and address bus), with
# timings based on a DDR4-2400 8 Gbit datasheet (Micron MT40A2G4)
# in an 16x4 configuration.
# Total channel capacity is 32GiB
# 16 devices/rank * 2 ranks/channel * 1GiB/device = 32GiB/channel
class CxlDRAM(DRAMInterface):
    # size of device
    device_size = '1GiB'

    # 16x4 configuration, 16 devices each with a 4-bit interface
    device_bus_width = 4

    # DDR4 is a BL8 device
    burst_length = 8

    # Each device has a page (row buffer) size of 512 byte (1K columns x4)
    device_rowbuffer_size = '512B'

    # 16x4 configuration, so 16 devices
    devices_per_rank = 16

    # Match our DDR3 configurations which is dual rank
    ranks_per_channel = 2

    # DDR4 has 2 (x16) or 4 (x4 and x8) bank groups
    # Set to 4 for x4 case
    bank_groups_per_rank = 4

    # DDR4 has 16 banks(x4,x8) and 8 banks(x16) (4 bank groups in all
    # configurations). Currently we do not capture the additional
    # constraints incurred by the bank groups
    banks_per_rank = 16

    # override the default buffer sizes and go for something larger to
    # accommodate the larger bank count
    write_buffer_size = 128
    read_buffer_size = 64

    # 1200 MHz
    tCK = '0.833ns'

    # 8 beats across an x64 interface translates to 4 clocks @ 1200 MHz
    # tBURST is equivalent to the CAS-to-CAS delay (tCCD)
    # With bank group architectures, tBURST represents the CAS-to-CAS
    # delay for bursts to different bank groups (tCCD_S)
    tBURST = '3.332ns'

    # @2400 data rate, tCCD_L is 6 CK
    # CAS-to-CAS delay for bursts to the same bank group
    # tBURST is equivalent to tCCD_S; no explicit parameter required
    # for CAS-to-CAS delay for bursts to different bank groups
    tCCD_L = '5ns';

    # DDR4-2400 17-17-17
    tRCD = '14.16ns'
    tCL = '14.16ns'
    tRP = '14.16ns'
    tRAS = '32ns'

    # RRD_S (different bank group) for 512B page is MAX(4 CK, 3.3ns)
    tRRD = '3.332ns'

    # RRD_L (same bank group) for 512B page is MAX(4 CK, 4.9ns)
    tRRD_L = '4.9ns';

    # tFAW for 512B page is MAX(16 CK, 13ns)
    tXAW = '13.328ns'
    activation_limit = 4
    # tRFC is 350ns
    tRFC = '350ns'

    tWR = '15ns'

    # Here using the average of WTR_S and WTR_L
    tWTR = '5ns'

    # Greater of 4 CK or 7.5 ns
    tRTP = '7.5ns'

    # Default same rank rd-to-wr bus turnaround to 2 CK, @1200 MHz = 1.666 ns
    tRTW = '1.666ns'

    # Default different rank bus delay to 2 CK, @1200 MHz = 1.666 ns
    tCS = '1.666ns'

    # <=85C, half for >85C
    tREFI = '7.8us'

    # active powerdown and precharge powerdown exit time
    tXP = '6ns'

    # self refresh exit time
    # exit delay to ACT, PRE, PREALL, REF, SREF Enter, and PD Enter is:
    # tRFC + 10ns = 340ns
    tXS = '340ns'

    # Current values from datasheet
    IDD0 = '43mA'
    IDD02 = '3mA'
    IDD2N = '34mA'
    IDD3N = '38mA'
    IDD3N2 = '3mA'
    IDD4W = '103mA'
    IDD4R = '110mA'
    IDD5 = '250mA'
    IDD3P1 = '32mA'
    IDD2P1 = '25mA'
    IDD6 = '30mA'
    VDD = '1.2V'
    VDD2 = '2.5V'


class CxlMemXar(CoherentXBar):
    # 128-bit crossbar by default
    width = 16

    # A handful pipeline stages for each portion of the latency
    # contributions.
    frontend_latency = 3
    forward_latency = 4
    response_latency = 2
    snoop_response_latency = 4

    # Use a snoop-filter by default
    snoop_filter = SnoopFilter(lookup_latency = 1)
    point_of_coherency = True
    point_of_unification = True



def create_mem_intf(intf, r, i, intlv_bits, intlv_size,
                    xor_low_bit):
    """
    Helper function for creating a single memoy controller from the given
    options.  This function is invoked multiple times in config_mem function
    to create an array of controllers.
    """

    import math
    intlv_low_bit = int(math.log(intlv_size, 2))

    # Use basic hashing for the channel selection, and preferably use
    # the lower tag bits from the last level cache. As we do not know
    # the details of the caches here, make an educated guess. 4 MByte
    # 4-way associative with 64 byte cache lines is 6 offset bits and
    # 14 index bits.
    if (xor_low_bit):
        xor_high_bit = xor_low_bit + intlv_bits - 1
    else:
        xor_high_bit = 0

    # Create an instance so we can figure out the address
    # mapping and row-buffer size
    interface = intf()

    # Only do this for DRAMs
    if issubclass(intf, m5.objects.DRAMInterface):
        # If the channel bits are appearing after the column
        # bits, we need to add the appropriate number of bits
        # for the row buffer size
        if interface.addr_mapping.value == 'RoRaBaChCo':
            # This computation only really needs to happen
            # once, but as we rely on having an instance we
            # end up having to repeat it for each and every
            # one
            rowbuffer_size = interface.device_rowbuffer_size.value * \
                interface.devices_per_rank.value

            intlv_low_bit = int(math.log(rowbuffer_size, 2))

    # Also adjust interleaving bits for NVM attached as memory
    # Will have separate range defined with unique interleaving
    if issubclass(intf, m5.objects.NVMInterface):
        # If the channel bits are appearing after the low order
        # address bits (buffer bits), we need to add the appropriate
        # number of bits for the buffer size
        if interface.addr_mapping.value == 'RoRaBaChCo':
            # This computation only really needs to happen
            # once, but as we rely on having an instance we
            # end up having to repeat it for each and every
            # one
            buffer_size = interface.per_bank_buffer_size.value

            intlv_low_bit = int(math.log(buffer_size, 2))

    # We got all we need to configure the appropriate address
    # range
    interface.range = m5.objects.AddrRange(r.start, size = r.size(),
                                      intlvHighBit = \
                                          intlv_low_bit + intlv_bits - 1,
                                      xorHighBit = xor_high_bit,
                                      intlvBits = intlv_bits,
                                      intlvMatch = i)
    return interface

def config_mem(options, system):
    """
    Create the memory controllers based on the options and attach them.

    If requested, we make a multi-channel configuration of the
    selected memory controller class by creating multiple instances of
    the specific class. The individual controllers have their
    parameters set such that the address range is interleaved between
    them.
    """

    # # Mandatory options
    # opt_mem_channels = options.mem_channels

    # # Semi-optional options
    # # Must have either mem_type or nvm_type or both
    # opt_mem_type = getattr(options, "mem_type", None)
    # opt_nvm_type = getattr(options, "nvm_type", None)
    # if not opt_mem_type and not opt_nvm_type:
    #     fatal("Must have option for either mem-type or nvm-type, or both")

    # # Optional options
    # opt_tlm_memory = getattr(options, "tlm_memory", None)
    # opt_external_memory_system = getattr(options, "external_memory_system",
    #                                      None)
    # opt_elastic_trace_en = getattr(options, "elastic_trace_en", False)
    # opt_mem_ranks = getattr(options, "mem_ranks", None)
    # opt_nvm_ranks = getattr(options, "nvm_ranks", None)
    # opt_hybrid_channel = getattr(options, "hybrid_channel", False)
    # opt_dram_powerdown = getattr(options, "enable_dram_powerdown", None)
    # opt_mem_channels_intlv = getattr(options, "mem_channels_intlv", 128)
    # opt_xor_low_bit = getattr(options, "xor_low_bit", 0)

    # Mandatory options
    opt_mem_channels = 2

    opt_mem_type = "DDR4_2400_4x16"
    opt_nvm_type = None
    if not opt_mem_type and not opt_nvm_type:
        fatal("Must have option for either mem-type or nvm-type, or both")

    # Optional options
    opt_tlm_memory = None #getattr(options, "tlm_memory", None)
    opt_external_memory_system = None # getattr(options, "external_memory_system",
                                      #   None)
    opt_elastic_trace_en = False # getattr(options, "elastic_trace_en", False)
    opt_mem_ranks = None # getattr(options, "mem_ranks", None)
    opt_nvm_ranks = None #getattr(options, "nvm_ranks", None)
    opt_hybrid_channel = False #getattr(options, "hybrid_channel", False)
    opt_dram_powerdown = None #getattr(options, "enable_dram_powerdown", None)
    opt_mem_channels_intlv = 128 #getattr(options, "mem_channels_intlv", 128)
    opt_xor_low_bit = 0 #getattr(options, "xor_low_bit", 0)

    if opt_mem_type == "HMC_2500_1x32":
        HMChost = HMC.config_hmc_host_ctrl(options, system)
        HMC.config_hmc_dev(options, system, HMChost.hmc_host)
        subsystem = system.hmc_dev
        xbar = system.hmc_dev.xbar
    else:
        subsystem = system
        xbar = system.InternalMemBus

    # if opt_tlm_memory:
    #     system.external_memory = m5.objects.ExternalSlave(
    #         port_type="tlm_slave",
    #         port_data=opt_tlm_memory,
    #         port=system.InternalMemBus.mem_side_ports,
    #         addr_ranges=system.mem_ranges)
    #     system.workload.addr_check = False
    #     return

    # if opt_external_memory_system:
    #     subsystem.external_memory = m5.objects.ExternalSlave(
    #         port_type=opt_external_memory_system,
    #         port_data="init_mem0", port=xbar.mem_side_ports,
    #         addr_ranges=system.mem_ranges)
    #     subsystem.workload.addr_check = False
    #     return

    nbr_mem_ctrls = opt_mem_channels

    import math
    from m5.util import fatal
    intlv_bits = int(math.log(nbr_mem_ctrls, 2))
    if 2 ** intlv_bits != nbr_mem_ctrls:
        fatal("Number of memory channels must be a power of 2")

    if opt_mem_type:
        intf = ObjectList.mem_list.get(opt_mem_type)
    if opt_nvm_type:
        n_intf = ObjectList.mem_list.get(opt_nvm_type)

    nvm_intfs = []
    mem_ctrls = []

    if opt_elastic_trace_en and not issubclass(intf, m5.objects.SimpleMemory):
        fatal("When elastic trace is enabled, configure mem-type as "
                "simple-mem.")

    # The default behaviour is to interleave memory channels on 128
    # byte granularity, or cache line granularity if larger than 128
    # byte. This value is based on the locality seen across a large
    # range of workloads.
    # intlv_size = max(opt_mem_channels_intlv, system.cache_line_size.value)
    intlv_size = max(opt_mem_channels_intlv, 128)

    # For every range (most systems will only have one), create an
    # array of memory interfaces and set their parameters to match
    # their address mapping in the case of a DRAM
    range_iter = 0
    for r in system.InternalMemRanges:
        print(r)
        # As the loops iterates across ranges, assign them alternatively
        # to DRAM and NVM if both configured, starting with DRAM
        range_iter += 1

        for i in range(nbr_mem_ctrls):
            if opt_mem_type and (not opt_nvm_type or range_iter % 2 != 0):
                # Create the DRAM interface
                print("Create DRAM Interface")
                dram_intf = create_mem_intf(intf, r, i,
                    intlv_bits, intlv_size, opt_xor_low_bit)

                # Set the number of ranks based on the command-line
                # options if it was explicitly set
                if issubclass(intf, m5.objects.DRAMInterface) and \
                   opt_mem_ranks:
                    dram_intf.ranks_per_channel = opt_mem_ranks

                # Enable low-power DRAM states if option is set
                if issubclass(intf, m5.objects.DRAMInterface):
                    dram_intf.enable_dram_powerdown = opt_dram_powerdown

                if opt_elastic_trace_en:
                    dram_intf.latency = '1ns'
                    print("For elastic trace, over-riding Simple Memory "
                        "latency to 1ns.")

                # Create the controller that will drive the interface
                mem_ctrl = dram_intf.controller()

                mem_ctrls.append(mem_ctrl)

            elif opt_nvm_type and (not opt_mem_type or range_iter % 2 == 0):
                nvm_intf = create_mem_intf(n_intf, r, i,
                    intlv_bits, intlv_size, opt_xor_low_bit)

                # Set the number of ranks based on the command-line
                # options if it was explicitly set
                if issubclass(n_intf, m5.objects.NVMInterface) and \
                   opt_nvm_ranks:
                    nvm_intf.ranks_per_channel = opt_nvm_ranks

                # Create a controller if not sharing a channel with DRAM
                # in which case the controller has already been created
                if not opt_hybrid_channel:
                    mem_ctrl = m5.objects.HeteroMemCtrl()
                    mem_ctrl.nvm = nvm_intf

                    mem_ctrls.append(mem_ctrl)
                else:
                    nvm_intfs.append(nvm_intf)

    # hook up NVM interface when channel is shared with DRAM + NVM
    for i in range(len(nvm_intfs)):
        mem_ctrls[i].nvm = nvm_intfs[i];

    # Connect the controller to the xbar port
    for i in range(len(mem_ctrls)):
        if opt_mem_type == "HMC_2500_1x32":
            # Connect the controllers to the membus
            mem_ctrls[i].port = xbar[i//4].mem_side_ports
            # Set memory device size. There is an independent controller
            # for each vault. All vaults are same size.
            mem_ctrls[i].dram.device_size = options.hmc_dev_vault_size
        else:
            # Connect the controllers to the membus
            mem_ctrls[i].port = xbar.mem_side_ports

    subsystem.InternalMemCtrls = mem_ctrls

def config_cache(options, sub_system, cxldev):
    #cxldev.cxlcache = CxlMemCache(addr_ranges = sub_system.mem_ranges + sub_system.cxlmem_ranges, size = '4MB')
    cxldev.cxlcache = CxlMemCache( size = '4MB')
    cxldev.dummybar = CxlMemXar()
    cxldev.cxlcache.mem_side = cxldev.InternalMemBus.cpu_side_ports
    cxldev.cxlcache.cpu_side = cxldev.dummybar.mem_side_ports

def config_core(options, sub_system, cxldev):
    cxldev.dummycore = DummyCore()
    cxldev.dummycore.dummyMemSidePort = cxldev.dummybar.cpu_side_ports
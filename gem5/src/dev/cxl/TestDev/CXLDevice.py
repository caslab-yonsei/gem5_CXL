from m5.SimObject import SimObject
from m5.params import *
from m5.proxy import *
from m5.objects.Device import DmaDevice
from m5.objects.CxlHost import PciHost
from m5.objects.PciDevice import PciDevice, PciBar, PciIoBar, PciMemBar
from m5.objects.PcieDevice import PcieDevice


class CXLDevice(PcieDevice):
    type = "CXLDevice"
    cxx_class = "gem5::CXLDevice"
    cxx_header = "dev/cxl/TestDev/CXLDevice.hh"
    abstract = False

    just_int = Param.Int(1, "test")

    pio_addr = Param.Addr("Address")
    pio_size = Param.Addr(0x8, "Size of address range")
    pio_delay = Param.Latency("10ns","delay")
    ret_data8 = Param.UInt8(0xFF, "Default data to return")
    ret_data16 = Param.UInt16(0xFFFF, "Default data to return")
    ret_data32 = Param.UInt32(0xFFFFFFFF, "Default data to return")
    ret_data64 = Param.UInt64(0xFFFFFFFFFFFFFFFF, "Default data to return")

    VendorID = 0x8086
    DeviceID = 0x2040
    Revision = 0xA2
    SubsystemID = 0
    SubsystemVendorID = 0
    Status = 0x0000
    SubClassCode = 0x08
    ClassCode = 0x80
    ProgIF = 0x00
    MaximumLatency = 0x00
    MinimumGrant = 0xFF
    InterruptLine = 0x20
    InterruptPin = 0x01

    BAR0 = PciMemBar(size="1KiB")

    coh_port = MasterPort("upstream masterport")
    in_port = MasterPort("downstream masterport")
    ex_port = SlavePort("downstream slaveport")




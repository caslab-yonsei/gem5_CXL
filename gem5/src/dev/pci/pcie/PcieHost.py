from m5.SimObject import SimObject
from m5.params import *
from m5.proxy import *
from m5.objects.Device import PioDevice
from m5.objects.PciDevice import PciDevice
from m5.objects.PciHost import PciHost, GenericPciHost
from m5.objects.Platform import Platform

class PcieHost(PciDevice):
    type = 'PcieHost'
    cxx_class = 'gem5::PcieHost'
    cxx_header = "dev/pci/pcie/host.hh"

    

    #legacy = Param.PciHost(Self.all, "legacy cont") #Param.GenericPciHost(NULL,"Legacy bus host")
    abstract = True

class GenericPcieHost(PcieHost):
    type = 'GenericPcieHost'
    cxx_class = 'gem5::GenericPcieHost'
    cxx_header = "dev/pci/pcie/host.hh"

    platform = Param.Platform(Parent.any, "Platform to use for interrupts")

    conf_base = Param.Addr("Config space base address")
    conf_size = Param.Addr("Config space base address")
    conf_device_bits = Param.UInt8(8, "Number of bits used to as an "
                                      "offset a devices address space")

    #legacy = Param.PciHost(NULL, "pp")

    # class 0x06 04 00
    ProgIF = 00 #Param.UInt8(0, "Programming Interface")
    SubClassCode = 0x04 #Param.UInt8(0, "Sub-Class Code")
    ClassCode = 0x06 #Param.UInt8(0, "Class Code")
    Revision = 0x2
    HeaderType = 0x01

    VendorID = 0x2010
    DeviceID = 0x0819

    pci_pio_base = Param.Addr(0, "Base address for PCI IO accesses")
    pci_mem_base = Param.Addr(0, "Base address for PCI memory accesses")
    pci_dma_base = Param.Addr(0, "Base address for DMA memory accesses")
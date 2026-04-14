from m5.SimObject import SimObject
from m5.params import *
from m5.proxy import *
from m5.objects.Device import DmaDevice
from m5.objects.CxlHost import PciHost
from m5.objects.PciDevice import PciDevice, PciBar, PciIoBar, PciMemBar
from m5.objects.PcieDevice import PcieDevice
from m5.objects.CxlDevice import CxlDevice
from m5.objects.Platform import Platform

class CXLplatform(Platform):
    type = 'CXLplatform'
    cxx_header = "dev/cxl/TestDev/CXLplatform.hh"
    cxx_class = 'gem5::CXLplatform'
    #_mem_regions = [ AddrRange(0, size='256MiB') ]
    _num_pci_dev = 0
    host_platform = Param.Platform(Parent.any, "Platform this device is part of.")

    

    def _on_chip_devices(self):
        return []

    def _off_chip_devices(self):
        return []

    def _on_chip_memory(self):
        return []

    def _off_chip_memory(self):
        return []

    _off_chip_ranges = []

    def _attach_memory(self, mem, bus, mem_ports=None):
        if hasattr(mem, "port"):
            if mem_ports is None:
                mem.port = bus.mem_side_ports
            else:
                mem_ports.append(mem.port)

    def _attach_device(self, device, bus, dma_ports=None):
        if hasattr(device, "pio"):
            device.pio = bus.mem_side_ports
        if hasattr(device, "dma"):
            if dma_ports is None:
                device.dma = bus.cpu_side_ports
            else:
                dma_ports.append(device.dma)

    def _attach_io(self, devices, *args, **kwargs):
        for d in devices:
            self._attach_device(d, *args, **kwargs)

    def _attach_mem(self, memories, *args, **kwargs):
        for mem in memories:
            self._attach_memory(mem, *args, **kwargs)

    def _attach_clk(self, devices, clkdomain):
        for d in devices:
            if hasattr(d, "clk_domain"):
                d.clk_domain = clkdomain

    
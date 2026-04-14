
from m5.params import *
from m5.proxy import *
from m5.objects.ClockedObject import ClockedObject


class CXLProcessor(ClockedObject):
    type = "CXLProcessor"
    cxx_class = "gem5::CXLProcessor"
    cxx_header = "dev/cxl/TestDev/CXLProcessor.hh"
    abstract = False
    system = Param.System(Parent.any, "System this device is part of")


    mem_side_ports = RequestPort("Memory side port, sends requests")

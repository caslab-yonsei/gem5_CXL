from m5.params import *
from m5.objects.AbstractMemory import AbstractMemory
from m5.objects.XBar import *
from m5.objects.ClockedObject import ClockedObject

class CxlMemInternalBus1(CoherentXBar):
    width = 512

class CxlMemOnlyDev(ClockedObject):
    type = 'CxlMemOnlyDev'
    cxx_header = "mem/cxl/cxl_mem_dev.hh"
    cxx_class = 'gem5::memory::CxlMemOnlyDev'

    abstract = False

    # 추후 더 나은 방식으로 수정 예정
    
    # 내부 용 메모리버스
    InternalMemBus = BaseXBar()
    # 외부 연결용 버스
    ExternalBus = BaseXBar()
    
    # 내부 메모리 버스에 연결된 메모리 장치 리스트
    #MemCtrls = []
    # 내부 컨트롤러 연결용
    to_internal_port = RequestPort("")
    from_internal_port = ResponsePort("")

    def connect_memctrls(self, memctrls):
        for i in memctrls:
            self.InternalMemBus.mem_side_ports = i
        self.InternalMemBus.cpu_side_ports = self.to_internal_port
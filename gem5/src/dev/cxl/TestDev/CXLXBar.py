from m5.params import *
from m5.proxy import *
from m5.objects.ClockedObject import ClockedObject
from m5.objects.System import System
from m5.SimObject import SimObject
from m5.objects.XBar import *

class CXLXBar(CoherentXBar):

    type = 'CXLXBar'
    cxx_class = 'gem5::CXLXBar'
    cxx_header = "dev/cxl/TestDev/CXLXBar.hh"
    abstract = False    

        # 128-bit crossbar by default
    width = 16

    # A handful pipeline stages for each portion of the latency
    # contributions.
    frontend_latency = 3
    forward_latency = 4
    response_latency = 2
    snoop_response_latency = 4

    # Use a snoop-filter by default
    snoop_filter = SnoopFilter(lookup_latency=1)
    # snoop_filter = NULL

    # This specialisation of the coherent crossbar is to be considered
    # the point of coherency, as there are no (coherent) downstream
    # caches.
    point_of_coherency = True

    # This specialisation of the coherent crossbar is to be considered
    # the point of unification, it connects the dcache and the icache
    # to the first level of unified cache. This is needed for systems
    # without caches where the SystemXBar is also the point of
    # unification.
    point_of_unification = True


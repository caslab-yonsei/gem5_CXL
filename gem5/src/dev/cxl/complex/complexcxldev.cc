#include "dev/cxl/complex/complexcxldev.hh"

#include "base/logging.hh"
// #include "dev/arm/base_gic.hh"

namespace gem5
{

ComplexCxlDev::ComplexCxlDev(const Params &p)
    : Platform(p), host_platform(p.host_platform)
{}

void
ComplexCxlDev::postConsoleInt()
{
    warn_once("Don't know what interrupt to post for console.\n");
    //panic("Need implementation\n");
}

void
ComplexCxlDev::clearConsoleInt()
{
    warn_once("Don't know what interrupt to clear for console.\n");
    //panic("Need implementation\n");
}

void
ComplexCxlDev::postPciInt(int line)
{
    host_platform->postPciInt(line);
}

void
ComplexCxlDev::clearPciInt(int line)
{
    host_platform->clearPciInt(line);
}

} // namespace gem5
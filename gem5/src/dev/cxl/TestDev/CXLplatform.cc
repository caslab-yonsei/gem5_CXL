#include "dev/cxl/TestDev/CXLplatform.hh"

#include "base/logging.hh"
// #include "dev/arm/base_gic.hh"

namespace gem5
{

CXLplatform::CXLplatform(const Params &p)
    : Platform(p), host_platform(p.host_platform)
{}

void
CXLplatform::postConsoleInt()
{
    warn_once("Don't know what interrupt to post for console.\n");
    //panic("Need implementation\n");
}

void
CXLplatform::clearConsoleInt()
{
    warn_once("Don't know what interrupt to clear for console.\n");
    //panic("Need implementation\n");
}

void
CXLplatform::postPciInt(int line)
{
    host_platform->postPciInt(line);
}

void
CXLplatform::clearPciInt(int line)
{
    host_platform->clearPciInt(line);
}

} // namespace gem5
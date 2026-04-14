#ifndef __DEV_CXL_COMPLEX_ComplexCxlDev_HH__
#define __DEV_CXL_COMPLEX_ComplexCxlDev_HH__

#include "dev/platform.hh"
#include "params/ComplexCxlDev.hh"

namespace gem5
{

//class BaseGic;
//class IdeController;

class ComplexCxlDev : public Platform
{
  public:
    //BaseGic *gic;
    Platform *host_platform;

  public:
    using Params = ComplexCxlDevParams;

    ComplexCxlDev(const Params &p);

    /** Give platform a pointer to interrupt controller */
    // void setGic(BaseGic *_gic) { gic = _gic; }

  public: // Public Platform interfaces
    void postConsoleInt() override;
    void clearConsoleInt() override;

    void postPciInt(int line) override;
    void clearPciInt(int line) override;
};

} // namespace gem5

#endif // __DEV_CXL_COMPLEX_ComplexCxlDev_HH__

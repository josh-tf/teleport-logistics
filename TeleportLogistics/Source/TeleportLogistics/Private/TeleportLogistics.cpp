#include "Modules/ModuleManager.h"
#include "TeleportLogisticsLog.h"
#include "TeleportLogisticsMap.h"

DEFINE_LOG_CATEGORY(LogTeleportLogistics);

class FTeleportLogisticsModule : public FDefaultGameModuleImpl
{
  public:
    virtual void StartupModule() override
    {
        TeleportLogisticsMap::RegisterTypes();
    }
};
IMPLEMENT_MODULE(FTeleportLogisticsModule, TeleportLogistics)

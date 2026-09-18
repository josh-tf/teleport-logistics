#include "TeleportLogisticsSettings.h"

#include "TeleportLogisticsLog.h"
#include "Configuration/ConfigManager.h"
#include "UObject/ConstructorHelpers.h"
#include "Configuration/Properties/ConfigPropertyBool.h"
#include "Configuration/Properties/ConfigPropertyFloat.h"
#include "Configuration/Properties/ConfigPropertySection.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

namespace
{
const TCHAR *RefreshKey = TEXT("RefreshSeconds");
const TCHAR *RouteKey = TEXT("ShowRouteInLookAt");
const TCHAR *ConfirmKey = TEXT("ConfirmFluidFlush");

FConfigId ConfigId()
{
    return FConfigId{TEXT("TeleportLogistics"), FString()};
}

UConfigPropertySection *RootSectionFor(const UObject *Context)
{
    const UWorld *World = Context ? Context->GetWorld() : nullptr;
    const UGameInstance *Instance = World ? World->GetGameInstance() : nullptr;
    auto *Manager = Instance ? Instance->GetSubsystem<UConfigManager>() : nullptr;
    return Manager ? Manager->GetConfigurationRootSection(ConfigId()) : nullptr;
}

template <typename PropertyType> PropertyType *Property(const UObject *Context, const TCHAR *Key)
{
    if (auto *Section = RootSectionFor(Context))
        if (auto *Found = Section->SectionProperties.Find(Key))
            return Cast<PropertyType>(*Found);
    return nullptr;
}

// SML implements CreateEditorWidget only on its Blueprint property subclasses; the
// native UConfigProperty returns NULL, so a configuration assembled from the native
// classes serialises correctly and then draws an empty panel. Build the properties
// from the Blueprint classes instead. Loading them here is safe because SML is a
// hard plugin dependency, so its content is mounted before this CDO is constructed.
template <typename PropertyType> UClass *WidgetCapableClass(const TCHAR *AssetPath)
{
    static ConstructorHelpers::FClassFinder<PropertyType> Found(AssetPath);
    if (Found.Succeeded())
        return Found.Class;
    UE_LOG(LogTeleportLogistics, Error,
           TEXT("TeleportLogistics: %s unavailable; settings will not be editable in the menu"),
           AssetPath);
    return PropertyType::StaticClass();
}
} // namespace

UTeleportLogisticsConfig::UTeleportLogisticsConfig()
{
    ConfigId = ::ConfigId();
    DisplayName = NSLOCTEXT("TeleportLogistics", "ConfigName", "Teleport Logistics");
    Description = NSLOCTEXT("TeleportLogistics", "ConfigDescription",
                            "Interface preferences. These apply to your game only and never change transport "
                            "rates, power draw or travel timings.");

    const TCHAR *Properties = TEXT("/SML/Interface/UI/Menu/Mods/ConfigProperties/");
    const auto Make = [this](const TCHAR *Name, UClass *Native, UClass *Concrete) {
        return CreateDefaultSubobject(Name, Native, Concrete, true, false);
    };
    auto *Section = Cast<UConfigPropertySection>(
        Make(TEXT("RootSection"), UConfigPropertySection::StaticClass(),
             WidgetCapableClass<UConfigPropertySection>(
                 *(FString(Properties) + TEXT("BP_ConfigPropertySection")))));
    UClass *const FloatClass = WidgetCapableClass<UConfigPropertyFloat>(
        *(FString(Properties) + TEXT("BP_ConfigPropertyFloat")));
    UClass *const BoolClass = WidgetCapableClass<UConfigPropertyBool>(
        *(FString(Properties) + TEXT("BP_ConfigPropertyBool")));

    auto *Refresh = Cast<UConfigPropertyFloat>(
        Make(TEXT("RefreshSeconds"), UConfigPropertyFloat::StaticClass(), FloatClass));
    Refresh->DisplayName = NSLOCTEXT("TeleportLogistics", "ConfigRefreshName", "Window refresh interval");
    Refresh->Tooltip = NSLOCTEXT("TeleportLogistics", "ConfigRefresh",
                                 "Seconds between refreshes of an open window, clamped to 0.1 to 2. Raise it "
                                 "on a large network if the interface costs you frames.");
    Refresh->DefaultValue = 0.35f;
    Refresh->Value = Refresh->DefaultValue;
    Section->SectionProperties.Add(RefreshKey, Refresh);

    auto *Route = Cast<UConfigPropertyBool>(
        Make(TEXT("ShowRouteInLookAt"), UConfigPropertyBool::StaticClass(), BoolClass));
    Route->DisplayName = NSLOCTEXT("TeleportLogistics", "ConfigRouteName", "Show route when looking at a building");
    Route->Tooltip = NSLOCTEXT("TeleportLogistics", "ConfigRoute",
                               "Name the channel and route in the look-at panel.");
    Route->DefaultValue = true;
    Route->Value = Route->DefaultValue;
    Section->SectionProperties.Add(RouteKey, Route);

    auto *Confirm = Cast<UConfigPropertyBool>(
        Make(TEXT("ConfirmFluidFlush"), UConfigPropertyBool::StaticClass(), BoolClass));
    Confirm->DisplayName = NSLOCTEXT("TeleportLogistics", "ConfigConfirmName", "Confirm before flushing fluid");
    Confirm->Tooltip = NSLOCTEXT("TeleportLogistics", "ConfigConfirm",
                                 "Ask before discarding a fluid endpoint's buffer. Taking items back is never "
                                 "destructive, so it is never confirmed.");
    Confirm->DefaultValue = true;
    Confirm->Value = Confirm->DefaultValue;
    Section->SectionProperties.Add(ConfirmKey, Confirm);

    RootSection = Section;
}

float UTeleportLogisticsConfig::RefreshSeconds(const UObject *Context)
{
    const auto *Found = Property<UConfigPropertyFloat>(Context, RefreshKey);
    return Found ? FMath::Clamp(Found->Value, 0.1f, 2.0f) : 0.35f;
}

bool UTeleportLogisticsConfig::ShowRouteInLookAt(const UObject *Context)
{
    const auto *Found = Property<UConfigPropertyBool>(Context, RouteKey);
    return Found ? Found->Value : true;
}

bool UTeleportLogisticsConfig::ConfirmFluidFlush(const UObject *Context)
{
    const auto *Found = Property<UConfigPropertyBool>(Context, ConfirmKey);
    return Found ? Found->Value : true;
}

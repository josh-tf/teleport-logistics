#pragma once
#include "CoreMinimal.h"
#include "UObject/StrongObjectPtr.h"

/**
 * Resolve a content path once and hold it for the life of the process.
 *
 * Every asset these buildings own is fetched from a constructor, and the engine
 * runs a constructor for each building as it spawns, not only once for the class
 * default object. A blocking load there lands while the world is streaming and
 * flushes every package in flight, which stops the game thread until the whole
 * queue has finished; FactoryGame.log caught one such flush holding a single
 * frame for 2.3 seconds. Only the first call for a path loads, and that one is
 * the class default object's, before any streaming is in motion.
 */
template <typename T> T *TeleportLogisticsAsset(const TCHAR *Path)
{
    check(IsInGameThread());
    static TMap<FString, TStrongObjectPtr<UObject>> Cache;
    TStrongObjectPtr<UObject> &Slot = Cache.FindOrAdd(Path);
    if (!Slot.IsValid())
        Slot.Reset(LoadObject<T>(nullptr, Path, nullptr, LOAD_NoWarn));
    return Cast<T>(Slot.Get());
}

#include "ScenecapsExtensionManager.h"
#include "ScenecapsModule.h"

#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"

FScenecapsExtensionManager& FScenecapsExtensionManager::Get()
{
    static FScenecapsExtensionManager Instance;
    return Instance;
}

void FScenecapsExtensionManager::Initialize()
{
    if (LoadedModuleHandles.Num() > 0)
    {
        UE_LOG(LogScenecaps, Log, TEXT("[Scenecaps.ExtensionManager] Already initialized (%d DLL(s) loaded), skipping."), LoadedModuleHandles.Num());
        return;
    }

    const FString ExtensionDir = FPaths::ConvertRelativePathToFull(
        FPaths::Combine(FPaths::ProjectDir(), TEXT("Plugins/Scenecaps/Extensions")));

    if (!FPaths::DirectoryExists(ExtensionDir))
    {
        UE_LOG(LogScenecaps, Log, TEXT("[Scenecaps.ExtensionManager] Extensions directory does not exist, skipping: %s"), *ExtensionDir);
        return;
    }

    TArray<FString> DllFiles;
    IFileManager::Get().FindFiles(DllFiles, *(ExtensionDir / TEXT("*.dll")), true, false);
    DllFiles.Sort();

    for (const FString& DllFile : DllFiles)
    {
        const FString DllPath = ExtensionDir / DllFile;
        void* Handle = FPlatformProcess::GetDllHandle(*DllPath);
        if (Handle)
        {
            LoadedModuleHandles.Add(Handle);
            UE_LOG(LogScenecaps, Log, TEXT("[Scenecaps.ExtensionManager] Loaded extension DLL: %s"), *DllPath);
        }
        else
        {
            UE_LOG(LogScenecaps, Warning, TEXT("[Scenecaps.ExtensionManager] Failed to load extension DLL: %s"), *DllPath);
        }
    }

    UE_LOG(LogScenecaps, Log, TEXT("[Scenecaps.ExtensionManager] Initialized: %d extension DLL(s) loaded."), LoadedModuleHandles.Num());
}

void FScenecapsExtensionManager::Shutdown()
{
    for (void* Handle : LoadedModuleHandles)
    {
        if (Handle)
        {
            FPlatformProcess::FreeDllHandle(Handle);
        }
    }

    const int32 FreedCount = LoadedModuleHandles.Num();
    LoadedModuleHandles.Empty();
    UE_LOG(LogScenecaps, Log, TEXT("[Scenecaps.ExtensionManager] Shutdown: freed %d extension DLL handle(s)."), FreedCount);
}

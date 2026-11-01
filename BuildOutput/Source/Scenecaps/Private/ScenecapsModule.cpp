// Plugins/Scenecaps/Source/Scenecaps/Private/ScenecapsModule.cpp

#include "ScenecapsModule.h"

#include "ScenecapsEntryRegistry.h"
#include "ScenecapsEntryHandlers.h"
#include "ScenecapsExtensionManager.h"
#include "ScenecapsDataDrivenEntryHandler.h"
#include "ScenecapsLocalization.h"

#include "HAL/IConsoleManager.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogScenecaps);

void FScenecapsModule::StartupModule()
{
    FScenecapsLocalization::Get().Reload();

    FScenecapsEntryRegistry& Registry = FScenecapsEntryRegistry::Get();
    Registry.ClearHandlers();

    // ---- 内置 handler（按优先级从上到下） ----
    Registry.RegisterHandler(MakeShared<FScenecapsBlueprintEntryHandler>());
#if SCENE_COLLECTION_WITH_NIAGARA
    Registry.RegisterHandler(MakeShared<FScenecapsNiagaraEntryHandler>());
#endif
    Registry.RegisterHandler(MakeShared<FScenecapsSoundEntryHandler>());
    Registry.RegisterHandler(MakeShared<FScenecapsNestedEntryHandler>());
    Registry.RegisterHandler(MakeShared<FScenecapsClassEntryHandler>());
    Registry.RegisterHandler(MakeShared<FScenecapsStaticMeshEntryHandler>());
    Registry.RegisterHandler(MakeShared<FScenecapsMaterialEntryHandler>());

    // ---- ★ 数据驱动 handler（读取 .scx 文件） ----
    // 加载顺序不影响：Handler 内部按 Entry.Priority 匹配
    // 放在内置 handler 之后，保证内置类型优先（避免误匹配）
    FScenecapsDataDrivenEntryHandler& DataDriven = FScenecapsDataDrivenEntryHandler::Get();
    DataDriven.LoadAll();

    // 用"不拥有所有权"的 SharedPtr 包装单例（Registry 内部不负责 delete）
    TSharedPtr<IScenecapsEntryHandler> DataDrivenHandler(
        &DataDriven,
        [](IScenecapsEntryHandler*) { /* 单例不销毁 */ });
    Registry.RegisterHandler(DataDrivenHandler);

    // ---- DLL 扩展管理器（保留上一轮方案） ----
    FScenecapsExtensionManager::Get().Initialize();

    // ---- 控制台命令：热重载 .scx ----
    {
        IConsoleObject* Cmd = IConsoleManager::Get().RegisterConsoleCommand(
            TEXT("Scenecaps.ReloadDataDriven"),
            TEXT("Reload all .scx data-driven extension files."),
            FConsoleCommandDelegate::CreateLambda([]()
                {
                    FScenecapsDataDrivenEntryHandler::Get().ReloadAll();
                    const auto& Handler = FScenecapsDataDrivenEntryHandler::Get();
                    UE_LOG(LogScenecaps, Log,
                        TEXT("[Scenecaps.DataDriven] Reloaded: %d entries, %d files, %d errors"),
                        Handler.GetEntryCount(),
                        Handler.GetLoadedFiles().Num(),
                        Handler.GetErrors().Num());
                }),
            ECVF_Default);
        (void)Cmd;
    }
}

void FScenecapsModule::ShutdownModule()
{
    FScenecapsExtensionManager::Get().Shutdown();
    FScenecapsEntryRegistry::Get().ClearHandlers();
}

IMPLEMENT_MODULE(FScenecapsModule, Scenecaps);
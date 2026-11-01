#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FScenecapsEditorToolkit;
class IConsoleObject;

class FScenecapsEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    /** Toolkit 在 InitEditor 时把自己登记进来，方便统一刷新。 */
    void RegisterToolkit(const TSharedRef<FScenecapsEditorToolkit>& Toolkit);

    /** 本地化数据重载后刷新所有打开中的 Scene Collection 编辑器。 */
    void RefreshAllToolkitsLocalizedUI();

private:
    void RegisterContentBrowserMenus();
    void RegisterConsoleCommands();

private:
    /** 弱引用：toolkit 销毁后自动失效，不需要反注册。 */
    TArray<TWeakPtr<FScenecapsEditorToolkit>> OpenToolkits;

    /** 需要在 ShutdownModule 里反注册的命令句柄。 */
    TArray<IConsoleObject*> RegisteredConsoleCommands;
};
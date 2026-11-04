# 编码规范

本文档规定 Scenecaps 项目的 C++ 编码规范，涵盖命名、文件组织、类型与内存、反射、格式、编辑器代码隔离与注释要求。

规范以 Unreal Engine 官方编码标准为基础，并结合本项目的模块划分作了具体约定。当本文与引擎官方约定存在差异时，以本文为准。

---

## 一、总则

- 代码应可读、可维护，优先考虑后来者的理解成本；
- 优先复用项目已有的类型、工具与实现方式，不重复造轮子；
- 改动保持最小必要范围，不混入与目标无关的重构或格式化；
- 不以「能编译」作为完成标准，需通过引用完整性与静态检查。

代码在 Development 与 Shipping 配置下均须无警告编译通过。项目的持续集成会对模块结构、依赖声明、头文件引用与公开导出宏进行检查。

---

## 二、命名约定

| 元素 | 规则 | 示例 |
| --- | --- | --- |
| 类（UObject 派生） | `U` + PascalCase | `UScenecapsAsset` |
| 类（Actor 派生） | `A` + PascalCase | `AScenecapsActor` |
| 结构体 | `F` + PascalCase | `FScenecapsEntry` |
| 枚举 | `E` + PascalCase | `EScenecapsModifierPhase` |
| 接口 | `I` + PascalCase | `IScenecapsModifierModule` |
| 模板 | `T` + PascalCase | `TArray` 用法以外自定 |
| 函数 / 方法 | PascalCase | `Rebuild()`, `ApplyModifiers()` |
| 变量 / 参数 / 成员 | camelCase | `spawnedActors`, `entryId` |
| 私有 / 受保护成员 | 不加前缀，必要时以下划线区分 | `cachedActor` |
| 布尔量 | `b` 前缀 | `bIsPlaying` |
| 布尔函数 | `Is` / `Has` / `Can` / `Should` | `IsValid()`, `CanExecute()` |
| 常量 / 宏 | UPPER_SNAKE_CASE | `MAX_SPAWN_COUNT` |
| 模块导出宏 | 模块名 + `_API` | `SCENECAPS_API` |

命名应表达用途，避免缩写与无意义的后缀。同一概念在全项目使用同一名称。

---

## 三、文件与目录组织

```text
Source/
├── Scenecaps/                  # Runtime 模块
│   ├── Public/                 # 公开 API（SCENECAPS_API）
│   └── Private/                # 内部实现
└── ScenecapsEditor/            # Editor 模块
    ├── Public/
    └── Private/
```

- 公开 API 放 `Public/`，并标注 `SCENECAPS_API`；
- 内部实现放 `Private/`，不对外暴露；
- 头文件名与其中主要类型对应；
- 模块的 `Build.cs` 必须声明全部依赖模块。

新增模块时，需在 `.uplugin` 中声明，并保持目录名、模块名与 `Build.cs` 文件名一致。

---

## 四、类型与内存

### 1. 指针类型

| 场景 | 类型 |
| --- | --- |
| UObject 成员引用 | `TObjectPtr<T>` |
| 非拥有引用 | `TWeakObjectPtr<T>` |
| 资产 / 延迟加载引用 | `TSoftObjectPtr<T>` / `TSoftClassPtr<T>` |
| 非 UObject 共享所有权 | `TSharedPtr<T>` / `TSharedRef<T>` |
| 独占所有权 | `TUniquePtr<T>` |

禁止使用裸 `UObject*` 作为成员变量，避免垃圾回收无法追踪。

### 2. 对象创建

- UObject 使用 `NewObject<>()` 创建，禁止直接 `new` / `delete`；
- Actor 使用 `World->SpawnActor<>()` 创建；
- 组件使用 `NewObject<UComponent>(Outer, ...)` 创建；
- 结构体使用值语义，避免不必要的指针与堆分配。

### 3. 容器

优先使用引擎容器 `TArray`、`TMap`、`TSet`、`TMultiMap`。避免使用标准库容器，以便与引擎内存与序列化机制保持一致。

---

## 五、反射与宏

对外暴露或需要序列化的类型与成员，应使用引擎反射宏：

```cpp
UCLASS(BlueprintType, Blueprintable)
class SCENECAPS_API UScenecapsAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Scenecaps|Scene")
    void Rebuild();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scenecaps|Scene")
    TArray<FScenecapsEntry> Entries;
};
```

约定如下：

- `Public/` 中的 `UCLASS` 与 `USTRUCT` 必须带模块导出宏；
- 可由蓝图调用的函数使用 `BlueprintCallable`，纯函数使用 `BlueprintPure`；
- 编辑期可见的成员标注 `EditAnywhere` 或 `VisibleAnywhere`，并归入合适的 `Category`；
- 运行时临时数据标注 `Transient`；
- 枚举底层类型明确为 `uint8`。

---

## 六、格式与风格

- 缩进使用 4 个空格，不使用制表符；
- 大括号采用 Allman 风格（另起一行）；
- 行宽建议不超过 120 字符；
- 逻辑块之间空一行；
- `#if` 预处理块单独成段，不与逻辑代码混排。

```cpp
void AScenecapsActor::Rebuild()
{
#if WITH_EDITOR
    Modify();
#endif

    Clear();

    if (ShouldDeferRebuild())
    {
        return;
    }

    UScenecapsAsset* Asset = CollectionAsset.LoadSynchronous();
    if (!Asset)
    {
        return;
    }
}
```

---

## 七、编辑器代码隔离

仅在编辑器可用的代码必须与运行时隔离：

- 使用 `WITH_EDITOR` 包裹的代码不得进入 Shipping 构建；
- 仅编辑器使用的成员变量使用 `WITH_EDITORONLY_DATA`；
- 编辑器专用 API（如对象标签、Data Layer 编辑接口）必须置于守卫之内；
- Editor 模块不得被 Runtime 模块引用。

这是保证 Game 目标可以独立编译与运行的前提，持续集成会对此进行检查。

---

## 八、注释

注释用于说明「为什么」，而非「是什么」。约定如下：

- 公开 API 应具备文档注释，说明用途、参数与返回值；
- 复杂逻辑或不直观的边界处理应添加行内说明；
- 不出现与实际行为不一致的注释；
- 不保留占位的 `TODO` / `FIXME` 而无具体内容；
- 不为自解释的代码逐行添加说明。

---

## 九、禁止事项

| 禁止项 | 替代方案 |
| --- | --- |
| `new` / `delete` UObject | `NewObject<>()` / `SpawnActor<>()` |
| 裸 `UObject*` 成员 | `TObjectPtr<>` / `TWeakObjectPtr<>` |
| 标准库容器 | `TArray` / `TMap` / `TString` |
| 直接访问私有成员 | Getter / Setter |
| 游戏线程阻塞操作 | 异步任务或延迟动作 |
| 硬编码资产路径 | `TSoftObjectPtr<>` / `TSoftClassPtr<>` |
| 魔法数字 | `constexpr` 常量或配置属性 |
| 未守卫的编辑器 API | `WITH_EDITOR` 守卫 |

---

## 十、评审清单

提交代码前，请自行确认：

- [ ] 无新增编译警告（Development 与 Shipping）；
- [ ] 命名与格式符合本规范；
- [ ] 无内存泄漏或 GC 追踪问题；
- [ ] 编辑器代码已用 `WITH_EDITOR` / `WITH_EDITORONLY_DATA` 守卫；
- [ ] 公开类型带 `SCENECAPS_API`，公开成员带必要反射标记；
- [ ] 无 UObject 裸 `new` / `delete`；
- [ ] 游戏线程无阻塞操作；
- [ ] 公开 API 具备文档注释；
- [ ] `Build.cs` 依赖声明完整。

# 编码规范

本文档定义 Scenecaps 项目的 C++ 编码规范，基于 Unreal Engine 官方编码标准。

---

## 通用原则

- 遵循 [Unreal Engine 编码规范](https://docs.unrealengine.com/5.8/en-US/epic-cplusplus-coding-standard-for-unreal-engine/)
- 代码需通过 UE 静态分析（Clang-Tidy / UE Style Checker）
- 无警告编译通过（Development + Shipping 配置）

---

## 命名约定

| 元素 | 约定 | 示例 |
|------|------|------|
| 类/结构体/枚举/接口 | PascalCase | `UScenecapsAsset`, `EScenecapsModifierPhase` |
| 函数/方法 | PascalCase | `GetSpawnedActors()`, `ApplyModifiers()` |
| 变量/参数/成员 | camelCase | `spawnedActors`, `entryId` |
| 私有/受保护成员 | 下划线前缀 `_cachedActor` | `_cachedDataLayerAsset` |
| 常量/宏 | UPPER_SNAKE_CASE | `MAX_SPAWN_COUNT`, `SCENECAPS_API` |
| 布尔函数/变量 | `b` 前缀或 `Is/Has/Can/Should` | `bIsPlaying`, `IsValid()`, `CanExecute()` |
| 模板参数 | PascalCase | `TArray<UScenecapsEntry*>` |
| 模块/插件前缀 | `FScenecaps` / `UScenecaps` | `FScenecapsEntryHandler` |

---

## 文件组织

```
Source/
├── Scenecaps/                 # Runtime 模块
│   ├── Public/                # 公开 API (SCENECAPS_API)
│   │   ├── ScenecapsAsset.h
│   │   └── ScenecapsTypes.h
│   └── Private/               # 内部实现
│       ├── ScenecapsAsset.cpp
│       └── ScenecapsActor.cpp
├── ScenecapsEditor/           # Editor 模块
│   ├── Public/
│   └── Private/
└── ThirdParty/                # 第三方依赖（如有）
```

- 公开 API 头文件放在 `Public/`，标记 `SCENECAPS_API`
- 私有实现放在 `Private/`
- 模块 `Build.cs` 必须声明所有依赖

---

## C++ 语言特性

### 智能指针使用

| 指针类型 | 适用场景 |
|----------|----------|
| `TObjectPtr<T>` | UObject 成员引用（GC 追踪） |
| `TWeakObjectPtr<T>` | 非拥有引用，避免循环引用 |
| `TSoftObjectPtr<T>` / `TSoftClassPtr<T>` | 延迟加载、资产引用 |
| `TSharedPtr<T>` / `TSharedRef<T>` | 非 UObject 共享所有权 |
| `TUniquePtr<T>` | 独占所有权，非 UObject |

**禁止**：裸指针 `UObject*` 作为成员变量（GC 无法追踪）

---

### 容器选择

| 容器 | 适用场景 |
|------|----------|
| `TArray<T>` | 有序序列，随机访问，频繁增删尾部 |
| `TMap<Key, Value>` | 键值查找，键唯一 |
| `TSet<T>` | 唯一元素集合，成员测试 |
| `TMultiMap<Key, Value>` | 一键多值 |
| `TArray<T>` + `Algo::Sort` | 需要排序的数组 |

**避免**：`std::vector`, `std::map`, `std::unordered_map` 等 STL 容器（GC 不兼容）

---

### 内存管理

- **禁止** 直接 `new`/`delete` UObject
- 使用 `NewObject<>()` 创建 UObject
- 组件用 `NewObject<UPointLightComponent>(Actor, ...)`
- Actor 用 `World->SpawnActor<AActor>(...)`
- 结构体用值语义，避免不必要的指针

---

## Unreal Engine 特有规范

### 宏与反射

```cpp
// 类
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "Scenecaps Asset"))
class SCENECAPS_API UScenecapsAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    // 公开 API
    UFUNCTION(BlueprintCallable, Category = "Scenecaps|Scene")
    void Rebuild();
};

// 结构体
USTRUCT(BlueprintType)
struct SCENECAPS_API FScenecapsEntry
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, Category = "Transform")
    FTransform RelativeTransform;
};

// 枚举
UENUM(BlueprintType)
enum class EScenecapsModifierPhase : uint8
{
    PreSpawn,
    PostSpawn,
    PerEntry,
};
```

### 属性规范

```cpp
// 基础属性
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene Collection")
TSoftObjectPtr<UScenecapsAsset> CollectionAsset;

// 编辑条件
UPROPERTY(EditAnywhere, Category = "Material", meta = (EditCondition = "bOverrideMaterial"))
TSoftObjectPtr<UMaterialInterface> MaterialOverride;

// 数组
UPROPERTY(EditAnywhere, Category = "Variants")
TArray<FScenecapsVariant> Variants;

// 只读/运行时
UPROPERTY(VisibleAnywhere, Category = "Runtime", Transient)
TArray<TObjectPtr<AActor>> SpawnedActors;

// 软引用
UPROPERTY(EditAnywhere, Category = "World Partition")
TSoftObjectPtr<UDataLayerAsset> TargetDataLayer;
```

### 函数规范

```cpp
// Blueprint 可调用
UFUNCTION(BlueprintCallable, Category = "Scenecaps|Scene")
void SetActiveVariant(FName InVariant, bool bRebuildImmediately = true);

// 纯函数（无副作用）
UFUNCTION(BlueprintPure, Category = "Scenecaps|Scene")
TArray<AActor*> GetSpawnedActorArray() const;

// 事件/委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVariantChanged, FName, NewVariant);
UPROPERTY(BlueprintAssignable, Category = "Scenecaps|Events")
FOnVariantChanged OnVariantChanged;
```

---

## 代码风格细节

### 缩进与格式

- 缩进：4 空格（无 Tab）
- 大括号：Allman 风格（另起一行）
- 行宽：≤ 120 字符
- 空行：逻辑块之间空一行

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

    // ... 实现逻辑
}
```

### 注释规范

```cpp
/**
 * 重新构建场景集合
 * 会清除现有 Actor 并根据当前变体重新生成
 */
void Rebuild();

/**
 * 获取当前激活的变体名称
 * @return 当前激活的变体名，NAME_None 表示基础状态
 */
UFUNCTION(BlueprintPure, Category = "Scenecaps|Variants")
FName GetActiveVariant() const;
```

- 公开 API 必须有文档注释
- 复杂逻辑添加行内注释
- `// TODO:` 标记待办，`// FIXME:` 标记已知问题

---

## 编译器警告

- 视警告为错误：`TreatWarningsAsErrors=true`
- 关闭不必要的警告：`#pragma warning(push/pop)`
- 无未使用变量/参数（用 `UNUSED_PARAM()` 或 `_` 前缀）

---

## 禁止事项

| 禁止项 | 替代方案 |
|--------|----------|
| `new`/`delete` UObject | `NewObject<>()` / `SpawnActor<>()` |
| 裸指针 `UObject*` 成员 | `TObjectPtr<>`, `TWeakObjectPtr<>` |
| `std::vector/map/string` | `TArray/TMap/TString` |
| 直接访问私有成员 | Getter/Setter |
| 游戏线程阻塞操作 | 异步任务 / Latent Action |
| 硬编码资产路径 | `TSoftObjectPtr<>`, `TSoftClassPtr<>` |
| 魔法数字 | `constexpr` 常量或配置属性 |

---

## 代码审查清单

提交 PR 前自查：

- [ ] 无编译警告（Development + Shipping）
- [ ] 符合命名与格式规范
- [ ] 无内存泄漏 / GC 问题（`obj list` 验证）
- [ ] 编辑器代码用 `WITH_EDITOR` / `WITH_EDITORONLY_DATA`
- [ ] 公开 API 有 `UFUNCTION`/`UPROPERTY` 标记
- [ ] 无 UObject 裸 `new`/`delete`
- [ ] 游戏线程无阻塞操作
- [ ] 公开 API 有文档注释
- [ ] `Build.cs` 依赖声明完整
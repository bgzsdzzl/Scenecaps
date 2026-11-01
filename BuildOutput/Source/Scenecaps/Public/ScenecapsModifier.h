// Plugins/Scenecaps/Source/Scenecaps/Public/ScenecapsModifier.h

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "UObject/Object.h"
#include "ScenecapsTypes.h"
#include "ScenecapsModifier.generated.h"

class UScenecapsAsset;
class UScenecapsEntry;
class AActor;
class UWorld;

UENUM(BlueprintType)
enum class EScenecapsModifierPhase : uint8
{
    PreSpawn,      // Spawn 前修改 Entry 数据
    PostSpawn,     // Actor 生成后修改 Actor
    PreVariant,    // 变体应用前
    PostVariant,   // 变体应用后
    Custom,        // 自定义阶段
};

UENUM(BlueprintType)
enum class EScenecapsModifierScope : uint8
{
    Entry,         // 单个 Entry
    Variant,       // 整个变体
    Asset,         // 整个 Asset
    World,         // 世界级别
};

USTRUCT(BlueprintType)
struct SCENECAPS_API FScenecapsModifierContext
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Modifier")
    EScenecapsModifierPhase Phase = EScenecapsModifierPhase::PreSpawn;

    UPROPERTY(BlueprintReadWrite, Category = "Modifier")
    EScenecapsModifierScope Scope = EScenecapsModifierScope::Entry;

    UPROPERTY(BlueprintReadWrite, Category = "Modifier")
    UScenecapsAsset* Asset = nullptr;

    UPROPERTY(BlueprintReadWrite, Category = "Modifier")
    FGuid EntryId;

    UPROPERTY(BlueprintReadWrite, Category = "Modifier")
    FName VariantName;

    UPROPERTY(BlueprintReadWrite, Category = "Modifier")
    AActor* Actor = nullptr;

    UPROPERTY(BlueprintReadWrite, Category = "Modifier")
    UWorld* World = nullptr;

    UPROPERTY(BlueprintReadWrite, Category = "Modifier")
    TMap<FName, float> FloatParams;

    UPROPERTY(BlueprintReadWrite, Category = "Modifier")
    TMap<FName, FVector> VectorParams;

    UPROPERTY(BlueprintReadWrite, Category = "Modifier")
    TMap<FName, FLinearColor> ColorParams;

    UPROPERTY(BlueprintReadWrite, Category = "Modifier")
    TMap<FName, bool> BoolParams;
};

class SCENECAPS_API IScenecapsModifier
{
public:
    virtual ~IScenecapsModifier() = default;

    virtual FName GetModifierName() const = 0;
    virtual int32 GetPriority() const { return 0; }
    virtual bool CanExecute(const FScenecapsModifierContext& Context) const { return true; }
    virtual void Execute(FScenecapsModifierContext& Context) = 0;
};

UCLASS(BlueprintType, Abstract, meta = (DisplayName = "Scenecaps Modifier Base"))
class SCENECAPS_API UScenecapsModifierBase : public UObject, public IScenecapsModifier
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
    int32 Priority = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
    bool bEnabled = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
    TArray<EScenecapsModifierPhase> SupportedPhases;

    virtual FName GetModifierName() const override { return GetClass()->GetFName(); }
    virtual int32 GetPriority() const override { return Priority; }
    virtual bool CanExecute(const FScenecapsModifierContext& Context) const override
    {
        return bEnabled && (SupportedPhases.Num() == 0 || SupportedPhases.Contains(Context.Phase));
    }
    virtual void Execute(FScenecapsModifierContext& Context) override { ExecuteImpl(Context); }

    virtual void ExecuteImpl(FScenecapsModifierContext& Context) { }
};

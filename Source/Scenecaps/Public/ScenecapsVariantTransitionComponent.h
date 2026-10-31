// Plugins/Scenecaps/Source/Scenecaps/Public/ScenecapsVariantTransitionComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ScenecapsTypes.h"
#include "ScenecapsVariantTransitionComponent.generated.h"

class UScenecapsAsset;
class UScenecapsStaticAsset;
class AActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVariantTransitionStarted, FName, FromVariant);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVariantTransitionEnded, FName, ToVariant);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVariantTransitionProgress, float, Alpha);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnVariantTransitionLooped);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnVariantTransitionPingPong);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent, DisplayName = "Scenecaps Variant Transition"))
class SCENECAPS_API UScenecapsVariantTransitionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UScenecapsVariantTransitionComponent();

    // ---- 目标 Asset ----

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scenecaps|Transition", meta = (AllowedClasses = "/Script/Scenecaps.ScenecapsAsset,/Script/Scenecaps.ScenecapsStaticAsset"))
    TSoftObjectPtr<UObject> TransitionAsset;

    // ---- 过渡设置 ----

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scenecaps|Transition")
    FName FromVariant = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scenecaps|Transition")
    FName ToVariant = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scenecaps|Transition", meta = (ClampMin = "0.01", UIMin = "0.01"))
    float Duration = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scenecaps|Transition")
    EScenecapsInterpolationEase EaseType = EScenecapsInterpolationEase::CubicInOut;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scenecaps|Transition")
    bool bLoop = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scenecaps|Transition")
    bool bPingPong = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scenecaps|Transition")
    bool bAutoPlay = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scenecaps|Transition")
    bool bUpdateISM = true;

    // ---- 运行时状态 (只读) ----

    UPROPERTY(BlueprintReadOnly, Category = "Scenecaps|Transition")
    bool bIsPlaying = false;

    UPROPERTY(BlueprintReadOnly, Category = "Scenecaps|Transition")
    bool bIsReversed = false;

    UPROPERTY(BlueprintReadOnly, Category = "Scenecaps|Transition")
    float CurrentAlpha = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Scenecaps|Transition")
    float ElapsedTime = 0.0f;

    // ---- 事件 ----

    UPROPERTY(BlueprintAssignable, Category = "Scenecaps|Transition")
    FOnVariantTransitionStarted OnTransitionStarted;

    UPROPERTY(BlueprintAssignable, Category = "Scenecaps|Transition")
    FOnVariantTransitionEnded OnTransitionEnded;

    UPROPERTY(BlueprintAssignable, Category = "Scenecaps|Transition")
    FOnVariantTransitionProgress OnTransitionProgress;

    UPROPERTY(BlueprintAssignable, Category = "Scenecaps|Transition")
    FOnVariantTransitionLooped OnTransitionLooped;

    UPROPERTY(BlueprintAssignable, Category = "Scenecaps|Transition")
    FOnVariantTransitionPingPong OnTransitionPingPong;

    // ---- 控制函数 ----

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|Transition")
    void Play();

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|Transition")
    void PlayFromTo(FName InFromVariant, FName InToVariant, float InDuration = -1.0f);

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|Transition")
    void Stop();

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|Transition")
    void Pause();

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|Transition")
    void Resume();

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|Transition")
    void Reverse();

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|Transition")
    void SetTime(float InTime);

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|Transition")
    void SetAlpha(float InAlpha);

    // ---- 查询函数 ----

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Scenecaps|Transition")
    bool IsPlaying() const { return bIsPlaying; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Scenecaps|Transition")
    FName GetCurrentFromVariant() const { return bIsReversed ? ToVariant : FromVariant; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Scenecaps|Transition")
    FName GetCurrentToVariant() const { return bIsReversed ? FromVariant : ToVariant; }

    // ---- ISM 同步 ----

    /** 手动触发一次插值应用到当前 Asset/ISM */
    UFUNCTION(BlueprintCallable, Category = "Scenecaps|Transition")
    void ApplyCurrentInterpolation();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void UpdateTransition(float DeltaTime);
    void OnTransitionComplete();
    void ApplyInterpolationToAsset(float Alpha);
    void ApplyInterpolationToISM(float Alpha);
};


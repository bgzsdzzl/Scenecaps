// Plugins/Scenecaps/Source/Scenecaps/Private/ScenecapsVariantTransitionComponent.cpp

#include "ScenecapsVariantTransitionComponent.h"
#include "ScenecapsAsset.h"
#include "ScenecapsISMComponent.h"
#include "Engine/World.h"

UScenecapsVariantTransitionComponent::UScenecapsVariantTransitionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.0f;
}

void UScenecapsVariantTransitionComponent::BeginPlay()
{
    Super::BeginPlay();

    if (bAutoPlay && !FromVariant.IsNone() && !ToVariant.IsNone())
    {
        Play();
    }
}

void UScenecapsVariantTransitionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bIsPlaying) return;

    UpdateTransition(DeltaTime);
}

void UScenecapsVariantTransitionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}

void UScenecapsVariantTransitionComponent::Play()
{
    if (FromVariant.IsNone() || ToVariant.IsNone()) return;

    bIsPlaying = true;
    bIsReversed = false;
    ElapsedTime = 0.0f;
    CurrentAlpha = 0.0f;

    OnTransitionStarted.Broadcast(FromVariant);
    ApplyCurrentInterpolation();
}

void UScenecapsVariantTransitionComponent::PlayFromTo(FName InFromVariant, FName InToVariant, float InDuration)
{
    FromVariant = InFromVariant;
    ToVariant = InToVariant;
    if (InDuration > 0.0f) Duration = InDuration;

    Play();
}

void UScenecapsVariantTransitionComponent::Stop()
{
    if (!bIsPlaying) return;

    bIsPlaying = false;
    ElapsedTime = 0.0f;
    CurrentAlpha = 0.0f;

    // 可选：停止时重置到 FromVariant
    ApplyCurrentInterpolation();
}

void UScenecapsVariantTransitionComponent::Pause()
{
    bIsPlaying = false;
}

void UScenecapsVariantTransitionComponent::Resume()
{
    if (FromVariant.IsNone() || ToVariant.IsNone()) return;
    bIsPlaying = true;
}

void UScenecapsVariantTransitionComponent::Reverse()
{
    bIsReversed = !bIsReversed;
    // 交换 From/To 的逻辑在 UpdateTransition 中处理
}

void UScenecapsVariantTransitionComponent::SetTime(float InTime)
{
    ElapsedTime = FMath::Clamp(InTime, 0.0f, Duration);
    CurrentAlpha = FMath::Clamp(ElapsedTime / Duration, 0.0f, 1.0f);
    CurrentAlpha = FScenecapsVariantTransition::EaseAlpha(CurrentAlpha, EaseType);
    ApplyCurrentInterpolation();
}

void UScenecapsVariantTransitionComponent::SetAlpha(float InAlpha)
{
    CurrentAlpha = FMath::Clamp(InAlpha, 0.0f, 1.0f);
    ElapsedTime = CurrentAlpha * Duration;
    ApplyCurrentInterpolation();
}

void UScenecapsVariantTransitionComponent::UpdateTransition(float DeltaTime)
{
    if (!bIsPlaying || Duration <= 0.0f) return;

    ElapsedTime += DeltaTime;
    float RawAlpha = FMath::Clamp(ElapsedTime / Duration, 0.0f, 1.0f);
    CurrentAlpha = FScenecapsVariantTransition::EaseAlpha(RawAlpha, EaseType);

    ApplyCurrentInterpolation();
    OnTransitionProgress.Broadcast(CurrentAlpha);

    if (RawAlpha >= 1.0f)
    {
        OnTransitionComplete();
    }
}

void UScenecapsVariantTransitionComponent::OnTransitionComplete()
{
    if (bPingPong)
    {
        // 反向播放
        bIsReversed = !bIsReversed;
        ElapsedTime = 0.0f;
        CurrentAlpha = 0.0f;
        OnTransitionPingPong.Broadcast();

        FName ActualFrom = bIsReversed ? ToVariant : FromVariant;
        FName ActualTo = bIsReversed ? FromVariant : ToVariant;
        OnTransitionStarted.Broadcast(ActualFrom);
    }
    else if (bLoop)
    {
        // 循环播放
        ElapsedTime = 0.0f;
        CurrentAlpha = 0.0f;
        OnTransitionLooped.Broadcast();
        OnTransitionStarted.Broadcast(FromVariant);
    }
    else
    {
        // 结束
        bIsPlaying = false;
        CurrentAlpha = 1.0f;
        OnTransitionEnded.Broadcast(ToVariant);
    }
}

void UScenecapsVariantTransitionComponent::ApplyCurrentInterpolation()
{
    if (!TransitionAsset.IsValid())
    {
        // 尝试加载
        if (TransitionAsset.IsPending())
        {
            TransitionAsset.LoadSynchronous();
        }
        if (!TransitionAsset.IsValid()) return;
    }

    float Alpha = bIsReversed ? (1.0f - CurrentAlpha) : CurrentAlpha;

    if (UScenecapsAsset* Asset = Cast<UScenecapsAsset>(TransitionAsset.Get()))
    {
        ApplyInterpolationToAsset(Alpha);
        if (bUpdateISM) ApplyInterpolationToISM(Alpha);
    }
    
}

void UScenecapsVariantTransitionComponent::ApplyInterpolationToAsset(float Alpha)
{
    // 这里可以实现对 Actor 级别的插值应用
    // 例如：根据插值结果更新场景中的 Actor 变换、可见性、材质参数等
    // 具体实现取决于使用场景
}

void UScenecapsVariantTransitionComponent::ApplyInterpolationToISM(float Alpha)
{
    AActor* Owner = GetOwner();
    if (!Owner) return;

    UScenecapsISMComponent* Manager = Owner->FindComponentByClass<UScenecapsISMComponent>();
    if (!Manager) return;

    // 如果 Asset 是 ScenecapsAsset/StaticAsset，可以获取插值后的条目并更新 ISM
    if (UScenecapsAsset* Asset = Cast<UScenecapsAsset>(TransitionAsset.Get()))
    {
        TArray<FScenecapsInterpolatedEntry> Entries = Asset->InterpolateAllEntries(
            bIsReversed ? ToVariant : FromVariant,
            bIsReversed ? FromVariant : ToVariant,
            Alpha
        );

        // 这里可以实现根据插值结果更新 ISM 实例
        // 例如：更新实例变换、可见性、CustomData 等
    }
    
}



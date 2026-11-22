// Plugins/Scenecaps/Source/Scenecaps/Private/ScenecapsVariantTransitionComponent.cpp

#include "ScenecapsVariantTransitionComponent.h"
#include "ScenecapsAsset.h"
#include "ScenecapsActor.h"
#include "ScenecapsISMComponent.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Materials/MaterialInterface.h"

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
    bIsReversed = false;  // 重置反向标志，确保应用到 FromVariant
    ElapsedTime = 0.0f;
    CurrentAlpha = 0.0f;

    // 停止时重置到 FromVariant
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

    // From/To 交换已经在下方 ApplyInterpolationTo* 中按 bIsReversed 处理，
    // 这里再反转 Alpha 会造成反向播放时插值位置错误（双重反转）
    const float Alpha = CurrentAlpha;

    if (UScenecapsAsset* Asset = Cast<UScenecapsAsset>(TransitionAsset.Get()))
    {
        ApplyInterpolationToAsset(Alpha);
        if (bUpdateISM) ApplyInterpolationToISM(Alpha);
    }
    
}

void UScenecapsVariantTransitionComponent::ApplyInterpolationToAsset(float Alpha)
{
    AActor* Owner = GetOwner();
    AScenecapsActor* SceneActor = Cast<AScenecapsActor>(Owner);
    if (!SceneActor) return;

    UScenecapsAsset* Asset = Cast<UScenecapsAsset>(TransitionAsset.Get());
    if (!Asset) return;

    // 获取插值后的所有条目
    TArray<FScenecapsInterpolatedEntry> InterpolatedEntries = Asset->InterpolateAllEntries(
        bIsReversed ? ToVariant : FromVariant,
        bIsReversed ? FromVariant : ToVariant,
        Alpha
    );

    // 建立 EntryId -> InterpolatedEntry 映射
    TMap<FGuid, FScenecapsInterpolatedEntry> EntryMap;
    for (const FScenecapsInterpolatedEntry& Entry : InterpolatedEntries)
    {
        EntryMap.Add(Entry.EntryId, Entry);
    }

    // 按 EntryId 精确匹配已生成的 Actor。旧实现让每个 Actor 应用全部条目，
    // 结果是所有 Actor 最终都变成最后一个条目的变换与外观。
    for (const FScenecapsSpawnedEntry& Spawned : SceneActor->GetSpawnedEntries())
    {
        AActor* Actor = Spawned.Actor.Get();
        if (!IsValid(Actor)) continue;

        const FScenecapsInterpolatedEntry* InterpEntry = EntryMap.Find(Spawned.EntryId);
        if (!InterpEntry) continue;

        if (InterpEntry->RelativeTransform.IsValid())
        {
            // 由生成时的世界变换反推父级世界，再叠加插值后的相对变换，兼容嵌套条目
            const FTransform ParentWorld =
                Spawned.SpawnTransform * Spawned.BaseRelativeTransform.Inverse();
            Actor->SetActorTransform(
                InterpEntry->RelativeTransform * ParentWorld,
                false, nullptr, ETeleportType::TeleportPhysics);
        }

        TArray<UPrimitiveComponent*> Prims;
        Actor->GetComponents(Prims);

        for (UPrimitiveComponent* Prim : Prims)
        {
            if (!Prim) continue;

            Prim->SetVisibility(InterpEntry->bVisible, true);

            if (InterpEntry->MaterialOverride.IsValid())
            {
                UMaterialInterface* Mat = InterpEntry->MaterialOverride.LoadSynchronous();
                if (Mat)
                {
                    if (UMeshComponent* MeshComp = Cast<UMeshComponent>(Prim))
                    {
                        const int32 SlotIdx = InterpEntry->MaterialSlotIndex;
                        if (SlotIdx < 0)
                        {
                            for (int32 i = 0; i < MeshComp->GetNumMaterials(); ++i)
                            {
                                MeshComp->SetMaterial(i, Mat);
                            }
                        }
                        else if (MeshComp->GetNumMaterials() > SlotIdx)
                        {
                            MeshComp->SetMaterial(SlotIdx, Mat);
                        }
                    }
                }
            }

            if (UMeshComponent* MeshComp = Cast<UMeshComponent>(Prim))
            {
                // CustomPrimitiveData 索引在所有参数类型之间共享，必须连续分配
                int32 DataIndex = 0;
                TArray<FName> Keys;
                InterpEntry->FloatParams.GenerateKeyArray(Keys);
                Keys.Sort([](const FName& A, const FName& B) { return A.FastLess(B); });
                for (const FName& Key : Keys)
                {
                    MeshComp->SetCustomPrimitiveDataFloat(DataIndex++, InterpEntry->FloatParams.FindRef(Key));
                }

                Keys.Reset();
                InterpEntry->VectorParams.GenerateKeyArray(Keys);
                Keys.Sort([](const FName& A, const FName& B) { return A.FastLess(B); });
                for (const FName& Key : Keys)
                {
                    const FVector V = InterpEntry->VectorParams.FindRef(Key);
                    MeshComp->SetCustomPrimitiveDataVector4(DataIndex++, FVector4(V.X, V.Y, V.Z, 0.0f));
                }

                Keys.Reset();
                InterpEntry->ColorParams.GenerateKeyArray(Keys);
                Keys.Sort([](const FName& A, const FName& B) { return A.FastLess(B); });
                for (const FName& Key : Keys)
                {
                    const FLinearColor C = InterpEntry->ColorParams.FindRef(Key);
                    MeshComp->SetCustomPrimitiveDataVector4(DataIndex++, FVector4(C.R, C.G, C.B, C.A));
                }

                Keys.Reset();
                InterpEntry->BoolParams.GenerateKeyArray(Keys);
                Keys.Sort([](const FName& A, const FName& B) { return A.FastLess(B); });
                for (const FName& Key : Keys)
                {
                    MeshComp->SetCustomPrimitiveDataFloat(DataIndex++, InterpEntry->BoolParams.FindRef(Key) ? 1.0f : 0.0f);
                }
            }
        }
    }
}

void UScenecapsVariantTransitionComponent::ApplyInterpolationToISM(float Alpha)
{
    AActor* Owner = GetOwner();
    if (!Owner) return;

    AScenecapsActor* SceneActor = Cast<AScenecapsActor>(Owner);

    // 优先使用场景 Actor 记录的批量容器；兼容过渡组件与 ISM 管理组件同处一个 Actor 的用法
    AActor* Host = SceneActor ? SceneActor->GetISMContainerActor() : nullptr;
    UScenecapsISMComponent* Manager = Host ? Host->FindComponentByClass<UScenecapsISMComponent>() : nullptr;
    if (!Manager)
    {
        Manager = Owner->FindComponentByClass<UScenecapsISMComponent>();
    }
    if (!Manager) return;

    UScenecapsAsset* Asset = Cast<UScenecapsAsset>(TransitionAsset.Get());
    if (!Asset) return;

    const TArray<FScenecapsISMEntryBinding>& Bindings = Manager->GetEntryBindings();
    if (Bindings.Num() == 0) return;

    // EntryId -> 插值结果
    TArray<FScenecapsInterpolatedEntry> InterpolatedEntries = Asset->InterpolateAllEntries(
        bIsReversed ? ToVariant : FromVariant,
        bIsReversed ? FromVariant : ToVariant,
        Alpha);
    TMap<FGuid, FScenecapsInterpolatedEntry> EntryMap;
    EntryMap.Reserve(InterpolatedEntries.Num());
    for (const FScenecapsInterpolatedEntry& Entry : InterpolatedEntries)
    {
        EntryMap.Add(Entry.EntryId, Entry);
    }

    // 写入单个实例的插值参数；槽位布局与生成阶段的 EntryHandlers 保持一致
    auto WriteInterpolatedCustomData = [](UInstancedStaticMeshComponent* ISM, int32 InstanceIndex,
        const FScenecapsISMEntryBinding& Binding, const FScenecapsInterpolatedEntry& Interp)
    {
        const int32 NumFloats = ISM->NumCustomDataFloats;

        // 先恢复生成时的基线数据，再用插值参数覆盖对应槽位
        for (int32 c = 0; c < Binding.BaseCustomData.Num() && c < NumFloats; ++c)
        {
            ISM->SetCustomDataValue(InstanceIndex, c, Binding.BaseCustomData[c], true);
        }

        int32 ParamIndex = 0;
        TArray<FName> Keys;

        Interp.FloatParams.GenerateKeyArray(Keys);
        Keys.Sort([](const FName& A, const FName& B) { return A.FastLess(B); });
        for (const FName& Key : Keys)
        {
            const int32 FloatIndex = ParamIndex++;
            if (FloatIndex < NumFloats)
            {
                ISM->SetCustomDataValue(InstanceIndex, FloatIndex, Interp.FloatParams.FindRef(Key), true);
            }
        }

        Keys.Reset();
        Interp.VectorParams.GenerateKeyArray(Keys);
        Keys.Sort([](const FName& A, const FName& B) { return A.FastLess(B); });
        for (const FName& Key : Keys)
        {
            const int32 VectorIndex = ParamIndex++;
            const FVector V = Interp.VectorParams.FindRef(Key);
            const float Components[4] = { V.X, V.Y, V.Z, 0.0f };
            for (int32 c = 0; c < 4; ++c)
            {
                const int32 FloatIndex = VectorIndex * 4 + c;
                if (FloatIndex < NumFloats)
                {
                    ISM->SetCustomDataValue(InstanceIndex, FloatIndex, Components[c], true);
                }
            }
        }

        Keys.Reset();
        Interp.ColorParams.GenerateKeyArray(Keys);
        Keys.Sort([](const FName& A, const FName& B) { return A.FastLess(B); });
        for (const FName& Key : Keys)
        {
            const int32 ColorIndex = ParamIndex++;
            const FLinearColor C = Interp.ColorParams.FindRef(Key);
            const float Components[4] = { C.R, C.G, C.B, C.A };
            for (int32 c = 0; c < 4; ++c)
            {
                const int32 FloatIndex = ColorIndex * 4 + c;
                if (FloatIndex < NumFloats)
                {
                    ISM->SetCustomDataValue(InstanceIndex, FloatIndex, Components[c], true);
                }
            }
        }

        Keys.Reset();
        Interp.BoolParams.GenerateKeyArray(Keys);
        Keys.Sort([](const FName& A, const FName& B) { return A.FastLess(B); });
        for (const FName& Key : Keys)
        {
            const int32 BoolIndex = ParamIndex++;
            if (BoolIndex < NumFloats)
            {
                ISM->SetCustomDataValue(InstanceIndex, BoolIndex, Interp.BoolParams.FindRef(Key) ? 1.0f : 0.0f, true);
            }
        }
    };

    TSet<UHierarchicalInstancedStaticMeshComponent*> HISMsToRebuild;

    for (const FScenecapsISMEntryBinding& Binding : Bindings)
    {
        UInstancedStaticMeshComponent* ISM = Binding.ISM.Get();
        if (!IsValid(ISM)) continue;
        if (Binding.InstanceIndex < 0 || Binding.InstanceIndex >= ISM->GetInstanceCount()) continue;

        const FScenecapsInterpolatedEntry* Interp = EntryMap.Find(Binding.EntryId);
        if (!Interp) continue;

        // 由生成时快照反推父级世界，兼容嵌套层级与程序化多实例
        const FTransform ParentWorld = Binding.SpawnTransform * Binding.BaseRelativeTransform.Inverse();
        FTransform NewWorld = Interp->RelativeTransform * ParentWorld;
        // 隐藏用零缩放，保留位置/旋转，避免实例跳到原点
        NewWorld.SetScale3D(Interp->bVisible ? NewWorld.GetScale3D() : FVector::ZeroVector);
        ISM->UpdateInstanceTransform(Binding.InstanceIndex, NewWorld, true, true);

        WriteInterpolatedCustomData(ISM, Binding.InstanceIndex, Binding, *Interp);

        if (UHierarchicalInstancedStaticMeshComponent* HISM = Cast<UHierarchicalInstancedStaticMeshComponent>(ISM))
        {
            HISMsToRebuild.Add(HISM);
        }
    }

    for (UHierarchicalInstancedStaticMeshComponent* HISM : HISMsToRebuild)
    {
        if (HISM)
        {
            HISM->BuildTreeIfOutdated(true, false);
        }
    }
}



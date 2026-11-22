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

    UScenecapsISMComponent* Manager = Owner->FindComponentByClass<UScenecapsISMComponent>();
    if (!Manager) return;

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

    // 获取所有 ISM 和 HISM
    TArray<UInstancedStaticMeshComponent*> ISMs = Manager->GetRegisteredISMs();
    TArray<UHierarchicalInstancedStaticMeshComponent*> HISMs = Manager->GetRegisteredHISMs();

    // 为每个 ISM/HISM 更新实例
    // 注意：这里需要一种方式将 Entry 映射到具体的 ISM 实例索引
    // 简化实现：假设 ISM 按 Entry 顺序创建，或者通过 Mesh 匹配
    
    // 更新 ISM 实例
    for (UInstancedStaticMeshComponent* ISM : ISMs)
    {
        if (!ISM || !ISM->GetStaticMesh()) continue;

        UStaticMesh* Mesh = ISM->GetStaticMesh();
        for (const FScenecapsEntry& BaseEntry : Asset->Entries)
        {
            if (BaseEntry.Asset.IsPending()) continue;
            UObject* EntryAsset = BaseEntry.Asset.LoadSynchronous();
            if (!EntryAsset || Cast<UStaticMesh>(EntryAsset) != Mesh) continue;

            const FScenecapsInterpolatedEntry* InterpEntry = EntryMap.Find(BaseEntry.EntryId);
            if (!InterpEntry || ISM->GetInstanceCount() <= 0) continue;

            // 旧实现用 FTransform(NewScale) 做可见性，会把实例位置/旋转重置到原点
            FTransform NewTransform = InterpEntry->RelativeTransform;
            NewTransform.SetScale3D(InterpEntry->bVisible ? NewTransform.GetScale3D() : FVector::ZeroVector);
            ISM->UpdateInstanceTransform(0, NewTransform, true, true);

            TArray<FName> FloatKeys;
            InterpEntry->FloatParams.GenerateKeyArray(FloatKeys);
            FloatKeys.Sort([](const FName& A, const FName& B) { return A.FastLess(B); });
            for (int32 i = 0; i < FloatKeys.Num() && i < ISM->NumCustomDataFloats; ++i)
            {
                ISM->SetCustomDataValue(0, i, InterpEntry->FloatParams.FindRef(FloatKeys[i]), true);
            }
            break; // 一个 ISM 只对应一个条目，避免多个条目反复覆盖实例 0
        }
    }

    // 更新 HISM 实例
    for (UHierarchicalInstancedStaticMeshComponent* HISM : HISMs)
    {
        if (!HISM || !HISM->GetStaticMesh()) continue;

        UStaticMesh* Mesh = HISM->GetStaticMesh();
        for (const FScenecapsEntry& BaseEntry : Asset->Entries)
        {
            if (BaseEntry.Asset.IsPending()) continue;
            UObject* EntryAsset = BaseEntry.Asset.LoadSynchronous();
            if (!EntryAsset || Cast<UStaticMesh>(EntryAsset) != Mesh) continue;

            const FScenecapsInterpolatedEntry* InterpEntry = EntryMap.Find(BaseEntry.EntryId);
            if (!InterpEntry || HISM->GetInstanceCount() <= 0) continue;

            FTransform NewTransform = InterpEntry->RelativeTransform;
            NewTransform.SetScale3D(InterpEntry->bVisible ? NewTransform.GetScale3D() : FVector::ZeroVector);
            HISM->UpdateInstanceTransform(0, NewTransform, true, true);

            TArray<FName> FloatKeys;
            InterpEntry->FloatParams.GenerateKeyArray(FloatKeys);
            FloatKeys.Sort([](const FName& A, const FName& B) { return A.FastLess(B); });
            for (int32 i = 0; i < FloatKeys.Num() && i < HISM->NumCustomDataFloats; ++i)
            {
                HISM->SetCustomDataValue(0, i, InterpEntry->FloatParams.FindRef(FloatKeys[i]), true);
            }
            break;
        }
    }
    
    // 如果有 HISM，重建树以更新剔除
    for (UHierarchicalInstancedStaticMeshComponent* HISM : HISMs)
    {
        if (HISM)
        {
            HISM->BuildTreeIfOutdated(true, false);
        }
    }
}



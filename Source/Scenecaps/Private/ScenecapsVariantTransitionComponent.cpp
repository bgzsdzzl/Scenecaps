// Plugins/Scenecaps/Source/Scenecaps/Private/ScenecapsVariantTransitionComponent.cpp

#include "ScenecapsVariantTransitionComponent.h"
#include "ScenecapsAsset.h"
#include "ScenecapsActor.h"
#include "ScenecapsISMComponent.h"
#include "Engine/World.h"
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

    float Alpha = bIsReversed ? (1.0f - CurrentAlpha) : CurrentAlpha;

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

    // 遍历生成的 Actor 并应用插值
    for (const TObjectPtr<AActor>& ActorPtr : SceneActor->SpawnedActors)
    {
        AActor* Actor = ActorPtr.Get();
        if (!Actor || !IsValid(Actor)) continue;

        // 尝试从 Actor 找到对应的 EntryId
        // 这里需要一种映射方式：可以通过 Actor 的 tag 或名字匹配
        // 简单做法：遍历所有插值条目，通过 Asset 引用匹配
        for (const auto& Pair : EntryMap)
        {
            const FScenecapsInterpolatedEntry& InterpEntry = Pair.Value;
            
            // 检查 Actor 是否对应这个 Entry（通过 Asset 匹配）
            // 注意：这是一个简化实现，实际可能需要更精确的映射
            TArray<UPrimitiveComponent*> Prims;
            Actor->GetComponents(Prims);
            
            for (UPrimitiveComponent* Prim : Prims)
            {
                if (!Prim) continue;

                // 应用变换（相对变换需要转为世界变换）
                // 这里假设 Actor 已经在正确的父级下，直接设置相对变换
                if (InterpEntry.RelativeTransform.IsValid())
                {
                    Prim->SetRelativeTransform(InterpEntry.RelativeTransform, false, nullptr, ETeleportType::TeleportPhysics);
                }

                // 应用可见性
                Prim->SetVisibility(InterpEntry.bVisible, true);

                // 应用材质覆盖
                if (InterpEntry.MaterialOverride.IsValid())
                {
                    UMaterialInterface* Mat = InterpEntry.MaterialOverride.LoadSynchronous();
                    if (Mat)
                    {
                        if (UMeshComponent* MeshComp = Cast<UMeshComponent>(Prim))
                        {
                            int32 SlotIdx = InterpEntry.MaterialSlotIndex;
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

                // 应用自定义参数到 CustomPrimitiveData
                if (UMeshComponent* MeshComp = Cast<UMeshComponent>(Prim))
                {
                    // FloatParams
                    TArray<FName> FloatKeys;
                    InterpEntry.FloatParams.GenerateKeyArray(FloatKeys);
                    FloatKeys.Sort([](const FName& A, const FName& B) { return A.FastLess(B); });
                    for (int32 i = 0; i < FloatKeys.Num(); ++i)
                    {
                        MeshComp->SetCustomPrimitiveDataFloat(i, InterpEntry.FloatParams[FloatKeys[i]]);
                    }

                    // VectorParams
                    TArray<FName> VectorKeys;
                    InterpEntry.VectorParams.GenerateKeyArray(VectorKeys);
                    VectorKeys.Sort([](const FName& A, const FName& B) { return A.FastLess(B); });
                    for (int32 i = 0; i < VectorKeys.Num(); ++i)
                    {
                        const FVector& V = InterpEntry.VectorParams[VectorKeys[i]];
                        MeshComp->SetCustomPrimitiveDataVector4(i, FVector4(V.X, V.Y, V.Z, 0.0f));
                    }

                    // ColorParams
                    TArray<FName> ColorKeys;
                    InterpEntry.ColorParams.GenerateKeyArray(ColorKeys);
                    ColorKeys.Sort([](const FName& A, const FName& B) { return A.FastLess(B); });
                    for (int32 i = 0; i < ColorKeys.Num(); ++i)
                    {
                        const FLinearColor& C = InterpEntry.ColorParams[ColorKeys[i]];
                        MeshComp->SetCustomPrimitiveDataVector4(i, FVector4(C.R, C.G, C.B, C.A));
                    }

                    // BoolParams
                    TArray<FName> BoolKeys;
                    InterpEntry.BoolParams.GenerateKeyArray(BoolKeys);
                    BoolKeys.Sort([](const FName& A, const FName& B) { return A.FastLess(B); });
                    for (int32 i = 0; i < BoolKeys.Num(); ++i)
                    {
                        MeshComp->SetCustomPrimitiveDataFloat(i, InterpEntry.BoolParams[BoolKeys[i]] ? 1.0f : 0.0f);
                    }
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
        
        int32 InstanceCount = ISM->GetInstanceCount();
        UStaticMesh* Mesh = ISM->GetStaticMesh();
        
        // 查找对应这个 Mesh 的插值条目
        // 这里需要遍历所有 Entry 找到使用这个 Mesh 的
        for (const FScenecapsEntry& BaseEntry : Asset->Entries)
        {
            if (BaseEntry.Asset.IsPending()) continue;
            UObject* EntryAsset = BaseEntry.Asset.LoadSynchronous();
            if (EntryAsset != Mesh) continue;
            
            const FScenecapsInterpolatedEntry* InterpEntry = EntryMap.Find(BaseEntry.EntryId);
            if (!InterpEntry) continue;

            // 对于 ISM，通常每个 Entry 对应一个实例（如果是单实例）
            // 或者多个实例（如果是程序化生成）
            // 这里简化：假设每个 Entry 对应一个实例，按顺序更新
            // 实际场景可能需要更复杂的映射逻辑
            
            // 尝试更新第一个实例作为演示
            if (InstanceCount > 0)
            {
                FTransform InstanceTransform;
                if (ISM->GetInstanceTransform(0, InstanceTransform, true))
                {
                    // 应用相对变换（这里简化处理）
                    FTransform NewTransform = InterpEntry->RelativeTransform;
                    ISM->UpdateInstanceTransform(0, NewTransform, true, true);
                }
                
                // 应用可见性（通过 scale 隐藏/显示）
                FVector NewScale = InterpEntry->bVisible ? FVector(1.0f) : FVector(0.0f);
                ISM->UpdateInstanceTransform(0, FTransform(NewScale), true, false);
                
                // 应用自定义参数
                TArray<FName> FloatKeys;
                InterpEntry->FloatParams.GenerateKeyArray(FloatKeys);
                FloatKeys.Sort([](const FName& A, const FName& B) { return A.FastLess(B); });
                for (int32 i = 0; i < FloatKeys.Num(); ++i)
                {
                    ISM->SetCustomDataValue(0, i, InterpEntry->FloatParams[FloatKeys[i]], true);
                }
            }
        }
    }

    // 更新 HISM 实例
    for (UHierarchicalInstancedStaticMeshComponent* HISM : HISMs)
    {
        if (!HISM || !HISM->GetStaticMesh()) continue;
        
        int32 InstanceCount = HISM->GetInstanceCount();
        UStaticMesh* Mesh = HISM->GetStaticMesh();
        
        for (const FScenecapsEntry& BaseEntry : Asset->Entries)
        {
            if (BaseEntry.Asset.IsPending()) continue;
            UObject* EntryAsset = BaseEntry.Asset.LoadSynchronous();
            if (EntryAsset != Mesh) continue;
            
            const FScenecapsInterpolatedEntry* InterpEntry = EntryMap.Find(BaseEntry.EntryId);
            if (!InterpEntry) continue;

            if (InstanceCount > 0)
            {
                FTransform InstanceTransform;
                if (HISM->GetInstanceTransform(0, InstanceTransform, true))
                {
                    FTransform NewTransform = InterpEntry->RelativeTransform;
                    HISM->UpdateInstanceTransform(0, NewTransform, true, true);
                }
                
                FVector NewScale = InterpEntry->bVisible ? FVector(1.0f) : FVector(0.0f);
                HISM->UpdateInstanceTransform(0, FTransform(NewScale), true, false);
                
                TArray<FName> FloatKeys;
                InterpEntry->FloatParams.GenerateKeyArray(FloatKeys);
                FloatKeys.Sort([](const FName& A, const FName& B) { return A.FastLess(B); });
                for (int32 i = 0; i < FloatKeys.Num(); ++i)
                {
                    HISM->SetCustomDataValue(0, i, InterpEntry->FloatParams[FloatKeys[i]], true);
                }
            }
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



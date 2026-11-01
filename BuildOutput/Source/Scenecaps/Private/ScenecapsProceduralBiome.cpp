// Plugins/Scenecaps/Source/Scenecaps/Private/ScenecapsProceduralBiome.cpp

#include "ScenecapsProceduralBiome.h"
#include "ScenecapsProceduralTypes.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Landscape.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"
#include "ScenecapsAsset.h"
#include "ScenecapsISMComponent.h"



TArray<FScenecapsBiomeEntry> UScenecapsBiomeAsset::QueryEntriesAtLocation(
    const FVector& WorldLocation,
    ALandscape* Landscape,
    UWorld* World) const
{
    TArray<FScenecapsBiomeEntry> Result;

    float Slope = 0.0f;
    float Height = WorldLocation.Z;
    bool bHasSlopeData = false;

    // Calculate slope if we have a World and Landscape
    if (World && Landscape)
    {
        FVector TraceStart = WorldLocation + FVector(0, 0, 1000.0f);
        FVector TraceEnd = WorldLocation - FVector(0, 0, 1000.0f);
        
        FHitResult HitResult;
        FCollisionQueryParams QueryParams;
        QueryParams.AddIgnoredActor(Landscape);
        
        if (World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
        {
            if (HitResult.bBlockingHit && HitResult.Normal.SizeSquared() > 0.0f)
            {
                FVector UpVector = FVector::UpVector;
                float DotProduct = FMath::Clamp(HitResult.Normal | UpVector, -1.0f, 1.0f);
                Slope = FMath::RadiansToDegrees(FMath::Acos(DotProduct));
                bHasSlopeData = true;
            }
        }
    }
    // If no landscape or trace failed, we don't have slope data

    for (const FScenecapsBiomeLayer& Layer : Layers)
    {
        if (!Layer.bEnabled) continue;

        for (const FScenecapsBiomeEntry& Entry : Layer.Entries)
        {
            // Only filter by slope if we have valid slope data
            if (bHasSlopeData)
            {
                if (Slope < Entry.MinSlope || Slope > Entry.MaxSlope) continue;
            }
            else
            {
                // If no slope data, skip slope filter unless entry has explicit slope constraints
                if (Entry.MinSlope > 0.0f || Entry.MaxSlope < 90.0f) continue;
            }
            
            if (Height < Entry.MinHeight || Height > Entry.MaxHeight) continue;

            float MaskWeight = 1.0f;

            float FinalWeight = Entry.Weight * Layer.GlobalWeight * MaskWeight;
            if (FinalWeight > 0.0f)
            {
                FScenecapsBiomeEntry WeightedEntry = Entry;
                WeightedEntry.Weight = FinalWeight;
                Result.Add(WeightedEntry);
            }
        }
    }

    Result.Sort([](const FScenecapsBiomeEntry& A, const FScenecapsBiomeEntry& B)
    {
        return A.Weight > B.Weight;
    });

    return Result;
}

UScenecapsBiomeSamplerComponent::UScenecapsBiomeSamplerComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

TArray<FTransform> UScenecapsBiomeSamplerComponent::PreviewGenerate(
    const FTransform& WorldTransform,
    int32 MaxInstances)
{
    TArray<FTransform> Result;
    
    UScenecapsBiomeAsset* Asset = BiomeAsset.LoadSynchronous();
    if (!Asset) return Result;

    ALandscape* Landscape = TargetLandscape.LoadSynchronous();
    
    FRandomStream Stream(Asset->GlobalProceduralParams.Seed);
    const FBox Bounds = Asset->GlobalProceduralParams.Bounds;
    const FVector Extent = Bounds.GetExtent();
    const FVector Center = Bounds.GetCenter();

    int32 Count = FMath::Min(MaxInstances, 
        FMath::Max(1, FMath::RoundToInt(Extent.Size() * Asset->GlobalProceduralParams.Density)));

    for (int32 i = 0; i < Count; ++i)
    {
        FVector SamplePos = Center + FVector(
            Stream.FRandRange(-Extent.X, Extent.X),
            Stream.FRandRange(-Extent.Y, Extent.Y),
            0
        );

        FVector WorldPos = WorldTransform.TransformPosition(SamplePos);
        TArray<FScenecapsBiomeEntry> Entries = Asset->QueryEntriesAtLocation(WorldPos, Landscape, GetWorld());

        if (Entries.Num() == 0) continue;

        float TotalWeight = 0.0f;
        for (const FScenecapsBiomeEntry& E : Entries) TotalWeight += E.Weight;
        
        float RandVal = Stream.FRandRange(0.0f, TotalWeight);
        float AccumWeight = 0.0f;
        const FScenecapsBiomeEntry* SelectedEntry = nullptr;

        for (const FScenecapsBiomeEntry& E : Entries)
        {
            AccumWeight += E.Weight;
            if (RandVal <= AccumWeight)
            {
                SelectedEntry = &E;
                break;
            }
        }

        if (!SelectedEntry) continue;

        FTransform LocalTransform = WorldTransform;
        LocalTransform.SetLocation(WorldPos);

        FVector Scale(
            Stream.FRandRange(SelectedEntry->ProceduralParams.MinScale.X, SelectedEntry->ProceduralParams.MaxScale.X),
            Stream.FRandRange(SelectedEntry->ProceduralParams.MinScale.Y, SelectedEntry->ProceduralParams.MaxScale.Y),
            Stream.FRandRange(SelectedEntry->ProceduralParams.MinScale.Z, SelectedEntry->ProceduralParams.MaxScale.Z)
        );
        LocalTransform.SetScale3D(LocalTransform.GetScale3D() * Scale);

        FRotator RotOffset(
            Stream.FRandRange(-SelectedEntry->ProceduralParams.MaxRotationOffset.Pitch, SelectedEntry->ProceduralParams.MaxRotationOffset.Pitch),
            Stream.FRandRange(-SelectedEntry->ProceduralParams.MaxRotationOffset.Yaw, SelectedEntry->ProceduralParams.MaxRotationOffset.Yaw),
            Stream.FRandRange(-SelectedEntry->ProceduralParams.MaxRotationOffset.Roll, SelectedEntry->ProceduralParams.MaxRotationOffset.Roll)
        );
        LocalTransform.SetRotation((LocalTransform.GetRotation() * RotOffset.Quaternion()).GetNormalized());

        LocalTransform.AddToTranslation(FVector(
            Stream.FRandRange(-SelectedEntry->ProceduralParams.LocationJitter.X, SelectedEntry->ProceduralParams.LocationJitter.X),
            Stream.FRandRange(-SelectedEntry->ProceduralParams.LocationJitter.Y, SelectedEntry->ProceduralParams.LocationJitter.Y),
            Stream.FRandRange(-SelectedEntry->ProceduralParams.LocationJitter.Z, SelectedEntry->ProceduralParams.LocationJitter.Z)
        ));

        Result.Add(LocalTransform);
    }

    return Result;
}

UInstancedStaticMeshComponent* UScenecapsBiomeSamplerComponent::GenerateToISM(
    UWorld* World,
    const FTransform& WorldTransform,
    AActor* ContainerActor)
{
    if (!World) return nullptr;

    UScenecapsBiomeAsset* Asset = BiomeAsset.LoadSynchronous();
    if (!Asset) return nullptr;

    // 生成预览变换
    TArray<FTransform> Transforms = PreviewGenerate(WorldTransform, 10000);
    if (Transforms.Num() == 0) return nullptr;

    // 按 Mesh 分组
    TMap<UStaticMesh*, TArray<FTransform>> MeshToTransforms;
    for (const FTransform& Transform : Transforms)
    {
        // 这里简化：使用第一个 Layer 的第一个 Entry 的 Mesh
        // 实际应该根据生成逻辑选择 Mesh
        if (Asset->Layers.Num() > 0 && Asset->Layers[0].Entries.Num() > 0)
        {
            UStaticMesh* Mesh = Asset->Layers[0].Entries[0].Mesh.LoadSynchronous();
            if (Mesh)
            {
                MeshToTransforms.FindOrAdd(Mesh).Add(Transform);
            }
        }
    }

    if (MeshToTransforms.Num() == 0) return nullptr;

    // 创建或使用容器 Actor
    AActor* TargetContainer = ContainerActor;
    bool bOwnsContainer = false;
    if (!TargetContainer)
    {
        TargetContainer = World->SpawnActor<AActor>();
        if (!TargetContainer) return nullptr;
#if WITH_EDITOR
        TargetContainer->SetActorLabel(TEXT("BiomeISMContainer"));
#endif
        bOwnsContainer = true;
    }

    // 添加管理组件
    UScenecapsISMComponent* Manager = TargetContainer->FindComponentByClass<UScenecapsISMComponent>();
    if (!Manager)
    {
        Manager = Cast<UScenecapsISMComponent>(TargetContainer->AddComponentByClass(UScenecapsISMComponent::StaticClass(), false, FTransform::Identity, false));
    }

    USceneComponent* Root = TargetContainer->GetRootComponent();
    if (!Root)
    {
        Root = NewObject<USceneComponent>(TargetContainer, TEXT("BiomeISMRoot"));
        TargetContainer->SetRootComponent(Root);
    }

    UInstancedStaticMeshComponent* FirstISM = nullptr;

    // 为每个 Mesh 创建 ISM
    for (auto& Pair : MeshToTransforms)
    {
        UStaticMesh* Mesh = Pair.Key;
        const TArray<FTransform>& MeshTransforms = Pair.Value;

        UInstancedStaticMeshComponent* ISM = NewObject<UInstancedStaticMeshComponent>(TargetContainer);
        ISM->SetStaticMesh(Mesh);
        ISM->RegisterComponentWithWorld(World);
        ISM->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);

        // 添加实例
        for (const FTransform& Transform : MeshTransforms)
        {
            ISM->AddInstance(Transform, true);
        }

        // Lumen 设置
        ISM->SetAffectDistanceFieldLighting(true);
        ISM->SetAffectDynamicIndirectLighting(true);
        ISM->SetCastShadow(true);
        ISM->SetLightingChannels(true, false, false);
        ISM->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        ISM->SetGenerateOverlapEvents(false);
        ISM->SetCanEverAffectNavigation(false);
        ISM->SetNumCustomDataFloats(4);

        if (!FirstISM) FirstISM = ISM;
    }

    if (Manager)
    {
        Manager->RegisterContainerActor(TargetContainer);
    }

    return FirstISM;
}

UHierarchicalInstancedStaticMeshComponent* UScenecapsBiomeSamplerComponent::GenerateToHISM(
    UWorld* World,
    const FTransform& WorldTransform,
    AActor* ContainerActor)
{
    if (!World) return nullptr;

    UScenecapsBiomeAsset* Asset = BiomeAsset.LoadSynchronous();
    if (!Asset) return nullptr;

    // 生成预览变换
    TArray<FTransform> Transforms = PreviewGenerate(WorldTransform, 10000);
    if (Transforms.Num() == 0) return nullptr;

    // 按 Mesh 分组
    TMap<UStaticMesh*, TArray<FTransform>> MeshToTransforms;
    for (const FTransform& Transform : Transforms)
    {
        if (Asset->Layers.Num() > 0 && Asset->Layers[0].Entries.Num() > 0)
        {
            UStaticMesh* Mesh = Asset->Layers[0].Entries[0].Mesh.LoadSynchronous();
            if (Mesh)
            {
                MeshToTransforms.FindOrAdd(Mesh).Add(Transform);
            }
        }
    }

    if (MeshToTransforms.Num() == 0) return nullptr;

    // 创建或使用容器 Actor
    AActor* TargetContainer = ContainerActor;
    bool bOwnsContainer = false;
    if (!TargetContainer)
    {
        TargetContainer = World->SpawnActor<AActor>();
        if (!TargetContainer) return nullptr;
#if WITH_EDITOR
        TargetContainer->SetActorLabel(TEXT("BiomeHISMContainer"));
#endif
        bOwnsContainer = true;
    }

    // 添加管理组件
    UScenecapsISMComponent* Manager = TargetContainer->FindComponentByClass<UScenecapsISMComponent>();
    if (!Manager)
    {
        Manager = Cast<UScenecapsISMComponent>(TargetContainer->AddComponentByClass(UScenecapsISMComponent::StaticClass(), false, FTransform::Identity, false));
    }

    USceneComponent* Root = TargetContainer->GetRootComponent();
    if (!Root)
    {
        Root = NewObject<USceneComponent>(TargetContainer, TEXT("BiomeHISMRoot"));
        TargetContainer->SetRootComponent(Root);
    }

    UHierarchicalInstancedStaticMeshComponent* FirstHISM = nullptr;

    // 为每个 Mesh 创建 HISM
    for (auto& Pair : MeshToTransforms)
    {
        UStaticMesh* Mesh = Pair.Key;
        const TArray<FTransform>& MeshTransforms = Pair.Value;

        UHierarchicalInstancedStaticMeshComponent* HISM = NewObject<UHierarchicalInstancedStaticMeshComponent>(TargetContainer);
        HISM->SetStaticMesh(Mesh);
        HISM->RegisterComponentWithWorld(World);
        HISM->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
        HISM->PerInstanceSMData.Empty();

        for (const FTransform& Transform : MeshTransforms)
        {
            HISM->AddInstance(Transform, true);
        }

        // Lumen 设置
        HISM->SetAffectDistanceFieldLighting(true);
        HISM->SetAffectDynamicIndirectLighting(true);
        HISM->SetCastShadow(true);
        HISM->SetLightingChannels(true, false, false);
        HISM->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        HISM->SetGenerateOverlapEvents(false);
        HISM->SetCanEverAffectNavigation(false);
        HISM->bEnableDensityScaling = true;

        // 强制重建 HISM 层级树
        HISM->BuildTreeIfOutdated(true, false);

        if (!FirstHISM) FirstHISM = HISM;
    }

    if (Manager)
    {
        Manager->RegisterContainerActor(TargetContainer);
    }

    return FirstHISM;
}


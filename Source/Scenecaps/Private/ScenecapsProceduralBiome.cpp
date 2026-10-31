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
    // Not implemented yet - use PreviewGenerate + SpawnCollectionAsISM from UScenecapsAsset
    return nullptr;
}

UHierarchicalInstancedStaticMeshComponent* UScenecapsBiomeSamplerComponent::GenerateToHISM(
    UWorld* World,
    const FTransform& WorldTransform,
    AActor* ContainerActor)
{
    // Not implemented yet - use PreviewGenerate + SpawnCollectionAsHISM from UScenecapsAsset
    return nullptr;
}


// Plugins/Scenecaps/Source/Scenecaps/Private/ScenecapsEntryHandlers.cpp

#include "ScenecapsEntryHandlers.h"

#include "Engine/Blueprint.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/DecalActor.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "Materials/Material.h"
#include "Components/StaticMeshComponent.h"
#include "Components/MeshComponent.h"
#include "Components/DecalComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/AudioComponent.h"
#include "Sound/AmbientSound.h"
#include "Sound/SoundBase.h"
#include "ScenecapsAsset.h"
#include "ScenecapsActor.h"
#include "MaterialDomain.h"

#if SCENE_COLLECTION_WITH_NIAGARA
#include "NiagaraSystem.h"
#include "NiagaraActor.h"
#include "NiagaraComponent.h"
#endif

// ---------- 通用覆盖 ----------

void ApplyEntryOverrides(AActor* Actor, const FScenecapsSpawnContext& Context)
{
    if (!Actor) return;

    TInlineComponentArray<UPrimitiveComponent*> Prims;
    Actor->GetComponents(Prims);

    for (UPrimitiveComponent* Prim : Prims)
    {
        if (!Prim) continue;

        if (Context.bOverrideMobility)
        {
            Prim->SetMobility(Context.Mobility);
        }
        if (Context.bOverrideCollision)
        {
            Prim->SetCollisionEnabled(Context.CollisionEnabled);
        }
        if (Context.bOverrideCastShadow)
        {
            Prim->SetCastShadow(Context.bCastShadow);
        }

        // 材质覆盖：只作用于 UMeshComponent（StaticMesh / SkeletalMesh 等）
        if (Context.MaterialOverride)
        {
            if (UMeshComponent* MeshComp = Cast<UMeshComponent>(Prim))
            {
                if (Context.MaterialSlotIndex < 0)
                {
                    const int32 NumSlots = MeshComp->GetNumMaterials();
                    for (int32 Index = 0; Index < NumSlots; ++Index)
                    {
                        MeshComp->SetMaterial(Index, Context.MaterialOverride);
                    }
                }
                else
                {
                    MeshComp->SetMaterial(Context.MaterialSlotIndex, Context.MaterialOverride);
                }
            }
        }

        // 自定义参数：应用到 UMeshComponent 的 PerInstanceCustomData / CustomPrimitiveData
        if (UMeshComponent* MeshComp = Cast<UMeshComponent>(Prim))
        {
            // FloatParams -> SetCustomPrimitiveDataFloat (稳定排序后的索引)
            TArray<FName> FloatKeys;
            Context.FloatParams.GenerateKeyArray(FloatKeys);
            FloatKeys.Sort([](const FName& A, const FName& B) { return A.FastLess(B); });
            for (int32 i = 0; i < FloatKeys.Num(); ++i)
            {
                MeshComp->SetCustomPrimitiveDataFloat(i, Context.FloatParams[FloatKeys[i]]);
            }

            // VectorParams -> SetCustomPrimitiveDataVector4
            TArray<FName> VectorKeys;
            Context.VectorParams.GenerateKeyArray(VectorKeys);
            VectorKeys.Sort([](const FName& A, const FName& B) { return A.FastLess(B); });
            for (int32 i = 0; i < VectorKeys.Num(); ++i)
            {
                const FVector& V = Context.VectorParams[VectorKeys[i]];
                MeshComp->SetCustomPrimitiveDataVector4(i, FVector4(V.X, V.Y, V.Z, 0.0f));
            }

            // ColorParams -> SetCustomPrimitiveDataVector4 (as LinearColor)
            TArray<FName> ColorKeys;
            Context.ColorParams.GenerateKeyArray(ColorKeys);
            ColorKeys.Sort([](const FName& A, const FName& B) { return A.FastLess(B); });
            for (int32 i = 0; i < ColorKeys.Num(); ++i)
            {
                const FLinearColor& C = Context.ColorParams[ColorKeys[i]];
                MeshComp->SetCustomPrimitiveDataVector4(i, FVector4(C.R, C.G, C.B, C.A));
            }

            // BoolParams -> SetCustomPrimitiveDataFloat (0 or 1)
            TArray<FName> BoolKeys;
            Context.BoolParams.GenerateKeyArray(BoolKeys);
            BoolKeys.Sort([](const FName& A, const FName& B) { return A.FastLess(B); });
            for (int32 i = 0; i < BoolKeys.Num(); ++i)
            {
                MeshComp->SetCustomPrimitiveDataFloat(i, Context.BoolParams[BoolKeys[i]] ? 1.0f : 0.0f);
            }
        }
    }
}

// ---------- Blueprint ----------

bool FScenecapsBlueprintEntryHandler::CanHandleAsset(const UObject* Asset) const
{
    const UBlueprint* BP = Cast<UBlueprint>(Asset);
    return BP && BP->GeneratedClass && BP->GeneratedClass->IsChildOf(AActor::StaticClass());
}

AActor* FScenecapsBlueprintEntryHandler::SpawnActor(
    const FScenecapsSpawnContext& Context, UObject* Asset) const
{
    UBlueprint* BP = Cast<UBlueprint>(Asset);
    if (!BP || !BP->GeneratedClass || !Context.World) return nullptr;
    if (!BP->GeneratedClass->IsChildOf(AActor::StaticClass())) return nullptr;

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Params.ObjectFlags |= RF_Transactional;

    AActor* Spawned = Context.World->SpawnActor<AActor>(BP->GeneratedClass, Context.Transform, Params);
    ApplyEntryOverrides(Spawned, Context);
    return Spawned;
}

UObject* FScenecapsBlueprintEntryHandler::ResolveAssetForStorage(UObject* DroppedObject) const
{
    UBlueprint* BP = Cast<UBlueprint>(DroppedObject);
    if (BP && BP->GeneratedClass && BP->GeneratedClass->IsChildOf(AActor::StaticClass()))
    {
        return BP;
    }
    return nullptr;
}

UObject* FScenecapsBlueprintEntryHandler::ResolveAssetFromActor(AActor* Actor) const
{
    if (!Actor)
    {
        return nullptr;
    }

#if WITH_EDITOR
    if (UBlueprint* BP = Cast<UBlueprint>(Actor->GetClass()->ClassGeneratedBy))
    {
        return BP;
    }
#endif

    return nullptr;
}

// ---------- Actor Class ----------

bool FScenecapsClassEntryHandler::CanHandleAsset(const UObject* Asset) const
{
    const UClass* Cls = Cast<UClass>(Asset);
    return Cls && Cls->IsChildOf(AActor::StaticClass());
}

AActor* FScenecapsClassEntryHandler::SpawnActor(
    const FScenecapsSpawnContext& Context, UObject* Asset) const
{
    UClass* Cls = Cast<UClass>(Asset);
    if (!Cls || !Cls->IsChildOf(AActor::StaticClass()) || !Context.World) return nullptr;

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Params.ObjectFlags |= RF_Transactional;

    AActor* Spawned = Context.World->SpawnActor<AActor>(Cls, Context.Transform, Params);
    ApplyEntryOverrides(Spawned, Context);
    return Spawned;
}

UObject* FScenecapsClassEntryHandler::ResolveAssetForStorage(UObject* DroppedObject) const
{
    UClass* Cls = Cast<UClass>(DroppedObject);
    if (Cls && Cls->IsChildOf(AActor::StaticClass()))
    {
        return Cls;
    }
    return nullptr;
}

UObject* FScenecapsClassEntryHandler::ResolveAssetFromActor(AActor* Actor) const
{
    if (Actor 
#if WITH_EDITOR
        && !Cast<UBlueprint>(Actor->GetClass()->ClassGeneratedBy)
#endif
    )
    {
        return Actor->GetClass();
    }
    return nullptr;
}

// ---------- StaticMesh ----------

bool FScenecapsStaticMeshEntryHandler::CanHandleAsset(const UObject* Asset) const
{
    return Cast<UStaticMesh>(Asset) != nullptr;
}

AActor* FScenecapsStaticMeshEntryHandler::SpawnActor(
    const FScenecapsSpawnContext& Context, UObject* Asset) const
{
    UStaticMesh* Mesh = Cast<UStaticMesh>(Asset);
    if (!Mesh || !Context.World) return nullptr;

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Params.ObjectFlags |= RF_Transactional;

    AStaticMeshActor* SMA = Context.World->SpawnActor<AStaticMeshActor>(
        AStaticMeshActor::StaticClass(), Context.Transform, Params);
    if (!SMA) return nullptr;

    UStaticMeshComponent* Comp = SMA->GetStaticMeshComponent();
    Comp->SetStaticMesh(Mesh);

    Comp->SetMobility(EComponentMobility::Movable);

    ApplyEntryOverrides(SMA, Context);
    return SMA;
}

UObject* FScenecapsStaticMeshEntryHandler::ResolveAssetForStorage(UObject* DroppedObject) const
{
    return Cast<UStaticMesh>(DroppedObject);
}

// ---------- Material ----------

bool FScenecapsMaterialEntryHandler::CanHandleAsset(const UObject* Asset) const
{
    return Cast<UMaterialInterface>(Asset) != nullptr;
}

AActor* FScenecapsMaterialEntryHandler::SpawnActor(
    const FScenecapsSpawnContext& Context, UObject* Asset) const
{
    UMaterialInterface* Material = Cast<UMaterialInterface>(Asset);
    if (!Material || !Context.World) return nullptr;

    UMaterial* BaseMaterial = Material->GetMaterial();
    if (BaseMaterial && BaseMaterial->MaterialDomain == MD_DeferredDecal)
    {
        FActorSpawnParameters DecalParams;
        DecalParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        DecalParams.ObjectFlags |= RF_Transactional;

        ADecalActor* DecalActor = Context.World->SpawnActor<ADecalActor>(
            ADecalActor::StaticClass(), Context.Transform, DecalParams);
        if (!DecalActor) return nullptr;

        DecalActor->SetDecalMaterial(Material);

        if (UDecalComponent* DecalComp = DecalActor->FindComponentByClass<UDecalComponent>())
        {
            DecalComp->DecalSize = Context.DecalSize;
        }

        ApplyEntryOverrides(DecalActor, Context);
        return DecalActor;
    }

    UStaticMesh* PreviewMesh = LoadObject<UStaticMesh>(
        nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (!PreviewMesh)
    {
        PreviewMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    }
    if (!PreviewMesh) return nullptr;

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Params.ObjectFlags |= RF_Transactional;

    AStaticMeshActor* SMA = Context.World->SpawnActor<AStaticMeshActor>(
        AStaticMeshActor::StaticClass(), Context.Transform, Params);
    if (!SMA) return nullptr;

    UStaticMeshComponent* Comp = SMA->GetStaticMeshComponent();
    Comp->SetStaticMesh(PreviewMesh);
    Comp->SetMaterial(0, Material);
    Comp->SetMobility(EComponentMobility::Movable);

    ApplyEntryOverrides(SMA, Context);
    return SMA;
}

UObject* FScenecapsMaterialEntryHandler::ResolveAssetForStorage(UObject* DroppedObject) const
{
    return Cast<UMaterialInterface>(DroppedObject);
}

// ---------- Sound ----------

bool FScenecapsSoundEntryHandler::CanHandleAsset(const UObject* Asset) const
{
    return Cast<USoundBase>(Asset) != nullptr;
}

AActor* FScenecapsSoundEntryHandler::SpawnActor(
    const FScenecapsSpawnContext& Context, UObject* Asset) const
{
    USoundBase* Sound = Cast<USoundBase>(Asset);
    if (!Sound || !Context.World) return nullptr;

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Params.ObjectFlags |= RF_Transactional;

    AAmbientSound* AmbientSound = Context.World->SpawnActor<AAmbientSound>(
        AAmbientSound::StaticClass(), Context.Transform, Params);
    if (!AmbientSound) return nullptr;

    if (UAudioComponent* Audio = AmbientSound->FindComponentByClass<UAudioComponent>())
    {
        Audio->SetSound(Sound);
    }

    ApplyEntryOverrides(AmbientSound, Context);
    return AmbientSound;
}

UObject* FScenecapsSoundEntryHandler::ResolveAssetForStorage(UObject* DroppedObject) const
{
    return Cast<USoundBase>(DroppedObject);
}

UObject* FScenecapsSoundEntryHandler::ResolveAssetFromActor(AActor* Actor) const
{
    if (!Actor) return nullptr;

    if (AAmbientSound* AmbientSound = Cast<AAmbientSound>(Actor))
    {
        if (UAudioComponent* Audio = AmbientSound->FindComponentByClass<UAudioComponent>())
        {
            return Audio->GetSound();
        }
    }
    return nullptr;
}

// ---------- Nested Scene Collection ----------

bool FScenecapsNestedEntryHandler::CanHandleAsset(const UObject* Asset) const
{
    return Cast<UScenecapsAsset>(Asset) != nullptr;
}

AActor* FScenecapsNestedEntryHandler::SpawnActor(
    const FScenecapsSpawnContext& Context, UObject* Asset) const
{
    UScenecapsAsset* Nested = Cast<UScenecapsAsset>(Asset);
    if (!Nested || !Context.World) return nullptr;

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Params.ObjectFlags |= RF_Transactional;

    AScenecapsActor* Inner = Context.World->SpawnActor<AScenecapsActor>(
        AScenecapsActor::StaticClass(), Context.Transform, Params);
    if (!Inner) return nullptr;

    Inner->CollectionAsset = Nested;
    Inner->bIncludeHidden = Context.bIncludeHidden;
    Inner->Rebuild();

    ApplyEntryOverrides(Inner, Context);
    return Inner;
}

UObject* FScenecapsNestedEntryHandler::ResolveAssetForStorage(UObject* DroppedObject) const
{
    return Cast<UScenecapsAsset>(DroppedObject);
}

UObject* FScenecapsNestedEntryHandler::ResolveAssetFromActor(AActor* Actor) const
{
    if (AScenecapsActor* Container = Cast<AScenecapsActor>(Actor))
    {
        return Container->CollectionAsset.LoadSynchronous();
    }
    return nullptr;
}

// ---------- Niagara ----------

#if SCENE_COLLECTION_WITH_NIAGARA

bool FScenecapsNiagaraEntryHandler::CanHandleAsset(const UObject* Asset) const
{
    return Cast<UNiagaraSystem>(Asset) != nullptr;
}

AActor* FScenecapsNiagaraEntryHandler::SpawnActor(
    const FScenecapsSpawnContext& Context, UObject* Asset) const
{
    UNiagaraSystem* System = Cast<UNiagaraSystem>(Asset);
    if (!System || !Context.World) return nullptr;

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Params.ObjectFlags |= RF_Transactional;

    ANiagaraActor* NiagaraActor = Context.World->SpawnActor<ANiagaraActor>(
        ANiagaraActor::StaticClass(), Context.Transform, Params);
    if (!NiagaraActor) return nullptr;

    if (UNiagaraComponent* Comp = NiagaraActor->GetNiagaraComponent())
    {
        Comp->SetAsset(System);
        Comp->Activate(true);
    }

    ApplyEntryOverrides(NiagaraActor, Context);
    return NiagaraActor;
}

UObject* FScenecapsNiagaraEntryHandler::ResolveAssetForStorage(UObject* DroppedObject) const
{
    return Cast<UNiagaraSystem>(DroppedObject);
}

UObject* FScenecapsNiagaraEntryHandler::ResolveAssetFromActor(AActor* Actor) const
{
    if (!Actor) return nullptr;

    if (ANiagaraActor* NiagaraActor = Cast<ANiagaraActor>(Actor))
    {
        if (UNiagaraComponent* Comp = NiagaraActor->GetNiagaraComponent())
        {
            return Comp->GetAsset();
        }
    }

    return nullptr;
}

#endif // SCENE_COLLECTION_WITH_NIAGARA
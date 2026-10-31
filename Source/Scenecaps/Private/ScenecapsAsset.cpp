// Plugins/Scenecaps/Source/Scenecaps/Private/ScenecapsAsset.cpp

#include "ScenecapsAsset.h"

#include "ScenecapsEntryRegistry.h"
#include "ScenecapsModule.h"

#include "Engine/Blueprint.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"
#include "ScenecapsISMComponent.h"
#include "ScenecapsModifierManager.h"

UScenecapsAsset::UScenecapsAsset()
{
    RootId = FGuid::NewGuid();
}

void UScenecapsAsset::PostLoad()
{
    Super::PostLoad();
    if (!RootId.IsValid())
    {
        RootId = FGuid::NewGuid();
    }
}

// ---------- Variant helpers ----------

TArray<FName> UScenecapsAsset::GetVariantNames() const
{
    TArray<FName> Out;
    Out.Reserve(Variants.Num());
    for (const FScenecapsVariant& V : Variants)
    {
        if (!V.Name.IsNone())
        {
            Out.Add(V.Name);
        }
    }
    return Out;
}

const FScenecapsVariant* UScenecapsAsset::FindVariant(FName VariantName) const
{
    if (VariantName.IsNone()) return nullptr;
    return Variants.FindByPredicate(
        [&](const FScenecapsVariant& V) { return V.Name == VariantName; });
}

const FScenecapsEntryOverride* UScenecapsAsset::FindOverride(
    const FScenecapsVariant& Variant, FGuid EntryId) const
{
    return Variant.Overrides.FindByPredicate(
        [&](const FScenecapsEntryOverride& O) { return O.EntryId == EntryId; });
}

FScenecapsEntry UScenecapsAsset::ApplyVariantToEntry(
    const FScenecapsEntry& BaseEntry, FName VariantName) const
{
    const FScenecapsVariant* Variant = FindVariant(VariantName);
    if (!Variant) return BaseEntry;

    const FScenecapsEntryOverride* Override = FindOverride(*Variant, BaseEntry.EntryId);
    if (!Override) return BaseEntry;

    FScenecapsEntry Result = BaseEntry;

    if (Override->bOverrideVisible)
    {
        Result.bVisible = Override->bVisible;
    }
    if (Override->bOverrideTransform)
    {
        Result.RelativeTransform = Override->RelativeTransform;
    }
    if (Override->bOverrideMaterial)
    {
        Result.MaterialOverride = Override->MaterialOverride;
        Result.MaterialSlotIndex = Override->MaterialSlotIndex;
    }
    // ---- 新增：自定义参数覆盖 ----
    if (Override->bOverrideFloatParams)
    {
        Result.FloatParams = Override->FloatParams;
    }
    if (Override->bOverrideVectorParams)
    {
        Result.VectorParams = Override->VectorParams;
    }
    if (Override->bOverrideColorParams)
    {
        Result.ColorParams = Override->ColorParams;
    }
    if (Override->bOverrideBoolParams)
    {
        Result.BoolParams = Override->BoolParams;
    }
    // ---- 新增：程序化参数覆盖 ----
    if (Override->bOverrideProceduralParams)
    {
        Result.ProceduralParams = Override->ProceduralParams;
    }

    return Result;
}

// ---- 新增：变体插值实现 ----

static float LerpFloat(float A, float B, float Alpha) { return A + (B - A) * Alpha; }
static FVector LerpVector(const FVector& A, const FVector& B, float Alpha) { return FMath::Lerp(A, B, Alpha); }
static FLinearColor LerpColor(const FLinearColor& A, const FLinearColor& B, float Alpha) { return FMath::Lerp(A, B, Alpha); }
static FTransform LerpTransform(const FTransform& A, const FTransform& B, float Alpha)
{
    return FTransform(
        FMath::Lerp(A.GetRotation(), B.GetRotation(), Alpha),
        FMath::Lerp(A.GetLocation(), B.GetLocation(), Alpha),
        FMath::Lerp(A.GetScale3D(), B.GetScale3D(), Alpha)
    );
}

FScenecapsInterpolatedEntry UScenecapsAsset::InterpolateEntry(
    const FScenecapsEntry& BaseEntry,
    FName FromVariant,
    FName ToVariant,
    float Alpha) const
{
    FScenecapsInterpolatedEntry Result;
    Result.EntryId = BaseEntry.EntryId;

    // 应用 From 变体
    FScenecapsEntry FromEntry = ApplyVariantToEntry(BaseEntry, FromVariant);
    // 应用 To 变体
    FScenecapsEntry ToEntry = ApplyVariantToEntry(BaseEntry, ToVariant);

    // 插值
    Alpha = FMath::Clamp(Alpha, 0.0f, 1.0f);

    Result.bVisible = Alpha < 0.5f ? FromEntry.bVisible : ToEntry.bVisible;
    Result.RelativeTransform = LerpTransform(FromEntry.RelativeTransform, ToEntry.RelativeTransform, Alpha);
    Result.MaterialOverride = Alpha < 0.5f ? FromEntry.MaterialOverride : ToEntry.MaterialOverride;
    Result.MaterialSlotIndex = Alpha < 0.5f ? FromEntry.MaterialSlotIndex : ToEntry.MaterialSlotIndex;

    // 插值 FloatParams
    for (const auto& Pair : FromEntry.FloatParams)
    {
        float FromVal = Pair.Value;
        float ToVal = ToEntry.FloatParams.Contains(Pair.Key) ? ToEntry.FloatParams[Pair.Key] : 0.0f;
        Result.FloatParams.Add(Pair.Key, LerpFloat(FromVal, ToVal, Alpha));
    }
    // To 中有但 From 中没有的
    for (const auto& Pair : ToEntry.FloatParams)
    {
        if (!FromEntry.FloatParams.Contains(Pair.Key))
        {
            Result.FloatParams.Add(Pair.Key, LerpFloat(0.0f, Pair.Value, Alpha));
        }
    }

    // 插值 VectorParams
    for (const auto& Pair : FromEntry.VectorParams)
    {
        FVector FromVal = Pair.Value;
        FVector ToVal = ToEntry.VectorParams.Contains(Pair.Key) ? ToEntry.VectorParams[Pair.Key] : FVector::ZeroVector;
        Result.VectorParams.Add(Pair.Key, LerpVector(FromVal, ToVal, Alpha));
    }
    for (const auto& Pair : ToEntry.VectorParams)
    {
        if (!FromEntry.VectorParams.Contains(Pair.Key))
        {
            Result.VectorParams.Add(Pair.Key, LerpVector(FVector::ZeroVector, Pair.Value, Alpha));
        }
    }

    // 插值 ColorParams
    for (const auto& Pair : FromEntry.ColorParams)
    {
        FLinearColor FromVal = Pair.Value;
        FLinearColor ToVal = ToEntry.ColorParams.Contains(Pair.Key) ? ToEntry.ColorParams[Pair.Key] : FLinearColor::Transparent;
        Result.ColorParams.Add(Pair.Key, LerpColor(FromVal, ToVal, Alpha));
    }
    for (const auto& Pair : ToEntry.ColorParams)
    {
        if (!FromEntry.ColorParams.Contains(Pair.Key))
        {
            Result.ColorParams.Add(Pair.Key, LerpColor(FLinearColor::Transparent, Pair.Value, Alpha));
        }
    }

    // BoolParams 不插值，用 Alpha 阈值
    for (const auto& Pair : FromEntry.BoolParams)
    {
        bool FromVal = Pair.Value;
        bool ToVal = ToEntry.BoolParams.Contains(Pair.Key) ? ToEntry.BoolParams[Pair.Key] : false;
        Result.BoolParams.Add(Pair.Key, Alpha < 0.5f ? FromVal : ToVal);
    }
    for (const auto& Pair : ToEntry.BoolParams)
    {
        if (!FromEntry.BoolParams.Contains(Pair.Key))
        {
            Result.BoolParams.Add(Pair.Key, Alpha >= 0.5f ? Pair.Value : false);
        }
    }

    return Result;
}

TArray<FScenecapsInterpolatedEntry> UScenecapsAsset::InterpolateAllEntries(
    FName FromVariant,
    FName ToVariant,
    float Alpha) const
{
    TArray<FScenecapsInterpolatedEntry> Results;
    Results.Reserve(Entries.Num());

    for (const FScenecapsEntry& Entry : Entries)
    {
        Results.Add(InterpolateEntry(Entry, FromVariant, ToVariant, Alpha));
    }

    return Results;
}

FScenecapsVariantTransition UScenecapsAsset::BeginTransition(
    FName FromVariant,
    FName ToVariant,
    float Duration,
    EScenecapsInterpolationEase EaseType,
    bool bLoop,
    bool bPingPong) const
{
    FScenecapsVariantTransition Transition;
    Transition.FromVariant = FromVariant;
    Transition.ToVariant = ToVariant;
    Transition.Duration = FMath::Max(Duration, 0.001f);
    Transition.EaseType = EaseType;
    Transition.bLoop = bLoop;
    Transition.bPingPong = bPingPong;
    Transition.bIsActive = true;
    Transition.ElapsedTime = 0.0f;
    Transition.Alpha = 0.0f;
    return Transition;
}

// ---------- Spawn ----------

TArray<AActor*> UScenecapsAsset::SpawnCollection(
    UObject* WorldContextObject,
    const FTransform& WorldTransform,
    bool bIncludeHidden,
    FName VariantName)
{
    TMap<FGuid, AActor*> Ignored;
    return SpawnCollectionEx(WorldContextObject, WorldTransform, bIncludeHidden, VariantName, Ignored);
}

TArray<AActor*> UScenecapsAsset::SpawnCollectionEx(UObject* WorldContextObject, const FTransform& WorldTransform, bool bIncludeHidden, FName VariantName, TMap<FGuid, AActor*>& OutEntryToActor)
{
    TArray<AActor*> SpawnedActors;

    UWorld* World = GEngine
        ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull)
        : nullptr;
    if (!World) return SpawnedActors;

    static thread_local TArray<const UScenecapsAsset*> GRecursionStack;
    constexpr int32 GMaxNestingDepth = 16;

    if (GRecursionStack.Contains(this))
    {
        UE_LOG(LogScenecaps, Error,
            TEXT("Circular reference detected: %s, skipping spawn"), *GetName());
        return SpawnedActors;
    }
    if (GRecursionStack.Num() >= GMaxNestingDepth)
    {
        UE_LOG(LogScenecaps, Error,
            TEXT("Nesting depth exceeds %d, skipping: %s"), GMaxNestingDepth, *GetName());
        return SpawnedActors;
    }

    GRecursionStack.Push(this);
    struct FRecursionGuard
    {
        ~FRecursionGuard() { GRecursionStack.Pop(); }
    } RecursionGuard;

    AActor* WorldContextActor = Cast<AActor>(WorldContextObject);

    TMap<FGuid, AActor*>    IdToActor;
    TMap<FGuid, FTransform> IdToWorld;
    TSet<FGuid>             Processed;

    TFunction<void(const FScenecapsEntry&, const FTransform&, bool)> SpawnEntry =
        [&](const FScenecapsEntry& BaseEntry, const FTransform& ParentWorld, bool bParentSkipped)
        {
            if (Processed.Contains(BaseEntry.EntryId))
            {
                return;
            }
            Processed.Add(BaseEntry.EntryId);

            // ★ 变体应用：所有下游都基于实际使用的 Entry
            const FScenecapsEntry Entry = ApplyVariantToEntry(BaseEntry, VariantName);

            const FTransform FinalWorld = ParentWorld * Entry.RelativeTransform;
            const bool bShouldSpawn = (Entry.bVisible || bIncludeHidden) && !bParentSkipped;

            if (bShouldSpawn && !Entry.Asset.IsNull())
            {
                if (UObject* Loaded = Entry.Asset.LoadSynchronous())
                {
                    FScenecapsSpawnContext Context;
                    Context.World = World;
                    Context.Transform = FinalWorld;
                    Context.bIsPreview = false;
                    Context.bIncludeHidden = bIncludeHidden;

                    Context.DecalSize = Entry.DecalSize;
                    Context.bOverrideMobility = Entry.bOverrideMobility;
                    Context.Mobility = Entry.Mobility;
                    Context.bOverrideCollision = Entry.bOverrideCollision;
                    Context.CollisionEnabled = Entry.CollisionEnabled;
                    Context.bOverrideCastShadow = Entry.bOverrideCastShadow;
                    Context.bCastShadow = Entry.bCastShadow;

                    Context.MaterialOverride = Entry.MaterialOverride.LoadSynchronous();
                    Context.MaterialSlotIndex = Entry.MaterialSlotIndex;

                    // 新增：传递自定义参数给 Handler
                    Context.FloatParams = Entry.FloatParams;
                    Context.VectorParams = Entry.VectorParams;
                    Context.ColorParams = Entry.ColorParams;
                    Context.BoolParams = Entry.BoolParams;

                    IScenecapsEntryHandler* Handler =
                        FScenecapsEntryRegistry::Get().FindHandlerForAsset(Loaded);

                    if (AActor* Spawned = Handler ? Handler->SpawnActor(Context, Loaded) : nullptr)
                    {
                        if (WorldContextActor && BaseEntry.ParentId == RootId)
                        {
                            Spawned->AttachToActor(
                                WorldContextActor,
                                FAttachmentTransformRules::KeepWorldTransform);
                        }

                        SpawnedActors.Add(Spawned);
                        IdToActor.Add(BaseEntry.EntryId, Spawned);
                        IdToWorld.Add(BaseEntry.EntryId, FinalWorld);
                        OutEntryToActor.Add(BaseEntry.EntryId, Spawned);
                    }
                }
            }

            for (const FScenecapsEntry& Child : Entries)
            {
                if (Child.ParentId == BaseEntry.EntryId)
                {
                    SpawnEntry(Child, FinalWorld, bParentSkipped || !bShouldSpawn);
                }
            }
        };

    for (const FScenecapsEntry& Entry : Entries)
    {
        if (Entry.ParentId == RootId)
        {
            SpawnEntry(Entry, WorldTransform, false);
        }
    }

    for (const FScenecapsEntry& Entry : Entries)
    {
        if (!Processed.Contains(Entry.EntryId))
        {
            SpawnEntry(Entry, WorldTransform, false);
        }
    }

    for (const FScenecapsEntry& Entry : Entries)
    {
        AActor** ChildPtr = IdToActor.Find(Entry.EntryId);
        AActor** ParentPtr = IdToActor.Find(Entry.ParentId);
        if (ChildPtr && ParentPtr && *ChildPtr && *ParentPtr && *ChildPtr != *ParentPtr)
        {
            (*ChildPtr)->AttachToActor(*ParentPtr, FAttachmentTransformRules::KeepWorldTransform);
        }
    }

    return SpawnedActors;
}

// ---- 新增：程序化生成 ----

TArray<FTransform> UScenecapsAsset::GenerateProceduralTransforms(
    const FScenecapsEntry& BaseEntry,
    const FTransform& WorldTransform,
    FName VariantName) const
{
    TArray<FTransform> Transforms;

    const FScenecapsEntry Entry = ApplyVariantToEntry(BaseEntry, VariantName);
    const FScenecapsProceduralParams& Params = Entry.ProceduralParams;

    if (!Params.IsValid())
    {
        return Transforms;
    }

    // 简化版实现：基于类型生成
    FRandomStream Stream(Params.Seed);
    const FBox& Bounds = Params.Bounds;
    const FVector Extent = Bounds.GetExtent();
    const FVector Center = Bounds.GetCenter();

    int32 Count = FMath::Max(1, FMath::RoundToInt(Extent.X * Extent.Y * 4.0f * Params.Density));

    switch (Params.Type)
    {
    case EScenecapsProceduralType::Grid:
    {
        float Spacing = FMath::Max(Params.GridSpacing, 1.0f);
        int32 NumX = FMath::Max(1, FMath::RoundToInt(Extent.X * 2 / Spacing));
        int32 NumY = FMath::Max(1, FMath::RoundToInt(Extent.Y * 2 / Spacing));

        for (int32 x = 0; x < NumX; ++x)
        {
            for (int32 y = 0; y < NumY; ++y)
            {
                FVector LocalPos(
                    Center.X - Extent.X + (x + 0.5f) * Spacing,
                    Center.Y - Extent.Y + (y + 0.5f) * Spacing,
                    Center.Z
                );
                FTransform LocalTransform(LocalPos);
                Transforms.Add(WorldTransform * Entry.RelativeTransform * LocalTransform);
            }
        }
        break;
    }

    case EScenecapsProceduralType::PoissonDisk:
    {
        // 简化的 Poisson Disk（Bridson 算法）
        float MinDist = FMath::Max(Params.MinDistance, 1.0f);
        float CellSize = MinDist / FMath::Sqrt(2.0f);
        int32 GridW = FMath::Max(1, FMath::CeilToInt(Extent.X * 2 / CellSize));
        int32 GridH = FMath::Max(1, FMath::CeilToInt(Extent.Y * 2 / CellSize));

        TArray<FVector2D> Grid;
        Grid.SetNumZeroed(GridW * GridH);

        TArray<FVector2D> ActiveList;
        FVector2D First(
            Center.X + Stream.FRandRange(-Extent.X, Extent.X),
            Center.Y + Stream.FRandRange(-Extent.Y, Extent.Y)
        );
        ActiveList.Add(First);
        int32 GridX = FMath::FloorToInt((First.X - (Center.X - Extent.X)) / CellSize);
        int32 GridY = FMath::FloorToInt((First.Y - (Center.Y - Extent.Y)) / CellSize);
        if (GridX >= 0 && GridX < GridW && GridY >= 0 && GridY < GridH)
            Grid[GridY * GridW + GridX] = First;

        while (ActiveList.Num() > 0 && Transforms.Num() < Count)
        {
            int32 Idx = Stream.RandRange(0, ActiveList.Num() - 1);
            FVector2D Sample = ActiveList[Idx];
            bool Found = false;

            for (int32 i = 0; i < 30; ++i)
            {
                float Angle = Stream.FRandRange(0, 2 * PI);
                float Dist = Stream.FRandRange(MinDist, 2 * MinDist);
                FVector2D Candidate = Sample + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Dist;

                int32 Gx = FMath::FloorToInt((Candidate.X - (Center.X - Extent.X)) / CellSize);
                int32 Gy = FMath::FloorToInt((Candidate.Y - (Center.Y - Extent.Y)) / CellSize);

                if (Gx < 0 || Gx >= GridW || Gy < 0 || Gy >= GridH)
                    continue;

                bool TooClose = false;
                for (int32 dx = -1; dx <= 1 && !TooClose; ++dx)
                {
                    for (int32 dy = -1; dy <= 1; ++dy)
                    {
                        int32 Nx = Gx + dx, Ny = Gy + dy;
                        if (Nx >= 0 && Nx < GridW && Ny >= 0 && Ny < GridH)
                        {
                            FVector2D Other = Grid[Ny * GridW + Nx];
                            if (!Other.IsZero() && (Candidate - Other).Size() < MinDist)
                            {
                                TooClose = true;
                                break;
                            }
                        }
                    }
                }

                if (!TooClose)
                {
                    ActiveList.Add(Candidate);
                    Grid[Gy * GridW + Gx] = Candidate;
                    FVector LocalPos(Candidate.X, Candidate.Y, Center.Z);
                    FTransform LocalTransform(LocalPos);
                    Transforms.Add(WorldTransform * Entry.RelativeTransform * LocalTransform);
                    Found = true;
                    break;
                }
            }

            if (!Found)
            {
                ActiveList.RemoveAtSwap(Idx);
            }
        }
        break;
    }

    case EScenecapsProceduralType::NoiseCluster:
    {
        // 简化的噪声聚类
        for (int32 i = 0; i < Count; ++i)
        {
            float Angle = Stream.FRandRange(0, 2 * PI);
            float Radius = Stream.FRandRange(0, Params.ClusterRadius);
            FVector2D Offset = FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius;

            // 叠加噪声
            float NoiseX = FMath::PerlinNoise2D(FVector2D(
                (Center.X + Offset.X) * Params.NoiseScale,
                (Center.Y + Offset.Y) * Params.NoiseScale)) * Params.ClusterRadius * 0.5f;
            float NoiseY = FMath::PerlinNoise2D(FVector2D(
                (Center.X + Offset.X) * Params.NoiseScale + 1000,
                (Center.Y + Offset.Y) * Params.NoiseScale + 1000)) * Params.ClusterRadius * 0.5f;

            FVector LocalPos(
                Center.X + Offset.X + NoiseX,
                Center.Y + Offset.Y + NoiseY,
                Center.Z
            );
            FTransform LocalTransform(LocalPos);
            Transforms.Add(WorldTransform * Entry.RelativeTransform * LocalTransform);
        }
        break;
    }

    case EScenecapsProceduralType::Radial:
    {
        float AngleStep = 2 * PI / FMath::Max(1, Params.RadialSegments);
        int32 PerSegment = FMath::Max(1, Count / FMath::Max(1, Params.RadialSegments));

        for (int32 seg = 0; seg < Params.RadialSegments; ++seg)
        {
            for (int32 i = 0; i < PerSegment && Transforms.Num() < Count; ++i)
            {
                float Radius = Stream.FRandRange(Params.RadialMinRadius, Params.RadialMaxRadius);
                float Angle = seg * AngleStep + Stream.FRandRange(-AngleStep * 0.5f, AngleStep * 0.5f);
                FVector LocalPos(
                    Center.X + FMath::Cos(Angle) * Radius,
                    Center.Y + FMath::Sin(Angle) * Radius,
                    Center.Z
                );
                FTransform LocalTransform(LocalPos);
                Transforms.Add(WorldTransform * Entry.RelativeTransform * LocalTransform);
            }
        }
        break;
    }

    default:
        break;
    }

    // 应用随机化：缩放、旋转、抖动
    for (FTransform& T : Transforms)
    {
        // 缩放
        FVector Scale(
            Stream.FRandRange(Params.MinScale.X, Params.MaxScale.X),
            Stream.FRandRange(Params.MinScale.Y, Params.MaxScale.Y),
            Stream.FRandRange(Params.MinScale.Z, Params.MaxScale.Z)
        );
        T.SetScale3D(T.GetScale3D() * Scale);

        // 旋转
        FRotator RotOffset(
            Stream.FRandRange(-Params.MaxRotationOffset.Pitch, Params.MaxRotationOffset.Pitch),
            Stream.FRandRange(-Params.MaxRotationOffset.Yaw, Params.MaxRotationOffset.Yaw),
            Stream.FRandRange(-Params.MaxRotationOffset.Roll, Params.MaxRotationOffset.Roll)
        );
        T.SetRotation((T.GetRotation() * RotOffset.Quaternion()).GetNormalized());

        // 位置抖动
        T.AddToTranslation(FVector(
            Stream.FRandRange(-Params.LocationJitter.X, Params.LocationJitter.X),
            Stream.FRandRange(-Params.LocationJitter.Y, Params.LocationJitter.Y),
            Stream.FRandRange(-Params.LocationJitter.Z, Params.LocationJitter.Z)
        ));
    }

    return Transforms;
}



// ================================================================
// ISM/HISM Batch Spawn
// ================================================================
// ISM/HISM Batch Spawn (UE 5.8 compatible)
// ================================================================

static void SetupISMForLumen(UInstancedStaticMeshComponent* ISM)
{
    if (!ISM || !ISM->GetStaticMesh()) return;

    // Lumen 关键设置
    ISM->SetAffectDistanceFieldLighting(true);        // 参与距离场光照（Lumen Surface Cache）
    ISM->SetAffectDynamicIndirectLighting(true);      // 参与动态间接光照
    ISM->SetCastShadow(true);                          // 投射阴影
    ISM->SetLightingChannels(false, true, false);     // Static Lighting 通道

    // 碰撞/导航
    ISM->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    ISM->SetGenerateOverlapEvents(false);
    ISM->SetCanEverAffectNavigation(false);
    ISM->bHasPerInstanceHitProxies = false;

    // Per-instance custom data（材质参数驱动）
    ISM->SetNumCustomDataFloats(4);
}

static void SetupHISMForLumen(UHierarchicalInstancedStaticMeshComponent* HISM)
{
    if (!HISM || !HISM->GetStaticMesh()) return;

    HISM->SetAffectDistanceFieldLighting(true);
    HISM->SetAffectDynamicIndirectLighting(true);
    HISM->SetCastShadow(true);
    HISM->SetLightingChannels(false, true, false);

    HISM->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    HISM->SetGenerateOverlapEvents(false);
    HISM->SetCanEverAffectNavigation(false);
    HISM->bHasPerInstanceHitProxies = false;

    // HISM 特有：LOD/剔除
    // HISM->SetEnableAutoLODs(true); // API changed in UE 5.x
    HISM->bEnableDensityScaling = true;
}

UInstancedStaticMeshComponent* UScenecapsAsset::SpawnCollectionAsISM(UObject* WorldContextObject, const FTransform& WorldTransform, FName VariantName)
{
    UWorld* World = GEngine
        ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull)
        : nullptr;
    if (!World) return nullptr;

    // 1. 按 StaticMesh 分组收集变换
    TMap<UStaticMesh*, TArray<FTransform>> MeshToTransforms;
    TMap<UStaticMesh*, TArray<FScenecapsSpawnContext>> MeshToContexts;

    TMap<FGuid, AActor*> DummyMap;
    TArray<AActor*> TempActors = this->SpawnCollectionEx(WorldContextObject, WorldTransform, false, VariantName, DummyMap);

    for (AActor* Actor : TempActors)
    {
        if (!IsValid(Actor)) continue;

        TArray<UStaticMeshComponent*> MeshComps;
        Actor->GetComponents<UStaticMeshComponent>(MeshComps);

        for (UStaticMeshComponent* MeshComp : MeshComps)
        {
            if (UStaticMesh* Mesh = MeshComp->GetStaticMesh())
            {
                FTransform InstanceTransform = MeshComp->GetComponentTransform();
                MeshToTransforms.FindOrAdd(Mesh).Add(InstanceTransform);

                FScenecapsSpawnContext Context;
                Context.Transform = InstanceTransform;
                Context.MaterialOverride = MeshComp->GetMaterial(0);
                MeshToContexts.FindOrAdd(Mesh).Add(Context);
            }
        }

        Actor->Destroy();
    }

if (MeshToTransforms.Num() == 0) return nullptr;

    // 2. 创建容器 Actor
    AActor* ContainerActor = World->SpawnActor<AActor>();
    if (!ContainerActor) return nullptr;
    ContainerActor->SetActorLabel(FString::Printf(TEXT("ScenecapsISMContainer_%s"), *GetName()));

    // 添加运行时管理组件
    UScenecapsISMComponent* Manager = Cast<UScenecapsISMComponent>(ContainerActor->AddComponentByClass(UScenecapsISMComponent::StaticClass(), false, FTransform::Identity, false));
    if (Manager)
    {
        Manager->RegisterContainerActor(ContainerActor);
    }

    USceneComponent* Root = ContainerActor->GetRootComponent();
    if (!Root)
    {
        Root = NewObject<USceneComponent>(ContainerActor, TEXT("ISMRoot"));
        ContainerActor->SetRootComponent(Root);
    }

    UInstancedStaticMeshComponent* FirstISM = nullptr;

    // 3. 为每个唯一 Mesh 创建 ISM
    for (auto& Pair : MeshToTransforms)
    {
        UStaticMesh* Mesh = Pair.Key;
        const TArray<FTransform>& Transforms = Pair.Value;
        const TArray<FScenecapsSpawnContext>* Contexts = MeshToContexts.Find(Mesh);

        UInstancedStaticMeshComponent* ISM = NewObject<UInstancedStaticMeshComponent>(ContainerActor);
        ISM->SetStaticMesh(Mesh);
        ISM->RegisterComponentWithWorld(World);
        ISM->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);

        // 添加实例
        for (int32 i = 0; i < Transforms.Num(); ++i)
        {
            ISM->AddInstance(Transforms[i], true);

            // 可选：写入 PerInstanceCustomData 供材质使用
            if (Contexts && Contexts->IsValidIndex(i))
            {
                const FScenecapsSpawnContext& Ctx = (*Contexts)[i];
                // 示例：ISM->SetCustomDataValue(i, 0, Ctx.FloatParams.FindRef("Density"), true);
            }
        }

        SetupISMForLumen(ISM);

        if (!FirstISM) FirstISM = ISM;
    }

    // 注意：距离场由 StaticMesh 编辑器设置 "Generate Distance Fields" 控制
    // 或通过 r.AOGlobalDistanceField 等 CVars 全局开启
    // 运行时无法直接触发构建，需确保 Mesh 资产已勾选该选项

    return FirstISM;
}

UHierarchicalInstancedStaticMeshComponent* UScenecapsAsset::SpawnCollectionAsHISM(UObject* WorldContextObject, const FTransform& WorldTransform, FName VariantName, int32 NumLODs, float LODScreenSize)
{
    UWorld* World = GEngine
        ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull)
        : nullptr;
    if (!World) return nullptr;

    TMap<UStaticMesh*, TArray<FTransform>> MeshToTransforms;
    TMap<UStaticMesh*, TArray<FScenecapsSpawnContext>> MeshToContexts;

    TMap<FGuid, AActor*> DummyMap;
    TArray<AActor*> TempActors = this->SpawnCollectionEx(WorldContextObject, WorldTransform, false, VariantName, DummyMap);

    for (AActor* Actor : TempActors)
    {
        if (!IsValid(Actor)) continue;

        TArray<UStaticMeshComponent*> MeshComps;
        Actor->GetComponents<UStaticMeshComponent>(MeshComps);

        for (UStaticMeshComponent* MeshComp : MeshComps)
        {
            if (UStaticMesh* Mesh = MeshComp->GetStaticMesh())
            {
                FTransform InstanceTransform = MeshComp->GetComponentTransform();
                MeshToTransforms.FindOrAdd(Mesh).Add(InstanceTransform);

                FScenecapsSpawnContext Context;
                Context.Transform = InstanceTransform;
                Context.MaterialOverride = MeshComp->GetMaterial(0);
                MeshToContexts.FindOrAdd(Mesh).Add(Context);
            }
        }

        Actor->Destroy();
    }

if (MeshToTransforms.Num() == 0) return nullptr;

    AActor* ContainerActor = World->SpawnActor<AActor>();
    if (!ContainerActor) return nullptr;
    ContainerActor->SetActorLabel(FString::Printf(TEXT("ScenecapsHISMContainer_%s"), *GetName()));

    // 添加运行时管理组件
    UScenecapsISMComponent* Manager = Cast<UScenecapsISMComponent>(ContainerActor->AddComponentByClass(UScenecapsISMComponent::StaticClass(), false, FTransform::Identity, false));
    if (Manager)
    {
        Manager->RegisterContainerActor(ContainerActor);
    }

    USceneComponent* Root = ContainerActor->GetRootComponent();
    if (!Root)
    {
        Root = NewObject<USceneComponent>(ContainerActor, TEXT("HISMRoot"));
        ContainerActor->SetRootComponent(Root);
    }

    UHierarchicalInstancedStaticMeshComponent* FirstHISM = nullptr;

    for (auto& Pair : MeshToTransforms)
    {
        UStaticMesh* Mesh = Pair.Key;
        const TArray<FTransform>& Transforms = Pair.Value;
        const TArray<FScenecapsSpawnContext>* Contexts = MeshToContexts.Find(Mesh);

        UHierarchicalInstancedStaticMeshComponent* HISM = NewObject<UHierarchicalInstancedStaticMeshComponent>(ContainerActor);
        HISM->SetStaticMesh(Mesh);
        HISM->RegisterComponentWithWorld(World);
        HISM->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);

        // HISM->NumLODs = FMath::Clamp(NumLODs, 1, 8); // Use SetNumLODs if available
        // HISM->LODScreenSize = LODScreenSize; // Use SetLODScreenSize if available
        HISM->PerInstanceSMData.Empty();

        for (int32 i = 0; i < Transforms.Num(); ++i)
        {
            HISM->AddInstance(Transforms[i], true);

            if (Contexts && Contexts->IsValidIndex(i))
            {
                // const FScenecapsSpawnContext& Ctx = (*Contexts)[i];
            }
        }

        SetupHISMForLumen(HISM);

        // 强制重建 HISM 层级树（用于视锥剔除和 LOD 选择）
        HISM->BuildTreeIfOutdated(true, false);

        if (!FirstHISM) FirstHISM = HISM;
    }

    return FirstHISM;
}











// ---- Modifier System ----

UScenecapsModifierManager* UScenecapsAsset::GetOrCreateModifierManager()
{
    if (!ModifierManager)
    {
        ModifierManager = NewObject<UScenecapsModifierManager>(this);
    }
    return ModifierManager;
}

void UScenecapsAsset::ApplyModifiers(EScenecapsModifierPhase Phase, FName VariantName, FGuid EntryId)
{
    if (UScenecapsModifierManager* Mgr = GetOrCreateModifierManager())
    {
        Mgr->ExecuteModifiers(this, Phase, VariantName, EntryId, nullptr);
    }
}

void UScenecapsAsset::ApplyModifiersToActor(AActor* Actor, FName VariantName)
{
    if (!Actor) return;
    if (UScenecapsModifierManager* Mgr = GetOrCreateModifierManager())
    {
        Mgr->ExecuteModifiersOnActor(Actor, this, VariantName);
    }
}











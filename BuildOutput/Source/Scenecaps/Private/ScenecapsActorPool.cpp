// Plugins/Scenecaps/Source/Scenecaps/Private/ScenecapsActorPool.cpp

#include "ScenecapsActorPool.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Engine/Engine.h"

UScenecapsActorPoolComponent::UScenecapsActorPoolComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 30.0f; // 每30秒清理一次
}

void UScenecapsActorPoolComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UScenecapsActorPoolComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    CleanupIdleActors();
}

void UScenecapsActorPoolComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // 销毁所有池中的 Actor
    for (FScenecapsPooledActor& Pooled : PooledActors)
    {
        if (Pooled.Actor && IsValid(Pooled.Actor))
        {
            Pooled.Actor->Destroy();
        }
    }
    PooledActors.Empty();
    ActiveCountPerClass.Empty();
    
    Super::EndPlay(EndPlayReason);
}

AActor* UScenecapsActorPoolComponent::AcquireActor(FName PoolKey, const FTransform& SpawnTransform, AActor* Owner)
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;

    // 查找可用的空闲 Actor
    for (FScenecapsPooledActor& Pooled : PooledActors)
    {
        if (!Pooled.bInUse && Pooled.PoolKey == PoolKey && Pooled.Actor && Pooled.Actor->IsValidLowLevel())
        {
            Pooled.bInUse = true;
            Pooled.LastUsedTime = FPlatformTime::Seconds();
            
            AActor* Actor = Pooled.Actor;
            Actor->SetActorTransform(SpawnTransform);
            Actor->SetActorHiddenInGame(false);
            Actor->SetActorEnableCollision(true);
            Actor->SetActorTickEnabled(true);
            
            if (Owner)
            {
                Actor->AttachToActor(Owner, FAttachmentTransformRules::KeepWorldTransform);
            }
            
            ActiveCountPerClass.FindOrAdd(PoolKey)++;
            return Actor;
        }
    }

    // 池中没有可用的，创建新的
    if (bAutoExpand && PoolClasses.Contains(PoolKey))
    {
        return CreatePooledActor(PoolKey, SpawnTransform, Owner);
    }

    return nullptr;
}

AActor* UScenecapsActorPoolComponent::CreatePooledActor(FName PoolKey, const FTransform& SpawnTransform, AActor* Owner)
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;

    TSubclassOf<AActor>* ActorClassPtr = PoolClasses.Find(PoolKey);
    if (!ActorClassPtr || !*ActorClassPtr) return nullptr;

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = Owner;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    SpawnParams.ObjectFlags |= RF_Transactional;

    AActor* Actor = World->SpawnActor<AActor>(*ActorClassPtr, SpawnTransform, SpawnParams);
    if (!Actor) return nullptr;

    FScenecapsPooledActor Pooled;
    Pooled.Actor = Actor;
    Pooled.bInUse = true;
    Pooled.PoolKey = PoolKey;
    Pooled.LastUsedTime = FPlatformTime::Seconds();

    PooledActors.Add(MoveTemp(Pooled));
    ActiveCountPerClass.FindOrAdd(PoolKey)++;

    return Actor;
}

void UScenecapsActorPoolComponent::ReleaseActor(AActor* Actor, FName PoolKey)
{
    if (!Actor) return;

    bool bFound = false;
    for (FScenecapsPooledActor& Pooled : PooledActors)
    {
        if (Pooled.Actor == Actor && Pooled.PoolKey == PoolKey)
        {
            Pooled.bInUse = false;
            Pooled.LastUsedTime = FPlatformTime::Seconds();
            
            ResetActor(Actor);
            bFound = true;
            break;
        }
    }
    
    if (bFound && ActiveCountPerClass.Contains(PoolKey))
    {
        ActiveCountPerClass[PoolKey] = FMath::Max(0, ActiveCountPerClass[PoolKey] - 1);
    }
}

void UScenecapsActorPoolComponent::ResetActor(AActor* Actor)
{
    if (!Actor) return;
    
    Actor->SetActorHiddenInGame(true);
    Actor->SetActorEnableCollision(false);
    Actor->SetActorTickEnabled(false);
    Actor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    Actor->SetActorLocation(FVector(0, 0, -10000)); // 移到地下
}

void UScenecapsActorPoolComponent::PrewarmPool(FName PoolKey, int32 Count)
{
    UWorld* World = GetWorld();
    if (!World || !PoolClasses.Contains(PoolKey)) return;

    TSubclassOf<AActor>* ActorClassPtr = PoolClasses.Find(PoolKey);
    if (!ActorClassPtr || !*ActorClassPtr) return;

    Count = FMath::Min(Count, MaxPoolSizePerClass);

    for (int32 i = 0; i < Count; ++i)
    {
        AActor* Actor = CreatePooledActor(PoolKey, FTransform::Identity, nullptr);
        if (Actor)
        {
            ReleaseActor(Actor, PoolKey);
        }
    }
}

void UScenecapsActorPoolComponent::CleanupIdleActors()
{
    double CurrentTime = FPlatformTime::Seconds();
    
    for (int32 i = PooledActors.Num() - 1; i >= 0; --i)
    {
        FScenecapsPooledActor& Pooled = PooledActors[i];
        
        if (!Pooled.bInUse && Pooled.Actor && IsValid(Pooled.Actor))
        {
            if (CurrentTime - Pooled.LastUsedTime > MaxIdleTimeSeconds)
            {
                Pooled.Actor->Destroy();
                PooledActors.RemoveAt(i);
            }
        }
    }
}

int32 UScenecapsActorPoolComponent::GetPoolSize(FName PoolKey) const
{
    int32 Count = 0;
    for (const FScenecapsPooledActor& Pooled : PooledActors)
    {
        if (Pooled.PoolKey == PoolKey)
        {
            Count++;
        }
    }
    return Count;
}

int32 UScenecapsActorPoolComponent::GetActiveCount(FName PoolKey) const
{
    return ActiveCountPerClass.FindRef(PoolKey);
}

int32 UScenecapsActorPoolComponent::GetTotalPoolSize() const
{
    return PooledActors.Num();
}

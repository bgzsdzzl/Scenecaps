// Plugins/Scenecaps/Source/Scenecaps/Private/ScenecapsDataDrivenEntryHandler.cpp

#include "ScenecapsDataDrivenEntryHandler.h"

#include "ScenecapsModule.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ConfigCacheIni.h"
#include "UObject/UnrealType.h"
#include "UObject/SoftObjectPtr.h"
#include "Internationalization/Text.h"
#include "Containers/StringConv.h"
#include "UObject/UObjectGlobals.h"

// ================================================================
// 内部二进制读取器
// ================================================================

namespace ScenecapsSCX
{
    class FReader
    {
    public:
        FReader(const TArray<uint8>& InData) : Data(InData) {}

        bool ReadUInt8(uint8& Out)
        {
            if (Position + 1 > Data.Num()) return false;
            Out = Data[Position++];
            return true;
        }

        bool ReadUInt32(uint32& Out)
        {
            if (Position + 4 > Data.Num()) return false;
            FMemory::Memcpy(&Out, Data.GetData() + Position, 4);
            Position += 4;
            return true;
        }

        bool ReadInt32(int32& Out)
        {
            if (Position + 4 > Data.Num()) return false;
            FMemory::Memcpy(&Out, Data.GetData() + Position, 4);
            Position += 4;
            return true;
        }

        bool ReadFString(FString& Out)
        {
            uint32 Len = 0;
            if (!ReadUInt32(Len)) return false;
            if (Len == 0) { Out.Empty(); return true; }
            if (Position + (int64)Len > Data.Num()) return false;

            FUTF8ToTCHAR Convert(reinterpret_cast<const ANSICHAR*>(Data.GetData() + Position), Len);
            Out = FString(Convert.Length(), Convert.Get());
            Position += Len;
            return true;
        }

        bool ReadBytes(TArray<uint8>& Out, uint32 Size)
        {
            if (Position + (int64)Size > Data.Num()) return false;
            Out.SetNumUninitialized(Size);
            if (Size > 0)
            {
                FMemory::Memcpy(Out.GetData(), Data.GetData() + Position, Size);
            }
            Position += Size;
            return true;
        }

        bool AtEnd() const { return Position >= Data.Num(); }
        int64 GetPosition() const { return Position; }

    private:
        const TArray<uint8>& Data;
        int64 Position = 0;
    };
}

// ================================================================
// Singleton
// ================================================================

FScenecapsDataDrivenEntryHandler& FScenecapsDataDrivenEntryHandler::Get()
{
    static FScenecapsDataDrivenEntryHandler Instance;
    return Instance;
}

// ================================================================
// Loading
// ================================================================

void FScenecapsDataDrivenEntryHandler::LoadAll()
{
    InternalLoadAll(/*bClearFirst=*/false);
}

void FScenecapsDataDrivenEntryHandler::ReloadAll()
{
    InternalLoadAll(/*bClearFirst=*/true);
}

void FScenecapsDataDrivenEntryHandler::InternalLoadAll(bool bClearFirst)
{
    FScopeLock Guard(&Lock);

    if (bClearFirst)
    {
        Entries.Reset();
        LoadedFiles.Reset();
        Errors.Reset();
    }

    // 读取目录列表
    TArray<FString> DirectoryPaths;
    GConfig->GetArray(TEXT("Scenecaps.DataDrivenExtensions"), TEXT("Directories"),
        DirectoryPaths, GEngineIni);

    if (DirectoryPaths.Num() == 0)
    {
        DirectoryPaths.Add(TEXT("ScenecapsExtensions"));
    }

    const FString ConfigRoot = FPaths::ProjectConfigDir();

    for (const FString& Dir : DirectoryPaths)
    {
        FString FullDir = Dir;
        if (!FullDir.StartsWith(TEXT("/")) && !FullDir.Contains(TEXT(":")))
        {
            FullDir = FPaths::Combine(ConfigRoot, Dir);
        }
        FPaths::NormalizeDirectoryName(FullDir);

        if (!IFileManager::Get().DirectoryExists(*FullDir))
        {
            UE_LOG(LogScenecaps, Verbose,
                TEXT("[Scenecaps.DataDriven] Directory not found: %s"), *FullDir);
            continue;
        }

        // 扫描 .scx
        TArray<FString> Files;
        IFileManager::Get().FindFilesRecursive(Files, *FullDir, TEXT("*.scx"),
            true, false);

        for (const FString& FilePath : Files)
        {
            TArray<FScenecapsSCXEntry> Parsed;
            if (LoadFile(FilePath, Parsed))
            {
                Entries.Append(MoveTemp(Parsed));
                LoadedFiles.Add(FilePath);
            }
        }
    }

    SortEntries();

    static FDelegateHandle ReloadHandle;
    if (!ReloadHandle.IsValid())
    {
        ReloadHandle = FCoreUObjectDelegates::ReloadCompleteDelegate.AddLambda([this](EReloadCompleteReason Reason)
            {
                this->ReloadAll();
            });
    }

    UE_LOG(LogScenecaps, Log,
        TEXT("[Scenecaps.DataDriven] Loaded %d entry(ies) from %d file(s), %d error(s)"),
        Entries.Num(), LoadedFiles.Num(), Errors.Num());
}

bool FScenecapsDataDrivenEntryHandler::LoadFile(
    const FString& FilePath, TArray<FScenecapsSCXEntry>& OutEntries)
{
    TArray<uint8> Data;
    if (!FFileHelper::LoadFileToArray(Data, *FilePath))
    {
        const FString Msg = FString::Printf(TEXT("Cannot read file: %s"), *FilePath);
        Errors.Add(Msg);
        UE_LOG(LogScenecaps, Warning, TEXT("[Scenecaps.DataDriven] %s"), *Msg);
        return false;
    }

    if (!ParseFile(Data, FilePath, OutEntries))
    {
        return false;
    }

    // 解析类
    for (FScenecapsSCXEntry& Entry : OutEntries)
    {
        Entry.SourceFile = FilePath;
        ResolveClass(Entry);

        if (!Entry.ResolvedActorClass)
        {
            const FString Msg = FString::Printf(
                TEXT("%s: cannot resolve ActorClass '%s' (Entry: %s)"),
                *FilePath, *Entry.ActorClassPath, *Entry.Description);
            Errors.Add(Msg);
            UE_LOG(LogScenecaps, Warning, TEXT("[Scenecaps.DataDriven] %s"), *Msg);
        }
    }

    // 过滤掉坏条目
    OutEntries.RemoveAll([](const FScenecapsSCXEntry& E) { return !E.ResolvedActorClass; });
    return OutEntries.Num() > 0;
}

bool FScenecapsDataDrivenEntryHandler::ParseFile(
    const TArray<uint8>& Data, const FString& FilePath,
    TArray<FScenecapsSCXEntry>& OutEntries)
{
    ScenecapsSCX::FReader Reader(Data);

    uint32 Magic = 0, Version = 0, EntryCount = 0, Reserved = 0;
    if (!Reader.ReadUInt32(Magic) || Magic != SCENECAPS_SCX_MAGIC)
    {
        Errors.Add(FString::Printf(TEXT("%s: bad magic"), *FilePath));
        return false;
    }
    if (!Reader.ReadUInt32(Version) || Version != SCENECAPS_SCX_VERSION)
    {
        Errors.Add(FString::Printf(TEXT("%s: unsupported version %u (expected %u)"),
            *FilePath, Version, SCENECAPS_SCX_VERSION));
        return false;
    }
    if (!Reader.ReadUInt32(EntryCount) || !Reader.ReadUInt32(Reserved))
    {
        Errors.Add(FString::Printf(TEXT("%s: truncated header"), *FilePath));
        return false;
    }

    for (uint32 i = 0; i < EntryCount; ++i)
    {
        FScenecapsSCXEntry Entry;
        if (!Reader.ReadFString(Entry.AssetClassPath) ||
            !Reader.ReadFString(Entry.ActorClassPath) ||
            !Reader.ReadInt32(Entry.Priority) ||
            !Reader.ReadFString(Entry.Description))
        {
            Errors.Add(FString::Printf(TEXT("%s: truncated entry #%u"), *FilePath, i));
            return false;
        }

        uint32 PropCount = 0;
        if (!Reader.ReadUInt32(PropCount))
        {
            Errors.Add(FString::Printf(TEXT("%s: truncated prop count #%u"), *FilePath, i));
            return false;
        }

        for (uint32 p = 0; p < PropCount; ++p)
        {
            FScenecapsSCXProperty Prop;
            uint8 ValueType = 0;
            uint32 ValueSize = 0;

            if (!Reader.ReadFString(Prop.Path) ||
                !Reader.ReadUInt8(ValueType) ||
                !Reader.ReadUInt32(ValueSize) ||
                !Reader.ReadBytes(Prop.ValueBytes, ValueSize))
            {
                Errors.Add(FString::Printf(TEXT("%s: truncated prop #%u.%u"), *FilePath, i, p));
                return false;
            }

            Prop.ValueType = (EScenecapsSCXValueType)ValueType;
            Entry.Properties.Add(MoveTemp(Prop));
        }

        OutEntries.Add(MoveTemp(Entry));
    }

    return true;
}

void FScenecapsDataDrivenEntryHandler::ResolveClass(FScenecapsSCXEntry& Entry)
{
    auto Resolve = [](const FString& Path) -> UClass*
        {
            if (Path.IsEmpty()) return nullptr;
            if (UClass* Cls = FindObject<UClass>(nullptr, *Path)) return Cls;
            return LoadObject<UClass>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
        };

    Entry.ResolvedAssetClass = Resolve(Entry.AssetClassPath);
    Entry.ResolvedActorClass = Resolve(Entry.ActorClassPath);

    if (!Entry.ResolvedActorClass || !Entry.ResolvedActorClass->IsChildOf(AActor::StaticClass()))
    {
        Entry.ResolvedActorClass = nullptr;
        return;
    }

    // ★ 核心优化：提前解析所有 Property
    for (FScenecapsSCXProperty& Prop : Entry.Properties)
    {
        FString ComponentName;
        FString PropertyName;

        int32 DotIndex = INDEX_NONE;
        if (Prop.Path.FindChar('.', DotIndex))
        {
            ComponentName = Prop.Path.Left(DotIndex);
            PropertyName = Prop.Path.Mid(DotIndex + 1);
        }
        else
        {
            PropertyName = Prop.Path;
        }

        Prop.CachedComponentName = ComponentName.IsEmpty() ? NAME_None : FName(*ComponentName);
        Prop.CachedPropertyName = FName(*PropertyName);

        // 如果不需要查组件，直接查 Actor 本身的属性
        UClass* TargetClass = Entry.ResolvedActorClass;

        // 如果指定了组件，我们需要在 Actor 的所有组件类里找这个属性
        // 这里的逻辑是：只要 Actor 包含了这个组件类，我们就缓存对应的 FProperty
        if (!Prop.CachedComponentName.IsNone())
        {
            bool bFoundComponentProperty = false;
            for (TFieldIterator<FObjectProperty> It(Entry.ResolvedActorClass); It; ++It)
            {
                if (It->PropertyClass && It->PropertyClass->IsChildOf(UActorComponent::StaticClass()))
                {
                    if (FProperty* FoundProp = FindFProperty<FProperty>(It->PropertyClass, Prop.CachedPropertyName))
                    {
                        Prop.CachedProperty = FoundProp;
                        bFoundComponentProperty = true;
                        break; // 找到了就跳出
                    }
                }
            }

            if (!bFoundComponentProperty)
            {
                UE_LOG(LogScenecaps, Warning, TEXT("[DataDriven] 无法在 %s 的组件中找到属性 %s"), *Entry.ActorClassPath, *Prop.Path);
            }
        }
        else
        {
            // 直接查 Actor 的属性
            Prop.CachedProperty = FindFProperty<FProperty>(Entry.ResolvedActorClass, Prop.CachedPropertyName);
            if (!Prop.CachedProperty)
            {
                UE_LOG(LogScenecaps, Warning, TEXT("[DataDriven] 无法在 %s 中找到属性 %s"), *Entry.ActorClassPath, *Prop.Path);
            }
        }
    }
}

void FScenecapsDataDrivenEntryHandler::SortEntries()
{
    Entries.Sort([](const FScenecapsSCXEntry& A, const FScenecapsSCXEntry& B)
        {
            return A.Priority > B.Priority;
        });
}

int32 FScenecapsDataDrivenEntryHandler::GetEntryCount() const
{
    FScopeLock Guard(&Lock);
    return Entries.Num();
}

TArray<FScenecapsSCXEntry> FScenecapsDataDrivenEntryHandler::GetEntries() const
{
    FScopeLock Guard(&Lock);
    return Entries; // Return a copy while holding the lock
}

TArray<FString> FScenecapsDataDrivenEntryHandler::GetLoadedFiles() const
{
    FScopeLock Guard(&Lock);
    return LoadedFiles;
}

TArray<FString> FScenecapsDataDrivenEntryHandler::GetErrors() const
{
    FScopeLock Guard(&Lock);
    return Errors;
}

// ================================================================
// Handler interface
// ================================================================

const FScenecapsSCXEntry* FScenecapsDataDrivenEntryHandler::FindMatchingEntry(
    const UObject* Asset) const
{
    if (!Asset) return nullptr;

    // 注：调用方需自行加锁（CanHandleAsset / SpawnActor 各自加锁）
    UClass* AssetClass = Asset->GetClass();
    for (const FScenecapsSCXEntry& Entry : Entries)
    {
        if (!Entry.ResolvedAssetClass) continue;
        if (AssetClass->IsChildOf(Entry.ResolvedAssetClass))
        {
            return &Entry;
        }
    }
    return nullptr;
}

bool FScenecapsDataDrivenEntryHandler::CanHandleAsset(const UObject* Asset) const
{
    FScopeLock Guard(&Lock);
    return FindMatchingEntry(Asset) != nullptr;
}

AActor* FScenecapsDataDrivenEntryHandler::SpawnActor(
    const FScenecapsSpawnContext& Context, UObject* Asset) const
{
    if (!Context.World) return nullptr;

    // 拷贝一份 Entry（因为我们要跨锁使用指针 + 避免持有锁期间 spawn）
    FScenecapsSCXEntry LocalEntry;
    {
        FScopeLock Guard(&Lock);
        const FScenecapsSCXEntry* Found = FindMatchingEntry(Asset);
        if (!Found) return nullptr;
        LocalEntry = *Found;
    }

    if (!LocalEntry.ResolvedActorClass) return nullptr;

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Params.ObjectFlags |= RF_Transactional;

    AActor* Spawned = Context.World->SpawnActor<AActor>(
        LocalEntry.ResolvedActorClass, Context.Transform, Params);
    if (!Spawned) return nullptr;

    for (const FScenecapsSCXProperty& Prop : LocalEntry.Properties)
    {
        ApplyPropertyOverride(Spawned, Prop);
    }

    ApplyEntryOverrides(Spawned, Context);
    return Spawned;
}

UObject* FScenecapsDataDrivenEntryHandler::ResolveAssetForStorage(
    UObject* DroppedObject) const
{
    if (!DroppedObject) return nullptr;

    FScopeLock Guard(&Lock);
    UClass* DroppedClass = DroppedObject->GetClass();
    for (const FScenecapsSCXEntry& Entry : Entries)
    {
        if (!Entry.ResolvedAssetClass) continue;
        if (DroppedClass->IsChildOf(Entry.ResolvedAssetClass))
        {
            return DroppedObject;
        }
    }
    return nullptr;
}

UObject* FScenecapsDataDrivenEntryHandler::ResolveAssetFromActor(AActor* Actor) const
{
    // 数据驱动方案下无法通用地从 Actor 反推资产实例，返回 nullptr
    // → GatherFromActors 会 fallback 到 Actor->GetClass()
    return nullptr;
}

// ================================================================
// Property application
// ================================================================

void FScenecapsDataDrivenEntryHandler::ApplyPropertyOverride(
    UObject* Target, const FScenecapsSCXProperty& Prop) const
{
    if (!Target || !Prop.CachedProperty) return;

    UObject* PropTarget = Target;

    // ★ 使用缓存的 FName 找组件
    if (!Prop.CachedComponentName.IsNone())
    {
        AActor* OwnerActor = Cast<AActor>(Target);
        if (!OwnerActor) return;

        TInlineComponentArray<UActorComponent*> Components;
        OwnerActor->GetComponents(Components);

        for (UActorComponent* Comp : Components)
        {
            if (Comp && Comp->GetFName() == Prop.CachedComponentName)
            {
                PropTarget = Comp;
                break;
            }
        }

        if (PropTarget == Target)
        {
            UE_LOG(LogScenecaps, Warning,
                TEXT("[DataDriven] Component '%s' not found on %s for property '%s'"),
                *Prop.CachedComponentName.ToString(), *Target->GetName(), *Prop.Path);
            return; // 没找到组件直接返回
        }
    }

    // ★ 直接使用缓存的 FProperty
    ApplyValueToProperty(PropTarget, Prop.CachedProperty, Prop);
}

bool FScenecapsDataDrivenEntryHandler::ApplyValueToProperty(
    UObject* Target, FProperty* Property, const FScenecapsSCXProperty& Prop) const
{
    const uint8* Bytes = Prop.ValueBytes.GetData();
    const int32 Size = Prop.ValueBytes.Num();

    switch (Prop.ValueType)
    {
    case EScenecapsSCXValueType::Bool:
    {
        FBoolProperty* BoolProp = CastField<FBoolProperty>(Property);
        if (!BoolProp || Size < 1) return false;
        BoolProp->SetPropertyValue_InContainer(Target, Bytes[0] != 0);
        return true;
    }
    case EScenecapsSCXValueType::Int32:
    {
        FIntProperty* IntProp = CastField<FIntProperty>(Property);
        if (!IntProp || Size < 4) return false;
        int32 Value;
        FMemory::Memcpy(&Value, Bytes, 4);
        IntProp->SetPropertyValue_InContainer(Target, Value);
        return true;
    }
    case EScenecapsSCXValueType::Float:
    {
        FFloatProperty* FloatProp = CastField<FFloatProperty>(Property);
        if (!FloatProp || Size < 4) return false;
        float Value;
        FMemory::Memcpy(&Value, Bytes, 4);
        FloatProp->SetPropertyValue_InContainer(Target, Value);
        return true;
    }
    case EScenecapsSCXValueType::String:
    {
        FStrProperty* StrProp = CastField<FStrProperty>(Property);
        if (!StrProp || Size < 4) return false;
        uint32 Len;
        FMemory::Memcpy(&Len, Bytes, 4);
        if (Size < 4 + (int32)Len) return false;
        FUTF8ToTCHAR Convert(reinterpret_cast<const ANSICHAR*>(Bytes + 4), Len);
        FString Value(Convert.Length(), Convert.Get());
        StrProp->SetPropertyValue_InContainer(Target, Value);
        return true;
    }
    case EScenecapsSCXValueType::Vector:
    {
        FStructProperty* StructProp = CastField<FStructProperty>(Property);
        if (!StructProp || StructProp->Struct != TBaseStructure<FVector>::Get() || Size < 24) return false;
        FVector Value;
        FMemory::Memcpy(&Value, Bytes, 24);
        StructProp->CopyCompleteValue_InContainer(Target, &Value);
        return true;
    }
    case EScenecapsSCXValueType::Rotator:
    {
        FStructProperty* StructProp = CastField<FStructProperty>(Property);
        if (!StructProp || StructProp->Struct != TBaseStructure<FRotator>::Get() || Size < 24) return false;
        FRotator Value;
        FMemory::Memcpy(&Value, Bytes, 24);
        StructProp->CopyCompleteValue_InContainer(Target, &Value);
        return true;
    }
    case EScenecapsSCXValueType::Transform:
    {
        FStructProperty* StructProp = CastField<FStructProperty>(Property);
        if (!StructProp || StructProp->Struct != TBaseStructure<FTransform>::Get() || Size < 80) return false;
        FTransform Value;
        FMemory::Memcpy(&Value, Bytes, 80);
        StructProp->CopyCompleteValue_InContainer(Target, &Value);
        return true;
    }
    case EScenecapsSCXValueType::ObjectPath:
    {
        if (Size < 4) return false;
        uint32 Len;
        FMemory::Memcpy(&Len, Bytes, 4);
        if (Size < 4 + (int32)Len) return false;
        FUTF8ToTCHAR Convert(reinterpret_cast<const ANSICHAR*>(Bytes + 4), Len);
        FString PathStr(Convert.Length(), Convert.Get());

        if (FSoftObjectProperty* SoftProp = CastField<FSoftObjectProperty>(Property))
        {
            // ★ 修复：分开构造，避免 explicit 转换错误和 Most Vexing Parse
            FSoftObjectPath NewPath(PathStr);
            FSoftObjectPtr NewValue(NewPath);

            SoftProp->SetPropertyValue_InContainer(Target, NewValue);
            return true;
        }
        if (FObjectProperty* ObjProp = CastField<FObjectProperty>(Property))
        {
            UObject* Loaded = LoadObject<UObject>(nullptr, *PathStr);
            ObjProp->SetObjectPropertyValue_InContainer(Target, Loaded);
            return true;
        }
        return false;
    }
    }

    return false;
}
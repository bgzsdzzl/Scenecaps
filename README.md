# Scenecaps - Scene Collection Plugin for Unreal Engine

A powerful scene collection and level assembly plugin for Unreal Engine 5, designed for non-destructive scene composition, variant management, and procedural generation.

## Features

### Scene Collection & Composition
- **Non-destructive scene assembly** - Build complex scenes from modular entries without modifying source assets
- **Hierarchical entries** - Parent-child transform hierarchy with inheritance
- **Per-entry overrides** - Transform, visibility, materials, custom parameters per entry
- **Instance-based rendering** - Automatic ISM/HISM batching for performance

### Variant System
- **Named variants** - Create multiple scene configurations (Day/Night, Seasons, etc.)
- **Sparse overrides** - Only store differences from base entries
- **Runtime interpolation** - Smooth transitions between variants with easing curves
- **Blueprint API** - Full variant control at runtime

### Procedural Generation
- **Biome system** - Rule-based procedural placement with slope/height/mask constraints
- **Multiple distributions** - Grid, Poisson Disk, Noise Cluster, Radial patterns
- **Custom parameters** - Float/Vector/Color/Bool parameters passed to materials/blueprints
- **Runtime & Editor preview** - Generate in-editor or at runtime

### Modifier System
- **Extensible modifiers** - Transform, Material, Visibility, Custom Parameters
- **Phase-based execution** - PreSpawn, PostSpawn, PerEntry phases
- **Priority system** - Control execution order
- **Blueprint & C++ API** - Create custom modifiers

### Editor Tools
- **Outliner** - Hierarchical entry management with drag-drop reparenting
- **Variant UI** - Visual variant switching and override management
- **Batch editing** - Multi-select property editing
- **Snapshot recorder** - Capture level actors as scene collection
- **Custom viewport** - Per-object point lights, no global sky/atmosphere

### World Partition Integration
- **Data Layer integration** - Automatic Data Layer assignment
- **Spatial rules** - Tag/bounds-based Data Layer assignment
- **HLOD registration** - ISM/HISM to HLOD conversion

## Requirements

- Unreal Engine 5.0+
- C++17 compatible compiler
- Niagara (optional, for Niagara entry support)

## Installation

1. Clone or download this repository
2. Place in your project's `Plugins/` directory
3. Regenerate project files (right-click .uproject → Generate Visual Studio Project Files)
4. Build and enable the plugin in Editor → Plugins

## Quick Start

1. Create a **Scenecaps Asset** (Content Browser → Right-click → Scenecaps → Scenecaps Asset)
2. Add entries by dragging assets (Static Meshes, Blueprints, Materials, Sounds, etc.)
3. Organize hierarchy in the Outliner
4. Create Variants for different configurations
3. Place **Scenecaps Actor** in level and assign the asset
4. Use Variants at runtime via Blueprint or C++

## Architecture

```
Scenecaps (Runtime Module)
├── Core
│   ├── ScenecapsAsset          # Main data asset
│   ├── ScenecapsEntry          # Scene entry with transforms/overrides
│   ├── ScenecapsVariant        # Variant with sparse overrides
│   └── ScenecapsActor          # Runtime actor for spawning
├── Entry System
│   ├── Entry Handlers          # Extensible asset→actor spawners
│   └── Entry Registry          # Handler registration
├── Modifier System
│   ├── Modifier Manager        # Phase-based execution
│   ├── Built-in Modifiers      # Transform, Material, Visibility, Custom Params
│   └── Modifier Library        # Blueprint factory functions
├── Variant System
│   ├── Variant Interpolation   # Smooth transitions
│   └── Transition Component    # Runtime playback
├── Procedural
│   ├── Biome Asset             # Rule-based generation
│   └── Biome Sampler           # Runtime generation
└── Utilities
    ├── Snapshot Recorder       # Level → Scene capture
    ├── Actor Pool              # Runtime object pooling
    └── ISM/HISM Conversion     # Actor ↔ Instanced mesh conversion

ScenecapsEditor (Editor Module)
├── Viewport
│   ├── Custom PreviewScene     # No sky/atmosphere, per-object point lights
│   ├── Custom ViewportClient   # Sky/atmosphere rendering disabled
│   └── Per-object Point Lights # 150 intensity, 300 radius, warm white
├── Outliner
│   ├── Hierarchical tree       # Drag-drop reparenting
│   ├── Variant management      # Add/remove from variants
│   └── Batch editing           # Multi-property editing
├── Asset Proxy                 # Editor-side asset manipulation
├── Snapshot Recorder UI        # Level → Scene capture
└── Property Customizations     # Custom detail panels
```

## Blueprint API

### Scenecaps Actor
```cpp
// Runtime variant switching
actor->SetActiveVariant(FName("Night"), true);

// Apply modifiers at runtime
actor->ApplyModifiers(EScenecapsModifierPhase::PostSpawn, "Night");

// Query spawned actors
TArray<AActor*> Actors = actor->GetSpawnedActorArray();
```

### Variant Transition Component
```cpp
// Play transition
transition->PlayFromTo("Day", "Night", 2.0f);

// Control playback
transition->Pause();
transition->Resume();
transition->SetAlpha(0.5f);
```

### Modifier Library (Blueprint)
```cpp
// Create modifiers
UScenecapsTransformModifier* TransMod = UScenecapsModifierLibrary::CreateTransformModifier(FVector(0,0,100), FRotator(0,90,0), FVector(1.2));
UScenecapsMaterialModifier* MatMod = UScenecapsModifierLibrary::CreateMaterialModifier(MyMaterial, 0);
UScenecapsVisibilityModifier* VisMod = UScenecapsModifierLibrary::CreateVisibilityModifier(false, "HiddenTag", true);
```

## Configuration

### Project Settings
- **Scenecaps Data Driven Extensions** - Paths for .scx data-driven entries
- **Default Preview Settings** - Default point light intensity/radius/color

### Editor Preferences
- **Viewport Lighting** - Point light intensity/radius/color per object
- **Viewport Grid** - Grid size/color customization

## Extending

### Custom Entry Handler (C++)
```cpp
class FMyCustomEntryHandler : public IScenecapsEntryHandler
{
public:
    virtual bool CanHandleAsset(const UObject* Asset) const override;
    virtual AActor* SpawnActor(const FScenecapsSpawnContext& Context, UObject* Asset) const override;
    virtual UObject* ResolveAssetForStorage(UObject* DroppedObject) const override;
};
// Register in module startup:
FScenecapsEntryRegistry::Get().RegisterHandler(MakeShareable(new FMyCustomEntryHandler()));
```

### Custom Modifier (C++)
```cpp
UCLASS()
class UMyCustomModifier : public UScenecapsModifierBase
{
    GENERATED_BODY()
public:
    virtual void Execute(FScenecapsModifierContext& Context) override;
    virtual bool CanExecute(const FScenecapsModifierContext& Context) const override;
};
```

## Performance

- **ISM/HISM Batching** - Automatic instancing for static meshes
- **Lumen Ready** - Distance field lighting, custom primitive data
- **World Partition Ready** - Data Layer integration, HLOD support
- **Object Pooling** - Runtime actor pooling for frequent spawn/destroy

## License

MIT License - see [LICENSE](LICENSE) for details.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Commit your changes
4. Open a Pull Request

## Support

- **Issues** - GitHub Issues for bugs/features
- **Discussions** - GitHub Discussions for questions/ideas
- **Wiki** - Documentation and tutorials

## Credits

Built with ❤️ for the Unreal Engine community.
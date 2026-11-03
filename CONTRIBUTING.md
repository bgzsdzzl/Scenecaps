# Scenecaps - Contributing Guide

Thank you for your interest in contributing to Scenecaps! This document outlines the process and guidelines for contributing.

## Getting Started

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/your-feature-name`
3. Make your changes
4. Run tests and ensure build passes
5. Submit a Pull Request

## Development Setup

1. Clone the repository
2. Open `Scenecaps.uplugin` in Unreal Engine 5.8
3. Build the project in Visual Studio / Rider
4. Run the editor to verify changes

## Coding Standards

### C++ Style
- Follow Unreal Engine Coding Standard
- Use `PascalCase` for classes, functions, types
- Use `camelCase` for variables, parameters
- Use `SCENECAPS_API` for public API
- Include module header first, then engine headers, then third-party

### Naming Conventions
- Classes: `PascalCase` (e.g., `UScenecapsAsset`)
- Functions: `PascalCase` (e.g., `GetSpawnedActors()`)
- Variables: `camelCase` (e.g., `spawnedActors`)
- Constants: `UPPER_SNAKE_CASE` (e.g., `MAX_SPAWN_COUNT`)
- Private members: prefix with underscore `_cachedActor`
- Boolean functions: prefix with `b` or `Is/Has/Can` (e.g., `bIsPlaying`, `IsValid()`)

### Code Organization
- Public API in `Public/` headers with `SCENECAPS_API`
- Private implementation in `Private/`
- Module Build.cs must declare all dependencies
- Use `WITH_EDITOR` / `WITH_EDITORONLY_DATA` for editor-only code

### Unreal Engine Specific
- Use `TObjectPtr<>` / `TWeakObjectPtr<>` / `TSoftObjectPtr<>` for UObject pointers
- Use `UPROPERTY()` for garbage-collected members
- Use `UFUNCTION()` for Blueprint-exposed functions
- Use `UCLASS()`, `USTRUCT()`, `UENUM()` with proper specifiers
- Prefer `TArray<>`, `TMap<>`, `TSet<>` over std containers

## Pull Request Process

1. **Branch naming**: `feature/`, `fix/`, `docs/`, `refactor/`, `test/`
2. **Commit messages**: Follow conventional commits
   - `feat: add per-object point lights to preview`
   - `fix: disable default lighting in preview scene`
   - `docs: update README with variant system`
3. **PR Requirements**:
   - All CI checks pass (build, static analysis)
   - Code follows style guidelines
   - No compiler warnings introduced
   - Relevant tests added/updated
   - Documentation updated if API changed
4. **Review**: At least one maintainer approval required

## Testing

- Test in Editor (Development) and Game (Development/Shipping)
- Test both Runtime and Editor modules
- Verify no memory leaks with `obj list` / `obj refs`
- Test with and without Niagara plugin enabled

## Code Review Checklist

- [ ] Compiles without warnings (Development + Shipping)
- [ ] Follows UE coding standard
- [ ] No memory leaks / GC issues
- [ ] Editor-only code wrapped in `WITH_EDITOR` / `WITH_EDITORONLY_DATA`
- [ ] Blueprint API marked with `UFUNCTION(BlueprintCallable)` etc.
- [ ] No raw `new`/`delete` for UObjects
- [ ] No blocking operations on game thread
- [ ] Documentation updated if public API changed

## Issue Reporting

- Use GitHub Issues for bugs and feature requests
- Include: UE version, plugin version, reproduction steps
- For crashes: include call stack and log excerpt

## Code of Conduct

Be respectful, inclusive, and constructive. See [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md).

## License

By contributing, you agree that your contributions will be licensed under the Apache License 2.0.
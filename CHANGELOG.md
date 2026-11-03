# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Editor preview viewport with per-object point lights (no global sky/atmosphere)
- Variant transition system with interpolation support
- Biome procedural generation system (Grid, Poisson Disk, Noise Cluster, Radial)
- Data Layer integration for World Partition
- Snapshot recorder (capture level actors as scene collection)
- Actor pool system for runtime pooling
- Data-driven entry handlers (.scx format)
- Modifier system with phase-based execution
- Variant transition component with interpolation

### Changed
- Editor preview: completely removed sky/atmosphere/global lights
- Editor preview: per-object point lights (150 intensity, 300 radius)
- CVS.bDefaultLighting = false in PreviewScene construction
- Disabled SkyLight, DirectionalLight, SkyAtmosphere in editor viewport

### Fixed
- Editor preview lighting (was too bright due to default lighting)
- Variant transition ApplyInterpolationToAsset/ISM implementations
- BiomeSampler GenerateToISM/HISM implementations
- DataLayer code wrapped in WITH_EDITOR for Game target
- SnapshotRecorder returns nullptr for Game target

### Removed
- Global SkyLight, DirectionalLight, SkyAtmosphere from editor preview

## [1.0.0] - 2024-XX-XX

### Added
- Initial release
- Scene collection asset system
- Variant system with sparse overrides
- ISM/HISM batch spawning
- Blueprint and C++ API

---

## Versioning Guidelines

This project follows [Semantic Versioning](https://semver.org/):

- **MAJOR** version for incompatible API changes
- **MINOR** version for backward-compatible functionality
- **PATCH** version for backward-compatible bug fixes

### Release Types

- **Major** (x.0.0): Breaking API changes, major architecture changes
- **Minor** (x.y.0): New features, backward compatible
- **Patch** (x.y.z): Bug fixes, backward compatible

### Pre-release Versions

- **Alpha** (x.y.z-alpha.N): Early testing, unstable
- **Beta** (x.y.z-beta.N): Feature complete, testing
- **Release Candidate** (x.y.z-rc.N): Ready for release, final testing

## Release Process

1. Update version in `Scenecaps.uplugin` (Version, VersionName)
2. Update `CHANGELOG.md` with release notes
3. Create git tag: `git tag v1.0.0`
4. Push tag: `git push origin v1.0.0`
5. GitHub Actions creates release with artifacts
6. Update `CHANGELOG.md` with final release notes
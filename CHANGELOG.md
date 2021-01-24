# Changelog

## 1.4.0

- Allow Raygun to be built as shared library, see `docs/shared_library.md`.

## 1.3.0 Cross-Vendor Ray Tracing

The ray tracer has been migrated to the, now standardized, Vulkan ray tracing extension.
This enables Raygun to also work on non-Nvidia hardware, given Vulkan ray tracing is supported.

- Rewrite shader binding table creation.
- Fix a copy-paste error in Raygun::computeSystem.
- Fix build system dependencies between shader and header files.
- Add video demo link to README.
- Re-enable multi processor compilication and LTC for Windows builds.
- Use PhysX fork for better out-of-the-box experience.
- Update Vulkan SDK to 1.2.162.
- Upgrade fmt to 6.1.2.

## 1.2.0 Render System Cleanup

- Remove experimental headless mode
- Replace reference OpenAL implementation with OpenAL Soft
- Use fade out effect on quit
- Update Vulkan SDK to 1.2.131
- CMake: Visual Studio solutions now feature debug symbols in release builds
- CMake: Visual Studio solutions use warning level 3
- Fix render pass attachment initial layout warning
- Attach names to commonly used Vulkan objects.
  These names are used by the validation layer.
- GPU should now be properly synced.
  Lots of barriers have been added.

## 1.1.0 Linux Support

The biggest change in this version is the migration from Visual Studio solutions to CMake.

With this change, Raygun can now be built on Linux.
Small adjustments were made to the code in order to fully support Linux.

- Use different application icon colour depending on build type
- Fix uninitialised data arrays in profiler
- Fix Vulkan resources use-after-free bug on engine shutdown
- Improve handling of window resize events

## 1.0.0 Initial Release

This is the first public release of Raygun. 🎉

While there is still more clean-up to be done, the overall architecture is not going to change much in the near future.
At this moment Windows is the only supported platform.
Our plan for version *1.1* is to switch form Visual Studio solutions to CMake and support Linux as well.

In addition to this, some parts of the render system are not yet properly synchronised.
So far we did not encounter any issues on our test systems, but we plan to fix missing synchronisation in version *1.2*.

Please see the related issues for more information.

- Models: Remove duplicated vertices from Raygun logo
- Fix crash due to missing pointer initialization (`~TopLevelAS`)

## 1.0.0rc3

- Add Raygun logo
- Models: Add decent walls to room
- Models: Add PH3 Games logo
- Audio: Replace background music track

## 1.0.0rc2

- Fix legacy paths in .gitattributes
- Fix OpenAL32.dll (was 32 bit, now 64 bit)

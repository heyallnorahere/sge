# sge ![build status](https://img.shields.io/github/workflow/status/yodasoda1219/sge/build) ![license](https://img.shields.io/github/license/yodasoda1219/sge)

Simple 2D game engine in C++17.
Please use [this project](https://github.com/yodasoda1219/TestProject) for testing.

**Remember:** SGE is currently in development. Stuff will break!

```bash
# configure
cmake --preset default

# build
cmake --build --preset default
```

## Adding a new asset type

To add a new asset type:
- Add an entry to `sge::asset_type` in [`sge/asset/asset.h`](sge/src/sge/asset/asset.h)
- Add a translation in [`sge/asset/asset_registry.cpp`](sge/src/sge/asset/asset_registry.cpp)
- Add a serializer in [`sge/asset/asset_serializers.cpp`](sge/src/sge/asset/asset_serializers.cpp)
- Add an entry to `SGE.AssetType` in [`Asset.cs`](csharp/SGE.Scriptcore/Asset.cs) to avoid enumeration conflicts
- Optional: add an extension entry in [`panels/content_browser_panel.cpp`](sgm/src/panels/content_browser_panel.cpp)
These are prebuild versions of mono v6.0.1 via the dotnet runtime

To build:
1. Check out the repo at the tag (https://github.com/dotnet/runtime/releases/tag/v6.0.1).
1. Make sure you have the dotnet SDK installed (down to the patch version) specified in `global.json` in that repo
1. Make sure that all dependencies are installed. Documented here: https://github.com/dotnet/runtime/blob/main/docs/workflow/README.md.
1. `./build.sh mono -c release`
1. Copy `libcoreclr.dylib` and `libmonosgen-2.0.a` from `artifacts/obj/mono/<platform>.Release/out/lib`
These are prebuilt versions of mono `v6.0.1` via the dotnet runtime

To build:
1. Check out the repo at the tag (https://github.com/dotnet/runtime/releases/tag/v6.0.1).
1. Make sure you have the dotnet SDK installed (down to the patch version) specified in `global.json` in that repo
1. Make sure that all dependencies are installed. Documented here: https://github.com/dotnet/runtime/blob/main/docs/workflow/README.md.
1. `./build.sh mono+libs -c release`
1. Copy `*coreclr.*` and `*monosgen-2.0.*` from their output directories into the applicable `lib/` subdirectories. Please follow the directory structure.
1. Copy `artifacts/bin/runtime/net6.0-OSX-Release-arm64/*.dylib` to `sge/assets/dotnet/unix/ARM64/`
# Building SAMPC

Everything below has been run and verified except where it says otherwise.

Two targets matter: the server (`mpsvr` on linux, `server.exe` on windows) and the
client (`samp.dll`, windows only). The server builds with nothing but a compiler.
The client needs three files that cannot live in this repository.


## Server, linux

32 bit only. The sync structs in `sdk/shared.h` cross the wire raw via `sizeof()`
and the plugin ABI is 32 bit, so a 64 bit build would not interoperate.

```sh
# fedora
sudo dnf install glibc-devel.i686 libstdc++-devel.i686
# debian, ubuntu
sudo apt install gcc-multilib g++-multilib

make -C server -j"$(nproc)"      # produces server/mpsvr
make -C announce -j"$(nproc)"    # produces announce/announce
```

`make debug` builds `mpsvrd` with `-O0 -g3` into a separate object directory.

Do not add `-fpack-struct`. The old Makefile had it and it broke two things at
once: it changed the wire layout away from what MSVC produces, and it gave
`std::istringstream` a packed layout that the prebuilt libstdc++ does not share,
which smashed the stack in `CPlugins::LoadPlugins`.


## Server, windows

```
msbuild server/server.sln /p:Configuration=Release /p:Platform=x86
```

Note `x86`, not `Win32`. The solution configurations are named `Debug|x86` and
`Release|x86`, and they map to the project platform `Win32`. Passing `Win32` to
the `.sln` fails with `MSB4126`.

Output lands in `server/Release/server.exe`.


## Client, windows

Toolchain: Visual Studio 2019 or newer. `client.vcxproj` asks for platform
toolset v142, override with `/p:PlatformToolset=` if you have a different one.

Three dependencies are missing from the repository because `*.lib` is gitignored
and D3DX is not redistributable:

| file | goes to | from |
| --- | --- | --- |
| d3dx9 headers and `d3dx9.lib` | `$(DXSDK_DIR)Include` and `$(DXSDK_DIR)Lib\x86` | any legacy DirectX SDK |
| `bass.lib` | `sdk/bass/bass.lib` | un4seen |
| `detours.lib` | `sdk/detours/detours.lib` | Microsoft Detours |

`DXSDK_DIR` is consumed unseparated as `$(DXSDK_DIR)Include`, so it must end with
a path separator.

Only `d3dx9.lib` has to come from the legacy SDK. `d3d9.lib` and `dxguid.lib` ship
with the modern Windows SDK.

If your SDK predates the `DxErr.h` rename it will have `dxerr9.h` instead. Add a
one line `DxErr.h` beside it that includes `dxerr9.h`, because
`sdk/dxut/dxstdafx.h` asks for the newer name. No dxerr library is needed, DXUT's
`DXTrace` is redirected to `DXTraceWrapper` in `sdk/dxut/DXUTmisc.cpp`.

```
msbuild client/client.sln /p:Configuration=Release /p:Platform=x86
```

Output lands in `client/Release/samp.dll`.


## Client, cross compiling on linux with msvc-wine

This works and is how the current `samp.dll` was verified. It needs
[msvc-wine](https://github.com/mstorsjo/msvc-wine) installed, `/opt/msvc` below.

Stage the d3dx9 pieces into a directory shaped like a DirectX SDK:

```sh
DX=~/dxsdk-sampc
mkdir -p $DX/Include $DX/Lib/x86
cp /path/to/sdk/d3dx9*.h /path/to/sdk/d3dx9math.inl /path/to/sdk/dxerr9.h $DX/Include/
printf '#pragma once\n#include <dxerr9.h>\n' > $DX/Include/DxErr.h
cp /path/to/sdk/d3dx9.lib $DX/Lib/x86/
```

Drop the other two libs in place:

```sh
cp /path/to/bass.lib    sdk/bass/bass.lib
cp /path/to/detours.lib sdk/detours/detours.lib
```

Build. `DXSDK_DIR` has to be a windows path because MSBuild concatenates it, and
the trailing backslash is required:

```sh
export PATH=/opt/msvc/bin/x86:$PATH
export DXSDK_DIR='Z:\home\youruser\dxsdk-sampc\'
msbuild client/client.sln /p:Configuration=Release /p:Platform=x86 \
        /p:PlatformToolset=v145
```

Pick the `PlatformToolset` your msvc-wine actually has, list them with
`ls /opt/msvc/MSBuild/Microsoft/VC/*/Platforms/Win32/PlatformToolsets/`.

MSBuild under wine takes roughly ten minutes for the client, so run it detached.

`launch3` does not build this way. It is MFC and msvc-wine does not ship the MFC
libraries, so `samp_debug.exe` is skipped. Everything downstream treats it as
optional.


## What a user still needs at runtime

`samp.dll` alone is an update, not an install. Check what it actually imports:

```
dumpbin /dependents client/Release/samp.dll
```

The current build wants `d3dx9_25.dll` from the DirectX end user runtime,
`BASS.dll` from un4seen, and `MSVCP140.dll` plus `VCRUNTIME140.dll` because
`client.vcxproj` links the dynamic CRT. Stock SA-MP linked the CRT statically and
needed none of the last two. Switch `RuntimeLibrary` to `MultiThreaded` if you
would rather not ship a redistributable.

A full install also needs `samp.exe`, the Delphi server browser under `exgui/`,
and `samp.saa`, built from `archive/filelist.txt` with `arctool2` out of GTA:SA
data files. Neither is produced by CI. See `nsis/samp.nsi` for the complete file
list the original installer shipped.


## CI

Both workflows are manual only, under Actions.

`ci.yml` builds and smoke tests. Its `client-windows` job stays skipped until the
`CLIENT_DEPS_URL` repository variable points at a zip laid out as:

```
include/     the d3dx9 headers, d3dx9math.inl, dxerr9.h, DxErr.h
lib/x86/     d3dx9.lib, bass.lib, detours.lib
```

Set it under Settings, Secrets and variables, Actions, Variables. That archive
contains third party binaries with their own licence terms, so host it somewhere
you control rather than publishing it.

`release.yml` takes a version, builds everything, writes release notes from
`git log` since the previous tag, and attaches the zips that `dist/package.sh`
produces. Without `CLIENT_DEPS_URL` it still publishes both server zips and says
in the notes why the client is absent.
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

RakNet's `DataStructures::List::Insert` used to call `memcpy(new_array, listArray,
0)` on the first insert, with `listArray` still null. `memcpy` is declared
`nonnull`, so GCC concluded the pointer could not be null and dropped the null
check from the `delete[] listArray` right after it, then faulted reading the array
cookie at `listArray[-1]`. MSVC does not make that inference, which is why only the
linux server crashed, and only once a client connected and `RangeList` grew its
first ACK range. The memcpy calls in `DS_List.h` are guarded by `list_size` now.

## Server, windows

```text
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

```text
msbuild client/client.sln /p:Configuration=Release /p:Platform=x86
```

Output lands in `client/Release/samp.dll`.

## Tools, windows

Two standalone utilities, no dependencies beyond the Windows SDK, both static CRT:

```text
msbuild rcon/rcon.vcxproj         /p:Configuration=Release /p:Platform=Win32
msbuild arctool2/arctool2.vcxproj /p:Configuration=Release /p:Platform=Win32
```

These are targeted as `.vcxproj` rather than through a solution, so there is no
solution platform name to get wrong. Both replaced dead `.vcproj` files that
MSBuild has not been able to read since VS 2010.

`arctool2` builds `samp.saa`. It compiles `client/archive` with `ARCTOOL` defined,
which unlocks the key generation and archive writing half that the client itself
never compiles, and it links `advapi32.lib` for the CryptoAPI calls. Its sources
used to `#include "../saco/archive/..."`, a path from the original SA-MP tree that
does not exist here.

## What cannot be built, and why

| project | produces | status |
| --- | --- | --- |
| `exgui/` | `samp.exe`, the server browser | Delphi with VCL. No free compiler exists, nothing on a GitHub runner can build it. |
| `exgui_lazarus/` | same, ported | Builds with `lazbuild`, and is now usable rather than a mockup: it queries servers, fills the server, player and rules views, and remembers the nickname and the favourites list. Only 2 of the Delphi project's 8 forms were ported, so Remote Console, Server Properties, Settings and Help are still empty handlers. Not a CI target, `lazbuild` is not on the runners. |
| `launch3/` | `samp_debug.exe` | MFC. Builds on a GitHub runner, fails under msvc-wine which ships no MFC libraries. Already wired up as optional. |
| `testbot/` | `bot.exe` | A development bot. Its `.vcproj` references `..\raknet`, a path from the original tree, and pulls in a much larger RakNet set than `sdk/raknet` provides. |
| `quicklauncher/` | `quick_launcher.exe` | An AutoIt `.au3` script, needs Aut2Exe. |

`samp.saa` is not a CI target either, only because `archive/filelist.txt` references
GTA:SA data files. Everything else it needs is in the tree, `archive/files/`,
`scm/main.scm`, `scm/script.img` and the key pair, so with the game data on hand:

```text
cd archive
arctool2.exe -c filelist.txt build/samp.saa
arctool2.exe -v filelist.txt build/samp.saa
```

The committed `archive/build/samp.saa` was stale and failed `-v` on its first entry,
so it has been rebuilt from the current `archive/files/`.

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

## Server browser, cross compiling on linux

`exgui_lazarus` produces `samp.exe`. A linux `lazbuild` cross compiling to win32
is not enough here, the project pulls in the win32 LCL widgetset, so use a
**windows** Lazarus running under wine in its own prefix:

```sh
WINEPREFIX=~/.wine-lazarus wine ~/.wine-lazarus/drive_c/lazarus/lazbuild.exe \
    'Z:\path\to\SAMPC\exgui_lazarus\samp.lpi'
```

The `.lpi` path has to be a windows path, and the output lands next to it as
`exgui_lazarus/samp.exe`. `dist/package-client.sh` reads
`exgui_lazarus/Release/samp.exe`, so copy it over before packaging.

Two Pascal traps worth remembering, both cost a build cycle:

* identifiers are case insensitive, so `var pHostEnt: PHostEnt` declares a
  variable that shadows its own type and fails with "Error in type definition"
* a local named `Tag` inside a `TfmMain` method collides with `TComponent.Tag`

## What a user still needs at runtime

`samp.dll` alone is an update, not an install. Check what it actually imports:

```text
dumpbin /dependents client/Release/samp.dll
```

The current build wants `d3dx9_25.dll` from the DirectX end user runtime and
`BASS.dll` from un4seen. Nothing else, because `client.vcxproj` links the CRT
statically (`RuntimeLibrary` is `MultiThreaded`), the way stock SA-MP did. Do not
switch it back to `MultiThreadedDLL`, that pulls in `MSVCP140.dll` and
`VCRUNTIME140.dll` and makes a redistributable a hard requirement for a DLL that
gets injected into `gta_sa.exe`.

`server.vcxproj` sets no `RuntimeLibrary` at all, so the windows server still gets
the MSVC default of dynamic. That one is a plain console exe, so it matters less,
but it does mean the windows server zip assumes a redistributable is present.

A full install also needs `samp.exe`, the Delphi server browser under `exgui/`,
and `samp.saa`, built from `archive/filelist.txt` with `arctool2` out of GTA:SA
data files. Neither is produced by CI. See `nsis/samp.nsi` for the complete file
list the original installer shipped.

## CI

Both workflows are manual only, under Actions.

**CI covers the server only.** `ci.yml` builds and smoke tests it, `release.yml`
takes a version, writes release notes from `git log` since the previous tag, and
attaches the two server zips that `dist/package.sh` produces.

The client is not built in CI and is uploaded to the release by hand as
`SAMPC-<version>-client.zip`. A runner cannot assemble it: it needs d3dx9,
`bass.lib` and `detours.lib`, none of them redistributable, plus loose assets the
repository never carried, namely `sampgui.png`, `mouse.png` and `samp-license.txt`.

Everything that goes in it is buildable from this tree though, see the sections
above plus `arctool2` for `samp.saa`. The rest has to come from an existing SA-MP
install.

`release.yml` also pulls a pinned upstream `pawncc` release so `dist/package.sh`
can precompile `gamemodes/bare.amx` into the server zips. It installs the binary
rather than building it, because the compiler tree does not build cleanly on a
current host: the `pawnruns` target trips
`assert_static(sizeof(f) <= sizeof(cell))` on x86_64, and the project's
`cmake_minimum_required` predates what recent CMake accepts. The step is allowed
to fail. If it does, the zips ship `bare.pwn` without the `.amx` and
`package.sh` comments out the `gamemode0` line so the server still starts.

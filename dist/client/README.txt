SA-MP Custom (SAMPC) 0.3.9 client
=================================

This project is an archived reverse engineering effort. Run it at your own risk.


Contents
--------

  samp.dll            the client modification
  samp_debug.exe      debug launcher, present only when the build produced it


What is NOT in this zip
-----------------------

This package holds only what the CI can build from source. A working install also
needs these, and none of them are redistributable from this repository:

  samp.exe            the server browser and launcher, a Delphi project under
                      exgui/ that does not build in CI
  samp.saa            the game asset archive, built from archive/filelist.txt
                      with arctool2 and requiring GTA:SA data files
  d3dx9_25.dll        from the DirectX end user runtime. Run
                      "dumpbin /dependents samp.dll" to confirm the number, it
                      follows whichever legacy DirectX SDK the build used
  bass.dll            from un4seen, the audio stream backend
  vcredist            Microsoft Visual C++ redistributable, matching the
                      toolset. client.vcxproj links the dynamic CRT, so samp.dll
                      imports MSVCP140.dll and VCRUNTIME140.dll. Stock SA-MP
                      linked statically and needed none of this

See nsis/samp.nsi for the file list the original installer shipped.


Building samp.dll yourself
--------------------------

Toolchain: Visual Studio 2019 or newer, platform toolset v142, Release|Win32.

Three dependencies are not in the repository:

  a legacy DirectX SDK   for the d3dx9 headers and d3dx9.lib. D3DX was removed
                         from the Windows SDK, so a legacy SDK is the only
                         source. Anything from the 2006 era through June 2010
                         works, the d3dx9_XX.dll a user needs at runtime just
                         follows whichever one you pick. client.vcxproj reads it
                         from $(DXSDK_DIR), used unseparated as
                         $(DXSDK_DIR)Include and $(DXSDK_DIR)Lib\x86, so the
                         variable has to keep its trailing backslash.

  bass.lib               from un4seen, dropped at sdk/bass/bass.lib.

  detours.lib            Microsoft Detours, dropped at sdk/detours/detours.lib.
                         detours.h has a #pragma comment(lib, "detours").

Both .lib paths are already listed in client.vcxproj, so the files just have to
exist. Note that *.lib is gitignored, which is why they were never committed.

No dxerr library is needed even though DXUT calls DXTrace. sdk/dxut/dxstdafx.h
redirects it to DXTraceWrapper, implemented in sdk/dxut/DXUTmisc.cpp.

Only d3dx9.lib has to come from the legacy SDK. d3d9.lib and dxguid.lib, the
other two the project links, ship with the modern Windows SDK.

See BUILDING.md for a working recipe, including cross compiling on linux.


Building samp.dll in CI
-----------------------

.github/workflows/ci.yml has a client job that stays skipped until the
CLIENT_DEPS_URL repository variable is set. Point it at a zip laid out like
this and the client build and the client release zip both start working:

  include/       the d3dx9 headers, plus d3dx9math.inl and dxerr9.h
  lib/x86/       d3dx9.lib, bass.lib, detours.lib

If your SDK is old enough to ship dxerr9.h rather than DxErr.h, add a one line
DxErr.h next to it that includes dxerr9.h. dxstdafx.h asks for the newer name.

Set it under Settings, Secrets and variables, Actions, Variables. Use a URL that
the runner can fetch without credentials, or switch the workflow step to a
secret if it needs one.


Install
-------

Copy samp.dll into your GTA: San Andreas directory, next to gta_sa.exe. Only
version 1.0 US and 1.0 EU of the game are supported, the address tables in
client/game/address.h are hardcoded for those.

Settings live in Documents\GTA San Andreas User Files\SAMP\sa-mp.cfg. Keys this
build understands include fontface, fontweight, fontsize, multicore,
nohudscalefix, directmode, timestamp, pagesize, audiomsgoff and ime.


Notes specific to this build
----------------------------

The network protocol is not compatible with stock SA-MP. Client and server from
the same release have to be used together.

See README.md in the source repository for the full changelog.
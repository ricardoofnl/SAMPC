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
  d3dx9_25.dll        from the DirectX June 2010 end user runtime
  bass.dll            from un4seen, the audio stream backend

See nsis/samp.nsi for the file list the original installer shipped.


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
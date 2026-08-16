SA-MP Custom (SAMPC) 0.3.9 tools
================================

Small standalone utilities from the SAMPC tree. Neither depends on samp.dll, and
both link the CRT statically so no Visual C++ redistributable is needed.


rcon.exe
--------

Standalone remote console client. The original SA-MP installer shipped this
alongside the client, see nsis/samp.nsi.

  rcon.exe <ip> <port> <password>

It then reads commands from stdin and sends them to the server's RCON port. The
server has to be running with "rcon 1" and a matching "rcon_password".

Bear in mind SA-MP's RCON protocol has no transport encryption. The password
crosses the network in a UDP packet, so do not use it over the open internet.


arctool2.exe
------------

Builds and verifies samp.saa, the signed archive the client reads its game asset
overrides from.

  arctool2.exe -c archive/filelist.txt samp.saa    build
  arctool2.exe -v archive/filelist.txt samp.saa    verify

Run it with no arguments for the full option list, including the single file and
key generation modes.

The bundled archive/filelist.txt and archive/filelistgtau.txt are the lists the
original build used. They reference GTA:SA data files plus scm/main.scm and
scm/script.img from the source tree, none of which are redistributable, so you
need those on hand before -c will produce anything.

Signing uses the key pair in archive/. A samp.saa you build with your own keys
will not verify against a client built with the committed ones.


Building these yourself
-----------------------

  msbuild rcon/rcon.vcxproj         /p:Configuration=Release /p:Platform=Win32
  msbuild arctool2/arctool2.vcxproj /p:Configuration=Release /p:Platform=Win32

arctool2 compiles client/archive with ARCTOOL defined, which unlocks the key
generation and archive writing half that the client itself never compiles.

See BUILDING.md in the source repository for the rest.
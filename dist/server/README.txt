SA-MP Custom (SAMPC) 0.3.9 dedicated server
===========================================

This project is an archived reverse engineering effort. Run it at your own risk
and do not expose it to an untrusted network.


Layout
------

  samp-server[.exe]   the server binary
  announce[.exe]      master list announcer, only used when "announce 1" is set
  server.cfg          server configuration
  gamemodes/          gamemode scripts, .amx next to the .pwn source
  filterscripts/      filterscripts, loaded via the "filterscripts" key
  scriptfiles/        working directory for file and database natives
  plugins/            server plugins, loaded via the "plugins" key
  models/             custom artwork, path set by the "artpath" key
  pawno/include/      Pawn includes for compiling your own scripts


Running
-------

Linux:
  chmod +x samp-server
  ./samp-server

Windows:
  samp-server.exe

The default server.cfg loads gamemodes/bare.amx and listens on port 7777. Change
"rcon_password" before running anything reachable from outside your machine.


Compiling scripts
-----------------

The Pawn compiler itself is not redistributed here. Get pawncc for your platform,
then point it at the bundled includes:

  pawncc gamemodes/mygamemode.pwn -ipawno/include

On Windows the usual setup is to drop pawno.exe and pawncc.exe into pawno/ next
to the include directory.


Notes specific to this build
----------------------------

The network protocol is not compatible with stock SA-MP. Client and server from
the same release have to be used together.

RPC_ServerJoin carries an is-NPC flag that stock SA-MP 0.3.7 does not send in the
same position, which is another reason not to mix versions.

See README.md in the source repository for the full changelog.
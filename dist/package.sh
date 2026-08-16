#!/usr/bin/env bash
#
# assemble a release zip, always on linux even for the windows packages so the
# layout comes from exactly one code path
#
# usage: dist/package.sh <target> <version> <artifact-dir> <output-dir>
#
#   target       server-linux | server-windows | client-windows
#   version      goes into the zip name, e.g. 0.3.9-A5
#   artifact-dir directory holding the built binaries
#   output-dir   where the zip is written
#
set -euo pipefail

TARGET=${1:?target required}
VERSION=${2:?version required}
ARTIFACTS=${3:?artifact dir required}
OUTDIR=${4:?output dir required}

ROOT=$(cd "$(dirname "$0")/.." && pwd)
SKEL=$ROOT/dist/server
NAME=sampc-$VERSION-$TARGET
STAGE=$(mktemp -d)
PKG=$STAGE/$NAME

mkdir -p "$PKG" "$OUTDIR"

# copy one required file, fail loudly if the build did not produce it
need() {
	local src=$1 dest=$2
	if [ ! -f "$src" ]; then
		echo "package.sh: missing required artifact: $src" >&2
		exit 1
	fi
	mkdir -p "$(dirname "$dest")"
	cp "$src" "$dest"
}

# copy if present, only for things a given build may legitimately skip
want() {
	local src=$1 dest=$2
	if [ -f "$src" ]; then
		mkdir -p "$(dirname "$dest")"
		cp "$src" "$dest"
	else
		echo "package.sh: note, optional artifact not present: $src" >&2
	fi
}

stage_server_common() {
	cp "$SKEL/server.cfg" "$PKG/server.cfg"
	cp "$SKEL/README.txt" "$PKG/README.txt"

	# runtime directories the server expects, see server/netgame.cpp,
	# server/filterscripts.cpp, server/plugins.cpp and the artpath variable
	mkdir -p "$PKG/gamemodes" "$PKG/filterscripts" "$PKG/scriptfiles" \
		"$PKG/plugins" "$PKG/models" "$PKG/pawno/include"

	cp "$SKEL/gamemodes/bare.pwn" "$PKG/gamemodes/bare.pwn"

	cp "$ROOT/pawno/include/"*.inc "$PKG/pawno/include/"
	cp "$ROOT/pawno/new.pwn" "$PKG/pawno/new.pwn"

	# a compiled default gamemode so the package runs out of the box
	if command -v pawncc >/dev/null 2>&1; then
		( cd "$PKG/gamemodes" && pawncc bare.pwn -i"$PKG/pawno/include" -obare.amx >/dev/null 2>&1 ) || true
	fi

	# without bare.amx the shipped gamemode0 line would just make the server
	# refuse to start, so comment it out and say why
	if [ ! -f "$PKG/gamemodes/bare.amx" ]; then
		echo "package.sh: warning, no bare.amx, disabling gamemode0 in server.cfg" >&2
		sed -i 's|^gamemode0 bare 1$|# gamemode0 bare 1   (bare.amx was not prebuilt, compile gamemodes/bare.pwn first)|' \
			"$PKG/server.cfg"
	fi

	# keep the empty directories in the zip
	for d in filterscripts scriptfiles plugins models; do
		: > "$PKG/$d/.keep"
	done
}

case $TARGET in
server-linux)
	stage_server_common
	need "$ARTIFACTS/mpsvr"    "$PKG/samp-server"
	want "$ARTIFACTS/announce" "$PKG/announce"
	chmod +x "$PKG/samp-server"
	[ -f "$PKG/announce" ] && chmod +x "$PKG/announce"
	;;
server-windows)
	stage_server_common
	need "$ARTIFACTS/server.exe"   "$PKG/samp-server.exe"
	want "$ARTIFACTS/announce.exe" "$PKG/announce.exe"
	;;
client-windows)
	cp "$ROOT/dist/client/README.txt" "$PKG/README.txt"
	need "$ARTIFACTS/samp.dll"       "$PKG/samp.dll"
	want "$ARTIFACTS/samp_debug.exe" "$PKG/samp_debug.exe"
	;;
*)
	echo "package.sh: unknown target: $TARGET" >&2
	exit 2
	;;
esac

( cd "$STAGE" && zip -qr "$NAME.zip" "$NAME" )
mv "$STAGE/$NAME.zip" "$OUTDIR/"
rm -rf "$STAGE"

echo "$OUTDIR/$NAME.zip"
#!/usr/bin/env bash
#
# Packages the Linux build into a self-contained, portable folder.
#
# Ubuntu doesn't ship SDL3/SDL3_mixer/SDL3_image packages yet, so the CI
# workflow builds them from source into a local prefix - this script
# bundles those custom-built .so files (and *their* transitive
# dependencies, via ldd) next to the game binary, along with a small
# launcher script that points LD_LIBRARY_PATH at the bundled libs. Base
# system libraries (libc, libGL, X11, ...) are intentionally left off the
# bundle: those vary by distro/GPU driver and are expected to already be
# present on any Linux desktop.
#
set -euo pipefail

BIN="build/f1spirit"
PKG_NAME="F1SpiritRemake-linux"
OUT="dist/${PKG_NAME}"
LOCAL_PREFIX="${1:-/opt/f1spirit-deps}"   # where the from-source SDL3/etc. libs were installed

if [ ! -f "$BIN" ]; then
	echo "error: $BIN not found - build the project first" >&2
	exit 1
fi

rm -rf dist
mkdir -p "$OUT/lib"

echo "== Copying executable and assets =="
cp "$BIN" "$OUT/f1spirit.bin"

for d in graphics sound tracks demos; do
	if [ -d "$d" ]; then
		cp -r "$d" "$OUT/"
		echo "  + $d/"
	fi
done

if [ -f f1spirit.cfg ]; then
	cp f1spirit.cfg "$OUT/"
	echo "  + f1spirit.cfg"
fi

# ---------------------------------------------------------------------
# Recursively resolve dependencies with ldd, bundling only libraries that
# resolve to our from-source install prefix (custom-built SDL3 and
# friends) - everything else is assumed to already be on the target
# machine as part of a normal Linux desktop install.
# ---------------------------------------------------------------------
echo "== Resolving .so dependencies under ${LOCAL_PREFIX} (ldd, recursive) =="

declare -A seen
pending=("$BIN")

while [ "${#pending[@]}" -gt 0 ]; do
	next=()

	for f in "${pending[@]}"; do
		while IFS= read -r line; do
			# Typical ldd line:
			#   libSDL3.so.0 => /opt/f1spirit-deps/lib/libSDL3.so.0 (0x00007f...)
			libpath=$(printf '%s\n' "$line" | sed -nE 's/.*=> (\/[^ ]+) \(0x[0-9a-fA-F]+\)/\1/p')
			[ -z "$libpath" ] && continue

			case "$libpath" in
				"${LOCAL_PREFIX}"/*)
					libname=$(basename "$libpath")

					if [ -z "${seen[$libname]:-}" ]; then
						seen[$libname]=1
						cp -n "$libpath" "$OUT/lib/"
						next+=("$libpath")
						echo "  + $libname"
					fi
					;;
			esac
		done < <(ldd "$f" 2>/dev/null || true)
	done

	pending=("${next[@]}")
done

echo "== Bundled libraries (${#seen[@]}) =="
ls -1 "$OUT/lib" | sort

# ---------------------------------------------------------------------
# Launcher script: sets LD_LIBRARY_PATH to the bundled lib/ folder (so
# the loader finds our custom-built SDL3 instead of whatever - if
# anything - the system provides) and runs from the right working
# directory so the game's relative "graphics/", "sound/", "tracks/"
# paths resolve correctly regardless of where the user unpacked it.
# ---------------------------------------------------------------------
cat > "$OUT/f1spirit.sh" << 'LAUNCHER'
#!/usr/bin/env bash
cd "$(dirname "$(readlink -f "$0")")"
export LD_LIBRARY_PATH="$PWD/lib:${LD_LIBRARY_PATH:-}"
exec ./f1spirit.bin "$@"
LAUNCHER
chmod +x "$OUT/f1spirit.sh"

# ---------------------------------------------------------------------
# Sanity check
# ---------------------------------------------------------------------
echo "== Verifying no missing dependencies remain =="
missing=$(LD_LIBRARY_PATH="$OUT/lib" ldd "$OUT/f1spirit.bin" 2>/dev/null | grep -i "not found" || true)

if [ -n "$missing" ]; then
	echo "warning: some dependencies were not resolved:" >&2
	echo "$missing" >&2
else
	echo "  all dependencies resolved."
fi

# ---------------------------------------------------------------------
# tar.gz it up
# ---------------------------------------------------------------------
echo "== Creating tarball =="
(cd dist && tar -czf "${PKG_NAME}.tar.gz" "${PKG_NAME}")
echo "Wrote dist/${PKG_NAME}.tar.gz"

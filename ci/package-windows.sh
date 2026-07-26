#!/usr/bin/env bash
#
# Packages the Windows build into a self-contained, zippable folder that
# runs on a clean Windows machine with no MSYS2/UCRT64 installed.
#
# Run from an MSYS2 UCRT64 shell, from the repository root, after building
# with CMake (expects build/f1spirit.exe to exist).
#
set -euo pipefail

EXE="build/f1spirit.exe"
PKG_NAME="F1SpiritRemake-windows"
OUT="dist/${PKG_NAME}"

if [ ! -f "$EXE" ]; then
	echo "error: $EXE not found - build the project first" >&2
	exit 1
fi

rm -rf "dist"
mkdir -p "$OUT"

echo "== Copying executable and assets =="
cp "$EXE" "$OUT/"

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
# Recursively resolve every DLL dependency with ldd, following the chain
# down through secondary/transitive dependencies (e.g. libSDL3_mixer.dll
# pulling in libFLAC.dll, which itself pulls in libogg.dll, etc.) so the
# game runs on a machine that never had MSYS2 installed.
#
# Only DLLs that resolve to a path inside the MSYS2 install tree
# (/ucrt64, /mingw64, /clang64, /usr) are bundled - everything else
# (ntdll.dll, KERNEL32.dll, USER32.dll, ...) already ships with Windows
# itself and must NOT be bundled.
# ---------------------------------------------------------------------
echo "== Resolving DLL dependencies (ldd, recursive) =="

declare -A seen
pending=("$EXE")

while [ "${#pending[@]}" -gt 0 ]; do
	next=()

	for f in "${pending[@]}"; do
		while IFS= read -r line; do
			# Typical ldd line:
			#   libSDL3.dll => /ucrt64/bin/libSDL3.dll (0x7ffb12340000)
			dllpath=$(printf '%s\n' "$line" | sed -nE 's/.*=> (\/[^ ]+) \(0x[0-9a-fA-F]+\)/\1/p')
			[ -z "$dllpath" ] && continue

			# Skip anything that's part of Windows itself
			# (C:\Windows\System32\..., however ldd capitalizes it) - bundle
			# everything else. This deliberately does NOT restrict to
			# /ucrt64/*: CMake's own post-build step already copies
			# SDL3.dll/SDL3_mixer.dll/SDL3_image.dll/libcurl-*.dll straight
			# into build/ (so the exe also runs from there directly), which
			# makes ldd resolve THOSE specific DLLs to a path inside
			# build/ instead of /ucrt64/bin - an /ucrt64/*-only whitelist
			# silently skipped exactly those, which is why they were
			# missing from the packaged zip.
			lower_path=$(printf '%s' "$dllpath" | tr '[:upper:]' '[:lower:]')

			case "$lower_path" in
				/c/windows/*)
					;; # part of the OS - never bundle
				*)
					dllname=$(basename "$dllpath")

					if [ -z "${seen[$dllname]:-}" ]; then
						seen[$dllname]=1
						cp -n "$dllpath" "$OUT/"
						next+=("$dllpath")
						echo "  + $dllname"
					fi
					;;
			esac
		done < <(ldd "$f" 2>/dev/null || true)
	done

	pending=("${next[@]}")
done

# ---------------------------------------------------------------------
# Defensive extra pass: SDL3_image and SDL3_mixer *dynamically load*
# (dlopen/LoadLibrary) some optional format/codec libraries at runtime
# rather than linking them at compile time (e.g. libpng, libjpeg,
# libwebp for images; libvorbis, libopus, libFLAC, libmpg123 for audio).
# ldd cannot see those - they never show up as PE import-table entries.
# Bundle any of the well-known optional codec DLLs that are present in
# ucrt64/bin, whether or not ldd already found them, just in case.
# ---------------------------------------------------------------------
echo "== Checking for dlopen-only codec libraries =="

codec_patterns=(
	"libpng*.dll" "zlib1.dll" "libjpeg*.dll" "libtiff*.dll" "libwebp*.dll"
	"libwebpdemux*.dll" "libavif*.dll" "libjxl*.dll" "libdav1d*.dll"
	"libvorbis*.dll" "libvorbisfile*.dll" "libogg*.dll" "libopus*.dll"
	"libopusfile*.dll" "libFLAC*.dll" "libmpg123*.dll" "libwavpack*.dll"
	"libxmp*.dll" "libmodplug*.dll" "libgme*.dll" "libfluidsynth*.dll"
	"libsndio*.dll"
)

for pattern in "${codec_patterns[@]}"; do
	for f in /ucrt64/bin/$pattern; do
		[ -e "$f" ] || continue
		dllname=$(basename "$f")

		if [ -z "${seen[$dllname]:-}" ]; then
			seen[$dllname]=1
			cp -n "$f" "$OUT/"
			echo "  + $dllname (dlopen candidate)"
		fi
	done
done

echo "== Bundled DLLs (${#seen[@]}) =="
ls -1 "$OUT"/*.dll | sort

# ---------------------------------------------------------------------
# Sanity check: re-run ldd against the packaged exe from inside the
# output folder and make sure nothing still resolves to "not found".
# This mirrors what happens on a clean machine with the DLLs sitting
# next to the exe, which is exactly how Windows resolves them.
# ---------------------------------------------------------------------
echo "== Verifying no missing dependencies remain =="
missing=$(cd "$OUT" && ldd "$(basename "$EXE")" 2>/dev/null | grep -i "not found" || true)

if [ -n "$missing" ]; then
	echo "warning: some dependencies were not resolved:" >&2
	echo "$missing" >&2
else
	echo "  all dependencies resolved."
fi

# ---------------------------------------------------------------------
# Zip it up
# ---------------------------------------------------------------------
echo "== Creating distribution zip =="

if command -v zip >/dev/null 2>&1; then
	(cd dist && zip -r -q "${PKG_NAME}.zip" "${PKG_NAME}")
else
	echo "  (zip not found - falling back to PowerShell's Compress-Archive)"
	powershell.exe -NoProfile -Command \
		"Compress-Archive -Path 'dist/${PKG_NAME}' -DestinationPath 'dist/${PKG_NAME}.zip' -Force"
fi

echo "Wrote dist/${PKG_NAME}.zip"

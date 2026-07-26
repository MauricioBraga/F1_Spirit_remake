#!/usr/bin/env bash
#
# Packages the macOS build into a self-contained, double-clickable
# F1SpiritRemake.app bundle.
#
# Unlike Linux/Windows, simply copying a .dylib next to the executable
# isn't enough on macOS: each Mach-O binary embeds the *absolute
# build-time path* of every library it links (e.g.
# /opt/f1spirit-deps/lib/libSDL3.0.dylib), and the dynamic loader looks
# there first. This script bundles every custom-built dylib into
# Contents/Frameworks and rewrites those embedded paths (with
# install_name_tool) to @executable_path-relative ones, in both the main
# binary and every bundled dylib (since they reference each other too),
# so the bundle runs on a Mac that never had these libraries installed.
#
set -euo pipefail

BIN="build/f1spirit"
APP_NAME="F1SpiritRemake.app"
OUT="dist/${APP_NAME}"
LOCAL_PREFIX="${1:-/opt/f1spirit-deps}"   # where the from-source SDL3/etc. libs were installed

if [ ! -f "$BIN" ]; then
	echo "error: $BIN not found - build the project first" >&2
	exit 1
fi

rm -rf dist
mkdir -p "$OUT/Contents/MacOS" "$OUT/Contents/Resources" "$OUT/Contents/Frameworks"

echo "== Copying executable and assets =="
cp "$BIN" "$OUT/Contents/MacOS/f1spirit"

for d in graphics sound tracks demos; do
	if [ -d "$d" ]; then
		cp -r "$d" "$OUT/Contents/Resources/"
		echo "  + $d/"
	fi
done

if [ -f f1spirit.cfg ]; then
	cp f1spirit.cfg "$OUT/Contents/Resources/"
	echo "  + f1spirit.cfg"
fi

# The game loads assets with plain relative paths ("graphics/...",
# "sound/...", "tracks/..."), so it needs its working directory to be
# Contents/Resources. A tiny launcher shim in Contents/MacOS handles
# that and then execs the real binary.
mv "$OUT/Contents/MacOS/f1spirit" "$OUT/Contents/MacOS/f1spirit.bin"
cat > "$OUT/Contents/MacOS/f1spirit" << 'LAUNCHER'
#!/usr/bin/env bash
cd "$(dirname "$0")/../Resources"
exec "$(dirname "$0")/f1spirit.bin" "$@"
LAUNCHER
chmod +x "$OUT/Contents/MacOS/f1spirit"

cat > "$OUT/Contents/Info.plist" << 'PLIST'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>CFBundleName</key>
	<string>F1 Spirit Remake</string>
	<key>CFBundleExecutable</key>
	<string>f1spirit</string>
	<key>CFBundleIdentifier</key>
	<string>com.mauriciobraga.f1spiritremake</string>
	<key>CFBundleVersion</key>
	<string>1.0</string>
	<key>CFBundlePackageType</key>
	<string>APPL</string>
	<key>NSHighResolutionCapable</key>
	<true/>
</dict>
</plist>
PLIST

# ---------------------------------------------------------------------
# Recursively resolve dependencies with otool -L (macOS's ldd
# equivalent), bundling only libraries under our from-source install
# prefix - everything else (libSystem, AppKit, OpenGL.framework, ...)
# ships with macOS itself.
# ---------------------------------------------------------------------
echo "== Resolving .dylib dependencies under ${LOCAL_PREFIX} (otool -L, recursive) =="

declare -A seen
pending=("$OUT/Contents/MacOS/f1spirit.bin")

while [ "${#pending[@]}" -gt 0 ]; do
	next=()

	for f in "${pending[@]}"; do
		while IFS= read -r line; do
			# Typical otool -L line (first line is the binary itself, skip it):
			#   /opt/f1spirit-deps/lib/libSDL3.0.dylib (compatibility version ...)
			libpath=$(printf '%s\n' "$line" | sed -nE 's/^\s*(\/[^ ]+\.dylib) \(compatibility.*/\1/p')
			[ -z "$libpath" ] && continue

			case "$libpath" in
				"${LOCAL_PREFIX}"/*)
					libname=$(basename "$libpath")

					if [ -z "${seen[$libname]:-}" ]; then
						seen["$libname"]="$libpath"
						cp -n "$libpath" "$OUT/Contents/Frameworks/"
						chmod +w "$OUT/Contents/Frameworks/$libname"
						next+=("$libpath")
						echo "  + $libname"
					fi
					;;
			esac
		done < <(otool -L "$f" 2>/dev/null | tail -n +2)
	done

	pending=("${next[@]}")
done

# ---------------------------------------------------------------------
# Rewrite embedded load-command paths to be relative to the bundle
# (@executable_path/../Frameworks/...), in the main binary AND in every
# bundled dylib (they reference each other by the same absolute
# build-time paths too).
# ---------------------------------------------------------------------
echo "== Rewriting load commands (install_name_tool) =="

rewrite_targets=("$OUT/Contents/MacOS/f1spirit.bin")
for libname in "${!seen[@]}"; do
	rewrite_targets+=("$OUT/Contents/Frameworks/$libname")
done

for target in "${rewrite_targets[@]}"; do
	for libname in "${!seen[@]}"; do
		old_path="${seen[$libname]}"
		install_name_tool -change "$old_path" "@executable_path/../Frameworks/${libname}" "$target" 2>/dev/null || true
	done

	# Give bundled dylibs their own @rpath-friendly identity too, so
	# anything that references them by install name resolves correctly.
	case "$target" in
		*/Frameworks/*)
			install_name_tool -id "@executable_path/../Frameworks/$(basename "$target")" "$target" 2>/dev/null || true
			;;
	esac
done

echo "== Bundled libraries (${#seen[@]}) =="
ls -1 "$OUT/Contents/Frameworks" | sort

# ---------------------------------------------------------------------
# Sanity check: nothing should still point at the build-time prefix.
# ---------------------------------------------------------------------
echo "== Verifying no references to ${LOCAL_PREFIX} remain =="
leftover=$(otool -L "$OUT/Contents/MacOS/f1spirit.bin" | grep -F "$LOCAL_PREFIX" || true)

for libname in "${!seen[@]}"; do
	leftover+=$'\n'"$(otool -L "$OUT/Contents/Frameworks/$libname" | grep -F "$LOCAL_PREFIX" || true)"
done

leftover=$(printf '%s\n' "$leftover" | sed '/^$/d')

if [ -n "$leftover" ]; then
	echo "warning: some load commands still reference the build prefix:" >&2
	echo "$leftover" >&2
else
	echo "  all load commands rewritten."
fi

# ---------------------------------------------------------------------
# Ad-hoc code sign (unsigned binaries downloaded from the internet get
# quarantined/blocked by Gatekeeper on modern macOS; ad-hoc signing lets
# it run locally without requiring a paid Apple Developer certificate).
# ---------------------------------------------------------------------
echo "== Ad-hoc code signing =="
codesign --force --deep --sign - "$OUT" || echo "warning: codesign failed (non-fatal)" >&2

# ---------------------------------------------------------------------
# Zip it up
# ---------------------------------------------------------------------
echo "== Creating zip =="
(cd dist && zip -r -q -y "F1SpiritRemake-macos.zip" "${APP_NAME}")
echo "Wrote dist/F1SpiritRemake-macos.zip"

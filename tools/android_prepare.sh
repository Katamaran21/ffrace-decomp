#!/bin/sh
set -e

SDL_VERSION=2.32.10
SDL_DIR=SDL2-$SDL_VERSION
SDL_TARBALL=$SDL_DIR.tar.gz
SDL_URL=https://github.com/libsdl-org/SDL/releases/download/release-$SDL_VERSION/$SDL_TARBALL
APP_ID=io.github.katamaran21.ffrace
VERSION_CODE=1
VERSION_NAME=1.0
MIN_SDK=21
APP_ABI="armeabi-v7a arm64-v8a x86 x86_64"

root=$(cd "$(dirname "$0")/.." && pwd)
work=$root/build-android
deps=$work/.deps
app=$work/app

mkdir -p "$deps"
if [ ! -d "$deps/$SDL_DIR" ]; then
    if [ ! -f "$deps/$SDL_TARBALL" ]; then
        echo "fetching $SDL_TARBALL"
        curl -fsSL -o "$deps/$SDL_TARBALL" "$SDL_URL"
    fi
    tar -xzf "$deps/$SDL_TARBALL" -C "$deps"
fi
sdl=$deps/$SDL_DIR

cp -R "$sdl/android-project/." "$work/"
chmod +x "$work/gradlew"

if [ ! -f "$app/jni/SDL/Android.mk" ]; then
    mkdir -p "$app/jni/SDL"
    cp -R "$sdl/." "$app/jni/SDL/"
fi

mkdir -p "$app/jni/src"
find "$app/jni/src" -maxdepth 1 \( -name '*.c' -o -name '*.h' \) -delete
cp "$root"/ff_*.c "$root"/ff_*.h "$app/jni/src/"
cp "$root/android/Android.mk" "$app/jni/src/Android.mk"
cp "$root/android/AndroidManifest.xml" "$app/src/main/AndroidManifest.xml"
cp "$root/android/strings.xml" "$app/src/main/res/values/strings.xml"

assets=$app/src/main/assets
mkdir -p "$assets"
rm -rf "$assets/BITMAP" "$assets/Sounds" "$assets/Musics"
cp -R "$root/BITMAP" "$root/Sounds" "$root/Musics" "$assets/"

grep -q applicationId "$app/build.gradle" ||
    sed -i "s|^\( *\)minSdkVersion|\1applicationId \"$APP_ID\"\n\1minSdkVersion|" "$app/build.gradle"

sed -i "s|^\( *\)versionCode .*|\1versionCode $VERSION_CODE|;s|^\( *\)versionName .*|\1versionName \"$VERSION_NAME\"|" "$app/build.gradle"

sed -i "s|minSdkVersion [0-9]*|minSdkVersion $MIN_SDK|;s|APP_PLATFORM=android-[0-9]*|APP_PLATFORM=android-$MIN_SDK|" "$app/build.gradle"
sed -i "s|APP_PLATFORM=android-[0-9]*|APP_PLATFORM=android-$MIN_SDK|" "$app/jni/Application.mk"
sed -i "s|^APP_ABI := .*|APP_ABI := $APP_ABI|" "$app/jni/Application.mk"

ndk=${ANDROID_NDK_HOME:-${ANDROID_NDK_ROOT:-${ANDROID_NDK_LATEST_HOME:-}}}
if [ -n "$ndk" ] && ! grep -q ndkPath "$app/build.gradle"; then
    sed -i "s|^\( *\)compileSdkVersion|\1ndkPath \"$ndk\"\n\1compileSdkVersion|" "$app/build.gradle"
fi

echo "android project ready in $work"
echo "build it with: cd build-android && ./gradlew assembleDebug"

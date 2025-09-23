GAME_NAME="GameName"

echo "Building $GAME_NAME (Mac OS)"

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
config_file_path="$script_dir/apple_environment.txt"

apple_dev_environment="Unknown"

if [ -r "$config_file_path" ]; then
	apple_dev_environment=$(<"$config_file_path")
fi

echo "$apple_dev_environment"

source ../shared_apple_platform/compiler_warnings.sh
source common_build_vars.sh

GAME_LIBRARY_CODE_PATH="../../code/game_library"

release_build=0
steam_build=0
mac_app_store_build=0
verbose=0

for arg in "$@"
do
    if [ "$arg" == "-release" ]
    then
        release_build=1
    fi

    if [ "$arg" == "-steam" ]
    then
        steam_build=1
    fi

    if [ "$arg" == "-macappstore" ]
    then
        mac_app_store_build=1
    fi

    if [ "$arg" == "-verbose" ]
    then
        verbose=1
    fi
done

if [ "$mac_app_store_build" -eq 1 ]
then
    OSX_LD_FLAGS="${OSX_LD_FLAGS}
                  -framework GameKit"
fi

if [ "$verbose" -eq 1 ]
then
    echo OSX Frameworks: ${OSX_LD_FLAGS}
fi

COMMON_COMPILER_FLAGS="$COMPILER_WARNING_FLAGS
                       $DISABLED_ERRORS
                       -DMACOS=1
                       -DIOS=0
                       -DWINDOWS=0
                       -DSLOW=1
                       -DINTERNAL=0
                       -std=c++11
                       $OSX_LD_FLAGS"

if [ "$release_build" -eq 1 ]
then
    COMMON_COMPILER_FLAGS="${COMMON_COMPILER_FLAGS}
                           -DLEVELEDITOR=0"
else
    COMMON_COMPILER_FLAGS="${COMMON_COMPILER_FLAGS}
                           -DLEVELEDITOR=1"
fi

if [ "$verbose" -eq 1 ]
then
    echo Common Compiler Flags: ${COMMON_COMPILER_FLAGS}
fi

GAME_BUNDLE_PATH="${GAME_NAME}.app"
GAME_BUNDLE_RESOURCES_PATH="${GAME_BUNDLE_PATH}/Contents/Resources"
GAME_BUNDLE_BUILD_PATH="${GAME_BUNDLE_PATH}/Contents/MacOS"
GAME_BUNDLE_CODE_RESOURCES_PATH="${GAME_BUNDLE_PATH}/Contents/CodeResources"

MAC_BUILD_DIRECTORY="../../build/mac_os"

mkdir -p $MAC_BUILD_DIRECTORY
pushd $MAC_BUILD_DIRECTORY

echo Building Game Application Bundle
rm -rf $GAME_BUNDLE_PATH

mkdir -p $GAME_BUNDLE_RESOURCES_PATH
mkdir -p $GAME_BUNDLE_BUILD_PATH
mkdir -p $GAME_BUNDLE_CODE_RESOURCES_PATH

STEAMBUILDCOMMAND=""

STOREFRONT="-DSTEAMSTORE=0 
            -DMACAPPSTORE=0"

#MIN_MAC_OS_VERSION=10.14
MIN_MAC_OS_VERSION=10.15

if [ "$steam_build" -eq 1 ]
then
    cp ../../code/steam_library/redistributable_bin/osx/libsteam_api.dylib "${GAME_BUNDLE_BUILD_PATH}/libsteam_api.dylib"
    STOREFRONT="-DSTEAMSTORE=1 
                -DMACAPPSTORE=0"
    STEAMBUILDCOMMAND="-L ${GAME_BUNDLE_BUILD_PATH} -l steam_api -Wl, -rpath @executable_path"
fi

if [ "$mac_app_store_build" -eq 1 ]
then
    STOREFRONT="-DSTEAMSTORE=0 
                -DMACAPPSTORE=1"

    MIN_MAC_OS_VERSION=10.15
fi

xcrun -sdk macosx metal -mmacosx-version-min=${MIN_MAC_OS_VERSION} -gline-tables-only -MO -g -c "${SHARED_APPLE_PLATFORMS_PATH}/Shaders.metal" -o Shaders.air
xcrun -sdk macosx metallib Shaders.air -o Shaders.metallib

echo Storefront: ${STOREFRONT}

if [ "$apple_dev_environment" != "Unknown" ]
then
SDK_PATH="/Applications/${apple_dev_environment}.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk"
fi

if [ "$release_build" -eq 1 ]
then
    echo Compiling Game Platform Layer \(Fast\)
    clang -O3 -lstdc++ ${COMMON_COMPILER_FLAGS} ${STOREFRONT} -mmacosx-version-min=${MIN_MAC_OS_VERSION} -o $GAME_NAME "${MAC_PLATFORM_LAYER_PATH}/osx_main.mm" ${STEAMBUILDCOMMAND}

else
    echo Compiling Game Platform Layer \(Slow\)
    clang -g -lstdc++ -isysroot "${SDK_PATH}" -I"${SDK_PATH}/usr/include" ${COMMON_COMPILER_FLAGS} ${STOREFRONT} -mmacosx-version-min=${MIN_MAC_OS_VERSION} -o $GAME_NAME "${MAC_PLATFORM_LAYER_PATH}/osx_main.mm" ${STEAMBUILDCOMMAND}
fi

cp $GAME_NAME ${GAME_BUNDLE_PATH}/Contents/MacOS/${GAME_NAME}

cp -r ${RESOURCES_PATH}/levels ${GAME_BUNDLE_RESOURCES_PATH}/levels
cp -r ${RESOURCES_PATH}/sounds ${GAME_BUNDLE_RESOURCES_PATH}/sounds
cp -r ${RESOURCES_PATH}/art ${GAME_BUNDLE_RESOURCES_PATH}/art

cp ${MAC_PLATFORM_LAYER_PATH}/resources/AppIcon.icns ${GAME_BUNDLE_RESOURCES_PATH}/AppIcon.icns

if [ "$release_build" -eq 0 ]
then
    if [ "$steam_build" -eq 1 ]
    then
        cp ${RESOURCES_PATH}/steam_appid.txt "${GAME_BUNDLE_BUILD_PATH}/steam_appid.txt"
    fi
fi

cp Shaders.metallib ${GAME_BUNDLE_RESOURCES_PATH}/Shaders.metallib
cp ${PLATFORM_RESOURCES_PATH}/GameInfo.plist ${GAME_BUNDLE_PATH}/Contents/Info.plist

if [ "$steam_build" -eq 1 ]
then
    cp ${PLATFORM_RESOURCES_PATH}/SteamEntitlements.plist ${GAME_BUNDLE_PATH}/Contents/Entitlements.plist
fi

if [ "$mac_app_store_build" -eq 1 ]
then
    cp ${PLATFORM_RESOURCES_PATH}/MacAppStoreEntitlements.plist ${GAME_BUNDLE_PATH}/Contents/Entitlements.plist
fi

if [ "$release_build" -eq 1 ]
then
    if [ "$mac_app_store_build" -eq 1 ]
    then
        cp ${PLATFORM_RESOURCES_PATH}/3rd_Party_Mac_Distribution.provisionprofile ${GAME_BUNDLE_PATH}/Contents/embedded.provisionprofile
        codesign -s "3rd Party Mac Developer Application: Send It Apps LLC" --timestamp -f -v --deep --options runtime --entitlements "${GAME_BUNDLE_PATH}/Contents/Entitlements.plist" ${GAME_BUNDLE_PATH}
        xcrun productbuild --component ${GAME_BUNDLE_PATH} /Applications ${GAME_NAME}.unsigned.pkg
        xcrun productsign --sign "3rd Party Mac Developer Installer: Send It Apps LLC" ${GAME_NAME}.unsigned.pkg mooselutions.pkg
    else
        codesign -s "Developer ID Application: Send It Apps LLC" --timestamp -f -v --deep --options runtime --entitlements "${GAME_BUNDLE_PATH}/Contents/Entitlements.plist" ${GAME_BUNDLE_PATH}
    fi
fi
popd


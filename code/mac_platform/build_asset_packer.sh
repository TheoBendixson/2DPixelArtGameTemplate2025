
APP_NAME="AssetPacker"
echo Building ${APP_NAME}

MAC_PLATFORM_LAYER_PATH="../../code/mac_platform"
ASSET_PACKER_LIBRARY_CODE_PATH="../../code/asset_packer"
MAC_BUILD_DIRECTORY="../../build/mac_os"

OSX_LD_FLAGS="-framework AppKit"

DISABLED_ERRORS="-Wno-c++11-compat-deprecated-writable-strings
                 -Wno-c++11-extensions"

mkdir -p $MAC_BUILD_DIRECTORY
pushd $MAC_BUILD_DIRECTORY

echo Compiling Asset Packer \(Slow\)
clang -g -lstdc++ $OSX_LD_FLAGS $DISABLED_ERRORS -o AssetPacker "${MAC_PLATFORM_LAYER_PATH}/mac_asset_packer_main.mm"

echo Building Asset Packer Application Bundle
APP_PACKAGE_NAME="${APP_NAME}.app"
APP_BUNDLE_BUILD_PATH="${APP_PACKAGE_NAME}/Contents/MacOS"

rm -rf $APP_PACKAGE_NAME

APP_BUNDLE_RESOURCES_PATH="${APP_PACKAGE_NAME}/Contents/Resources"

mkdir -p $APP_BUNDLE_RESOURCES_PATH
mkdir -p $APP_BUNDLE_BUILD_PATH

cp $APP_NAME "${APP_BUNDLE_BUILD_PATH}/${APP_NAME}"

RESOURCES_PATH="../../resources"

cp -r "${RESOURCES_PATH}/art" "${APP_BUNDLE_RESOURCES_PATH}/art"

PLATFORM_RESOURCES_PATH="../../code/mac_platform/resources"

cp "${PLATFORM_RESOURCES_PATH}/AssetPackerInfo.plist" "${APP_PACKAGE_NAME}/Contents/Info.plist"

popd

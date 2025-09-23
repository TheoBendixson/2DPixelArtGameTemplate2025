
RESOURCES_PATH="../../resources"

source ../shared_apple_platform/compiler_warnings.sh

release_build=0
simulator_build=0
device_build=0
ipad_test_build=0

for arg in "$@"
do
    if [ "$arg" == "-simulator" ]
    then
        simulator_build=1
    fi

    if [ "$arg" == "-device" ]
    then
        device_build=1
    fi

    if [ "$arg" == "-ipadtest" ]
    then
        ipad_test_build=1
    fi

    if [ "$arg" == "-release" ]
    then
        release_build=1
        device_build=1

        echo "Release build. Disregarding all other arguments."
        simulator_build=0
        ipad_test_build=0
    fi
done


if [ "$simulator_build" -eq 0 ] &&
   [ "$device_build" -eq 0 ]
then
    echo "Neither device nor simulator specified. Doing a device build."
    device_build=1
fi

IOS_SIMULATOR_BUILD_DIRECTORY="../../build/ios_simulator"
IOS_BUILD_DIRECTORY="../../build/ios"
PLATFORM_DESCRIPTION=""

if [ "$simulator_build" -eq 1 ]
then
    PLATFORM_DESCRIPTION="iOS Simulator"
    mkdir -p $IOS_SIMULATOR_BUILD_DIRECTORY
    pushd $IOS_SIMULATOR_BUILD_DIRECTORY
fi

if [ "$device_build" -eq 1 ]
then
    PLATFORM_DESCRIPTION="iOS"
    mkdir -p $IOS_BUILD_DIRECTORY
    pushd $IOS_BUILD_DIRECTORY
fi

GAME_NAME=GameName
echo "Building ${GAME_NAME} ($PLATFORM_DESCRIPTION)"

IOS_LD_FLAGS="-framework UIKit
              -framework CoreVideo
              -framework Metal
              -framework MetalKit
              -framework QuartzCore
              -framework Foundation
              -framework AudioToolbox
              -framework AVFoundation
              -framework GameKit"

COMMON_COMPILER_FLAGS="$COMPILER_WARNING_FLAGS
                       $DISABLED_ERRORS
                       -DDEBUG=1
                       -DIOS=1
                       -DMACOS=0
                       -DLEVELEDITOR=0
                       -DWINDOWS=0
                       -DSLOW=1
                       -DSTEAMSTORE=0
                       -DMACAPPSTORE=0
                       -std=c++11
                       $IOS_LD_FLAGS"

IOS_PLATFORM_LAYER_PATH="../../code/ios_platform"

SHARED_APPLE_PLATFORMS_PATH="../../code/shared_apple_platform"

METAL_COMPILE_OS=""
XCODE_VERSION="Xcode_15_2"
SDK_PATH=""

if [ "$ipad_test_build" -eq 1 ]
then
    XCODE_VERSION="Xcode_14_3"
    echo "iPad Test Build."
fi

echo "Xcode Version: ${XCODE_VERSION}"

if [ "$simulator_build" -eq 1 ]
then
    METAL_COMPILE_OS="iphonesimulator"
    SDK_PATH="/Applications/${XCODE_VERSION}.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk"
    ARCHS="x86_64"
fi

if [ "$device_build" -eq 1 ]
then
    METAL_COMPILE_OS="iphoneos"
    SDK_PATH="/Applications/${XCODE_VERSION}.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk"
    ARCHS="arm64"
fi

echo "SDK Path: ${SDK_PATH}"

xcrun -sdk ${METAL_COMPILE_OS} metal -gline-tables-only -MO -g -c "${SHARED_APPLE_PLATFORMS_PATH}/Shaders.metal" -o Shaders.air
xcrun -sdk ${METAL_COMPILE_OS} metallib Shaders.air -o Shaders.metallib

rm -rf "Mooselutions.dSYM"

if [ "$release_build" -eq 1 ]
then
    echo "Compiling Game Platform Layer (Fast)"
    clang -g -O3 -lstdc++ -arch $ARCHS -isysroot "${SDK_PATH}" -I"${SDK_PATH}/usr/include" ${COMMON_COMPILER_FLAGS} -o ${GAME_NAME} "${IOS_PLATFORM_LAYER_PATH}/ios_main.mm"
else
    echo "Compiling Game Platform Layer (Slow)"
    clang -g -lstdc++ -arch $ARCHS -isysroot "${SDK_PATH}" -I"${SDK_PATH}/usr/include" ${COMMON_COMPILER_FLAGS} -o ${GAME_NAME} "${IOS_PLATFORM_LAYER_PATH}/ios_main.mm"
fi

echo Building Game Application Bundle

BUNDLE_NAME="${GAME_NAME}.app"
rm -rf $BUNDLE_NAME

mkdir -p $BUNDLE_NAME
mkdir -p $BUNDLE_NAME/Base.lproj

xcrun ibtool --compile ${BUNDLE_NAME}/Base.lproj/Main.storyboardc ${IOS_PLATFORM_LAYER_PATH}/resources/Main.storyboard
xcrun ibtool --compile ${BUNDLE_NAME}/Base.lproj/LaunchScreen.storyboardc ${IOS_PLATFORM_LAYER_PATH}/resources/LaunchScreen.storyboard

SHORT_VERSION="1.0.1"
BUILD_NUMBER="28"
INFO_PLIST_NAME=""
SUPPORTED_PLATFORMS=""
ARCHITECHTURES=""
DT_PLATFORM_NAME=""
DT_PLATFORM_VERSION="17.2"

if [ "$simulator_build" -eq 1 ]
then
    INFO_PLIST_NAME="SimulatorInfo.plist"
    SUPPORTED_PLATFORMS="iPhoneSimulator"
    ARCHITECHTURES="armv7"
    DT_PLATFORM_NAME="iphonesimulator"
fi

if [ "$device_build" -eq 1 ]
then
    INFO_PLIST_NAME="DeviceInfo.plist"
    SUPPORTED_PLATFORMS="iPhoneOS"
    ARCHITECHTURES="arm64"
    DT_PLATFORM_NAME="iphoneos"
fi

if [ "$ipad_test_build" -eq 1 ]
then
    DT_PLATFORM_VERSION="16.3"
fi

OUTPUT_FILE="GeneratedInfo.plist"

printf "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" > "$OUTPUT_FILE"
printf "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" " >> "$OUTPUT_FILE"
printf "\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n" >> "$OUTPUT_FILE"
printf "<plist version=\"1.0\">\n" >> "$OUTPUT_FILE"
printf "<dict>\n" >> "$OUTPUT_FILE"
printf "    <key>BuildMachineOSBuild</key>\n" >> "$OUTPUT_FILE"
printf "    <string>22H313</string>\n" >> "$OUTPUT_FILE"
printf "    <key>CFBundleDevelopmentRegion</key>\n" >> "$OUTPUT_FILE"
printf "    <string>en</string>\n" >> "$OUTPUT_FILE"
printf "    <key>CFBundleExecutable</key>\n" >> "$OUTPUT_FILE"
printf "    <string>${GAME_NAME}</string>\n" >> "$OUTPUT_FILE"
printf "    <key>CFBundleIcons</key>\n" >> "$OUTPUT_FILE"
printf "    <dict>\n" >> "$OUTPUT_FILE"
printf "	    <key>CFBundlePrimaryIcon</key>\n" >> "$OUTPUT_FILE"
printf "		<dict>\n" >> "$OUTPUT_FILE"
printf "            <key>CFBundleIconFiles</key>\n" >> "$OUTPUT_FILE"
printf "		    <array>\n" >> "$OUTPUT_FILE"
printf "                <string>AppIcon60x60</string>\n" >> "$OUTPUT_FILE"
printf "            </array>\n" >> "$OUTPUT_FILE"
printf "		    <key>CFBundleIconName</key>\n" >> "$OUTPUT_FILE"
printf "		    <string>AppIcon</string>\n" >> "$OUTPUT_FILE"
printf "	    </dict>\n" >> "$OUTPUT_FILE"
printf "    </dict>\n" >> "$OUTPUT_FILE"
printf "	<key>CFBundleIcons~ipad</key>\n" >> "$OUTPUT_FILE"
printf "	<dict>\n" >> "$OUTPUT_FILE"
printf "	    <key>CFBundlePrimaryIcon</key>\n" >> "$OUTPUT_FILE"
printf "        <dict>\n" >> "$OUTPUT_FILE"
printf "            <key>CFBundleIconFiles</key>\n" >> "$OUTPUT_FILE"
printf "			<array>\n" >> "$OUTPUT_FILE"
printf "				<string>AppIcon60x60</string>\n" >> "$OUTPUT_FILE"
printf "				<string>AppIcon76x76</string>\n" >> "$OUTPUT_FILE"
printf "			</array>\n" >> "$OUTPUT_FILE"
printf "			<key>CFBundleIconName</key>\n" >> "$OUTPUT_FILE"
printf "			<string>AppIcon</string>\n" >> "$OUTPUT_FILE"
printf "		</dict>\n" >> "$OUTPUT_FILE"
printf "	</dict>\n" >> "$OUTPUT_FILE"
printf "	<key>CFBundleIdentifier</key>\n " >> "$OUTPUT_FILE"
printf "	<string>com.company.game</string>\n" >> "$OUTPUT_FILE"
printf "    <key>CFBundleInfoDictionaryVersion</key>\n" >> "$OUTPUT_FILE"
printf "    <string>6.0</string>\n" >> "$OUTPUT_FILE"
printf "    <key>CFBundleName</key>\n" >> "$OUTPUT_FILE"
printf "    <string>${GAME_NAME}</string>\n" >> "$OUTPUT_FILE"
printf "    <key>CFBundlePackageType</key>\n" >> "$OUTPUT_FILE"
printf "    <string>APPL</string>\n" >> "$OUTPUT_FILE"
printf "	<key>CFBundleShortVersionString</key>\n" >> "$OUTPUT_FILE"
printf "	<string>${SHORT_VERSION}</string>\n" >> "$OUTPUT_FILE"
printf "    <key>CFBundleSupportedPlatforms</key>\n" >> "$OUTPUT_FILE"
printf "    <array>\n" >> "$OUTPUT_FILE"
printf "        <string>${SUPPORTED_PLATFORMS}</string>\n" >> "$OUTPUT_FILE"
printf "    </array>\n" >> "$OUTPUT_FILE"
printf "	<key>CFBundleVersion</key>\n" >> "$OUTPUT_FILE"
printf "	<string>${BUILD_NUMBER}</string>\n" >> "$OUTPUT_FILE"
printf "	<key>DTCompiler</key>\n" >> "$OUTPUT_FILE"
printf "	<string>com.apple.compilers.llvm.clang.1_0</string>\n" >> "$OUTPUT_FILE"
printf "    <key>DTPlatformBuild</key>\n" >> "$OUTPUT_FILE"
printf "    <string>21C52</string>\n" >> "$OUTPUT_FILE"
printf "    <key>DTPlatformName</key>\n" >> "$OUTPUT_FILE"
printf "    <string>iphoneos</string>\n" >> "$OUTPUT_FILE"
printf "	<key>DTPlatformVersion</key>\n" >> "$OUTPUT_FILE"
printf "	<string>${DT_PLATFORM_VERSION}</string>\n" >> "$OUTPUT_FILE"
printf "    <key>DTSDKBuild</key>\n" >> "$OUTPUT_FILE"
printf "    <string>21C52</string>\n" >> "$OUTPUT_FILE"
printf "	<key>DTSDKName</key>\n" >> "$OUTPUT_FILE"
printf "	<string>iphoneos17.2</string>\n" >> "$OUTPUT_FILE"
printf "	<key>DTXcode</key>\n" >> "$OUTPUT_FILE"
printf "	<string>1520</string>\n" >> "$OUTPUT_FILE"
printf "    <key>DTXcodeBuild</key>\n" >> "$OUTPUT_FILE"
printf "    <string>15C500b</string>\n" >> "$OUTPUT_FILE"
printf "    <key>LSRequiresIPhoneOS</key>\n" >> "$OUTPUT_FILE"
printf "    <true/>\n" >> "$OUTPUT_FILE"
printf "    <key>MinimumOSVersion</key>\n" >> "$OUTPUT_FILE"
printf "    <string>${DT_PLATFORM_VERSION}</string>\n" >> "$OUTPUT_FILE"
printf "    <key>UIApplicationSupportsIndirectInputEvents</key>\n" >> "$OUTPUT_FILE"
printf "    <false/>\n" >> "$OUTPUT_FILE"
printf "    <key>UIDeviceFamily</key>\n" >> "$OUTPUT_FILE"
printf "    <array>\n" >> "$OUTPUT_FILE"
printf "        <integer>1</integer>\n" >> "$OUTPUT_FILE"
printf "        <integer>2</integer>\n" >> "$OUTPUT_FILE"
printf "    </array>\n" >> "$OUTPUT_FILE"
printf "	<key>UILaunchStoryboardName</key>\n" >> "$OUTPUT_FILE"
printf "	<string>LaunchScreen</string>\n" >> "$OUTPUT_FILE"
printf "    <key>UIMainStoryboardFile</key>\n" >> "$OUTPUT_FILE"
printf "    <string>Main</string>\n" >> "$OUTPUT_FILE"
printf "    <key>UIRequiredDeviceCapabilities</key>\n" >> "$OUTPUT_FILE"
printf "    <array>\n" >> "$OUTPUT_FILE"
printf "        <string>${ARCHITECHTURES}</string>\n" >> "$OUTPUT_FILE"
printf "    </array>\n" >> "$OUTPUT_FILE"
printf "    <key>UISupportedInterfaceOrientations</key>\n" >> "$OUTPUT_FILE"
printf "    <array>\n" >> "$OUTPUT_FILE"
printf "        <string>UIInterfaceOrientationLandscapeLeft</string>\n" >> "$OUTPUT_FILE"
printf "        <string>UIInterfaceOrientationLandscapeRight</string>\n" >> "$OUTPUT_FILE"
printf "    </array>\n" >> "$OUTPUT_FILE"
printf "    <key>UIRequiresFullScreen</key>\n" >> "$OUTPUT_FILE"
printf "    <true/>\n" >> "$OUTPUT_FILE"
printf "    <key>UIViewControllerBasedStatusBarAppearance</key>\n" >> "$OUTPUT_FILE"
printf "    <true/>\n" >> "$OUTPUT_FILE"
printf "</dict>\n" >> "$OUTPUT_FILE"
printf "</plist>\n" >> "$OUTPUT_FILE"

cp $GAME_NAME ${BUNDLE_NAME}/${GAME_NAME}
plutil -convert binary1 -o ${BUNDLE_NAME}/Info.plist GeneratedInfo.plist
cp Shaders.metallib ${BUNDLE_NAME}/Shaders.metallib
cp -r ${RESOURCES_PATH}/art ${BUNDLE_NAME}/art
cp -r ${RESOURCES_PATH}/levels ${BUNDLE_NAME}/levels
cp -r ${RESOURCES_PATH}/sounds ${BUNDLE_NAME}/sounds

cp ${IOS_PLATFORM_LAYER_PATH}/resources/AppIcon/AppIcon60x60@2x.png ${BUNDLE_NAME}/AppIcon60x60@2x.png
cp ${IOS_PLATFORM_LAYER_PATH}/resources/AppIcon/AppIcon76x76@2x~ipad.png ${BUNDLE_NAME}/AppIcon76x76@2x~ipad.png
cp ${IOS_PLATFORM_LAYER_PATH}/resources/AppIcon/Assets.car ${BUNDLE_NAME}/Assets.car
cp ${IOS_PLATFORM_LAYER_PATH}/resources/PkgInfo ${BUNDLE_NAME}/PkgInfo

if [ "$device_build" -eq 1 ]
then
    
    if [ "$release_build" -eq 1 ]
    then
        cp ${IOS_PLATFORM_LAYER_PATH}/resources/Game_iOS_Distribution.mobileprovision ${GAME_NAME}.app/embedded.mobileprovision
        codesign -s "Apple Distribution: Your Company LLC (??????)" --entitlements ${IOS_PLATFORM_LAYER_PATH}/resources/Entitlements.plist --deep -f --timestamp -v ${GAME_NAME}.app

        rm -rf ${GAME_NAME}.xcarchive
        mkdir -p ${GAME_NAME}.xcarchive/Products/Applications
        cp -r ${BUNDLE_NAME} ${GAME_NAME}.xcarchive/Products/Applications/

        # Create dSYM Archive and Copy It
        mkdir -p ${GAME_NAME}.xcarchive/dSYMs
        cp -r ${GAME_NAME}.dSYM Mooselutions.xcarchive/dSYMs/${GAME_NAME}.dSYM


        OUTPUT_FILE="${GAME_NAME}.xcarchive/Info.plist"
        printf "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" > "$OUTPUT_FILE"
        printf "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" " >> "$OUTPUT_FILE"
        printf "\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n" >> "$OUTPUT_FILE"
        printf "<plist version=\"1.0\">\n" >> "$OUTPUT_FILE"
        printf "<dict>\n" >> "$OUTPUT_FILE"
        printf "    <key>ApplicationProperties</key>\n" >> "$OUTPUT_FILE"
        printf "    <dict>\n" >> "$OUTPUT_FILE"
        printf "        <key>ApplicationPath</key>\n" >> "$OUTPUT_FILE"
        printf "        <string>Applications/Mooselutions.app</string>\n" >> "$OUTPUT_FILE"
        printf "        <key>Architectures</key>\n" >> "$OUTPUT_FILE"
        printf "            <array>\n" >> "$OUTPUT_FILE"
        printf "                <string>arm64</string>\n" >> "$OUTPUT_FILE"
        printf "            </array>\n" >> "$OUTPUT_FILE"
        printf "        <key>CFBundleIdentifier</key>\n" >> "$OUTPUT_FILE"
        printf "        <string>com.company.game</string>\n" >> "$OUTPUT_FILE"
        printf "        <key>CFBundleShortVersionString</key>\n" >> "$OUTPUT_FILE"
        printf "        <string>${SHORT_VERSION}</string>\n" >> "$OUTPUT_FILE"
        printf "        <key>CFBundleVersion</key>\n" >> "$OUTPUT_FILE"
        printf "        <string>${BUILD_NUMBER}</string>\n" >> "$OUTPUT_FILE"
        printf "        <key>SigningIdentity</key>\n" >> "$OUTPUT_FILE"
        printf "        <string>Apple Distribution: Company Name (??????)</string>\n" >> "$OUTPUT_FILE"
        printf "        <key>Team</key>\n" >> "$OUTPUT_FILE"
        printf "        <string>TEAM_ID</string>\n" >> "$OUTPUT_FILE"
        printf "    </dict>\n" >> "$OUTPUT_FILE"
	    printf "    <key>ArchiveVersion</key>\n" >> "$OUTPUT_FILE"
	    printf "    <integer>2</integer>\n" >> "$OUTPUT_FILE"
	    printf "    <key>CreationDate</key>\n" >> "$OUTPUT_FILE"
	    printf "    <date>2025-01-17T19:55:32Z</date>\n" >> "$OUTPUT_FILE"
	    printf "    <key>Name</key>\n" >> "$OUTPUT_FILE"
	    printf "    <string>${GAME_NAME}</string>\n" >> "$OUTPUT_FILE"
	    printf "    <key>SchemeName</key>\n" >> "$OUTPUT_FILE"
	    printf "    <string>${GAME_NAME}</string>\n" >> "$OUTPUT_FILE"
        printf "</dict>\n" >> "$OUTPUT_FILE"
        printf "</plist>\n" >> "$OUTPUT_FILE"

        rm -rf AppStoreRelease
        mkdir -p AppStoreRelease
        pushd AppStoreRelease

        IPA_FILE=${GAME_NAME}.ipa
        rm -rf ${GAME_NAME}
        rm -rf $IPA_FILE

        mkdir -p ${GAME_NAME}/Payload

        mkdir -p Payload
        cp -r ../${BUNDLE_NAME} Payload/
        zip -r $IPA_FILE Payload

    else
        cp ${IOS_PLATFORM_LAYER_PATH}/resources/Game_iOS_Development.mobileprovision ${GAME_NAME}.app/embedded.mobileprovision
        codesign -s "Apple Development: Name (??????)" --entitlements ${IOS_PLATFORM_LAYER_PATH}/resources/DevEntitlements.plist --deep -f --timestamp -v ${GAME_NAME}.app 
    fi

fi

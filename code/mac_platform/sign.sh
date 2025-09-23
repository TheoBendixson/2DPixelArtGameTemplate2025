
MAC_BUILD_DIRECTORY="../../build/mac_os"

pushd $MAC_BUILD_DIRECTORY
    codesign -s "Developer ID Application: Send It Apps LLC" --timestamp -f --deep Mooselutions.app
popd

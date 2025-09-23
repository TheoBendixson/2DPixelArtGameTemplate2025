
MAC_BUILD_DIRECTORY="../../build/mac_os"

pushd $MAC_BUILD_DIRECTORY
    xcrun stapler staple -v Mooselutions.app 
popd

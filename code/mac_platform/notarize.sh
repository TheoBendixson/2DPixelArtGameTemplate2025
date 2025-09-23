
MAC_BUILD_DIRECTORY="../../build/mac_os"

pushd $MAC_BUILD_DIRECTORY
    xcrun notarytool submit Mooselutions.zip -v --keychain-profile "SendItApps" --wait
popd



MAC_BUILD_DIRECTORY="../../build/mac_os"

pushd $MAC_BUILD_DIRECTORY
    xcrun altool --upload-app --type osx -f mooselutions.pkg -u bendixso@gmail.com -apiKey 4Q367T887C -apiIssuer 69a6de8d-c068-47e3-e053-5b8c7c11a4d1 --verbose
popd

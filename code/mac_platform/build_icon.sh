
ICON_DIRECTORY="../../mac_os_icon/"
ICON_NAME="mac_os_icon.png"
ICON_DIRECTORY_NAME="AppIcon.iconset"

pushd $ICON_DIRECTORY
    sips -z 16 16   $ICON_NAME --out ${ICON_DIRECTORY_NAME}/icon_16x16.png
    sips -z 32 32   $ICON_NAME --out ${ICON_DIRECTORY_NAME}/icon_16x16@2x.png
    sips -z 32 32   $ICON_NAME --out ${ICON_DIRECTORY_NAME}/icon_32x32.png
    sips -z 64 64   $ICON_NAME --out ${ICON_DIRECTORY_NAME}/icon_32x32@2x.png
    sips -z 128 128 $ICON_NAME --out ${ICON_DIRECTORY_NAME}/icon_128x128.png
    sips -z 256 256 $ICON_NAME --out ${ICON_DIRECTORY_NAME}/icon_128x128@2x.png
    sips -z 256 256 $ICON_NAME --out ${ICON_DIRECTORY_NAME}/icon_256x256.png
    sips -z 512 512 $ICON_NAME --out ${ICON_DIRECTORY_NAME}/icon_256x256@2x.png
    sips -z 512 512 $ICON_NAME --out ${ICON_DIRECTORY_NAME}/icon_512x512.png
    cp $ICON_NAME ${ICON_DIRECTORY_NAME}/icon_512x512@2x.png
    iconutil -c icns -o AppIcon.icns AppIcon.iconset
popd

cp ${ICON_DIRECTORY}/AppIcon.icns resources/AppIcon.icns


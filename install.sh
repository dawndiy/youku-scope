#/bin/bash

./build-click-package.sh youku ubuntu-sdk-15.04 vivid

CLICK_PATH=$(find ./ -name "*.click")
echo $CLICK_PATH
adb push $CLICK_PATH /home/phablet
adb shell pkcon install-local --allow-untrusted $CLICK_PATH

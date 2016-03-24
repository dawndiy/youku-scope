#/bin/bash

# Function that executes a given command and compares its return command with a given one.
# In case the expected and the actual return codes are different it exits
# the script.
# Parameters:
#               $1: Command to be executed (string)
#               $2: Expected return code (number), may be undefined.
function executeCommand()
{
    # gets the command
    CMD=$1
    # sets the return code expected
    # if it's not definedset it to 0
    OK_CODE=$2
    if [ -n $2 ]
    then
        OK_CODE=0
    fi
    # executes the command
    ${CMD}

    # checks if the command was executed successfully
    RET_CODE=$?
    if [ $RET_CODE -ne $OK_CODE ]
    then
	echo ""
        echo "ERROR executing command: \"$CMD\""
        echo "Exiting..."
        exit 1
    fi
}

# ******************************************************************************
# *                                   MAIN                                     *
# ******************************************************************************

if [ $# -ne 3 ]
then
    echo "usage: $0 SCOPE_NAME FRAMEWORK_CHROOT SERIES_CHROOT"
    exit 1
fi

SCOPE_NAME=$1
CHROOT=$2
SERIES=$3

#CURRENT_DIR=`pwd`
CURRENT_DIR="/home/dawndiy/workspace/golang/"
GOROOT="/usr/local/lib/go"

echo -n "Removing $SCOPE_NAME directory... "
executeCommand "rm -rf ./$SCOPE_NAME"
echo "Done"

echo -n "Creating clean $SCOPE_NAME directory... "
executeCommand "mkdir -p $SCOPE_NAME/$SCOPE_NAME"
echo "Done"

echo -n "Copying scope files... "
executeCommand "cp $SCOPE_NAME.ini $SCOPE_NAME/$SCOPE_NAME/${SCOPE_NAME}.ubuntu-dawndiy_${SCOPE_NAME}.ini"
executeCommand "cp ${SCOPE_NAME}-settings.ini $SCOPE_NAME/$SCOPE_NAME/${SCOPE_NAME}.ubuntu-dawndiy_${SCOPE_NAME}-settings.ini"
executeCommand "cp *.png $SCOPE_NAME/$SCOPE_NAME/"
executeCommand "cp manifest.json $SCOPE_NAME/"
executeCommand "cp apparmor.json $SCOPE_NAME/"
executeCommand "cp account-plugin-apparmor.json $SCOPE_NAME/"
executeCommand "cp $SCOPE_NAME.application $SCOPE_NAME/"
executeCommand "cp $SCOPE_NAME.provider $SCOPE_NAME/"
executeCommand "cp $SCOPE_NAME.service $SCOPE_NAME/"
executeCommand "cp -R qml-plugin $SCOPE_NAME/"
executeCommand "cp -R data $SCOPE_NAME/$SCOPE_NAME/"
echo "Done"

echo -n "Cross compiling $SCOPE_NAME..."
cd src
executeCommand "click chroot -aarmhf -f$CHROOT -s $SERIES run CGO_ENABLED=1 GOARCH=arm GOARM=7 PKG_CONFIG_LIBDIR=/usr/lib/arm-linux-gnueabihf/pkgconfig:/usr/lib/pkgconfig:/usr/share/pkgconfig GOROOT=$GOROOT GOPATH=/usr/share/gocode/:$CURRENT_DIR CC=arm-linux-gnueabihf-gcc CXX=arm-linux-gnueabihf-g++ $GOROOT/bin/go build -ldflags '-extld=arm-linux-gnueabihf-g++' -o ../$SCOPE_NAME/$SCOPE_NAME/$SCOPE_NAME"
cd ../
echo "Done"

echo -n "Building click package ... "
executeCommand "click build $SCOPE_NAME"
echo "Done"

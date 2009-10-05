#!/bin/sh
#
# Creates a HarmonyCartOSX disk image (dmg) from the command line.
# usage:
#    Create_build.sh <version>
#
# The result will be a file called ~/Desktop/HarmonyCartOSX<ver>.dmg

if [ $# != 1 ]; then
	echo "usage: Create_build.sh version"
	exit 0
fi

VER="$1"
DMG="HarmonyCartOSX${VER}.dmg"
DISK="/Volumes/HarmonyCartOSX"

echo "Creating ${DMG} file ..."
gunzip -c template.dmg.gz > ${DMG}

echo "Mounting ${DMG} file ..."
hdiutil attach ${DMG}

echo "Adding Qt framework ..."
macdeployqt ../HarmonyCart.app/ -no-plugins

echo "Copying documentation ..."
ditto ../Announce.txt ../Changes.txt ../Copyright.txt ../License.txt ../Readme.txt ../Todo.txt ${DISK}

echo "Copying application ..."
cp -r ../HarmonyCart.app ${DISK}

echo "Ejecting ${DMG} ..."
hdiutil eject ${DISK}

echo "Compressing image, moving to Desktop ..."
hdiutil convert ${DMG} -format UDZO -imagekey zlib-level=9 -o ~/Desktop/${DMG}
rm -f ${DMG}

#!/bin/bash
# dear packagers, please run this script as follows in a chroot jail
# PREFIX=/path/to/install ./install.sh

if [ "${PREFIX}" == "" ]
then
	echo "You are installing sandbox globally, if you wish to abort, hit Ctrl-C now"
	echo ""
	echo "Enter installation path..."
	echo "default: /usr/local/share/games"
	read PREFIX

	if [ "${PREFIX}" == "" ]
	then
		PREFIX="/usr/local/share/games"
	fi

	echo "Sandbox will be installed in \"${PREFIX}/sandbox\""
	echo "Press enter to continue"
	read CC
fi

if [ -e "${PREFIX}/sandbox" ]
then
	echo "NOTE, \"${PREFIX}/sandbox\" already exists"
	rm -rf "${PREFIX}/sandbox"
fi

if [ -e "/usr/bin/sandbox" ]
then
	echo "NOTE, \"/usr/bin/sandbox\" already exists"
	rm -f "/usr/bin/sandbox"
fi

echo "Creating directory \"${PREFIX}/sandbox\""
mkdir -p "${PREFIX}/sandbox"
if [ $? -ne 0 ]
then
	echo "Failed to create directory, do you have permission?"
	exit 1
fi

echo "Copying files to \"${PREFIX}/sandbox\" (this may take a while)"
cp -r . "${PREFIX}/sandbox"
if [ $? -ne 0 ]
then
	echo "Failed to copy files, do you have permission?"
	exit 1
fi

echo "Creating symlink from \"/usr/bin/sandbox\" to \"${PREFIX}/sandbox/sandbox_unix\""
ln -s "${PREFIX}/sandbox/sandbox_unix" "/usr/bin/sandbox"
if [ $? -ne 0 ]
then
	echo "Failed to create symlink, do you have permission?"
	exit 1
fi

echo "Installing pixmaps to /usr/share/pixmaps"
cp ./linux/*.png "/usr/share/pixmaps"

echo "Installing .desktop files"
cp ./linux/*.desktop "/usr/share/applications"

echo "Sandbox has been successfully installed"
echo ""
echo "To fully uninstall sandbox, run the following commands"
echo "rm /usr/bin/sandbox"
echo "rm -r \"${PREFIX}/sandbox\""
echo "rm /usr/share/pixmaps/sandbox_*"
echo "rm /usr/share/applications/sandbox_*"
echo ""
echo "To run sandbox, simply enter \"sandbox\" into your shell of choice"
echo "or select one of the launcher from your desktop's menu under the \"Games\" subtree"
#!/bin/bash
# drodzy twórcy pakietów, proszę uruchomić ten skrypt za pomocą ograniczenia chroot
# PREFIX=/path/to/install ./install.sh

if [ "${PREFIX}" == "" ]
then
	echo "Instalujesz sandbox globalnie, jeśli chcesz zrezygnować, naciśnij teraz Ctrl-C"
	echo ""
	echo "Wprowadź ścieżkę instalowania..."
	echo "domyslnie: /usr/local/share/games"
	read PREFIX

	if [ "${PREFIX}" == "" ]
	then
		PREFIX="/usr/local/share/games"
	fi

	echo "Sandbox będzie zainstalowany w \"${PREFIX}/sandbox\""
	echo "Wciśnij enter aby kontynuować"
	read CC
fi

if [ -e "${PREFIX}/sandbox" ]
then
	echo "UWAGA, \"${PREFIX}/sandbox\" już istnieje"
	rm -rf "${PREFIX}/sandbox"
fi

if [ -e "/usr/bin/sandbox" ]
then
	echo "UWAGA, \"/usr/bin/sandbox\" już istnieje"
	rm -f "/usr/bin/sandbox"
fi

echo "Tworzenie katalogu \"${PREFIX}/sandbox\""
mkdir -p "${PREFIX}/sandbox"
if [ $? -ne 0 ]
then
	echo "Błąd tworzenia katalogu, masz uprawnienia?"
	exit 1
fi

echo "Kopiowanie plików do \"${PREFIX}/sandbox\" (to może trochę potrwać)"
cp -r . "${PREFIX}/sandbox"
if [ $? -ne 0 ]
then
	echo "Błąd kopiowania plików, masz uprawnienia?"
	exit 1
fi

echo "Tworzenie łącza symbolicznego z \"/usr/bin/sandbox\" do \"${PREFIX}/sandbox/sandbox_unix\""
ln -s "${PREFIX}/sandbox/sandbox_unix" "/usr/bin/sandbox"
if [ $? -ne 0 ]
then
	echo "Błąd tworzenia łącza symbolicznego, masz uprawnienia?"
	exit 1
fi

echo "Instalowanie pixmap do /usr/share/pixmaps"
cp ./linux/*.png "/usr/share/pixmaps"

echo "Instalowanie plików .desktop"
cp ./linux/*.desktop "/usr/share/applications"

echo "Sandbox został poprawnie zainstalowany"
echo ""
echo "Aby odinstalować sandbox, wprowadź poniższe polecenia"
echo "rm /usr/bin/sandbox"
echo "rm -r \"${PREFIX}/sandbox\""
echo "rm /usr/share/pixmaps/sandbox_*"
echo "rm /usr/share/applications/sandbox_*"
echo ""
echo "Aby uruchomić sandbox, po prostu wprowadź \"sandbox\" w terminalu lub wierszu poleceń"
echo "lub wybierz jeden z elementów uruchamiających w menu w kategorii \"Gry\" "

#!/bin/bash
# SANDBOX_DIR powinien wskazywać na katalog, w którym znajduje się sandbox.
#SANDBOX_DIR=~/sandbox
#SANDBOX_DIR=/usr/local/sandbox
SANDBOX_DIR=.

SANDBOX_OPTIONS="-r -t0"
#SANDBOX_OPTIONS="-r -q${HOME}/.pas271pl01"

if [ -a ${SANDBOX_DIR}/bin/sandbox_client_64_krs ]
then
    cd ${SANDBOX_DIR}
    chmod +x bin/sandbox_*
    exec ./bin/sandbox_client_64_krs $SANDBOX_OPTIONS $*
else
    echo "A problem was encountered, please check which of the following it is."
    echo "1) the executable was moved"
    echo "2) There isn't an executable"
    echo "the script will attempt to create a client in 10 seconds, press CTRL-C to cancel"
    sleep 10
    cd src
    chmod +x build.sh
    exec build.sh
    cd ../
    exit 1
fi

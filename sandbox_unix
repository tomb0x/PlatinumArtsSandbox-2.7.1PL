#!/bin/bash
# SANDBOX_DIR should refer to the directory in which sandbox is placed, the default should be good enough.
#SANDBOX_DIR=~/sandbox
#SANDBOX_DIR=/usr/local/sandbox
SANDBOX_DIR=$(dirname $(readlink -f $0))

SANDBOX_OPTIONS=""
SANDBOX_MODULE="fps"
SANDBOX_HOME="${HOME}/.pas271pl01"
SANDBOX_TYPE="client"
SANDBOX_PREFIX="sandbox"
SANDBOX_EXEC=""

while [ $# -ne 0 ]
do
    case $1 in
        "-h"|"-?"|"-help"|"--help"|"-pomoc"|"--pomoc")
            echo ""
            echo "Skrypt uruchamiający Sandboksa na Linuksie"
            echo "Przykład: ./sandbox_unix -gssp -t1"
            echo ""
            echo "   Argumenty skryptu"
            echo "  -h|-?|-help|--help  pokazuje ten tekst pomocy"
            echo "  -g<napis>       ustaw tryb gry na <napis> (domyślnie: fps)"
            echo "              dostępne moduły: fps, krs, movie, rpg, ssp"
            echo "  --server        uruchamia dedykowany program serwera"
            echo "  --master        uruchamia serwer główny do rejestracji serwera"
            echo "              NOTE: compiled masterserver not included"
            echo "  --debug     starts the debug build(s) inside GDB"
            echo "              note that all arguments passed to this script will be"
            echo "              passed to sandbox when run is invokved in gdb."
            echo "              it's recommended that you do this in windowed mode (-t0)"
            echo ""
            echo "   Opcje programu klienta"
            echo "  -q<napis>       użyj <napis> jako katalogu domowego (domyślny: ~/.platinumarts)"
            echo "  -k<napis>       montuje <napis> jako katalog pakietów"
            echo "  -r<napis>       wykonuje <napis> przed uruchomieniem, użyj zamiast argumentów takich jak -t, -a, -h, itp"
            echo "  -t<num>     ustaw pełny ekran <num>"
            echo "  -d<num>     obsługuje serwer dedykowany (0) lub serwer nasłuchowy (1)"
            echo "  -w<num>     ustawia szerokość okna, wysokość jest ustawiana na: szerokość*3/4, chyba że podano również inną opcję"
            echo "  -h<num>     ustawia wysokość okna, szerokość jest ustawiana na: wysokość*4/3, chyba że podano również inną opcję"
            echo "  -z<num>     ustawia bity bufora głębi (z) (nie dotykaj)"
            echo "  -b<num>     ustawia bity kolorów (zwykle 32-bitowe)"
            echo "  -a<num>     ustawia antypostrzępienie na wartość <num>"
            echo "  -v<num>     ustawia vsync na <num> -- -1 to wartość automatyczna"
            echo "  -t<num>     ustawia pełny ekran na <num>"
            echo "  -s<num>     ustawia bity bufora szablonu na <num> (nie dotykaj)"
            echo "  -f<num>     ustawia precyzję ścieżki renderowania/cieniowania"
            echo "      0 - wyłącza światłocienie"
            echo "      1 - 3 ustawia precyzję cieniowaczy dla ścieżki renderowania ASM/GLSL"
            echo "      4 - 6 ustawia precyzję cieniowaczy dla ścieżki renderowania GLSL"
            echo "  -l<napis>       po zainicjowaniu środowiska uruchamia mapę <napis>"
            echo "  -x<napis>       po zainicjowaniu środowiska uruchamia skrypt <napis>"
            echo ""
            echo "  Opcje serwera"
            echo "  INFORMACJA: program dedykowanego serwera używa pliku initserver.cfg, co zastępuje poniższe"
            echo "  -u<num>     ustawia prędkość wysyłania na <num>"
            echo "  -c<num>     ustawia maksymalną liczbę klientów (maks: 127 ludzi)"
            echo "  -i<napis>       ustawia adres ip serwera (używaj ostrożnie)"
            echo "  -j<num>     ustawia port używany do połączeń z serwerem"
            echo "  -m<napis>       ustawia adres URL serwera głównego na <napis>"
            echo "  -q<napis>       używa <napis> jako katalog domowy (domyślnie: ~/.platinumarts)"
            echo "  -k<napis>       podłącza <napis> jaka katalog pakietów"
            echo ""
            echo "Skrypt uruchamiający Sandbox, autor Kevin \"Hirato Kirata\" Meyer"
            echo "(c) 2008-2011 - na licencji zlib/libpng"
            echo ""

            exit 1
        ;;
    esac

    tag=$(expr substr "$1" 1 2)
    argument=$(expr substr "$1" 3 1022)

    case $tag in
        "-g")
            SANDBOX_MODULE=$argument
        ;;
        "-q")
            SANDBOX_HOME="\"$argument\""
        ;;
        "--")
            case $argument in
            "server")
                SANDBOX_TYPE="server"
            ;;
            "master")
                SANDBOX_TYPE="master"
            ;;
            "debug")
                SANDBOX_PREFIX="debug"
                SANDBOX_EXEC="gdb --args"
            ;;
            esac
        ;;
        *)
            SANDBOX_OPTIONS+=" \"$tag$argument\""
        ;;
    esac

    shift
done

case $(uname -m) in
i386|i486|i586|i686)
  MACHINE_BIT=32
  ;;
*)
  MACHINE_BIT=64 #assume 64bit otherwise
  ;;
esac

function failed {
    echo ""
    echo "A problem was encountered, please check which of the following it is."
    echo "1) there's no ${SANDBOX_TYPE} for module ${SANDBOX_MODULE}"
    echo "2) There isn't an available executable for your architecture; $(uname -m)"
    echo "3) the executable was moved"
    echo "please make sure that ${SANDBOX_DIR}/bin/${SANDBOX_PREFIX}_${SANDBOX_TYPE}_${MACHINE_BIT}_${SANDBOX_MODULE} exists. If it doesn't..."
    echo "install the sdl, sdl-image and sdl-mixer DEVELOPMENT libraries and use \"make -C ${SANDBOX_DIR}/src install\" to compile a binary"
    echo ""
    exit 1
}


cd ${SANDBOX_DIR}
case ${SANDBOX_TYPE} in
    "client")
        if [ -a bin/${SANDBOX_PREFIX}_client_${MACHINE_BIT}_${SANDBOX_MODULE} ]
        then
            eval ${SANDBOX_EXEC} ./bin/${SANDBOX_PREFIX}_client_${MACHINE_BIT}_${SANDBOX_MODULE} -q${SANDBOX_HOME} -r ${SANDBOX_OPTIONS}
        else
            failed
        fi
    ;;
    "server")
        if [ -a bin/${SANDBOX_PREFIX}_server_${MACHINE_BIT}_${SANDBOX_MODULE} ]
        then
            eval ${SANDBOX_EXEC} ./bin/${SANDBOX_PREFIX}_server_${MACHINE_BIT}_${SANDBOX_MODULE}  -q${SANDBOX_HOME} ${SANDBOX_OPTIONS}
        else
            failed
        fi
    ;;
    "master")
        if [ -a bin/${SANDBOX_PREFIX}_master ]
        then
            eval ${SANDBOX_EXEC} ./bin/${SANDBOX_PREFIX}_master
        else
            failed
        fi
    ;;
esac

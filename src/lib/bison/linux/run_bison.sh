DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

export BISON_PKGDATADIR="${DIR}/share/bison/"
echo Bison packages found at $BISON_PKGDATADIR ...
echo Params: "$@"
./bin/bison "$@"

#!/bin/sh

#-------------------------------------------------------------------------------
# XXX move to make/cmake/etc

EXT="./ext"
CMD=$1; shift

PHP_MODULE_DIR="/usr/lib/php/modules"

function do_prepare {
    echo -n "writing current proto definitions to source tree "

    TMP_FILE=$( mktemp )
    PROTO_FILES_C=$( ls $EXT/proto/*.c | rev | awk -F\/ '{ print $1"/"$2 }' | rev )
    PROTO_FILES_H=$( ls $EXT/proto/*.h | rev | awk -F\/ '{ print $1"/"$2 }' | rev )

    # write phproto.h
    for PROTO_FILE in $PROTO_FILES_H; do 
        echo "#include \"$PROTO_FILE\"" >> $TMP_FILE
    done
    
    echo "static const ProtobufCMessageDescriptor* phproto_messages[] = {" >> $TMP_FILE
    for PROTO_FILE in $PROTO_FILES_H; do 
        egrep -o '\&([a-z_]+__descriptor)' $EXT/$PROTO_FILE | sed 's,&,,g' | while read LINE; do
            echo "    { &$LINE }," >> $TMP_FILE
        done
    done
    echo "};" >> $TMP_FILE
    echo -n "."

    echo "" > $EXT/phproto.h
    cat $EXT/phproto.h.template >> $EXT/phproto.h
    cat $TMP_FILE >> $EXT/phproto.h
    echo "#endif" >> $EXT/phproto.h
    echo -n "."

    # write config.m4
    sed "s,{PROTOFILES},$( echo $PROTO_FILES_C ),g" $EXT/config.m4.template > $EXT/config.m4
    echo ". done"

    exit 0
}

function do_build {
    echo -n "building extension "

    TMP_FILE=$( mktemp )
    (
        cd $EXT
        phpize
        ./configure --enable-phproto
        make
        cd ..
    ) >> $TMP_FILE 2>&1
    echo ". done"
}

function do_install {
    echo -n "installing "

    if [ -f "$EXT/modules/phproto.so" ]; then
        sudo cp $EXT/modules/phproto.so $PHP_MODULE_DIR/.
        echo ". done"
    else
        echo ". failed."
    fi
}

function do_clean {
    echo -n "cleaning up "
    cd $EXT
    make clean > /dev/null 2>&1
    echo -n "."
    phpize --clean > /dev/null 2>&1
    echo -n "."
    cd ..
    echo ". done"
}

case "$CMD" in
    "prepare")
        do_prepare
    ;;
    "build")
        do_build
    ;;
    "install")
        do_install
    ;;
    "clean")
        do_clean
    ;;
    *)
        echo "usage: $0 prepare|build|install|clean"
        exit 1
    ;;
esac
#!/bin/sh

BASE_PATH="./ext"
LOG="$( pwd )/$( mktemp --tmpdir=. build.log.XXX )"
PROTO_PATH="${BASE_PATH}/proto"
PHP_MODULE_DIR="/usr/lib/php/modules"

HTTPD_USER="http"
HTTPD_GROUP="http"

PROTOC_C=$( which protoc-c )
EGREP=$( which egrep )
SED=$( which sed )
PHPIZE=$( which phpize )
MAKE=$( which make )

#
# print a relative list of files in $PROTO_PATH, matching a pattern
#
get_proto_files()
{
    find $PROTO_PATH -type f -name $1 -printf '%f\n'
}

#
# compile a proto file in/to $PROTO_PATH
#
compile_proto_file()
{
    if [ -f "${PROTO_PATH}/$1" ]; then
        cd $PROTO_PATH
        if ! $PROTOC_C --c_out=. $1 >> $LOG 2>&1; then
            cd - > /dev/null 2>&1
            return 1
        else
            cd - > /dev/null 2>&1
            return 0
        fi
    fi
    return 42 # usage foo
}

#
# extracts all descriptors from a compiled header file
#
extract_descriptors()
{
    $EGREP -o '\&([a-z_]+__descriptor)' $PROTO_PATH/$1 | $SED 's,&,,g'
}

#
# generates a phproto_message[] array from all extracted descriptors
#
generate_descriptor_array()
{
    for FILE in $( get_proto_files "*.h" ); do
        echo "#include \"proto/${FILE}\""
    done

    echo

    echo "static const ProtobufCMessageDescriptor* phproto_messages[] = {"

    for FILE in $( get_proto_files "*.h" ); do
        for DESCRIPTOR in $( extract_descriptors $FILE ); do
            echo "    &${DESCRIPTOR}, "
        done
    done

    echo "};"
}

#
# prepare for build
#
prepare()
{
    # compile all .proto files
    for FILE in $( get_proto_files "*.proto" ); do
        echo -n "compiling $FILE.. "
        if ! compile_proto_file $FILE; then
            echo "FAILED"
            return 1
        else
            echo "SUCCESS"
        fi
    done

    # generate phproto.h from template and descriptors
    echo -n "generating phproto.h.. "
    if ! cp $BASE_PATH/phproto.h.template $BASE_PATH/phproto.h >> $LOG 2>&1; then
        echo "FAILED"
        return 1
    else
        generate_descriptor_array >> $BASE_PATH/phproto.h
        echo "#endif" >> $BASE_PATH/phproto.h
        echo "SUCCESS"
    fi

    # generate config.m4 by adding compiled .c files
    echo -n "generating config.m4.. "
    PROTOFILES=$( get_proto_files '*.c' | $SED 's,^,proto/,g' | tr '\n' ' ' )
    if ! $SED "s,{PROTOFILES},${PROTOFILES},g" $BASE_PATH/config.m4.template > $BASE_PATH/config.m4 2>&1; then
        echo "FAILED"
        return 1
    else
        echo "SUCCESS"
    fi
}

#
# build
#
build()
{
    cd $BASE_PATH
   
    echo -n "PHPizing source tree.. "
    if ! $PHPIZE >> $LOG 2>&1; then
        echo "FAILED"
        return 1
    else
        echo "SUCCESS"
    fi

    echo -n "configuring.. "
    if ! ./configure --enable-phproto >> $LOG 2>&1; then
        echo "FAILED"
        return 1
    else
        echo "SUCCESS"
    fi

    echo -n "building.. "
    NUM_PROCS=$(( $( grep processor /proc/cpuinfo | wc -l ) * 2))
    if ! $MAKE -j$NUM_PROCS >> $LOG 2>&1; then
        echo "FAILED"
        return 1
    else
        echo "SUCCESS"
    fi

    cd - > /dev/null 2>&1
    return 0
}

#
# "deploy" :)
#
deploy()
{
    echo -n "deploying.. "
    if ! install -o$HTTPD_USER -g$HTTPD_GROUP -m0644 $BASE_PATH/modules/phproto.so \
        $PHP_MODULE_DIR/. >> $LOG 2>&1; then
    
        echo "FAILED"
        return 1
    else
        echo "SUCCESS"
    fi
    return 0
}

#
#------------- main
#
if ! prepare; then
    exit 1
fi

if ! build; then
    exit 1
fi

if ! deploy; then
    exit 1;
fi

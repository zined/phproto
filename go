#!/bin/sh

#-------------------------------------------------------------------------------
# XXX move to make/cmake/etc

EXT="./ext"
PHP_MODULE_DIR="/usr/lib/php/modules"
TMP_FILE=$( mktemp )
PROTO_FILES_C=$( ls $EXT/proto/*.c | rev | awk -F\/ '{ print $1"/"$2 }' | rev )
PROTO_FILES_H=$( ls $EXT/proto/*.h | rev | awk -F\/ '{ print $1"/"$2 }' | rev )

# protobufs
echo " -> compiling protobufs"

for PROTO in $( ls $EXT/proto/*.proto ); do 
    cd $EXT/proto/
    protoc-c --c_out=. $( basename $PROTO )
    cd -
done

echo " -> writing current proto definitions to source tree "

# write phproto.h
for PROTO_FILE in $PROTO_FILES_H; do 
    echo "#include \"$PROTO_FILE\"" >> $TMP_FILE
done

echo "static const ProtobufCMessageDescriptor* phproto_messages[] = {" >> $TMP_FILE
for PROTO_FILE in $PROTO_FILES_H; do 
    egrep -o '\&([a-z_]+__descriptor)' $EXT/$PROTO_FILE | sed 's,&,,g' | while read LINE; do
        echo "    &$LINE," >> $TMP_FILE
    done
done
echo "};" >> $TMP_FILE

echo "" > $EXT/phproto.h
cat $EXT/phproto.h.template >> $EXT/phproto.h
cat $TMP_FILE >> $EXT/phproto.h
echo "#endif" >> $EXT/phproto.h

# write config.m4
sed "s,{PROTOFILES},$( echo $PROTO_FILES_C ),g" $EXT/config.m4.template > $EXT/config.m4

echo " -> building"

cd $EXT
if [ ! -f "./Makefile" ]; then
    phpize
    ./configure --enable-phproto
fi
make -j8
cd -

echo " -> deploy"

sudo cp -v $EXT/modules/phproto.so $PHP_MODULE_DIR/.

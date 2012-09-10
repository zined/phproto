phproto
=======

## isn't there already ...?

yes, there is. but not compiled. one great aspect of protobuffers is that they're being compiled. se we do that. use protoc-c to compile our protobufs to c, then build the extension on top of that.

## why?

because protobufs are awesome

## how?

get libprotobuf-c and the protoc-c compiler. place your .proto files in ext/proto/. and build the extension. put it in your php installation and ... profit.


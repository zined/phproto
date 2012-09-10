
#include "php.h"
#include "zend_exceptions.h"
#include "google/protobuf-c/protobuf-c.h"

int php_message (const ProtobufCMessage*, zval*);
void message_php (zval*, const ProtobufCMessage*);

void int32_php_single (zval*, char*, const int32_t*);
void int32_php_repeated (zval*, char*, const int32_t**, unsigned int);
void uint32_php_single (zval*, char*, const uint32_t*);
void uint32_php_repeated (zval*, char*, const uint32_t**, unsigned int);
void string_php_single (zval*, char*, char* const*);
void string_php_repeated (zval*, char*, char** const*, unsigned int);

int string_proto (const ProtobufCMessage*, const ProtobufCFieldDescriptor*, zval**);
int int32_proto (const ProtobufCMessage*, const ProtobufCFieldDescriptor*, zval**);
int uint32_proto (const ProtobufCMessage*, const ProtobufCFieldDescriptor*, zval**);
int message_proto (const ProtobufCMessage*, const ProtobufCFieldDescriptor*, zval**);
int bool_proto (const ProtobufCMessage*, const ProtobufCFieldDescriptor*, zval**);

// TODO
void bool_php (zval*, const char*, protobuf_c_boolean);
void double_php (zval*, const char*, uint64_t);
void float_php (zval*, const char*, uint32_t);
void sint32_php (zval*, const char*, int32_t);
void fixed32_php (zval*, const char*, uint32_t);
void sfixed32_php (zval*, const char*, uint32_t);
void int64_php (zval*, const char*, uint64_t);
void sint64_php (zval*, const char*, int64_t);
void uint64_php (zval*, const char*, uint64_t);
void fixed64_php (zval*, const char*, uint64_t);
void sfixed64_php (zval*, const char*, uint64_t);


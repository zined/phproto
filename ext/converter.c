
#include "converter.h"

#include <syslog.h>

/******************************************************************************/
/* static helper methods */

/* Return the pointer of the quantifier for a given field in a message, which can be
   used both for the "has_XXX" and "n_XXX" fields. */
static unsigned int*
get_quantifier (const ProtobufCMessage* message, const ProtobufCFieldDescriptor* field) {
    unsigned int* quantifier = (unsigned int*)(((const char*)message) + field->quantifier_offset);
    return quantifier;
}

/* Return the pointer of a field's position inside a message. */
static const void*
get_member (const ProtobufCMessage* message, const ProtobufCFieldDescriptor* field) {
    const void* member = ((const char*)message) + field->offset;
    return member;
}

/******************************************************************************/
/* external interface methods */

/* Iterate over the given php zval** and transform into protobuf compatible values.
   Write those into the given protobuf message pointer. */
int
php_message (const ProtobufCMessage* message, zval* val)
{
    HashPosition pos;
    HashTable* hash_table = Z_ARRVAL_P(val);

    zval** data;

    char* key;
    int i, j, key_len;
    long index;

    // check if all required fields are existent in the given php array
    for (j=0 ; j < message->descriptor->n_fields ; ++j) {
        const ProtobufCFieldDescriptor* tmp_field = message->descriptor->fields + j;
        if (tmp_field->label != PROTOBUF_C_LABEL_REQUIRED) continue;
        if (! zend_symtable_exists(hash_table, (char*)tmp_field->name, strlen((char*)(tmp_field->name)) + 1)) {

            zend_throw_exception_ex(zend_exception_get_default(TSRMLS_C), 0 TSRMLS_CC, "cannot find required field '%s'", tmp_field->name);
            return 1;
        }
    }

    // iterate php array
    zend_hash_internal_pointer_reset_ex(hash_table, &pos);
    for (;; zend_hash_move_forward_ex(hash_table, &pos)) {
        zend_hash_get_current_data_ex(hash_table, (void**)&data, &pos);

        i = zend_hash_get_current_key_ex(hash_table, &key, &key_len, &index, 0, &pos);

        if (i == HASH_KEY_NON_EXISTANT) {
            break;
        } else if (i == HASH_KEY_IS_STRING) {

            // find corresponding protobuf field for array key
            const ProtobufCFieldDescriptor* field = NULL;
            for (j=0 ; j < message->descriptor->n_fields ; ++j) {
                const ProtobufCFieldDescriptor* tmp_field = message->descriptor->fields + j;
                if (! strcmp(key, tmp_field->name)) {
                    field = tmp_field;
                    break;
                }
            }

            if (field == NULL) {
                continue;
            }

            // string
            if (field->type == PROTOBUF_C_TYPE_STRING) {
                string_proto(message, field, data);
            }

            // long => int32
            else if (field->type == PROTOBUF_C_TYPE_INT32) {
                int32_proto(message, field, data);
            }

            // uint32
            else if (field->type == PROTOBUF_C_TYPE_UINT32) {
                uint32_proto(message, field, data);
            }

            else if (field->type == PROTOBUF_C_TYPE_MESSAGE) {
                message_proto(message, field, data);
            }

            // not implemented
            else {
                php_printf("type not implemented yet.\n");
            }
        }
    }

    return 0;
}

/* Iterate over the given protobuf message pointer and transform values into native
   php types. writes an associative array into the given zval**. */
void message_php (zval* return_value, const ProtobufCMessage *message)
{
    unsigned int i, j;
    for (i=0;i<message->descriptor->n_fields;i++) {
        const ProtobufCFieldDescriptor *field = message->descriptor->fields + i;

        const void* member = get_member(message, field);
        unsigned int* num_fields = get_quantifier(message, field);

        const ProtobufCMessage** sub_messages;
        const ProtobufCMessage* sub_message;

        zval *inner, *inner_repeated;

        switch (field->type) {

            case PROTOBUF_C_TYPE_INT32:
                if (field->label == PROTOBUF_C_LABEL_OPTIONAL && (protobuf_c_boolean)*num_fields) {
                    int32_php_single(return_value, (char*)field->name, (const int32_t*)member);
                } else if (field->label == PROTOBUF_C_LABEL_REQUIRED) {
                    int32_php_single(return_value, (char*)field->name, (const int32_t*)member);
                } else if (field->label == PROTOBUF_C_LABEL_REPEATED) {
                    int32_php_repeated(return_value, (char*)field->name, (const int32_t**)member, *num_fields);
                }
            break;

            case PROTOBUF_C_TYPE_UINT32:
                if (field->label == PROTOBUF_C_LABEL_OPTIONAL && (protobuf_c_boolean)*num_fields) {
                    uint32_php_single(return_value, (char*)field->name, (const uint32_t*)member);
                } else if (field->label == PROTOBUF_C_LABEL_REQUIRED) {
                    uint32_php_single(return_value, (char*)field->name, (const uint32_t*)member);
                } else if (field->label == PROTOBUF_C_LABEL_REPEATED) {
                    uint32_php_repeated(return_value, (char*)field->name, (const uint32_t**)member, *num_fields);
                }
            break;

            case PROTOBUF_C_TYPE_STRING:
                if (field->label == PROTOBUF_C_LABEL_OPTIONAL) {
                    char** pos = (char**)member;
                    if (*pos != NULL) {
                        string_php_single(return_value, (char*)field->name, (char* const*)member);
                    }
                } else if (field->label == PROTOBUF_C_LABEL_REQUIRED) {
                    string_php_single(return_value, (char*)field->name, (char* const*)member);
                } else if (field->label == PROTOBUF_C_LABEL_REPEATED) {
                    string_php_repeated(return_value, (char*)field->name, (char** const*)member, *num_fields);
                }
            break;

            case PROTOBUF_C_TYPE_BOOL:
                bool_php(return_value, field->name, *(const protobuf_c_boolean *)member);
            break;

            case PROTOBUF_C_TYPE_MESSAGE:

                // optional or repeated field with no entries shouldn't make it to PHP
                if (field->label == PROTOBUF_C_LABEL_OPTIONAL || field->label == PROTOBUF_C_LABEL_REPEATED) {
                    if (*((const ProtobufCMessage**)member) == NULL) {
                        break;
                    }
                }

                MAKE_STD_ZVAL(inner);
                array_init(inner);

                if (field->label == PROTOBUF_C_LABEL_OPTIONAL || field->label == PROTOBUF_C_LABEL_REQUIRED) {
                    message_php(inner, *((const ProtobufCMessage**)member));
                } else if (field->label == PROTOBUF_C_LABEL_REPEATED) {
                    sub_messages = (const ProtobufCMessage**)*((const ProtobufCMessage**)member);
                    for (j=0;j<*num_fields;++j) {
                        MAKE_STD_ZVAL(inner_repeated);
                        array_init(inner_repeated);
                        message_php(inner_repeated, sub_messages[j]);
                        add_next_index_zval(inner, inner_repeated);
                    }
                }

                add_assoc_zval(return_value, (char*)field->name, inner);

            break;
                
            case PROTOBUF_C_TYPE_DOUBLE:
//                double_php(return_value, field->name, *(const uint64_t *)member);
            case PROTOBUF_C_TYPE_FLOAT:
//                float_php(return_value, field->name, *(const uint32_t *)member);
            case PROTOBUF_C_TYPE_SINT32:
//                sint32_php(return_value, field->name, *(const int32_t *)member);
            case PROTOBUF_C_TYPE_FIXED32:
//                fixed32_php(return_value, field->name, *(const uint32_t *)member);
            case PROTOBUF_C_TYPE_SFIXED32:
//                sfixed32_php(return_value, field->name, *(const uint32_t *)member);
            case PROTOBUF_C_TYPE_INT64:
//                int64_php(return_value, field->name, *(const uint64_t *)member);
            case PROTOBUF_C_TYPE_SINT64:
//                sint64_php(return_value, field->name, *(const int64_t *)member);
            case PROTOBUF_C_TYPE_UINT64:
//                uint64_php(return_value, field->name, *(const uint64_t *)member);
            case PROTOBUF_C_TYPE_FIXED64:
//                fixed64_php(return_value, field->name, *(const uint64_t *)member);
            case PROTOBUF_C_TYPE_SFIXED64:
//                sfixed64_php(return_value, field->name, *(const uint64_t *)member);
            // XXX unimplemented XXX
            case PROTOBUF_C_TYPE_ENUM:
            case PROTOBUF_C_TYPE_BYTES:
            default:
                break;
        }
    }
}

/******************************************************************************/
/* proto to php conversions */

/* Add single string value to zval by key */
void
string_php_single (zval* out, char* name, char* const* val)
{
    // <3 folding
    add_assoc_string(out, name, (char*)val[0], 1);
}

/* Add numeric indexed string value array to given zval by key */
void
string_php_repeated (zval* out, char* name, char** const* val, unsigned int num_fields)
{
    unsigned int i;
    char** arr = *val;
    zval* repeated;
    MAKE_STD_ZVAL(repeated);
    array_init(repeated);

    for (i=0;i<num_fields;++i) {
        add_next_index_string(repeated, arr[i], 1);
    }

    add_assoc_zval(out, name, repeated);
}

/* Add single int32 value to zval by key */
void
int32_php_single (zval* out, char* name, const int32_t* val)
{
    // <3 folding
    add_assoc_long(out, name, (long)val[0]);
}

/* Add single int32 value to zval by key */
/* Add numeric indexed int32 value array to given zval by key */
void
int32_php_repeated (zval* out, char* name, const int32_t** val, unsigned int num_fields)
{
    unsigned int i;
    const int32_t* arr = *val;
    zval* repeated;
    MAKE_STD_ZVAL(repeated);
    array_init(repeated);

    for (i=0;i<num_fields;++i) {
        add_next_index_long(repeated, (long)arr[i]);
    }
    
    add_assoc_zval(out, name, repeated);
}

/* Add single uint32 value to zval by key */
void
uint32_php_single (zval* out, char* name, const uint32_t* val)
{
    // <3 folding
    add_assoc_long(out, name, (long)val[0]);
}

/* Add numeric indexed uint32 value array to given zval by key */
void
uint32_php_repeated (zval* out, char* name, const uint32_t** val, unsigned int num_fields)
{
    unsigned int i;
    const uint32_t* arr = *val;
    zval* repeated;
    MAKE_STD_ZVAL(repeated);
    array_init(repeated);

    for (i=0;i<num_fields;++i) {
        add_next_index_long(repeated, (long)arr[i]);
    }

    add_assoc_zval(out, name, repeated);
}

/******************************************************************************/
/* php to proto conversions */

/* Extract string values from the given zval**, and write them into the given protobuf
   message pointer. handle optional/required/repeated strings */
void
string_proto (const ProtobufCMessage* message, const ProtobufCFieldDescriptor* field, zval** val)
{
    const void* member = get_member(message, field);
    unsigned int* quantifier = get_quantifier(message, field);

    if (field->label == PROTOBUF_C_LABEL_REQUIRED || field->label == PROTOBUF_C_LABEL_OPTIONAL)
    {
        char* value = Z_STRVAL_PP(val);
        memcpy((void*)member, (void*)&value, sizeof(void*));
    }

    else if (field->label == PROTOBUF_C_LABEL_REPEATED)
    {
        if (Z_TYPE_PP(val) != IS_ARRAY)
            return; // XXX error handling XXX

        HashPosition pos;
        HashTable* hash_table = Z_ARRVAL_PP(val);
        size_t num_elements = (size_t)zend_hash_num_elements(hash_table);
        zval** data;
        char* key;
        int i, key_len, curr = 0;
        long index;
        char** values = emalloc(sizeof(char*) * num_elements);

        zend_hash_internal_pointer_reset_ex(hash_table, &pos);
        for (;; zend_hash_move_forward_ex(hash_table, &pos)) {
            zend_hash_get_current_data_ex(hash_table, (void**)&data, &pos);

            i = zend_hash_get_current_key_ex(hash_table, &key, &key_len, &index, 0, &pos);
            if (i == HASH_KEY_NON_EXISTANT) {
                break;
            }

            if (Z_TYPE_PP(data) == IS_STRING) {
                values[curr++] = (char*)(Z_STRVAL_PP(data));
            }
        }

        *quantifier = (int32_t)num_elements;
        memcpy((void*)member, (void*)&values, sizeof(void*));
    }
}

/* Extract a single optional/required or multiple repeated sub messages from native
   php types into the given protobuf message. Allocates memory for the created messages */
void
message_proto (const ProtobufCMessage* message, const ProtobufCFieldDescriptor* field, zval** val)
{
    if (Z_TYPE_PP(val) != IS_ARRAY)
        return; // XXX error handling XXX

    const void* member = get_member(message, field);
    unsigned int* quantifier = get_quantifier(message, field);
    ProtobufCMessageDescriptor* descriptor = (ProtobufCMessageDescriptor*)field->descriptor;

    if (field->label == PROTOBUF_C_LABEL_REQUIRED)
    {
        ProtobufCMessage* base = emalloc(descriptor->sizeof_message);
        protobuf_c_message_init(descriptor, base);
        php_message(base, val[0]);
        memcpy((void*)member, (void*)&base, sizeof(void*));
    }
    else if (field->label == PROTOBUF_C_LABEL_REPEATED)
    {
        HashPosition pos;
        HashTable* hash_table = Z_ARRVAL_PP(val);

        size_t* num_elements = emalloc(sizeof(size_t));
        num_elements[0] = (size_t)zend_hash_num_elements(hash_table);

        zval** data;
        char* key;
        int i, key_len, curr = 0;
        long index;
        
        void** values = emalloc(sizeof(void*) * *num_elements);

        zend_hash_internal_pointer_reset_ex(hash_table, &pos);
        for (;; zend_hash_move_forward_ex(hash_table, &pos)) {
            zend_hash_get_current_data_ex(hash_table, (void**)&data, &pos);

            i = zend_hash_get_current_key_ex(hash_table, &key, &key_len, &index, 0, &pos);
            if (i == HASH_KEY_NON_EXISTANT) {
                break;
            }

            void* curr_value = emalloc(descriptor->sizeof_message);
            protobuf_c_message_init(descriptor, curr_value);
            php_message((ProtobufCMessage*)curr_value, data[0]);
            values[curr] = curr_value;
            curr++;
        }

        memcpy((void*)member, (void*)&values, sizeof(void*));
        memcpy((void*)quantifier, (void*)num_elements, sizeof(void*));
    }
}

/* Extract int32_t values from the given zval** and write them into the given protobuf
   message pointer. handle optional/required/repeated */
void
int32_proto (const ProtobufCMessage* message, const ProtobufCFieldDescriptor* field, zval** val)
{
    int32_t* member = (int32_t*)get_member(message, field);
    unsigned int* quantifier = get_quantifier(message, field);

    if (field->label == PROTOBUF_C_LABEL_REQUIRED || field->label == PROTOBUF_C_LABEL_OPTIONAL) {
        if (Z_TYPE_PP(val) == IS_LONG)
            *member = (int32_t)Z_LVAL_PP(val);
        else if (Z_TYPE_PP(val) == IS_DOUBLE)
            *member = (int32_t)Z_DVAL_PP(val);
        else
            return; // XXX error handling XXX

        if (field->label == PROTOBUF_C_LABEL_OPTIONAL)
            *quantifier = 1;
    
    }
    else if (field->label == PROTOBUF_C_LABEL_REPEATED)
    {
        if (Z_TYPE_PP(val) != IS_ARRAY)
            return; // XXX error handling XXX

        HashPosition pos;
        HashTable* hash_table = Z_ARRVAL_PP(val);        
        size_t num_elements = (size_t)zend_hash_num_elements(hash_table);
        zval** data;
        char* key;
        int i, key_len, curr = 0;
        long index;
        int32_t* values = emalloc(sizeof(int32_t) * num_elements);

        zend_hash_internal_pointer_reset_ex(hash_table, &pos);
        for (;; zend_hash_move_forward_ex(hash_table, &pos)) {
            zend_hash_get_current_data_ex(hash_table, (void**)&data, &pos);

            i = zend_hash_get_current_key_ex(hash_table, &key, &key_len, &index, 0, &pos);
            if (i == HASH_KEY_NON_EXISTANT)
                break;

            if (Z_TYPE_PP(data) == IS_LONG)
                values[curr++] = (int32_t)(Z_LVAL_PP(data));
            else if (Z_TYPE_PP(data) == IS_DOUBLE)
                values[curr++] = (int32_t)(Z_DVAL_PP(data));
        }

        *quantifier = num_elements;
        memcpy((void*)member, (void*)&values, sizeof(void*));
    }
}

/* Extract uint32_t values from the given zval** and write them into the given protobuf
   message pointer. handle optional/required/repeated */
void
uint32_proto (const ProtobufCMessage* message, const ProtobufCFieldDescriptor* field, zval** val)
{
    uint32_t* member = (uint32_t*)get_member(message, field);
    unsigned int* quantifier = get_quantifier(message, field);

    if (field->label == PROTOBUF_C_LABEL_REQUIRED || field->label == PROTOBUF_C_LABEL_OPTIONAL)
    {
        if (Z_TYPE_PP(val) == IS_LONG)
            *member = (uint32_t)Z_LVAL_PP(val);
        else if (Z_TYPE_PP(val) == IS_DOUBLE)
            *member = (uint32_t)Z_DVAL_PP(val);

        if (field->label == PROTOBUF_C_LABEL_OPTIONAL)
            *quantifier = 1;

    }
    else if (field->label == PROTOBUF_C_LABEL_REPEATED)
    {
        if (Z_TYPE_PP(val) != IS_ARRAY)
            return; // XXX error handling XXX

        HashPosition pos;
        HashTable* hash_table = Z_ARRVAL_PP(val);        
        size_t num_elements = (size_t)zend_hash_num_elements(hash_table);
        zval** data;
        char* key;
        int i, key_len, curr = 0;
        long index;
        uint32_t* values = emalloc(sizeof(uint32_t) * num_elements);

        zend_hash_internal_pointer_reset_ex(hash_table, &pos);
        for (;; zend_hash_move_forward_ex(hash_table, &pos)) {
            zend_hash_get_current_data_ex(hash_table, (void**)&data, &pos);

            i = zend_hash_get_current_key_ex(hash_table, &key, &key_len, &index, 0, &pos);
            if (i == HASH_KEY_NON_EXISTANT)
                break;

            if (Z_TYPE_PP(data) == IS_LONG)
                values[curr++] = (uint32_t)(Z_LVAL_PP(data));
            else if (Z_TYPE_PP(data) == IS_DOUBLE)
                values[curr++] = (uint32_t)(Z_DVAL_PP(data));
        }

        *quantifier = num_elements;
        memcpy((void*)member, (void*)&values, sizeof(void*));
    }
}

/* Extract boolean values from the given zval** and write them into the given protobuf
   message pointer. handle optional/required/repeated */
void
bool_proto (const ProtobufCMessage* message, const ProtobufCFieldDescriptor* field, zval** val)
{
    protobuf_c_boolean* member = (protobuf_c_boolean*)get_member(message, field);
    unsigned int* quantifier = get_quantifier(message, field);

    if (field->label == PROTOBUF_C_LABEL_REQUIRED || field->label == PROTOBUF_C_LABEL_OPTIONAL)
    {
        if (Z_TYPE_PP(val) == IS_BOOL)
            *member = (protobuf_c_boolean)Z_LVAL_PP(val);
        else
            return; // XXX error handling XXX
    
        if (field->label == PROTOBUF_C_LABEL_OPTIONAL)
            *quantifier = 1;

    }
    else if (field->label == PROTOBUF_C_LABEL_REPEATED)
    {
        if (Z_TYPE_PP(val) != IS_ARRAY)
            return; // XXX error handling XXX

        HashPosition pos;
        HashTable* hash_table = Z_ARRVAL_PP(val);
        size_t num_elements = (size_t)zend_hash_num_elements(hash_table);
        zval** data;
        char* key;
        int i, key_len, curr = 0;
        long index;
        protobuf_c_boolean* values = emalloc(sizeof(protobuf_c_boolean) * num_elements);

        zend_hash_internal_pointer_reset_ex(hash_table, &pos);
        for (;; zend_hash_move_forward_ex(hash_table, &pos)) {
            zend_hash_get_current_data_ex(hash_table, (void**)&data, &pos);

            i = zend_hash_get_current_key_ex(hash_table, &key, &key_len, &index, 0, &pos);
            if (i == HASH_KEY_NON_EXISTANT)
                break;

            if (Z_TYPE_PP(data) == IS_BOOL)
                values[curr++] = (protobuf_c_boolean)Z_LVAL_PP(data);
        }

        *quantifier = num_elements;
        memcpy((void*)member, (void*)&values, sizeof(void*));
    }
}

/******************************************************************************/
/* php to proto conversions */

/* === unimplemented === */
void double_php (zval* out, const char* idx, uint64_t val) {
    add_assoc_double(out, (char*)idx, (double)val);
}

void float_php (zval* out, const char* idx, uint32_t val) {
    add_assoc_double(out, (char*)idx, (double)val);
}

void sint32_php (zval* out, const char* idx, int32_t val) {
    add_assoc_double(out, (char*)idx, (double)val);
}

void fixed32_php (zval* out, const char* idx, uint32_t val) {
    add_assoc_double(out, (char*)idx, (double)val);
}

void sfixed32_php (zval* out, const char* idx, uint32_t val) {
    add_assoc_double(out, (char*)idx, (double)val);
}

void int64_php (zval* out, const char* idx, uint64_t val) {
    add_assoc_double(out, (char*)idx, (double)val);
}

void sint64_php (zval* out, const char* idx, int64_t val) {
    add_assoc_double(out, (char*)idx, (double)val);
}

void uint64_php (zval* out, const char* idx, uint64_t val) {
    add_assoc_double(out, (char*)idx, (double)val);
}

void fixed64_php (zval* out, const char* idx, uint64_t val) {
    add_assoc_double(out, (char*)idx, (double)val);
}

void sfixed64_php (zval* out, const char* idx, uint64_t val) {
    add_assoc_double(out, (char*)idx, (double)val);
}

void bool_php (zval* out, const char* idx, protobuf_c_boolean val) {
    add_assoc_bool(out, (char*)idx, (int)val);
}



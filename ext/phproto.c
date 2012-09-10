/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2012 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: zined@adrianer.de                                            |
  +----------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "zend_exceptions.h"
#include "php_phproto.h"
#include "converter.h"
#include <syslog.h>

/* If you declare any globals in php_phproto.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(phproto)
*/

/* True global resources - no need for thread safety here */
static int le_phproto;

/* {{{ phproto_functions[]
 *
 * Every user visible function must have an entry in phproto_functions[].
 */
const zend_function_entry phproto_functions[] = {
	PHP_FE(phproto_messages, NULL)
	PHP_FE(phproto_pack, NULL)
	PHP_FE(phproto_unpack, NULL)
	PHP_FE_END	/* Must be the last line in phproto_functions[] */
};
/* }}} */

/* {{{ phproto_module_entry
 */
zend_module_entry phproto_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"phproto",
	phproto_functions,
	PHP_MINIT(phproto),
	PHP_MSHUTDOWN(phproto),
	PHP_RINIT(phproto),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(phproto),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(phproto),
#if ZEND_MODULE_API_NO >= 20010901
	"0.1", /* Replace with version number for your extension */
#endif
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_PHPROTO
ZEND_GET_MODULE(phproto)
#endif

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("phproto.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_phproto_globals, phproto_globals)
    STD_PHP_INI_ENTRY("phproto.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_phproto_globals, phproto_globals)
PHP_INI_END()
*/
/* }}} */

/* {{{ php_phproto_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_phproto_init_globals(zend_phproto_globals *phproto_globals)
{
	phproto_globals->global_value = 0;
	phproto_globals->global_string = NULL;
}
*/
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(phproto)
{
	/* If you have INI entries, uncomment these lines 
	REGISTER_INI_ENTRIES();
	*/

	openlog("phproto", LOG_PID, LOG_DAEMON);
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(phproto)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	closelog();
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(phproto)
{
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(phproto)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(phproto)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "phproto support", "enabled");
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */

/* {{{ phproto_messages
*/
PHP_FUNCTION(phproto_messages)
{
	const ProtobufCMessageDescriptor* descriptor = NULL;
	unsigned i, total = sizeof(phproto_messages)/sizeof(ProtobufCMessageDescriptor*);
	array_init(return_value);

	for (i=0;i<total;++i) {
		descriptor = phproto_messages[i];

		unsigned j;	
		zval *info, *fields;
		MAKE_STD_ZVAL(info); MAKE_STD_ZVAL(fields);
		array_init(info); array_init(fields);

		add_assoc_long(info, "magic", descriptor->magic);
		add_assoc_long(info, "n_fields", descriptor->n_fields);

		for (j=0;j<descriptor->n_fields;++j) {
			const ProtobufCFieldDescriptor* field = descriptor->fields + j;

			zval* fieldinfo;
			MAKE_STD_ZVAL(fieldinfo);
			array_init(fieldinfo);

			add_assoc_string(fieldinfo, "name", (char*)field->name, 1);
			add_assoc_long(fieldinfo, "id", field->id);
			switch (field->label) {
				case PROTOBUF_C_LABEL_REQUIRED:
					add_assoc_string(fieldinfo, "label", "REQUIRED", 1);
					break;
				case PROTOBUF_C_LABEL_OPTIONAL:
					add_assoc_string(fieldinfo, "label", "OPTIONAL", 1);
					break;
				case PROTOBUF_C_LABEL_REPEATED:
					add_assoc_string(fieldinfo, "label", "REPEATED", 1);
					break;
				default: break;
			}
			switch(field->type) {
				case PROTOBUF_C_TYPE_INT32:
					add_assoc_string(fieldinfo, "type", "INT32", 1);
					break;
				case PROTOBUF_C_TYPE_UINT32:
					add_assoc_string(fieldinfo, "type", "UINT32", 1);
					break;
				case PROTOBUF_C_TYPE_STRING:
					add_assoc_string(fieldinfo, "type", "STRING", 1);
					break;
				case PROTOBUF_C_TYPE_BOOL:
					add_assoc_string(fieldinfo, "type", "BOOL", 1);
					break;
				case PROTOBUF_C_TYPE_MESSAGE:
					add_assoc_string(fieldinfo, "type", "MESSAGE", 1);
					break;
				default:
					add_assoc_string(fieldinfo, "type", "[not implemented]", 1);
					break;
			}

			add_index_zval(fields, field->id, fieldinfo);
		}

		add_assoc_zval(info, "fields", fields);
		add_assoc_zval(return_value, (char*)descriptor->name, info);
	}


}

/* }}} */

/* {{{ phproto_pack
*/
PHP_FUNCTION(phproto_pack)
{
	zval* arr;
	char* message_name;
	unsigned message_name_len;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sa", &message_name, &message_name_len, &arr) == FAILURE) {
		return;
	}
	
	void* buffer;
	unsigned len, i, total = sizeof(phproto_messages)/sizeof(ProtobufCMessageDescriptor*);
	const ProtobufCMessageDescriptor* descriptor = NULL;

	for (i=0;i<total;++i) {
		descriptor = phproto_messages[i];
		if (strcmp(descriptor->name, message_name) != 0) descriptor = NULL;
		else break;
	}

	if (descriptor == NULL) {
		zend_throw_exception_ex(zend_exception_get_default(TSRMLS_C), 0 TSRMLS_CC, "message not found: '%s'", message_name);
		return;
	}

	// emalloc
	ProtobufCMessage* message = emalloc(descriptor->sizeof_message);

	message->descriptor = descriptor;
	message->n_unknown_fields = 0;
	message->unknown_fields = NULL;

	if (php_message(message, arr) != 0) RETURN_FALSE;

	len = protobuf_c_message_get_packed_size((const ProtobufCMessage*)message);
	buffer = emalloc(len);
	protobuf_c_message_pack((const ProtobufCMessage*)message, buffer);

	RETVAL_STRINGL(buffer, len, 1);
	efree(buffer);
	efree(message);
}
/* }}}*/

/* {{{ phproto_unpack
*/
PHP_FUNCTION(phproto_unpack)
{
	uint8_t* buffer;
	unsigned buffer_len, message_name_len;
	char* message_name;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss",
		&message_name, &message_name_len, &buffer, &buffer_len) == FAILURE) return;

	int i, total = sizeof(phproto_messages)/sizeof(ProtobufCMessageDescriptor*);
	const ProtobufCMessageDescriptor* descriptor = NULL;

	for (i=0;i<total;++i) {
		descriptor = phproto_messages[i];
		if (strcmp(descriptor->name, message_name) != 0) descriptor = NULL;
		else break;
	}

	if (descriptor == NULL) {
		zend_throw_exception_ex(zend_exception_get_default(TSRMLS_C), 0 TSRMLS_CC, "message not found: '%s'", message_name);
		return;
	}

	ProtobufCMessage* message;

	if ((message = protobuf_c_message_unpack(descriptor, NULL, buffer_len, buffer)) == NULL) {
		zend_throw_exception_ex(zend_exception_get_default(TSRMLS_C), 0 TSRMLS_CC, "cannot unpack message '%s'", message_name);
		return;
	}

	array_init(return_value);
	message_php(return_value, message);
}
/* }}}*/

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */

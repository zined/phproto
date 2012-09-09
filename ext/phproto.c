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
	PHP_FE(phproto_info, NULL)
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

/* {{{ phproto_info
*/
PHP_FUNCTION(phproto_info)
{
	ProtobufCMessageDescriptor* current = NULL;
	unsigned i, total = sizeof(phproto_messages)/sizeof(ProtobufCMessageDescriptor*);
	
	array_init(return_value);
	for (i=0;i<total;++i) {
		current = phproto_messages[i];
		add_index_string(return_value, current->magic, current->short_name, 1);
	}
}
/* }}}*/

/* {{{ phproto_pack
*/
PHP_FUNCTION(phproto_pack)
{
	zval* arr;
	unsigned magic;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "la", &magic, &arr) == FAILURE) {
		return;
	}
	
	void* buffer;
	unsigned len, i, total = sizeof(phproto_messages)/sizeof(ProtobufCMessageDescriptor*);
	ProtobufCMessageDescriptor* descriptor = NULL;

	for (i=0;i<total;++i) {
		descriptor = phproto_messages[i];
		if (descriptor->magic != magic) descriptor = NULL;
		else break;
	}

	if (descriptor == NULL) {
		zend_throw_exception_ex(zend_exception_get_default(TSRMLS_C), 0 TSRMLS_CC, "cannot find message '%d'", magic);
		return;
	}

	ProtobufCMessage message = { descriptor, 0, NULL };

	if (php_message(&message, arr) != 0) RETURN_FALSE;

	len = protobuf_c_message_get_packed_size((const ProtobufCMessage*)&message);
	buffer = emalloc(len);
	protobuf_c_message_pack((const ProtobufCMessage*)&message, buffer);

	php_printf("message with magic '%d' and short_name '%s' will be '%d' bytes packed.\n",
		magic, descriptor->short_name, len);

	RETVAL_STRINGL(buffer, len, 1);
	efree(buffer);
}
/* }}}*/

/* {{{ phproto_unpack
*/
PHP_FUNCTION(phproto_unpack)
{
	uint8_t* buffer;
	unsigned buffer_len, magic;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ls", &magic, &buffer, &buffer_len) == FAILURE) return;

	int i, total = sizeof(phproto_messages)/sizeof(ProtobufCMessageDescriptor*);
	ProtobufCMessageDescriptor* descriptor = NULL;

	for (i=0;i<total;++i) {
		descriptor = phproto_messages[i];
		if (descriptor->magic != magic) descriptor = NULL;
		else break;
	}

	if (descriptor == NULL) {
		zend_throw_exception_ex(zend_exception_get_default(TSRMLS_C), 0 TSRMLS_CC, "cannot find message '%d'", magic);
		return;
	}

	ProtobufCMessage* message;

	if ((message = protobuf_c_message_unpack(descriptor, NULL, buffer_len, buffer)) == NULL) {
		zend_throw_exception_ex(zend_exception_get_default(TSRMLS_C), 0 TSRMLS_CC, "cannot unpack message '%d'", magic);
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

/*
   +----------------------------------------------------------------------+
   | PHP version 4.0                                                      |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997, 1998, 1999, 2000, 2001 The PHP Group             |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.02 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license/2_02.txt.                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author: Eduard Koucky <eduard.koucky@czech-tv.cz>                    |
   +----------------------------------------------------------------------+
   | PHP 8 port: resource management, Zend argument parsing and module    |
   | globals updated for Zend Engine 4.x (PHP 8.3 / 8.4 / 8.5).           |
   +----------------------------------------------------------------------+
*/

#ifndef PHP_SAPRFC_H
#define PHP_SAPRFC_H

#include "php.h"

/* rfccal.h pulls in the classic SAP RFCSDK headers (saprfc.h, sapitab.h,
 * rfcsi.h). Those headers are supplied by the SDK selected with
 * --with-saprfc=DIR; this extension does not bundle or replace them. */
#include "rfccal.h"

extern zend_module_entry saprfc_module_entry;
#define phpext_saprfc_ptr &saprfc_module_entry

#ifdef PHP_WIN32
#define PHP_SAPRFC_API __declspec(dllexport)
#else
#define PHP_SAPRFC_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

PHP_MINIT_FUNCTION(saprfc);
PHP_MSHUTDOWN_FUNCTION(saprfc);
PHP_RINIT_FUNCTION(saprfc);
PHP_RSHUTDOWN_FUNCTION(saprfc);
PHP_MINFO_FUNCTION(saprfc);

PHP_FUNCTION(saprfc_open);
PHP_FUNCTION(saprfc_function_discover);
PHP_FUNCTION(saprfc_function_define);
PHP_FUNCTION(saprfc_function_interface);
PHP_FUNCTION(saprfc_function_debug_info);
PHP_FUNCTION(saprfc_optional);
PHP_FUNCTION(saprfc_import);
PHP_FUNCTION(saprfc_export);
PHP_FUNCTION(saprfc_table_init);
PHP_FUNCTION(saprfc_table_append);
PHP_FUNCTION(saprfc_table_insert);
PHP_FUNCTION(saprfc_table_modify);
PHP_FUNCTION(saprfc_table_remove);
PHP_FUNCTION(saprfc_table_read);
PHP_FUNCTION(saprfc_table_rows);
PHP_FUNCTION(saprfc_call_and_receive);
PHP_FUNCTION(saprfc_exception);
PHP_FUNCTION(saprfc_error);
PHP_FUNCTION(saprfc_function_free);
PHP_FUNCTION(saprfc_close);
PHP_FUNCTION(saprfc_set_code_page);
PHP_FUNCTION(saprfc_attributes);
PHP_FUNCTION(saprfc_server_accept);
PHP_FUNCTION(saprfc_server_import);
PHP_FUNCTION(saprfc_server_export);
PHP_FUNCTION(saprfc_server_dispatch);
PHP_FUNCTION(saprfc_trfc_install);
PHP_FUNCTION(saprfc_trfc_dispatch);
PHP_FUNCTION(saprfc_trfc_call);
PHP_FUNCTION(saprfc_trfc_tid);
PHP_FUNCTION(saprfc_set_trace);
PHP_FUNCTION(saprfc_server_register_check);
PHP_FUNCTION(saprfc_server_register_cancel);
PHP_FUNCTION(saprfc_function_name);
PHP_FUNCTION(saprfc_allow_start_program);
PHP_FUNCTION(saprfc_get_ticket);

ZEND_BEGIN_MODULE_GLOBALS(saprfc)
    int   trfc_install_flag;
    char *trfc_tid_check;
    char *trfc_tid_commit;
    char *trfc_tid_rollback;
    char *trfc_tid_confirm;
    char *trfc_dispatcher;
ZEND_END_MODULE_GLOBALS(saprfc)

ZEND_EXTERN_MODULE_GLOBALS(saprfc)

#ifdef ZTS
#define SAPRFCG(v) ZEND_MODULE_GLOBALS_ACCESSOR(saprfc, v)
#define SAPRFCLS_FETCH() \
    zend_saprfc_globals *saprfc_globals = ZEND_MODULE_GLOBALS_BULK(saprfc)
#else
#define SAPRFCG(v) (saprfc_globals.v)
#define SAPRFCLS_FETCH()
#endif

typedef struct RFC_RESOURCE RFC_RESOURCE;
typedef struct FCE_RESOURCE FCE_RESOURCE;

/* A live RFC connection. handle == RFC_HANDLE_NULL means the handle has been
 * closed. functions is the list of function-module resources discovered or
 * defined against this connection; closing the connection invalidates them so
 * a later call cannot pass a dangling SDK handle. */
struct RFC_RESOURCE {
    RFC_HANDLE    handle;
    int           client;
    FCE_RESOURCE *functions;
};

/* A function module interface. handle is the connection this module was
 * discovered/defined against, or RFC_HANDLE_NULL for an offline definition
 * created with saprfc_function_define(0, ...). owner is NULL for offline
 * definitions and after the connection has been closed. */
struct FCE_RESOURCE {
    RFC_HANDLE            handle;
    CALD_FUNCTION_MODULE *fce;
    RFC_RESOURCE         *owner;
    FCE_RESOURCE         *next;
};

#endif /* PHP_SAPRFC_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */

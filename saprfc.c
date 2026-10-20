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
   | PHP 8 port:                                                          |
   |   - zend_parse_parameters + arginfo for every function               |
   |   - zend_resource API, zend_string API, PHP 8 HashTable iteration    |
   |   - zval stack allocation instead of MAKE_STD_ZVAL/FREE_ZVAL         |
   |   - bounds/NULL checking around SDK buffers                          |
   +----------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_saprfc.h"
#include "zend_smart_str.h"

#include <strings.h>

#define SAPRFC_VERSION  "1.4.2-php8"
#define SAPRFC_RELEASE  "2026/10/20"

ZEND_DECLARE_MODULE_GLOBALS(saprfc)

/* Resource type ids. */
static int le_rfc = -1;
static int le_function = -1;

/* ------------------------------------------------------------------ */
/* Argument info                                                       */
/* ------------------------------------------------------------------ */

ZEND_BEGIN_ARG_INFO_EX(arginfo_saprfc_open, 0, 0, 1)
    ZEND_ARG_INFO(0, conn)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_saprfc_function_discover, 0, 0, 2)
    ZEND_ARG_INFO(0, rfc)
    ZEND_ARG_INFO(0, function_module)
    ZEND_ARG_INFO(0, not_trim)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_saprfc_function_define, 0, 0, 3)
    ZEND_ARG_INFO(0, rfc)
    ZEND_ARG_INFO(0, function_module)
    ZEND_ARG_INFO(0, iface)
    ZEND_ARG_INFO(0, not_trim)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_saprfc_function_interface, 0, 0, 1)
    ZEND_ARG_INFO(0, fce)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_saprfc_function_debug_info, 0, 0, 1)
    ZEND_ARG_INFO(0, fce)
    ZEND_ARG_INFO(0, only_values)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_saprfc_optional, 0, 0, 3)
    ZEND_ARG_INFO(0, fce)
    ZEND_ARG_INFO(0, name)
    ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_saprfc_import, 0, 0, 3)
    ZEND_ARG_INFO(0, fce)
    ZEND_ARG_INFO(0, name)
    ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_saprfc_export, 0, 0, 2)
    ZEND_ARG_INFO(0, fce)
    ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_saprfc_table_init, 0, 0, 2)
    ZEND_ARG_INFO(0, fce)
    ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_saprfc_table_append, 0, 0, 3)
    ZEND_ARG_INFO(0, fce)
    ZEND_ARG_INFO(0, name)
    ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_saprfc_table_insert, 0, 0, 4)
    ZEND_ARG_INFO(0, fce)
    ZEND_ARG_INFO(0, name)
    ZEND_ARG_INFO(0, value)
    ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_saprfc_table_modify, 0, 0, 4)
    ZEND_ARG_INFO(0, fce)
    ZEND_ARG_INFO(0, name)
    ZEND_ARG_INFO(0, value)
    ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_saprfc_table_remove, 0, 0, 3)
    ZEND_ARG_INFO(0, fce)
    ZEND_ARG_INFO(0, name)
    ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_saprfc_table_read, 0, 0, 3)
    ZEND_ARG_INFO(0, fce)
    ZEND_ARG_INFO(0, name)
    ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_saprfc_table_rows, 0, 0, 2)
    ZEND_ARG_INFO(0, fce)
    ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_saprfc_call_and_receive, 0, 0, 1)
    ZEND_ARG_INFO(0, fce)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_saprfc_none, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_saprfc_one, 0, 0, 1)
    ZEND_ARG_INFO(0, arg)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_saprfc_set_code_page, 0, 0, 2)
    ZEND_ARG_INFO(0, rfc)
    ZEND_ARG_INFO(0, codepage)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_saprfc_server_export, 0, 0, 3)
    ZEND_ARG_INFO(0, fce)
    ZEND_ARG_INFO(0, name)
    ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_saprfc_server_dispatch, 0, 0, 2)
    ZEND_ARG_INFO(0, rfc)
    ZEND_ARG_INFO(0, list)
    ZEND_ARG_INFO(0, timeout)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_saprfc_trfc_install, 0, 0, 5)
    ZEND_ARG_INFO(0, tid_check)
    ZEND_ARG_INFO(0, tid_commit)
    ZEND_ARG_INFO(0, tid_rollback)
    ZEND_ARG_INFO(0, tid_confirm)
    ZEND_ARG_INFO(0, dispatcher)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_saprfc_trfc_call, 0, 0, 2)
    ZEND_ARG_INFO(0, fce)
    ZEND_ARG_INFO(0, tid)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_saprfc_set_trace, 0, 0, 2)
    ZEND_ARG_INFO(0, rfc)
    ZEND_ARG_INFO(0, level)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_saprfc_register, 0, 0, 3)
    ZEND_ARG_INFO(0, tpid)
    ZEND_ARG_INFO(0, gwhost)
    ZEND_ARG_INFO(0, gwserver)
ZEND_END_ARG_INFO()

/* ------------------------------------------------------------------ */
/* Module entry                                                        */
/* ------------------------------------------------------------------ */

static const zend_function_entry saprfc_functions[] = {
    PHP_FE(saprfc_open,                     arginfo_saprfc_open)
    PHP_FE(saprfc_function_discover,        arginfo_saprfc_function_discover)
    PHP_FE(saprfc_function_define,          arginfo_saprfc_function_define)
    PHP_FE(saprfc_function_interface,       arginfo_saprfc_function_interface)
    PHP_FE(saprfc_function_debug_info,      arginfo_saprfc_function_debug_info)
    PHP_FE(saprfc_optional,                 arginfo_saprfc_optional)
    PHP_FE(saprfc_import,                   arginfo_saprfc_import)
    PHP_FE(saprfc_export,                   arginfo_saprfc_export)
    PHP_FE(saprfc_table_init,               arginfo_saprfc_table_init)
    PHP_FE(saprfc_table_append,             arginfo_saprfc_table_append)
    PHP_FE(saprfc_table_insert,             arginfo_saprfc_table_insert)
    PHP_FE(saprfc_table_modify,             arginfo_saprfc_table_modify)
    PHP_FE(saprfc_table_remove,             arginfo_saprfc_table_remove)
    PHP_FE(saprfc_table_read,               arginfo_saprfc_table_read)
    PHP_FE(saprfc_table_rows,               arginfo_saprfc_table_rows)
    PHP_FE(saprfc_call_and_receive,         arginfo_saprfc_call_and_receive)
    PHP_FE(saprfc_error,                    arginfo_saprfc_none)
    PHP_FE(saprfc_function_free,            arginfo_saprfc_one)
    PHP_FE(saprfc_close,                    arginfo_saprfc_one)
    PHP_FE(saprfc_set_code_page,            arginfo_saprfc_set_code_page)
    PHP_FE(saprfc_attributes,               arginfo_saprfc_one)
    PHP_FE(saprfc_server_accept,            arginfo_saprfc_one)
    PHP_FE(saprfc_server_import,            arginfo_saprfc_export)
    PHP_FE(saprfc_server_export,            arginfo_saprfc_server_export)
    PHP_FE(saprfc_server_dispatch,          arginfo_saprfc_server_dispatch)
    PHP_FE(saprfc_trfc_install,             arginfo_saprfc_trfc_install)
    PHP_FALIAS(saprfc_trfc_dispatch, saprfc_server_dispatch, arginfo_saprfc_server_dispatch)
    PHP_FE(saprfc_trfc_call,                arginfo_saprfc_trfc_call)
    PHP_FE(saprfc_trfc_tid,                 arginfo_saprfc_one)
    PHP_FE(saprfc_set_trace,                arginfo_saprfc_set_trace)
    PHP_FE(saprfc_server_register_check,    arginfo_saprfc_register)
    PHP_FE(saprfc_server_register_cancel,   arginfo_saprfc_register)
    PHP_FE(saprfc_function_name,            arginfo_saprfc_one)
    PHP_FE(saprfc_exception,                arginfo_saprfc_one)
    PHP_FE(saprfc_allow_start_program,      arginfo_saprfc_one)
    PHP_FE(saprfc_get_ticket,               arginfo_saprfc_one)
    PHP_FE_END
};

zend_module_entry saprfc_module_entry = {
    STANDARD_MODULE_HEADER,
    "saprfc",
    saprfc_functions,
    PHP_MINIT(saprfc),
    PHP_MSHUTDOWN(saprfc),
    PHP_RINIT(saprfc),
    PHP_RSHUTDOWN(saprfc),
    PHP_MINFO(saprfc),
    SAPRFC_VERSION,
    STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_SAPRFC
ZEND_GET_MODULE(saprfc)
#endif

/* ------------------------------------------------------------------ */
/* Helpers                                                             */
/* ------------------------------------------------------------------ */

static void saprfc_attach_function(RFC_RESOURCE *rfc_resource,
                                   FCE_RESOURCE *fce_resource)
{
    fce_resource->owner = rfc_resource;
    fce_resource->next = rfc_resource->functions;
    rfc_resource->functions = fce_resource;
}

static void saprfc_detach_function(FCE_RESOURCE *fce_resource)
{
    RFC_RESOURCE *owner = fce_resource->owner;
    FCE_RESOURCE **link;

    if (!owner) {
        return;
    }
    link = &owner->functions;
    while (*link) {
        if (*link == fce_resource) {
            *link = fce_resource->next;
            break;
        }
        link = &(*link)->next;
    }
    fce_resource->owner = NULL;
    fce_resource->next = NULL;
}

static void free_resource_rfc(zend_resource *rsrc)
{
    RFC_RESOURCE *rfc_resource = (RFC_RESOURCE *)rsrc->ptr;

    if (rfc_resource) {
        FCE_RESOURCE *function = rfc_resource->functions;

        /* The SDK connection handle is about to disappear. Function module
         * resources keep their own CAL state, but their connection handle must
         * not be used again. */
        while (function) {
            function->handle = RFC_HANDLE_NULL;
            function->owner = NULL;
            function = function->next;
        }
        if (rfc_resource->handle) {
            CAL_CLOSE(rfc_resource->handle);
        }
        efree(rfc_resource);
    }
    rsrc->ptr = NULL;
}

static void free_resource_function(zend_resource *rsrc)
{
    FCE_RESOURCE *fce_resource = (FCE_RESOURCE *)rsrc->ptr;

    if (fce_resource) {
        saprfc_detach_function(fce_resource);
        if (fce_resource->fce) {
            CAL_DELETE(fce_resource->fce);
        }
        efree(fce_resource);
    }
    rsrc->ptr = NULL;
}

static void php_saprfc_init_globals(zend_saprfc_globals *globals)
{
    globals->trfc_install_flag = 0;
    globals->trfc_tid_check = NULL;
    globals->trfc_tid_commit = NULL;
    globals->trfc_tid_rollback = NULL;
    globals->trfc_tid_confirm = NULL;
    globals->trfc_dispatcher = NULL;
}

static RFC_RESOURCE *saprfc_fetch_rfc(zval *zv)
{
    if (!zv || Z_TYPE_P(zv) != IS_RESOURCE || !Z_RES_P(zv)->ptr) {
        return NULL;
    }
    if (Z_RES_P(zv)->type != le_rfc) {
        return NULL;
    }
    return (RFC_RESOURCE *)Z_RES_P(zv)->ptr;
}

static FCE_RESOURCE *saprfc_fetch_fce(zval *zv)
{
    if (!zv || Z_TYPE_P(zv) != IS_RESOURCE || !Z_RES_P(zv)->ptr) {
        return NULL;
    }
    if (Z_RES_P(zv)->type != le_function) {
        return NULL;
    }
    return (FCE_RESOURCE *)Z_RES_P(zv)->ptr;
}

static RFC_RESOURCE *saprfc_require_rfc(zval *zv)
{
    RFC_RESOURCE *resource = saprfc_fetch_rfc(zv);

    if (!resource) {
        php_error_docref(NULL, E_WARNING, "Invalid resource for RFC connection");
    }
    return resource;
}

static FCE_RESOURCE *saprfc_require_fce(zval *zv)
{
    FCE_RESOURCE *resource = saprfc_fetch_fce(zv);

    if (!resource) {
        php_error_docref(NULL, E_WARNING, "Invalid resource for function module");
    }
    return resource;
}

static char saprfc_def_type(CALD_FUNCTION_MODULE *fce, int type,
                             const char *name, const char *item)
{
    switch (type) {
        case CALC_IMPORT:
            return CAL_DEF_IMPORT_STRUCT_TYPE(fce, (char *)name, (char *)item);
        case CALC_EXPORT:
            return CAL_DEF_EXPORT_STRUCT_TYPE(fce, (char *)name, (char *)item);
        case CALC_TABLE:
            return CAL_DEF_TABLE_TYPE(fce, (char *)name, (char *)item);
        default:
            return ' ';
    }
}

static int saprfc_def_length(CALD_FUNCTION_MODULE *fce, int type,
                             const char *name, const char *item)
{
    switch (type) {
        case CALC_IMPORT:
            return CAL_DEF_IMPORT_STRUCT_LENGTH(fce, (char *)name, (char *)item);
        case CALC_EXPORT:
            return CAL_DEF_EXPORT_STRUCT_LENGTH(fce, (char *)name, (char *)item);
        case CALC_TABLE:
            return CAL_DEF_TABLE_LENGTH(fce, (char *)name, (char *)item);
        default:
            return 0;
    }
}

static int saprfc_set_value(CALD_FUNCTION_MODULE *fce, int type,
                            const char *name, const char *item, char *value)
{
    switch (type) {
        case CALC_IMPORT:
            if (item && *item) {
                return CAL_SET_IMPORT_STRUCT(fce, (char *)name, (char *)item, value);
            }
            return CAL_SET_IMPORT(fce, (char *)name, value);
        case CALC_EXPORT:
            if (item && *item) {
                return CAL_SET_EXPORT_STRUCT(fce, (char *)name, (char *)item, value);
            }
            return CAL_SET_EXPORT(fce, (char *)name, value);
        case CALC_TABLE:
            return CAL_SET_TABLE(fce, (char *)name, (char *)item, value);
        default:
            return -1;
    }
}

/* Copy every string-keyed array element into the CAL interface buffer.
 * RAW (TYPX) values are copied through a zero-filled, correctly sized buffer
 * so the SDK never reads past the PHP string. */
static int saprfc_apply_array(CALD_FUNCTION_MODULE *fce, int type,
                              const char *name, HashTable *values)
{
    zend_string *key;
    zval *value;

    ZEND_HASH_FOREACH_STR_KEY_VAL(values, key, value) {
        char *raw_buffer = NULL;
        zend_string *converted = NULL;
        char *argument;
        int length;
        int rc;

        if (!key) {
            continue;
        }

        converted = zval_get_string(value);
        if (!converted || EG(exception)) {
            if (converted) {
                zend_string_release(converted);
            }
            return -1;
        }

        length = saprfc_def_length(fce, type, name, ZSTR_VAL(key));
        if (saprfc_def_type(fce, type, name, ZSTR_VAL(key)) == TYPX && length > 0) {
            size_t copy_len = ZSTR_LEN(converted);
            if (copy_len > (size_t)length) {
                copy_len = (size_t)length;
            }
            raw_buffer = ecalloc(1, (size_t)length);
            memcpy(raw_buffer, ZSTR_VAL(converted), copy_len);
            argument = raw_buffer;
        } else {
            argument = ZSTR_VAL(converted);
        }

        rc = saprfc_set_value(fce, type, name, ZSTR_VAL(key), argument);

        if (raw_buffer) {
            efree(raw_buffer);
        }
        zend_string_release(converted);

        if (rc != 0) {
            php_error_docref(NULL, E_WARNING, "%s", CAL_DEBUG_MESSAGE());
            return -1;
        }
    } ZEND_HASH_FOREACH_END();

    return 0;
}

static int saprfc_apply_scalar(CALD_FUNCTION_MODULE *fce, int type,
                               const char *name, zval *value)
{
    zend_string *converted;
    char *raw_buffer = NULL;
    char *argument;
    int length;
    int rc;

    converted = zval_get_string(value);
    if (!converted || EG(exception)) {
        if (converted) {
            zend_string_release(converted);
        }
        return -1;
    }

    length = saprfc_def_length(fce, type, name, "");
    if (saprfc_def_type(fce, type, name, "") == TYPX && length > 0) {
        size_t copy_len = ZSTR_LEN(converted);
        if (copy_len > (size_t)length) {
            copy_len = (size_t)length;
        }
        raw_buffer = ecalloc(1, (size_t)length);
        memcpy(raw_buffer, ZSTR_VAL(converted), copy_len);
        argument = raw_buffer;
    } else {
        argument = ZSTR_VAL(converted);
    }

    rc = saprfc_set_value(fce, type, name, "", argument);

    if (raw_buffer) {
        efree(raw_buffer);
    }
    zend_string_release(converted);

    if (rc != 0) {
        php_error_docref(NULL, E_WARNING, "%s", CAL_DEBUG_MESSAGE());
        return -1;
    }
    return 0;
}

/* Build the PHP representation of an import or export parameter. */
static void saprfc_return_parameter(CALD_FUNCTION_MODULE *fce, int type,
                                    const char *name, zval *return_value)
{
    CALD_INTERFACE_INFO *iinfo;
    int i;

    iinfo = CAL_SINGLE_INTERFACE_INFO(fce, (char *)name, type);
    if (!iinfo) {
        php_error_docref(NULL, E_WARNING, "%s", CAL_DEBUG_MESSAGE());
        ZVAL_FALSE(return_value);
        return;
    }
    if (iinfo->size <= 0 || !iinfo->typeinfo) {
        php_error_docref(NULL, E_WARNING,
                         "Invalid interface definition for parameter '%s'", name);
        CAL_DEL_INTERFACE(iinfo);
        ZVAL_FALSE(return_value);
        return;
    }

    if (iinfo->typeinfo[0].name[0] != '\0') {
        /* Structure */
        array_init(return_value);
        for (i = 0; i < iinfo->size; i++) {
            char *value;
            if (type == CALC_IMPORT) {
                value = CAL_GET_IMPORT_STRUCT(fce, (char *)name,
                                              iinfo->typeinfo[i].name);
            } else {
                value = CAL_GET_EXPORT_STRUCT(fce, (char *)name,
                                              iinfo->typeinfo[i].name);
            }
            if (!value) {
                value = "";
            }
            if (iinfo->typeinfo[i].abap == 'X') {
                add_assoc_stringl(return_value, iinfo->typeinfo[i].name,
                                  value, iinfo->typeinfo[i].length);
            } else {
                add_assoc_string(return_value, iinfo->typeinfo[i].name, value);
            }
        }
    } else {
        char *value;
        if (type == CALC_IMPORT) {
            value = CAL_GET_IMPORT(fce, (char *)name);
        } else {
            value = CAL_GET_EXPORT(fce, (char *)name);
        }
        if (!value) {
            php_error_docref(NULL, E_WARNING, "%s", CAL_DEBUG_MESSAGE());
            CAL_DEL_INTERFACE(iinfo);
            ZVAL_FALSE(return_value);
            return;
        }
        if (iinfo->typeinfo[0].abap == 'X') {
            ZVAL_STRINGL(return_value, value, iinfo->typeinfo[0].length);
        } else {
            ZVAL_STRING(return_value, value);
        }
    }

    CAL_DEL_INTERFACE(iinfo);
}

/* ------------------------------------------------------------------ */
/* tRFC callbacks                                                      */
/* ------------------------------------------------------------------ */

static RFC_RC DLL_CALL_BACK_FUNCTION callback_dispatch(RFC_HANDLE rfc_handle)
{
    RFC_RC rfc_rc;
    RFC_FUNCTIONNAME function_name;
    FCE_RESOURCE *fce_resource;
    CALD_FUNCTION_MODULE *function_module = NULL;
    zval callback_name;
    zval callback_arg;
    zval callback_retval;
    zval function_zv;
    zval retval;
    char abort_text[1024];

    SAPRFCLS_FETCH();

    abort_text[0] = '\0';
    ZVAL_UNDEF(&callback_retval);
    ZVAL_UNDEF(&retval);

    rfc_rc = SAL_GET_NAME(rfc_handle, function_name);
    if (rfc_rc != RFC_OK) {
        return rfc_rc;
    }

    if (SAPRFCG(trfc_dispatcher) == NULL) {
        ZVAL_STRING(&callback_name, "__saprfc_callback_dispatch");
    } else {
        ZVAL_STRING(&callback_name, SAPRFCG(trfc_dispatcher));
    }
    ZVAL_STRING(&callback_arg, function_name);

    if (call_user_function(EG(function_table), NULL, &callback_name,
                           &callback_retval, 1, &callback_arg) != SUCCESS
        || EG(exception)) {
        snprintf(abort_text, sizeof(abort_text),
                 "The function __saprfc_callback_dispatch() is not implemented");
        rfc_rc = RFC_FAILURE;
        goto done;
    }

    fce_resource = saprfc_fetch_fce(&callback_retval);
    if (!fce_resource || !fce_resource->fce) {
        snprintf(abort_text, sizeof(abort_text),
                 "Invalid function handle returned by dispatcher()");
        rfc_rc = RFC_FAILURE;
        goto done;
    }

    function_module = fce_resource->fce;
    CAL_INIT_INTERFACE(function_module);

    rfc_rc = SAL_GET_DATA(rfc_handle, function_module);
    if (rfc_rc != RFC_OK) {
        if (rfc_rc == -1) {
            snprintf(abort_text, sizeof(abort_text),
                     "Error %s in RfcGetData", CAL_DEBUG_MESSAGE());
        } else {
            snprintf(abort_text, sizeof(abort_text),
                     "Error %s in RfcGetData", CAL_RFC_LAST_ERROR());
        }
        CAL_INIT_INTERFACE(function_module);
        function_module = NULL;
        goto done;
    }

    ZVAL_STRING(&function_zv, function_name);
    if (call_user_function(EG(function_table), NULL, &function_zv,
                           &retval, 1, &callback_retval) != SUCCESS
        || EG(exception)) {
        snprintf(abort_text, sizeof(abort_text),
                 "PHP function %s can't be called", function_name);
        rfc_rc = RFC_FAILURE;
    } else if (Z_TYPE(retval) == IS_STRING && Z_STRLEN(retval) > 0) {
        SAL_RAISE(rfc_handle, function_module, Z_STRVAL(retval));
        rfc_rc = RFC_OK;
    } else {
        rfc_rc = SAL_SEND_DATA(rfc_handle, function_module);
        if (rfc_rc != RFC_OK) {
            if (rfc_rc == -1) {
                snprintf(abort_text, sizeof(abort_text),
                         "Error %s in RfcSendData", CAL_DEBUG_MESSAGE());
            } else {
                snprintf(abort_text, sizeof(abort_text),
                         "Error %s in RfcSendData", CAL_RFC_LAST_ERROR());
            }
        }
    }
    zval_ptr_dtor(&function_zv);

    CAL_INIT_INTERFACE(function_module);
    function_module = NULL;

done:
    if (!Z_ISUNDEF(callback_retval)) {
        zval_ptr_dtor(&callback_retval);
    }
    if (!Z_ISUNDEF(retval)) {
        zval_ptr_dtor(&retval);
    }
    zval_ptr_dtor(&callback_arg);
    zval_ptr_dtor(&callback_name);

    if (rfc_rc != RFC_OK) {
        if (abort_text[0] == '\0') {
            snprintf(abort_text, sizeof(abort_text),
                     "RFC callback failed with code %ld", (long)rfc_rc);
        }
        php_error(E_WARNING, "%s", abort_text);
        SAL_ABORT(rfc_handle, abort_text);
    }

    return rfc_rc;
}

static int DLL_CALL_BACK_FUNCTION callback_tid_check(RFC_TID tid)
{
    int rc = 0;
    zval callback_name, callback_tid, retval;

    SAPRFCLS_FETCH();

    if (SAPRFCG(trfc_tid_check) == NULL) {
        return 0;
    }

    ZVAL_UNDEF(&retval);
    ZVAL_STRING(&callback_name, SAPRFCG(trfc_tid_check));
    ZVAL_STRING(&callback_tid, tid);

    if (call_user_function(EG(function_table), NULL, &callback_name,
                           &retval, 1, &callback_tid) == SUCCESS
        && Z_TYPE(retval) == IS_LONG) {
        rc = (int)Z_LVAL(retval);
    }
    if (!Z_ISUNDEF(retval)) {
        zval_ptr_dtor(&retval);
    }
    zval_ptr_dtor(&callback_tid);
    zval_ptr_dtor(&callback_name);
    return rc;
}

static void DLL_CALL_BACK_FUNCTION callback_tid_void(const char *callback,
                                                     RFC_TID tid)
{
    zval callback_name, callback_tid, retval;

    if (callback == NULL) {
        return;
    }

    ZVAL_UNDEF(&retval);
    ZVAL_STRING(&callback_name, callback);
    ZVAL_STRING(&callback_tid, tid);
    call_user_function(EG(function_table), NULL, &callback_name,
                       &retval, 1, &callback_tid);
    if (!Z_ISUNDEF(retval)) {
        zval_ptr_dtor(&retval);
    }
    zval_ptr_dtor(&callback_tid);
    zval_ptr_dtor(&callback_name);
}

static void DLL_CALL_BACK_FUNCTION callback_tid_commit(RFC_TID tid)
{
    SAPRFCLS_FETCH();
    callback_tid_void(SAPRFCG(trfc_tid_commit), tid);
}

static void DLL_CALL_BACK_FUNCTION callback_tid_rollback(RFC_TID tid)
{
    SAPRFCLS_FETCH();
    callback_tid_void(SAPRFCG(trfc_tid_rollback), tid);
}

static void DLL_CALL_BACK_FUNCTION callback_tid_confirm(RFC_TID tid)
{
    SAPRFCLS_FETCH();
    callback_tid_void(SAPRFCG(trfc_tid_confirm), tid);
}

/* ------------------------------------------------------------------ */
/* Module lifecycle                                                    */
/* ------------------------------------------------------------------ */

PHP_MINIT_FUNCTION(saprfc)
{
    ZEND_INIT_MODULE_GLOBALS(saprfc, php_saprfc_init_globals, NULL);

    CAL_INIT();

    le_rfc = zend_register_list_destructors_ex(free_resource_rfc, NULL,
                                               "saprfc handle", module_number);
    le_function = zend_register_list_destructors_ex(free_resource_function, NULL,
                                                    "saprfc function module",
                                                    module_number);

    REGISTER_LONG_CONSTANT("SAPRFC_OK", RFC_OK, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("SAPRFC_FAILURE", RFC_FAILURE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("SAPRFC_EXCEPTION", RFC_EXCEPTION, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("SAPRFC_SYS_EXCEPTION", RFC_SYS_EXCEPTION, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("SAPRFC_CALL", RFC_CALL, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("SAPRFC_INTERNAL_COM", RFC_INTERNAL_COM, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("SAPRFC_CLOSED", RFC_CLOSED, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("SAPRFC_RETRY", RFC_RETRY, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("SAPRFC_NO_TID", RFC_NO_TID, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("SAPRFC_EXECUTED", RFC_EXECUTED, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("SAPRFC_SYNCHRONIZE", RFC_SYNCHRONIZE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("SAPRFC_MEMORY_INSUFFICIENT", RFC_MEMORY_INSUFFICIENT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("SAPRFC_VERSION_MISMATCH", RFC_VERSION_MISMATCH, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("SAPRFC_NOT_FOUND", RFC_NOT_FOUND, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("SAPRFC_CALL_NOT_SUPPORTED", RFC_CALL_NOT_SUPPORTED, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("SAPRFC_NOT_OWNER", RFC_NOT_OWNER, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("SAPRFC_NOT_INITIALIZED", RFC_NOT_INITIALIZED, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("SAPRFC_SYSTEM_CALLED", RFC_SYSTEM_CALLED, CONST_CS | CONST_PERSISTENT);
    /* Kept for compatibility with the original extension. */
    REGISTER_LONG_CONSTANT("SAPRFC_INVALID_HANDLE", RFC_SYSTEM_CALLED + 1, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("SAPRFC_INVALID_PARAMETER", RFC_SYSTEM_CALLED + 2, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("SAPRFC_CANCELED", RFC_SYSTEM_CALLED + 3, CONST_CS | CONST_PERSISTENT);

    return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(saprfc)
{
    CAL_DONE();
    return SUCCESS;
}

PHP_RINIT_FUNCTION(saprfc)
{
    return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(saprfc)
{
    SAPRFCLS_FETCH();

    SAPRFCG(trfc_install_flag) = 0;
    if (SAPRFCG(trfc_tid_check)) {
        efree(SAPRFCG(trfc_tid_check));
        SAPRFCG(trfc_tid_check) = NULL;
    }
    if (SAPRFCG(trfc_tid_commit)) {
        efree(SAPRFCG(trfc_tid_commit));
        SAPRFCG(trfc_tid_commit) = NULL;
    }
    if (SAPRFCG(trfc_tid_rollback)) {
        efree(SAPRFCG(trfc_tid_rollback));
        SAPRFCG(trfc_tid_rollback) = NULL;
    }
    if (SAPRFCG(trfc_tid_confirm)) {
        efree(SAPRFCG(trfc_tid_confirm));
        SAPRFCG(trfc_tid_confirm) = NULL;
    }
    if (SAPRFCG(trfc_dispatcher)) {
        efree(SAPRFCG(trfc_dispatcher));
        SAPRFCG(trfc_dispatcher) = NULL;
    }

    return SUCCESS;
}

PHP_MINFO_FUNCTION(saprfc)
{
    php_info_print_table_start();
    php_info_print_table_header(2, "SAPRFC support", "enabled");
    php_info_print_table_row(2, "Version", SAPRFC_VERSION);
    php_info_print_table_row(2, "Release date", SAPRFC_RELEASE);
    php_info_print_table_row(2, "PHP API", "Zend Engine 4.x (PHP 8.3+)");
    php_info_print_table_row(2, "RFC Library", CAL_RFC_LIB_VERSION());
    php_info_print_table_end();
}

/* ------------------------------------------------------------------ */
/* Connection / function module API                                    */
/* ------------------------------------------------------------------ */

/* {{{ proto resource|false saprfc_open(array conn) */
PHP_FUNCTION(saprfc_open)
{
    zval *conn;
    HashTable *hash;
    zend_string *key;
    zval *value;
    smart_str connect = {0};
    RFC_HANDLE rfc;
    RFC_RESOURCE *rfc_resource;
    const char *connect_string = "";

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY(conn)
    ZEND_PARSE_PARAMETERS_END();

    hash = Z_ARRVAL_P(conn);
    ZEND_HASH_FOREACH_STR_KEY_VAL(hash, key, value) {
        zend_string *converted;

        if (!key) {
            continue;
        }
        converted = zval_get_string(value);
        if (!converted || EG(exception)) {
            if (converted) {
                zend_string_release(converted);
            }
            smart_str_free(&connect);
            RETURN_FALSE;
        }
        smart_str_append(&connect, key);
        smart_str_appendc(&connect, '=');
        smart_str_append(&connect, converted);
        smart_str_appendc(&connect, ' ');
        zend_string_release(converted);
    } ZEND_HASH_FOREACH_END();
    smart_str_0(&connect);
    if (connect.s) {
        connect_string = ZSTR_VAL(connect.s);
    }

    rfc = CAL_OPEN((char *)connect_string);
    smart_str_free(&connect);

    if (rfc == RFC_HANDLE_NULL) {
        php_error_docref(NULL, E_WARNING, "%s", CAL_DEBUG_MESSAGE());
        RETURN_FALSE;
    }

    rfc_resource = emalloc(sizeof(RFC_RESOURCE));
    rfc_resource->handle = rfc;
    rfc_resource->client = 1;
    rfc_resource->functions = NULL;

    RETURN_RES(zend_register_resource(rfc_resource, le_rfc));
}
/* }}} */

/* {{{ proto resource|false saprfc_function_discover(resource rfc, string function_module [, bool not_trim]) */
PHP_FUNCTION(saprfc_function_discover)
{
    zval *rfc_zv;
    zend_string *function_module;
    bool not_trim = false;
    RFC_RESOURCE *rfc_resource;
    FCE_RESOURCE *fce_resource;
    CALD_FUNCTION_MODULE *fce;
    int rc;

    ZEND_PARSE_PARAMETERS_START(2, 3)
        Z_PARAM_RESOURCE(rfc_zv)
        Z_PARAM_STR(function_module)
        Z_PARAM_OPTIONAL
        Z_PARAM_BOOL(not_trim)
    ZEND_PARSE_PARAMETERS_END();

    rfc_resource = saprfc_require_rfc(rfc_zv);
    if (!rfc_resource) {
        RETURN_FALSE;
    }

    fce = CAL_NEW(ZSTR_VAL(function_module));
    if (!fce) {
        php_error_docref(NULL, E_WARNING,
                         "fail allocate internal memory for function %s",
                         ZSTR_VAL(function_module));
        RETURN_FALSE;
    }

    rc = CAL_DISCOVER_INTERFACE(fce, rfc_resource->handle);
    if (rc != 0) {
        php_error_docref(NULL, E_WARNING, "%s", CAL_DEBUG_MESSAGE());
        CAL_DELETE(fce);
        RETURN_FALSE;
    }

    fce_resource = emalloc(sizeof(FCE_RESOURCE));
    fce_resource->handle = rfc_resource->handle;
    fce_resource->fce = fce;
    fce_resource->owner = NULL;
    fce_resource->next = NULL;
    saprfc_attach_function(rfc_resource, fce_resource);
    if (not_trim) {
        CAL_SET_RAWSTR(fce);
    }

    RETURN_RES(zend_register_resource(fce_resource, le_function));
}
/* }}} */

/* {{{ proto resource|false saprfc_function_define(mixed rfc, string function_module, array iface [, bool not_trim]) */
PHP_FUNCTION(saprfc_function_define)
{
    zval *rfc_zv;
    zend_string *function_module;
    zval *iface;
    bool not_trim = false;
    RFC_RESOURCE *rfc_resource = NULL;
    FCE_RESOURCE *fce_resource;
    CALD_FUNCTION_MODULE *fce;
    bool rfc_undef = false;
    zend_ulong i;
    zend_ulong iface_count;

    ZEND_PARSE_PARAMETERS_START(3, 4)
        Z_PARAM_ZVAL(rfc_zv)
        Z_PARAM_STR(function_module)
        Z_PARAM_ARRAY(iface)
        Z_PARAM_OPTIONAL
        Z_PARAM_BOOL(not_trim)
    ZEND_PARSE_PARAMETERS_END();

    if (Z_TYPE_P(rfc_zv) == IS_LONG && Z_LVAL_P(rfc_zv) == 0) {
        rfc_undef = true;
    } else {
        rfc_resource = saprfc_require_rfc(rfc_zv);
        if (!rfc_resource) {
            RETURN_FALSE;
        }
    }

    fce = CAL_NEW(ZSTR_VAL(function_module));
    if (!fce) {
        php_error_docref(NULL, E_WARNING,
                         "fail allocate internal memory for function %s",
                         ZSTR_VAL(function_module));
        RETURN_FALSE;
    }

    iface_count = zend_hash_num_elements(Z_ARRVAL_P(iface));
    for (i = 0; i < iface_count; i++) {
        zval *param = zend_hash_index_find(Z_ARRVAL_P(iface), i);
        zval *zname, *ztype, *zoptional, *zdef;
        const char *param_name;
        const char *param_type;
        bool optional = false;
        zend_ulong j;
        zend_ulong def_count;
        int interface_type;

        if (!param || Z_TYPE_P(param) != IS_ARRAY) {
            continue;
        }

        zname = zend_hash_str_find(Z_ARRVAL_P(param), "name", sizeof("name") - 1);
        ztype = zend_hash_str_find(Z_ARRVAL_P(param), "type", sizeof("type") - 1);
        zoptional = zend_hash_str_find(Z_ARRVAL_P(param), "optional", sizeof("optional") - 1);
        zdef = zend_hash_str_find(Z_ARRVAL_P(param), "def", sizeof("def") - 1);

        if (!zname || Z_TYPE_P(zname) != IS_STRING
            || !ztype || Z_TYPE_P(ztype) != IS_STRING
            || !zdef || Z_TYPE_P(zdef) != IS_ARRAY) {
            php_error_docref(NULL, E_WARNING,
                             "Invalid interface description for parameter #%lu",
                             (unsigned long)i);
            continue;
        }

        param_name = Z_STRVAL_P(zname);
        param_type = Z_STRVAL_P(ztype);
        if (zoptional) {
            optional = zend_is_true(zoptional);
        }

        if (strcasecmp(param_type, "IMPORT") == 0) {
            interface_type = CALC_IMPORT;
        } else if (strcasecmp(param_type, "EXPORT") == 0) {
            interface_type = CALC_EXPORT;
        } else if (strcasecmp(param_type, "TABLE") == 0) {
            interface_type = CALC_TABLE;
        } else {
            php_error_docref(NULL, E_WARNING,
                             "Unknown interface type '%s' for parameter %s",
                             param_type, param_name);
            continue;
        }

        def_count = zend_hash_num_elements(Z_ARRVAL_P(zdef));
        for (j = 0; j < def_count; j++) {
            zval *item = zend_hash_index_find(Z_ARRVAL_P(zdef), j);
            zval *zitem_name;
            zval *zabap, *zlen, *zdec, *zoffset;
            const char *item_name = "";
            char abap = 'C';
            zend_long length = 0;
            zend_long decimals = 0;
            zend_long offset = -1;

            if (!item || Z_TYPE_P(item) != IS_ARRAY) {
                continue;
            }

            zitem_name = zend_hash_str_find(Z_ARRVAL_P(item), "name", sizeof("name") - 1);
            zabap = zend_hash_str_find(Z_ARRVAL_P(item), "abap", sizeof("abap") - 1);
            zlen = zend_hash_str_find(Z_ARRVAL_P(item), "len", sizeof("len") - 1);
            zdec = zend_hash_str_find(Z_ARRVAL_P(item), "dec", sizeof("dec") - 1);
            zoffset = zend_hash_str_find(Z_ARRVAL_P(item), "offset", sizeof("offset") - 1);

            if (zitem_name && Z_TYPE_P(zitem_name) == IS_STRING) {
                item_name = Z_STRVAL_P(zitem_name);
            }
            if (zabap && Z_TYPE_P(zabap) == IS_STRING && Z_STRLEN_P(zabap) > 0) {
                abap = Z_STRVAL_P(zabap)[0];
            }
            if (zlen) {
                length = zval_get_long(zlen);
            }
            if (zdec) {
                decimals = zval_get_long(zdec);
            }
            if (zoffset) {
                offset = zval_get_long(zoffset);
            }
            if (length < 0) {
                length = 0;
            }
            if (decimals < 0) {
                decimals = 0;
            }
            if (offset < -1) {
                offset = -1;
            }

            /* A one-field definition is an elementary parameter. */
            if (def_count == 1) {
                item_name = "";
            }

            fce->par_offset = (int)offset;
            switch (interface_type) {
                case CALC_IMPORT:
                    CAL_INTERFACE_IMPORT_RAW(fce, (char *)param_name,
                                             (char *)item_name, abap,
                                             (unsigned)length, (unsigned)decimals);
                    CAL_INTERFACE_IMPORT_OPT(fce, (char *)param_name, optional);
                    break;
                case CALC_EXPORT:
                    CAL_INTERFACE_EXPORT_RAW(fce, (char *)param_name,
                                             (char *)item_name, abap,
                                             (unsigned)length, (unsigned)decimals);
                    CAL_INTERFACE_EXPORT_OPT(fce, (char *)param_name, optional);
                    break;
                case CALC_TABLE:
                    CAL_INTERFACE_TABLE_RAW(fce, (char *)param_name,
                                            (char *)item_name, abap,
                                            (unsigned)length, (unsigned)decimals);
                    CAL_INTERFACE_TABLE_OPT(fce, (char *)param_name, optional);
                    break;
            }
            fce->par_offset = -1;
        }
    }

    CAL_INIT_INTERFACE(fce);

    fce_resource = emalloc(sizeof(FCE_RESOURCE));
    fce_resource->handle = rfc_undef ? RFC_HANDLE_NULL : rfc_resource->handle;
    fce_resource->fce = fce;
    fce_resource->owner = NULL;
    fce_resource->next = NULL;
    if (!rfc_undef) {
        saprfc_attach_function(rfc_resource, fce_resource);
    }
    if (not_trim) {
        CAL_SET_RAWSTR(fce);
    }

    RETURN_RES(zend_register_resource(fce_resource, le_function));
}
/* }}} */

/* {{{ proto array|false saprfc_function_interface(resource fce) */
PHP_FUNCTION(saprfc_function_interface)
{
    zval *fce_zv;
    FCE_RESOURCE *fce_resource;
    CALD_FUNCTION_MODULE *fce;
    CALD_INTERFACE_INFO *iinfo;
    int i, j;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_RESOURCE(fce_zv)
    ZEND_PARSE_PARAMETERS_END();

    fce_resource = saprfc_require_fce(fce_zv);
    if (!fce_resource) {
        RETURN_FALSE;
    }
    fce = fce_resource->fce;

    iinfo = CAL_INTERFACE_INFO(fce);
    if (!iinfo) {
        php_error_docref(NULL, E_WARNING, "%s", CAL_DEBUG_MESSAGE());
        RETURN_FALSE;
    }
    array_init(return_value);

    for (i = 0; iinfo[i].name != NULL; i++) {
        zval param;
        zval def;
        const char *type_name;

        array_init(&param);
        add_assoc_string(&param, "name", iinfo[i].name);
        switch (iinfo[i].type) {
            case CALC_IMPORT: type_name = "IMPORT"; break;
            case CALC_EXPORT: type_name = "EXPORT"; break;
            case CALC_TABLE:  type_name = "TABLE";  break;
            default:          type_name = "UNDEF";  break;
        }
        add_assoc_string(&param, "type", (char *)type_name);
        add_assoc_long(&param, "optional", iinfo[i].is_optional);

        array_init(&def);
        if (iinfo[i].typeinfo) {
            for (j = 0; j < iinfo[i].size; j++) {
                zval item;
                char abap[2];

                array_init(&item);
                add_assoc_string(&item, "name", iinfo[i].typeinfo[j].name);
                abap[0] = iinfo[i].typeinfo[j].abap;
                abap[1] = '\0';
                add_assoc_string(&item, "abap", abap);
                add_assoc_long(&item, "len", iinfo[i].typeinfo[j].length);
                add_assoc_long(&item, "dec", iinfo[i].typeinfo[j].decimals);
                add_assoc_long(&item, "offset", iinfo[i].typeinfo[j].offset);
                add_next_index_zval(&def, &item);
            }
        }
        add_assoc_zval(&param, "def", &def);
        add_next_index_zval(return_value, &param);
    }

    CAL_DEL_INTERFACE(iinfo);
}
/* }}} */

/* {{{ proto void saprfc_function_debug_info(resource fce [, bool only_values]) */
PHP_FUNCTION(saprfc_function_debug_info)
{
    zval *fce_zv;
    bool only_values = false;
    FCE_RESOURCE *fce_resource;
    CALD_FUNCTION_MODULE *f;
    CALD_INTERFACE_INFO *iinfo;
    CALD_INTERFACE_INFO *p;
    int i, j, k;
    int flag;

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_RESOURCE(fce_zv)
        Z_PARAM_OPTIONAL
        Z_PARAM_BOOL(only_values)
    ZEND_PARSE_PARAMETERS_END();

    fce_resource = saprfc_require_fce(fce_zv);
    if (!fce_resource) {
        RETURN_FALSE;
    }
    f = fce_resource->fce;

    iinfo = CAL_INTERFACE_INFO(f);
    if (!iinfo) {
        php_error_docref(NULL, E_WARNING, "%s", CAL_DEBUG_MESSAGE());
        RETURN_FALSE;
    }

    if (f->unicode == 0) {
        php_printf("<h3>Function module: %s (remote SAP R/3: %s)</h3>\n",
                   f->name, f->rfcsaprl);
    } else {
        php_printf("<h3>Function module: %s (remote SAP R/3: %s Unicode)</h3>\n",
                   f->name, f->rfcsaprl);
    }

    if (!only_values) {
        for (j = CALC_IMPORT; j <= CALC_TABLE; j++) {
            switch (j) {
                case CALC_IMPORT: php_printf("<h3>IMPORT</h3>\n<table>"); break;
                case CALC_EXPORT: php_printf("<h3>EXPORT</h3>\n<table>"); break;
                case CALC_TABLE:  php_printf("<h3>TABLE</h3>\n<table>");  break;
                default:          php_printf("<h3>UNDEF</h3>\n<table>");  break;
            }
            p = iinfo;
            while (p->name) {
                if (p->type == j) {
                    flag = 1;
                    for (i = 0; i < p->size && p->typeinfo; i++) {
                        php_printf("<tr>");
                        if (flag) {
                            if (p->is_optional) {
                                php_printf("<td><b>%s /o/</b></td>", p->name);
                            } else {
                                php_printf("<td><b>%s</b></td>", p->name);
                            }
                        } else {
                            php_printf("<td></td>");
                        }
                        flag = 0;
                        php_printf("<td>%s</td><td>%c</td><td>%d</td><td>%d</td><td>offset = %d</td></tr>\n",
                                   p->typeinfo[i].name,
                                   p->typeinfo[i].abap,
                                   p->typeinfo[i].length,
                                   p->typeinfo[i].decimals,
                                   p->typeinfo[i].offset);
                    }
                }
                p++;
            }
            php_printf("</table>");
        }
    }

    for (j = CALC_IMPORT; j <= CALC_TABLE; j++) {
        p = iinfo;
        while (p->name) {
            if (p->type == j) {
                const char *optstr = p->is_optional ? " (optional)" : "";

                switch (j) {
                    case CALC_IMPORT:
                        php_printf("<h3>Value of import (input) parameter <b>%s</b>%s (memory = %d):</h3>",
                                   p->name, optstr, p->buflen);
                        break;
                    case CALC_EXPORT:
                        php_printf("<h3>Value of export (output) parameter <b>%s</b>%s (memory = %d):</h3>",
                                   p->name, optstr, p->buflen);
                        break;
                    case CALC_TABLE:
                        php_printf("<h3>Internal table <b>%s</b>%s (memory = %d):</h3>",
                                   p->name, optstr, p->buflen);
                        break;
                }

                if (p->typeinfo && p->size > 0 && p->typeinfo[0].name[0] != '\0') {
                    php_printf("<table><tr>");
                    for (i = 0; i < p->size; i++) {
                        php_printf("<td><b>%s</b></td>", p->typeinfo[i].name);
                    }
                    php_printf("</tr>");
                }

                if (p->typeinfo && p->size > 0 && p->typeinfo[0].name[0] == '\0') {
                    char *value = (j == CALC_IMPORT)
                                      ? CAL_GET_IMPORT(f, p->name)
                                      : CAL_GET_EXPORT(f, p->name);
                    php_printf("\"%s\"", value ? value : "");
                } else if (j != CALC_TABLE && p->typeinfo) {
                    php_printf("<tr>");
                    for (i = 0; i < p->size; i++) {
                        char *value = (j == CALC_IMPORT)
                                          ? CAL_GET_IMPORT_STRUCT(f, p->name, p->typeinfo[i].name)
                                          : CAL_GET_EXPORT_STRUCT(f, p->name, p->typeinfo[i].name);
                        php_printf("<td>\"%s\"</td>", value ? value : "");
                    }
                    php_printf("</tr></table>");
                } else if (j == CALC_TABLE) {
                    int size = CAL_TBL_LENGTH(f, p->name);
                    for (k = 1; k <= size; k++) {
                        php_printf("<tr>");
                        CAL_TBL_READ(f, p->name, k);
                        for (i = 0; i < p->size && p->typeinfo; i++) {
                            char *value = CAL_GET_TABLE(f, p->name, p->typeinfo[i].name);
                            php_printf("<td>\"%s\"</td>", value ? value : "");
                        }
                        php_printf("</tr>");
                    }
                    php_printf("</table><p>%d rows", size);
                }
            }
            p++;
        }
    }

    CAL_DEL_INTERFACE(iinfo);
}
/* }}} */

/* {{{ proto bool saprfc_optional(resource fce, string name, bool value) */
PHP_FUNCTION(saprfc_optional)
{
    zval *fce_zv;
    zend_string *name;
    bool value;
    FCE_RESOURCE *fce_resource;
    int rc;

    ZEND_PARSE_PARAMETERS_START(3, 3)
        Z_PARAM_RESOURCE(fce_zv)
        Z_PARAM_STR(name)
        Z_PARAM_BOOL(value)
    ZEND_PARSE_PARAMETERS_END();

    fce_resource = saprfc_require_fce(fce_zv);
    if (!fce_resource) {
        RETURN_FALSE;
    }

    rc = CAL_INTERFACE_IMPORT_OPT(fce_resource->fce, ZSTR_VAL(name),
                                  value ? 1 : 0);
    if (rc != 0) {
        php_error_docref(NULL, E_WARNING, "%s", CAL_DEBUG_MESSAGE());
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool saprfc_import(resource fce, string name, mixed value) */
PHP_FUNCTION(saprfc_import)
{
    zval *fce_zv;
    zend_string *name;
    zval *value;
    FCE_RESOURCE *fce_resource;
    int rc;

    ZEND_PARSE_PARAMETERS_START(3, 3)
        Z_PARAM_RESOURCE(fce_zv)
        Z_PARAM_STR(name)
        Z_PARAM_ZVAL(value)
    ZEND_PARSE_PARAMETERS_END();

    fce_resource = saprfc_require_fce(fce_zv);
    if (!fce_resource) {
        RETURN_FALSE;
    }

    if (Z_TYPE_P(value) == IS_ARRAY) {
        rc = saprfc_apply_array(fce_resource->fce, CALC_IMPORT,
                                ZSTR_VAL(name), Z_ARRVAL_P(value));
    } else {
        rc = saprfc_apply_scalar(fce_resource->fce, CALC_IMPORT,
                                 ZSTR_VAL(name), value);
    }
    if (rc != 0) {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto mixed saprfc_export(resource fce, string name) */
PHP_FUNCTION(saprfc_export)
{
    zval *fce_zv;
    zend_string *name;
    FCE_RESOURCE *fce_resource;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_RESOURCE(fce_zv)
        Z_PARAM_STR(name)
    ZEND_PARSE_PARAMETERS_END();

    fce_resource = saprfc_require_fce(fce_zv);
    if (!fce_resource) {
        RETURN_FALSE;
    }

    saprfc_return_parameter(fce_resource->fce, CALC_EXPORT,
                            ZSTR_VAL(name), return_value);
}
/* }}} */

/* {{{ proto bool saprfc_table_init(resource fce, string name) */
PHP_FUNCTION(saprfc_table_init)
{
    zval *fce_zv;
    zend_string *name;
    FCE_RESOURCE *fce_resource;
    int rc;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_RESOURCE(fce_zv)
        Z_PARAM_STR(name)
    ZEND_PARSE_PARAMETERS_END();

    fce_resource = saprfc_require_fce(fce_zv);
    if (!fce_resource) {
        RETURN_FALSE;
    }

    rc = CAL_REFRESH_INTERNAL_BUFFER(fce_resource->fce, ZSTR_VAL(name),
                                     CALC_TABLE);
    if (rc != 0) {
        php_error_docref(NULL, E_WARNING, "%s", CAL_DEBUG_MESSAGE());
        RETURN_FALSE;
    }
    rc = CAL_TBL_INIT(fce_resource->fce, ZSTR_VAL(name));
    if (rc != 0) {
        php_error_docref(NULL, E_WARNING, "%s", CAL_DEBUG_MESSAGE());
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
/* }}} */

static bool saprfc_table_prepare(zval *fce_zv, zend_string *name,
                                 zval *value, FCE_RESOURCE **out)
{
    FCE_RESOURCE *fce_resource = saprfc_require_fce(fce_zv);
    int rc;

    if (!fce_resource) {
        return false;
    }

    rc = CAL_REFRESH_INTERNAL_BUFFER(fce_resource->fce, ZSTR_VAL(name),
                                     CALC_TABLE);
    if (rc != 0) {
        php_error_docref(NULL, E_WARNING, "%s", CAL_DEBUG_MESSAGE());
        return false;
    }

    if (Z_TYPE_P(value) == IS_ARRAY) {
        rc = saprfc_apply_array(fce_resource->fce, CALC_TABLE,
                                ZSTR_VAL(name), Z_ARRVAL_P(value));
    } else {
        rc = saprfc_apply_scalar(fce_resource->fce, CALC_TABLE,
                                 ZSTR_VAL(name), value);
    }
    if (rc != 0) {
        return false;
    }

    *out = fce_resource;
    return true;
}

/* {{{ proto bool saprfc_table_append(resource fce, string name, array value) */
PHP_FUNCTION(saprfc_table_append)
{
    zval *fce_zv;
    zend_string *name;
    zval *value;
    FCE_RESOURCE *fce_resource;
    int rc;

    ZEND_PARSE_PARAMETERS_START(3, 3)
        Z_PARAM_RESOURCE(fce_zv)
        Z_PARAM_STR(name)
        Z_PARAM_ZVAL(value)
    ZEND_PARSE_PARAMETERS_END();

    if (!saprfc_table_prepare(fce_zv, name, value, &fce_resource)) {
        RETURN_FALSE;
    }

    rc = CAL_TBL_APPEND(fce_resource->fce, ZSTR_VAL(name));
    if (rc != 0) {
        php_error_docref(NULL, E_WARNING, "%s", CAL_DEBUG_MESSAGE());
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool saprfc_table_insert(resource fce, string name, array value, int index) */
PHP_FUNCTION(saprfc_table_insert)
{
    zval *fce_zv;
    zend_string *name;
    zval *value;
    zend_long index;
    FCE_RESOURCE *fce_resource;
    int rc;

    ZEND_PARSE_PARAMETERS_START(4, 4)
        Z_PARAM_RESOURCE(fce_zv)
        Z_PARAM_STR(name)
        Z_PARAM_ZVAL(value)
        Z_PARAM_LONG(index)
    ZEND_PARSE_PARAMETERS_END();

    if (!saprfc_table_prepare(fce_zv, name, value, &fce_resource)) {
        RETURN_FALSE;
    }

    rc = CAL_TBL_INSERT(fce_resource->fce, ZSTR_VAL(name), (int)index);
    if (rc != 0) {
        php_error_docref(NULL, E_WARNING, "%s", CAL_DEBUG_MESSAGE());
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool saprfc_table_modify(resource fce, string name, array value, int index) */
PHP_FUNCTION(saprfc_table_modify)
{
    zval *fce_zv;
    zend_string *name;
    zval *value;
    zend_long index;
    FCE_RESOURCE *fce_resource;
    int rc;

    ZEND_PARSE_PARAMETERS_START(4, 4)
        Z_PARAM_RESOURCE(fce_zv)
        Z_PARAM_STR(name)
        Z_PARAM_ZVAL(value)
        Z_PARAM_LONG(index)
    ZEND_PARSE_PARAMETERS_END();

    if (!saprfc_table_prepare(fce_zv, name, value, &fce_resource)) {
        RETURN_FALSE;
    }

    rc = CAL_TBL_MODIFY(fce_resource->fce, ZSTR_VAL(name), (int)index);
    if (rc != 0) {
        php_error_docref(NULL, E_WARNING, "%s", CAL_DEBUG_MESSAGE());
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool saprfc_table_remove(resource fce, string name, int index) */
PHP_FUNCTION(saprfc_table_remove)
{
    zval *fce_zv;
    zend_string *name;
    zend_long index;
    FCE_RESOURCE *fce_resource;
    int rc;

    ZEND_PARSE_PARAMETERS_START(3, 3)
        Z_PARAM_RESOURCE(fce_zv)
        Z_PARAM_STR(name)
        Z_PARAM_LONG(index)
    ZEND_PARSE_PARAMETERS_END();

    fce_resource = saprfc_require_fce(fce_zv);
    if (!fce_resource) {
        RETURN_FALSE;
    }

    rc = CAL_TBL_REMOVE(fce_resource->fce, ZSTR_VAL(name), (int)index);
    if (rc != 0) {
        php_error_docref(NULL, E_WARNING, "%s", CAL_DEBUG_MESSAGE());
        RETURN_FALSE;
    }
    /* The 1.4.1 implementation fell through without setting a return value;
     * the documented contract is bool, so return true on success. */
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto array|false saprfc_table_read(resource fce, string name, int index) */
PHP_FUNCTION(saprfc_table_read)
{
    zval *fce_zv;
    zend_string *name;
    zend_long index;
    FCE_RESOURCE *fce_resource;
    CALD_INTERFACE_INFO *iinfo;
    int i;
    int rc;

    ZEND_PARSE_PARAMETERS_START(3, 3)
        Z_PARAM_RESOURCE(fce_zv)
        Z_PARAM_STR(name)
        Z_PARAM_LONG(index)
    ZEND_PARSE_PARAMETERS_END();

    fce_resource = saprfc_require_fce(fce_zv);
    if (!fce_resource) {
        RETURN_FALSE;
    }

    rc = CAL_TBL_READ(fce_resource->fce, ZSTR_VAL(name), (int)index);
    if (rc != 0) {
        php_error_docref(NULL, E_WARNING, "%s", CAL_DEBUG_MESSAGE());
        RETURN_FALSE;
    }

    iinfo = CAL_SINGLE_INTERFACE_INFO(fce_resource->fce, ZSTR_VAL(name),
                                      CALC_TABLE);
    if (!iinfo) {
        php_error_docref(NULL, E_WARNING, "%s", CAL_DEBUG_MESSAGE());
        RETURN_FALSE;
    }
    if (iinfo->size <= 0 || !iinfo->typeinfo) {
        CAL_DEL_INTERFACE(iinfo);
        RETURN_FALSE;
    }

    array_init(return_value);
    for (i = 0; i < iinfo->size; i++) {
        char *value = CAL_GET_TABLE(fce_resource->fce, ZSTR_VAL(name),
                                    iinfo->typeinfo[i].name);
        if (!value) {
            value = "";
        }
        if (iinfo->typeinfo[i].abap == 'X') {
            add_assoc_stringl(return_value, iinfo->typeinfo[i].name,
                              value, iinfo->typeinfo[i].length);
        } else {
            add_assoc_string(return_value, iinfo->typeinfo[i].name, value);
        }
    }

    CAL_DEL_INTERFACE(iinfo);
}
/* }}} */

/* {{{ proto int saprfc_table_rows(resource fce, string name) */
PHP_FUNCTION(saprfc_table_rows)
{
    zval *fce_zv;
    zend_string *name;
    FCE_RESOURCE *fce_resource;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_RESOURCE(fce_zv)
        Z_PARAM_STR(name)
    ZEND_PARSE_PARAMETERS_END();

    fce_resource = saprfc_require_fce(fce_zv);
    if (!fce_resource) {
        RETURN_FALSE;
    }
    RETURN_LONG(CAL_TBL_LENGTH(fce_resource->fce, ZSTR_VAL(name)));
}
/* }}} */

/* {{{ proto int saprfc_call_and_receive(resource fce) */
PHP_FUNCTION(saprfc_call_and_receive)
{
    zval *fce_zv;
    FCE_RESOURCE *fce_resource;
    RFC_RC rfc_rc;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_RESOURCE(fce_zv)
    ZEND_PARSE_PARAMETERS_END();

    fce_resource = saprfc_require_fce(fce_zv);
    if (!fce_resource) {
        RETURN_LONG(-1);
    }
    if (fce_resource->handle == RFC_HANDLE_NULL) {
        php_error_docref(NULL, E_WARNING,
                         "RFC function module is not associated with a connection");
        RETURN_LONG(-1);
    }

    CAL_INIT_INTERFACE_EXPORT(fce_resource->fce);
    rfc_rc = CAL_CALL(fce_resource->fce, fce_resource->handle);
    if (rfc_rc != 0) {
        php_error_docref(NULL, E_WARNING, "%s", CAL_DEBUG_MESSAGE());
    }
    CAL_INIT_INTERFACE_IMPORT(fce_resource->fce);
    RETURN_LONG(rfc_rc);
}
/* }}} */

/* {{{ proto string saprfc_error() */
PHP_FUNCTION(saprfc_error)
{
    ZEND_PARSE_PARAMETERS_NONE();
    RETURN_STRING(CAL_RFC_LAST_ERROR());
}
/* }}} */

/* {{{ proto string|false saprfc_exception(resource fce) */
PHP_FUNCTION(saprfc_exception)
{
    zval *fce_zv;
    FCE_RESOURCE *fce_resource;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_RESOURCE(fce_zv)
    ZEND_PARSE_PARAMETERS_END();

    fce_resource = saprfc_require_fce(fce_zv);
    if (!fce_resource) {
        RETURN_FALSE;
    }
    if (fce_resource->fce->exception) {
        RETURN_STRING(fce_resource->fce->exception);
    }
    RETURN_FALSE;
}
/* }}} */

/* {{{ proto bool saprfc_function_free(resource fce) */
PHP_FUNCTION(saprfc_function_free)
{
    zval *fce_zv;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_RESOURCE(fce_zv)
    ZEND_PARSE_PARAMETERS_END();

    if (!saprfc_fetch_fce(fce_zv)) {
        php_error_docref(NULL, E_WARNING, "Invalid resource for function module");
        RETURN_FALSE;
    }
    zend_list_close(Z_RES_P(fce_zv));
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool saprfc_close(resource rfc) */
PHP_FUNCTION(saprfc_close)
{
    zval *rfc_zv;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_RESOURCE(rfc_zv)
    ZEND_PARSE_PARAMETERS_END();

    if (!saprfc_fetch_rfc(rfc_zv)) {
        php_error_docref(NULL, E_WARNING, "Invalid resource for RFC connection");
        RETURN_FALSE;
    }
    zend_list_close(Z_RES_P(rfc_zv));
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool saprfc_set_code_page(resource rfc, string codepage) */
PHP_FUNCTION(saprfc_set_code_page)
{
    zval *rfc_zv;
    zend_string *codepage;
    RFC_RESOURCE *rfc_resource;
    int rc;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_RESOURCE(rfc_zv)
        Z_PARAM_STR(codepage)
    ZEND_PARSE_PARAMETERS_END();

    rfc_resource = saprfc_require_rfc(rfc_zv);
    if (!rfc_resource) {
        RETURN_FALSE;
    }

    rc = CAL_SET_CODE_PAGE(rfc_resource->handle, ZSTR_VAL(codepage));
    if (rc != 0) {
        php_error_docref(NULL, E_WARNING, "Cannot set codepage %s",
                         ZSTR_VAL(codepage));
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto array|false saprfc_attributes(resource rfc) */
PHP_FUNCTION(saprfc_attributes)
{
    zval *rfc_zv;
    RFC_RESOURCE *rfc_resource;
    RFC_ATTRIBUTES attributes;
    int rc;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_RESOURCE(rfc_zv)
    ZEND_PARSE_PARAMETERS_END();

    rfc_resource = saprfc_require_rfc(rfc_zv);
    if (!rfc_resource) {
        RETURN_FALSE;
    }

    memset(&attributes, 0, sizeof(attributes));
    rc = CAL_ATTRIBUTES(rfc_resource->handle, &attributes);
    if (rc != 0) {
        php_error_docref(NULL, E_WARNING, "RfcGetAttributes error");
        RETURN_FALSE;
    }

    array_init(return_value);

    add_assoc_string(return_value, "dest", (char *)attributes.dest);
    add_assoc_string(return_value, "own_host", (char *)attributes.own_host);
    add_assoc_string(return_value, "partner_host", (char *)attributes.partner_host);
    add_assoc_string(return_value, "systnr", (char *)attributes.systnr);
    add_assoc_string(return_value, "sysid", (char *)attributes.sysid);
    add_assoc_string(return_value, "client", (char *)attributes.client);
    add_assoc_string(return_value, "user", (char *)attributes.user);
    add_assoc_string(return_value, "language", (char *)attributes.language);
    add_assoc_stringl(return_value, "trace", (char *)&attributes.trace, 1);
    add_assoc_string(return_value, "ISO_language", (char *)attributes.ISO_language);
    add_assoc_string(return_value, "own_codepage", (char *)attributes.own_codepage);
    add_assoc_string(return_value, "partner_codepage", (char *)attributes.partner_codepage);
    add_assoc_stringl(return_value, "rfc_role", (char *)&attributes.rfc_role, 1);
    add_assoc_stringl(return_value, "own_type", (char *)&attributes.own_type, 1);
    add_assoc_string(return_value, "own_rel", (char *)attributes.own_rel);
    add_assoc_stringl(return_value, "partner_type", (char *)&attributes.partner_type, 1);
    add_assoc_string(return_value, "partner_rel", (char *)attributes.partner_rel);
    add_assoc_string(return_value, "kernel_rel", (char *)attributes.kernel_rel);
    add_assoc_string(return_value, "CPIC_convid", (char *)attributes.CPIC_convid);
}
/* }}} */

/* ------------------------------------------------------------------ */
/* RFC server API                                                      */
/* ------------------------------------------------------------------ */

/* {{{ proto resource|false saprfc_server_accept(mixed args) */
PHP_FUNCTION(saprfc_server_accept)
{
    zval *args;
    RFC_HANDLE rfc;
    RFC_RESOURCE *rfc_resource;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ZVAL(args)
    ZEND_PARSE_PARAMETERS_END();

    if (Z_TYPE_P(args) == IS_ARRAY) {
        int argc = (int)zend_hash_num_elements(Z_ARRVAL_P(args));
        char **argv = ecalloc((size_t)argc + 1, sizeof(char *));
        zend_string **keep = ecalloc((size_t)argc + 1, sizeof(zend_string *));
        int i;

        for (i = 0; i < argc; i++) {
            zval *param = zend_hash_index_find(Z_ARRVAL_P(args), i);
            if (!param) {
                continue;
            }
            keep[i] = zval_get_string(param);
            if (!keep[i] || EG(exception)) {
                break;
            }
            argv[i] = ZSTR_VAL(keep[i]);
        }
        rfc = SAL_ACCEPT(argv);
        for (i = 0; i < argc; i++) {
            if (keep[i]) {
                zend_string_release(keep[i]);
            }
        }
        efree(keep);
        efree(argv);
    } else {
        zend_string *cmdline = zval_get_string(args);
        if (!cmdline) {
            RETURN_FALSE;
        }
        rfc = SAL_ACCEPT_EXT(ZSTR_VAL(cmdline));
        zend_string_release(cmdline);
    }

    if (rfc == RFC_HANDLE_NULL) {
        php_error_docref(NULL, E_WARNING, "%s", CAL_RFC_LAST_ERROR());
        RETURN_FALSE;
    }

    rfc_resource = emalloc(sizeof(RFC_RESOURCE));
    rfc_resource->handle = rfc;
    rfc_resource->client = 0;
    rfc_resource->functions = NULL;

    RETURN_RES(zend_register_resource(rfc_resource, le_rfc));
}
/* }}} */

/* {{{ proto mixed saprfc_server_import(resource fce, string name) */
PHP_FUNCTION(saprfc_server_import)
{
    zval *fce_zv;
    zend_string *name;
    FCE_RESOURCE *fce_resource;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_RESOURCE(fce_zv)
        Z_PARAM_STR(name)
    ZEND_PARSE_PARAMETERS_END();

    fce_resource = saprfc_require_fce(fce_zv);
    if (!fce_resource) {
        RETURN_FALSE;
    }

    saprfc_return_parameter(fce_resource->fce, CALC_IMPORT,
                            ZSTR_VAL(name), return_value);
}
/* }}} */

/* {{{ proto bool saprfc_server_export(resource fce, string name, mixed value) */
PHP_FUNCTION(saprfc_server_export)
{
    zval *fce_zv;
    zend_string *name;
    zval *value;
    FCE_RESOURCE *fce_resource;
    int rc;

    ZEND_PARSE_PARAMETERS_START(3, 3)
        Z_PARAM_RESOURCE(fce_zv)
        Z_PARAM_STR(name)
        Z_PARAM_ZVAL(value)
    ZEND_PARSE_PARAMETERS_END();

    fce_resource = saprfc_require_fce(fce_zv);
    if (!fce_resource) {
        RETURN_FALSE;
    }

    if (Z_TYPE_P(value) == IS_ARRAY) {
        rc = saprfc_apply_array(fce_resource->fce, CALC_EXPORT,
                                ZSTR_VAL(name), Z_ARRVAL_P(value));
    } else {
        rc = saprfc_apply_scalar(fce_resource->fce, CALC_EXPORT,
                                 ZSTR_VAL(name), value);
    }
    if (rc != 0) {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto int saprfc_server_dispatch(resource rfc, array list [, int timeout]) */
PHP_FUNCTION(saprfc_server_dispatch)
{
    zval *rfc_zv;
    zval *list;
    zend_long timeout = 0;
    RFC_RESOURCE *rfc_resource;
    FCE_RESOURCE *fce_resource;
    CALD_FUNCTION_MODULE *function_module;
    zval *fce_zv;
    RFC_RC rfc_rc = RFC_OK;
    RFC_FUNCTIONNAME function_name;
    char abort_text[1024];

    ZEND_PARSE_PARAMETERS_START(2, 3)
        Z_PARAM_RESOURCE(rfc_zv)
        Z_PARAM_ARRAY(list)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(timeout)
    ZEND_PARSE_PARAMETERS_END();

    rfc_resource = saprfc_require_rfc(rfc_zv);
    if (!rfc_resource) {
        RETURN_LONG(-1);
    }

    abort_text[0] = '\0';

    if (timeout != 0) {
        rfc_rc = SAL_WAIT_FOR_REQUEST(rfc_resource->handle, (int)timeout);
        if (rfc_rc == RFC_RETRY) {
            RETURN_LONG(rfc_rc);
        }
    }

    rfc_rc = SAL_GET_NAME(rfc_resource->handle, function_name);
    if (rfc_rc == RFC_SYSTEM_CALLED) {
        RETURN_LONG(RFC_OK);
    }
    if (rfc_rc != RFC_OK) {
        RETURN_LONG(rfc_rc);
    }

    fce_zv = zend_hash_str_find(Z_ARRVAL_P(list), function_name,
                                strlen(function_name));
    if (!fce_zv) {
        snprintf(abort_text, sizeof(abort_text),
                 "RFC function %s is not implemented in this server program",
                 function_name);
        rfc_rc = RFC_FAILURE;
        goto abort_request;
    }

    fce_resource = saprfc_fetch_fce(fce_zv);
    if (!fce_resource) {
        snprintf(abort_text, sizeof(abort_text),
                 "RFC function %s is not implemented in this server program",
                 function_name);
        rfc_rc = RFC_FAILURE;
        goto abort_request;
    }

    function_module = fce_resource->fce;
    CAL_INIT_INTERFACE(function_module);

    rfc_rc = SAL_GET_DATA(rfc_resource->handle, function_module);
    if (rfc_rc != RFC_OK) {
        if (rfc_rc == -1) {
            snprintf(abort_text, sizeof(abort_text),
                     "Error %s in RfcGetData", CAL_DEBUG_MESSAGE());
        } else {
            snprintf(abort_text, sizeof(abort_text),
                     "Error %s in RfcGetData", CAL_RFC_LAST_ERROR());
        }
        CAL_INIT_INTERFACE(function_module);
        goto abort_request;
    }

    {
        zval function_zv;
        zval retval;

        ZVAL_UNDEF(&retval);
        ZVAL_STRING(&function_zv, function_name);
        if (call_user_function(EG(function_table), NULL, &function_zv,
                               &retval, 1, fce_zv) != SUCCESS
            || EG(exception)) {
            snprintf(abort_text, sizeof(abort_text),
                     "PHP function %s can't be called", function_name);
            rfc_rc = RFC_FAILURE;
        } else if (Z_TYPE(retval) == IS_STRING && Z_STRLEN(retval) > 0) {
            SAL_RAISE(rfc_resource->handle, function_module,
                      Z_STRVAL(retval));
            rfc_rc = RFC_OK;
        } else {
            rfc_rc = SAL_SEND_DATA(rfc_resource->handle, function_module);
            if (rfc_rc != RFC_OK) {
                if (rfc_rc == -1) {
                    snprintf(abort_text, sizeof(abort_text),
                             "Error %s in RfcSendData", CAL_DEBUG_MESSAGE());
                } else {
                    snprintf(abort_text, sizeof(abort_text),
                             "Error %s in RfcSendData", CAL_RFC_LAST_ERROR());
                }
            }
        }
        if (!Z_ISUNDEF(retval)) {
            zval_ptr_dtor(&retval);
        }
        zval_ptr_dtor(&function_zv);
    }

    CAL_INIT_INTERFACE(function_module);

abort_request:
    if (rfc_rc != RFC_OK) {
        if (abort_text[0] == '\0') {
            snprintf(abort_text, sizeof(abort_text),
                     "RFC server request failed with code %ld", (long)rfc_rc);
        }
        SAL_ABORT(rfc_resource->handle, abort_text);
        zend_list_close(Z_RES_P(rfc_zv));
        php_error_docref(NULL, E_WARNING, "%s", abort_text);
    }

    RETURN_LONG(rfc_rc);
}
/* }}} */

/* {{{ proto bool saprfc_trfc_install(string tid_check, string tid_commit, string tid_rollback, string tid_confirm, string dispatcher) */
PHP_FUNCTION(saprfc_trfc_install)
{
    zend_string *tid_check, *tid_commit, *tid_rollback, *tid_confirm;
    zend_string *dispatcher;
    RFC_RC rfc_rc;

    SAPRFCLS_FETCH();

    ZEND_PARSE_PARAMETERS_START(5, 5)
        Z_PARAM_STR(tid_check)
        Z_PARAM_STR(tid_commit)
        Z_PARAM_STR(tid_rollback)
        Z_PARAM_STR(tid_confirm)
        Z_PARAM_STR(dispatcher)
    ZEND_PARSE_PARAMETERS_END();

    if (SAPRFCG(trfc_install_flag) == 1) {
        php_error_docref(NULL, E_WARNING, "tRFC managment is already installed.");
        RETURN_FALSE;
    }

    rfc_rc = SAL_INSTALL_FUNCTION("%%USER_GLOBAL_SERVER",
                                  callback_dispatch, NULL);
    if (rfc_rc != RFC_OK) {
        php_error_docref(NULL, E_WARNING,
                         "RfcInstallFunction() failed with code %d", (int)rfc_rc);
        RETURN_FALSE;
    }

    SAL_INSTALL_TRANSACTION_CONTROL(callback_tid_check,
                                    callback_tid_commit,
                                    callback_tid_rollback,
                                    callback_tid_confirm);
    SAPRFCG(trfc_install_flag) = 1;

#define SAPRFC_REPLACE_GLOBAL(field, value)                               \
    do {                                                                  \
        if (SAPRFCG(field)) {                                             \
            efree(SAPRFCG(field));                                        \
        }                                                                 \
        SAPRFCG(field) = estrndup(ZSTR_VAL(value), ZSTR_LEN(value));      \
    } while (0)

    SAPRFC_REPLACE_GLOBAL(trfc_tid_check, tid_check);
    SAPRFC_REPLACE_GLOBAL(trfc_tid_commit, tid_commit);
    SAPRFC_REPLACE_GLOBAL(trfc_tid_rollback, tid_rollback);
    SAPRFC_REPLACE_GLOBAL(trfc_tid_confirm, tid_confirm);
    SAPRFC_REPLACE_GLOBAL(trfc_dispatcher, dispatcher);

#undef SAPRFC_REPLACE_GLOBAL

    RETURN_TRUE;
}
/* }}} */

/* {{{ proto int saprfc_trfc_call(resource fce, string tid) */
PHP_FUNCTION(saprfc_trfc_call)
{
    zval *fce_zv;
    zend_string *tid;
    FCE_RESOURCE *fce_resource;
    RFC_RC rfc_rc;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_RESOURCE(fce_zv)
        Z_PARAM_STR(tid)
    ZEND_PARSE_PARAMETERS_END();

    fce_resource = saprfc_require_fce(fce_zv);
    if (!fce_resource) {
        RETURN_LONG(-1);
    }
    if (fce_resource->handle == RFC_HANDLE_NULL) {
        php_error_docref(NULL, E_WARNING,
                         "RFC function module is not associated with a connection");
        RETURN_LONG(-1);
    }

    CAL_INIT_INTERFACE_EXPORT(fce_resource->fce);
    rfc_rc = CAL_INDIRECT_CALL(fce_resource->fce, fce_resource->handle,
                               ZSTR_VAL(tid));
    if (rfc_rc != 0) {
        php_error_docref(NULL, E_WARNING, "%s", CAL_DEBUG_MESSAGE());
    }
    CAL_INIT_INTERFACE_IMPORT(fce_resource->fce);
    RETURN_LONG(rfc_rc);
}
/* }}} */

/* {{{ proto string|false saprfc_trfc_tid(resource rfc) */
PHP_FUNCTION(saprfc_trfc_tid)
{
    zval *rfc_zv;
    RFC_RESOURCE *rfc_resource;
    RFC_TID tid;
    RFC_RC rfc_rc = RFC_FAILURE;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_RESOURCE(rfc_zv)
    ZEND_PARSE_PARAMETERS_END();

    rfc_resource = saprfc_require_rfc(rfc_zv);
    if (!rfc_resource) {
        RETURN_FALSE;
    }

    memset(tid, 0, sizeof(tid));
    rfc_rc = CAL_CREATE_TID(rfc_resource->handle, tid);
    if (rfc_rc == RFC_OK) {
        tid[RFC_TID_LN] = '\0';
        RETURN_STRING(tid);
    }
    RETURN_FALSE;
}
/* }}} */

/* {{{ proto void saprfc_set_trace(mixed rfc, bool level) */
PHP_FUNCTION(saprfc_set_trace)
{
    zval *rfc_zv;
    bool level;
    RFC_RESOURCE *rfc_resource;
    RFC_HANDLE handle = RFC_HANDLE_NULL;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_ZVAL(rfc_zv)
        Z_PARAM_BOOL(level)
    ZEND_PARSE_PARAMETERS_END();

    rfc_resource = saprfc_fetch_rfc(rfc_zv);
    if (rfc_resource) {
        handle = rfc_resource->handle;
    }
    CAL_SET_TRACE(handle, level ? 1 : 0);
}
/* }}} */

/* {{{ proto array|int saprfc_server_register_check(string tpid, string gwhost, string gwserver) */
PHP_FUNCTION(saprfc_server_register_check)
{
    zend_string *tpid, *gwhost, *gwserver;
    RFC_RC rfc_rc;
    int ntotal = 0, ninit = 0, nready = 0, nbusy = 0;
    RFC_ERROR_INFO_EX error_info;

    ZEND_PARSE_PARAMETERS_START(3, 3)
        Z_PARAM_STR(tpid)
        Z_PARAM_STR(gwhost)
        Z_PARAM_STR(gwserver)
    ZEND_PARSE_PARAMETERS_END();

    memset(&error_info, 0, sizeof(error_info));
    rfc_rc = SAL_CHECK_REGISTER(ZSTR_VAL(tpid), ZSTR_VAL(gwhost),
                                ZSTR_VAL(gwserver), &ntotal, &ninit,
                                &nready, &nbusy, &error_info);
    if (rfc_rc != RFC_OK) {
        RETURN_LONG(rfc_rc);
    }

    array_init(return_value);
    add_assoc_long(return_value, "ntotal", ntotal);
    add_assoc_long(return_value, "ninit", ninit);
    add_assoc_long(return_value, "nready", nready);
    add_assoc_long(return_value, "nbusy", nbusy);
}
/* }}} */

/* {{{ proto array|int saprfc_server_register_cancel(string tpid, string gwhost, string gwserver) */
PHP_FUNCTION(saprfc_server_register_cancel)
{
    zend_string *tpid, *gwhost, *gwserver;
    RFC_RC rfc_rc;
    int ntotal = 0, ncancel = 0;
    RFC_ERROR_INFO_EX error_info;

    ZEND_PARSE_PARAMETERS_START(3, 3)
        Z_PARAM_STR(tpid)
        Z_PARAM_STR(gwhost)
        Z_PARAM_STR(gwserver)
    ZEND_PARSE_PARAMETERS_END();

    memset(&error_info, 0, sizeof(error_info));
    rfc_rc = SAL_CANCEL_REGISTER(ZSTR_VAL(tpid), ZSTR_VAL(gwhost),
                                 ZSTR_VAL(gwserver), &ntotal, &ncancel,
                                 &error_info);
    if (rfc_rc != RFC_OK) {
        RETURN_LONG(rfc_rc);
    }

    array_init(return_value);
    add_assoc_long(return_value, "ntotal", ntotal);
    add_assoc_long(return_value, "ncancel", ncancel);
}
/* }}} */

/* {{{ proto string|false saprfc_function_name(resource fce) */
PHP_FUNCTION(saprfc_function_name)
{
    zval *fce_zv;
    FCE_RESOURCE *fce_resource;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_RESOURCE(fce_zv)
    ZEND_PARSE_PARAMETERS_END();

    fce_resource = saprfc_require_fce(fce_zv);
    if (!fce_resource) {
        RETURN_FALSE;
    }
    RETURN_STRING(fce_resource->fce->name);
}
/* }}} */

/* {{{ proto bool saprfc_allow_start_program([string program_list]) */
PHP_FUNCTION(saprfc_allow_start_program)
{
    zend_string *program_list = NULL;
    RFC_RC rfc_rc;

    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_STR(program_list)
    ZEND_PARSE_PARAMETERS_END();

    if (program_list) {
        rfc_rc = CAL_ALLOW_START_PROGRAM(ZSTR_VAL(program_list));
    } else {
        rfc_rc = CAL_ALLOW_START_PROGRAM(NULL);
    }

    if (rfc_rc == RFC_OK) {
        RETURN_TRUE;
    }
    php_error_docref(NULL, E_WARNING,
                     "RfcAllowStartProgram failed with code = %d", (int)rfc_rc);
    RETURN_FALSE;
}
/* }}} */

/* {{{ proto string|false saprfc_get_ticket(resource rfc) */
PHP_FUNCTION(saprfc_get_ticket)
{
    zval *rfc_zv;
    RFC_RESOURCE *rfc_resource;
    RFC_CHAR ticket[1024];
    RFC_RC rfc_rc;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_RESOURCE(rfc_zv)
    ZEND_PARSE_PARAMETERS_END();

    rfc_resource = saprfc_require_rfc(rfc_zv);
    if (!rfc_resource) {
        RETURN_FALSE;
    }

    memset(ticket, 0, sizeof(ticket));
    rfc_rc = CAL_GET_TICKET(rfc_resource->handle, ticket);
    if (rfc_rc == RFC_OK) {
        ticket[sizeof(ticket) - 1] = '\0';
        RETURN_STRING((char *)ticket);
    }
    php_error_docref(NULL, E_WARNING, "Unable get SSO ticket");
    RETURN_FALSE;
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */

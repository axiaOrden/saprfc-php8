/*
 * rfccm_mock.c - TEST DOUBLE for the classic SAP RFC SDK (librfccm).
 *
 * This library implements just enough of the documented classic RFC API for
 * SAPRFC's PHP-level tests to run without SAP. It never opens a network
 * connection. It is NOT the SAP RFCSDK and must not be used in production.
 *
 * Deliberate mock behaviour:
 *   - RfcOpenEx() succeeds only when the connect string contains "MOCK=1".
 *   - RfcCallReceive("Z_MOCK_ECHO") copies import ECHO_IN to export ECHO_OUT.
 *   - RfcCallReceive("Z_MOCK_FAIL") returns RFC_FAILURE and sets RfcLastError.
 *   - RfcCallReceive("Z_MOCK_TABLE") appends two rows to table T_DATA.
 *   - All other function modules succeed and leave caller-provided buffers.
 */

#include "saprfc.h"
#include "sapitab.h"
#include "rfcsi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MOCK_MAX_COLUMNS 64

struct mock_itab {
    unsigned  line_length;
    unsigned  rows;
    unsigned  capacity;
    char     *data;
};

static int mock_handle_storage;
static char mock_last_error[512];

static unsigned mock_min_u(unsigned a, unsigned b)
{
    return a < b ? a : b;
}

static void mock_error_info(RFC_ERROR_INFO_EX *info, const char *message)
{
    if (!info) {
        return;
    }
    memset(info, 0, sizeof(*info));
    info->group = 1;
    snprintf(info->key, sizeof(info->key), "MOCK_CONNECT_FAILED");
    snprintf(info->message, sizeof(info->message), "%s", message);
}

static RFC_PARAMETER *mock_find_parameter(RFC_PARAMETER *list, const char *name)
{
    if (!list) {
        return NULL;
    }
    while (list->name) {
        if (strcmp(list->name, name) == 0) {
            return list;
        }
        list++;
    }
    return NULL;
}

static RFC_TABLE *mock_find_table(RFC_TABLE *list, const char *name)
{
    if (!list) {
        return NULL;
    }
    while (list->name) {
        if (strcmp(list->name, name) == 0) {
            return list;
        }
        list++;
    }
    return NULL;
}

RFC_HANDLE RfcOpenEx(char *connect_param, RFC_ERROR_INFO_EX *error_info)
{
    if (!connect_param) {
        mock_error_info(error_info, "empty connect parameter");
        return RFC_HANDLE_NULL;
    }
    if (strstr(connect_param, "MOCK=1") == NULL) {
        mock_error_info(error_info,
                        "mock RFCSDK: only MOCK=1 connections are supported");
        return RFC_HANDLE_NULL;
    }
    mock_error_info(error_info, "");
    return (RFC_HANDLE)&mock_handle_storage;
}

RFC_RC RfcClose(RFC_HANDLE rfc)
{
    (void)rfc;
    return RFC_OK;
}

RFC_RC RfcSetCodePage(RFC_HANDLE rfc, const char *codepage)
{
    (void)rfc;
    (void)codepage;
    return RFC_OK;
}

RFC_RC RfcSetSystemCodePage(const char *codepage)
{
    (void)codepage;
    return RFC_OK;
}

RFC_RC RfcGetAttributes(RFC_HANDLE rfc, RFC_ATTRIBUTES *attributes)
{
    if (!rfc || !attributes) {
        return RFC_FAILURE;
    }
    memset(attributes, 0, sizeof(*attributes));
    snprintf((char *)attributes->dest, sizeof(attributes->dest), "MOCK_DEST");
    snprintf((char *)attributes->own_host, sizeof(attributes->own_host), "mock-host");
    snprintf((char *)attributes->partner_host, sizeof(attributes->partner_host), "mock-partner");
    snprintf((char *)attributes->systnr, sizeof(attributes->systnr), "00");
    snprintf((char *)attributes->sysid, sizeof(attributes->sysid), "MOCK");
    snprintf((char *)attributes->client, sizeof(attributes->client), "100");
    snprintf((char *)attributes->user, sizeof(attributes->user), "MOCKUSER");
    snprintf((char *)attributes->language, sizeof(attributes->language), "EN");
    attributes->trace = '0';
    snprintf((char *)attributes->ISO_language, sizeof(attributes->ISO_language), "EN");
    snprintf((char *)attributes->own_codepage, sizeof(attributes->own_codepage), "4103");
    snprintf((char *)attributes->partner_codepage, sizeof(attributes->partner_codepage), "4103");
    attributes->rfc_role = 'C';
    attributes->own_type = 'E';
    snprintf((char *)attributes->own_rel, sizeof(attributes->own_rel), "620");
    attributes->partner_type = 'R';
    snprintf((char *)attributes->partner_rel, sizeof(attributes->partner_rel), "731");
    snprintf((char *)attributes->kernel_rel, sizeof(attributes->kernel_rel), "753");
    snprintf((char *)attributes->CPIC_convid, sizeof(attributes->CPIC_convid), "MOCKCONV");
    return RFC_OK;
}

RFC_RC RfcCreateTransID(RFC_HANDLE rfc, RFC_TID tid)
{
    if (!rfc || !tid) {
        return RFC_NO_TID;
    }
    memset(tid, 0, RFC_TID_LN + 1);
    snprintf(tid, RFC_TID_LN + 1, "MOCKTID0123456789ABCDEF");
    return RFC_OK;
}

RFC_RC RfcSetTrace(RFC_HANDLE rfc, int level)
{
    (void)rfc;
    (void)level;
    return RFC_OK;
}

RFC_RC RfcAllowStartProgram(const char *program_list)
{
    (void)program_list;
    return RFC_OK;
}

RFC_RC RfcGetTicket(RFC_HANDLE rfc, RFC_CHAR *ticket)
{
    if (!rfc || !ticket) {
        return RFC_FAILURE;
    }
    snprintf((char *)ticket, 1024, "MOCK-SSO-TICKET");
    return RFC_OK;
}

RFC_RC RfcInstallStructure(const char *name, RFC_TYPE_ELEMENT *def,
                           unsigned count, RFC_TYPEHANDLE *handle)
{
    (void)name;
    (void)def;
    if (!handle) {
        return RFC_FAILURE;
    }
    /* Structured types get a handle >= 100; elementary types stay < 100. */
    *handle = 100 + count;
    return RFC_OK;
}

static void mock_fill_elementary(void *addr, unsigned length, const char *value)
{
    if (!addr || length == 0) {
        return;
    }
    memset(addr, ' ', length);
    if (value) {
        memcpy(addr, value, mock_min_u((unsigned)strlen(value), length));
    }
}

RFC_RC RfcCallReceive(RFC_HANDLE rfc, const char *function_name,
                      RFC_PARAMETER *importing, RFC_PARAMETER *exporting,
                      RFC_TABLE *tables, char **exception)
{
    RFC_PARAMETER *param;

    if (!rfc || !function_name) {
        return RFC_FAILURE;
    }
    if (exception) {
        *exception = NULL;
    }

    if (strcmp(function_name, "Z_MOCK_FAIL") == 0) {
        snprintf(mock_last_error, sizeof(mock_last_error),
                 "mock RFCSDK: deliberate failure in Z_MOCK_FAIL");
        return RFC_FAILURE;
    }

    if (strcmp(function_name, "Z_MOCK_ECHO") == 0) {
        const char *value = "";
        param = mock_find_parameter(importing, "ECHO_IN");
        if (param && param->addr) {
            static char echo_buffer[4096];
            unsigned copy_len = mock_min_u(param->leng,
                                           (unsigned)sizeof(echo_buffer) - 1);
            memcpy(echo_buffer, param->addr, copy_len);
            echo_buffer[copy_len] = '\0';
            value = echo_buffer;
        }
        param = mock_find_parameter(exporting, "ECHO_OUT");
        if (param) {
            mock_fill_elementary(param->addr, param->leng, value);
        }
        return RFC_OK;
    }

    if (strcmp(function_name, "Z_MOCK_TABLE") == 0) {
        RFC_TABLE *table = mock_find_table(tables, "T_DATA");
        if (table && table->ithandle) {
            int i;
            for (i = 0; i < 2; i++) {
                char *line = ItAppLine(table->ithandle);
                if (line) {
                    unsigned len = ItLeng(table->ithandle);
                    memset(line, ' ', len);
                    if (len > 0) {
                        line[0] = (char)('0' + i + 1);
                    }
                }
            }
        }
        return RFC_OK;
    }

    /* Generic success: fill elementary exports with a deterministic marker. */
    for (param = exporting; param && param->name; param++) {
        if (param->type < 100) {
            mock_fill_elementary(param->addr, param->leng, "MOCK");
        }
    }
    return RFC_OK;
}

RFC_RC RfcIndirectCallEx(RFC_HANDLE rfc, const char *function_name,
                         RFC_PARAMETER *importing, RFC_TABLE *tables,
                         const char *tid)
{
    (void)importing;
    (void)tables;
    (void)tid;
    if (!rfc || !function_name) {
        return RFC_FAILURE;
    }
    return RFC_OK;
}

RFC_RC RfcConfirmTransID(RFC_HANDLE rfc, const char *tid)
{
    (void)rfc;
    (void)tid;
    return RFC_OK;
}

RFC_RC RfcGetData(RFC_HANDLE rfc, RFC_PARAMETER *importing, RFC_TABLE *tables)
{
    (void)rfc;
    (void)importing;
    (void)tables;
    return RFC_OK;
}

RFC_RC RfcSendData(RFC_HANDLE rfc, RFC_PARAMETER *exporting, RFC_TABLE *tables)
{
    (void)rfc;
    (void)exporting;
    (void)tables;
    return RFC_OK;
}

RFC_RC RfcRaise(RFC_HANDLE rfc, const char *exception)
{
    (void)rfc;
    (void)exception;
    return RFC_OK;
}

RFC_RC RfcRaiseTables(RFC_HANDLE rfc, const char *exception, RFC_TABLE *tables)
{
    (void)rfc;
    (void)exception;
    (void)tables;
    return RFC_OK;
}

void RfcLastError(RFC_ERROR_INFO *error_info)
{
    if (!error_info) {
        return;
    }
    memset(error_info, 0, sizeof(*error_info));
    snprintf(error_info->key, sizeof(error_info->key), "MOCK_ERROR");
    snprintf(error_info->status, sizeof(error_info->status), "FAILURE");
    snprintf(error_info->message, sizeof(error_info->message), "%s",
             mock_last_error[0] ? mock_last_error : "mock RFCSDK: no error");
    snprintf(error_info->intstat, sizeof(error_info->intstat), "MOCK");
}

void RfcGetAllLibVersions(char *buffer, unsigned length)
{
    if (!buffer || length == 0) {
        return;
    }
    snprintf(buffer, length,
             "mock classic RFC SDK test double (NOT SAP RFCSDK)");
}

void RfcEnvironment(RFC_ENV *env)
{
    (void)env;
}

void RfcConvertBcdToChar(void *bcd, unsigned len, unsigned decimals,
                         char *buffer, unsigned buffer_len)
{
    (void)bcd;
    (void)len;
    (void)decimals;
    if (buffer && buffer_len) {
        memset(buffer, ' ', buffer_len);
        buffer[0] = '0';
        buffer[buffer_len - 1] = '\0';
    }
}

void RfcConvertCharToBcd(char *str, unsigned len, unsigned *decimals,
                         void *bcd, unsigned bcd_len)
{
    (void)str;
    (void)len;
    (void)decimals;
    if (bcd && bcd_len) {
        memset(bcd, 0, bcd_len);
    }
}

RFC_HANDLE RfcAccept(char **argv)
{
    (void)argv;
    return RFC_HANDLE_NULL;
}

RFC_HANDLE RfcAcceptExt(char *cmdline)
{
    (void)cmdline;
    return RFC_HANDLE_NULL;
}

RFC_RC RfcWaitForRequest(RFC_HANDLE rfc, int timeout)
{
    (void)rfc;
    (void)timeout;
    return RFC_RETRY;
}

RFC_RC RfcGetNameEx(RFC_HANDLE rfc, RFC_FUNCTIONNAME function_name)
{
    (void)rfc;
    (void)function_name;
    return RFC_FAILURE;
}

RFC_RC RfcAbort(RFC_HANDLE rfc, const char *text)
{
    (void)rfc;
    (void)text;
    return RFC_OK;
}

RFC_RC RfcDispatch(RFC_HANDLE rfc)
{
    (void)rfc;
    return RFC_OK;
}

RFC_RC RfcInstallTransactionControl(RFC_ON_CHECK_TID tid_check,
                                    RFC_ON_COMMIT tid_commit,
                                    RFC_ON_ROLLBACK tid_rollback,
                                    RFC_ON_CONFIRM_TID tid_confirm)
{
    (void)tid_check;
    (void)tid_commit;
    (void)tid_rollback;
    (void)tid_confirm;
    return RFC_OK;
}

RFC_RC RfcInstallFunction(const char *name, RFC_ONCALL callback,
                          const char *docu)
{
    (void)name;
    (void)callback;
    (void)docu;
    return RFC_OK;
}

RFC_RC RfcCheckRegisterServer(const char *tpid, const char *gwhost,
                              const char *gwserver, int *ntotal, int *ninit,
                              int *nready, int *nbusy,
                              RFC_ERROR_INFO_EX *error_info)
{
    (void)tpid;
    (void)gwhost;
    (void)gwserver;
    if (error_info) {
        memset(error_info, 0, sizeof(*error_info));
    }
    if (ntotal) *ntotal = 1;
    if (ninit)  *ninit = 0;
    if (nready) *nready = 1;
    if (nbusy)  *nbusy = 0;
    return RFC_OK;
}

RFC_RC RfcCancelRegisterServer(const char *tpid, const char *gwhost,
                               const char *gwserver, int *ntotal,
                               int *ncancel, RFC_ERROR_INFO_EX *error_info)
{
    (void)tpid;
    (void)gwhost;
    (void)gwserver;
    if (error_info) {
        memset(error_info, 0, sizeof(*error_info));
    }
    if (ntotal)  *ntotal = 1;
    if (ncancel) *ncancel = 1;
    return RFC_OK;
}

/* ------------------------------------------------------------------ */
/* Internal tables                                                     */
/* ------------------------------------------------------------------ */

static struct mock_itab *mock_itab(ITAB_H handle)
{
    return (struct mock_itab *)handle;
}

ITAB_H ItCreate(const char *name, unsigned line_length,
                unsigned max_lines, unsigned mode)
{
    struct mock_itab *it;

    (void)name;
    (void)mode;
    if (line_length == 0) {
        return ITAB_NULL;
    }
    it = (struct mock_itab *)calloc(1, sizeof(*it));
    if (!it) {
        return ITAB_NULL;
    }
    it->line_length = line_length;
    it->capacity = max_lines ? max_lines : 16;
    it->data = (char *)calloc(it->capacity, line_length);
    if (!it->data) {
        free(it);
        return ITAB_NULL;
    }
    return (ITAB_H)it;
}

void ItDelete(ITAB_H handle)
{
    struct mock_itab *it = mock_itab(handle);
    if (!it) {
        return;
    }
    free(it->data);
    free(it);
}

void ItFree(ITAB_H handle)
{
    struct mock_itab *it = mock_itab(handle);
    if (!it) {
        return;
    }
    memset(it->data, 0, (size_t)it->capacity * it->line_length);
    it->rows = 0;
}

unsigned ItLeng(ITAB_H handle)
{
    struct mock_itab *it = mock_itab(handle);
    return it ? it->line_length : 0;
}

unsigned ItFill(ITAB_H handle)
{
    struct mock_itab *it = mock_itab(handle);
    return it ? it->rows : 0;
}

static char *mock_row(struct mock_itab *it, unsigned index)
{
    return it->data + (size_t)index * it->line_length;
}

static int mock_reserve(struct mock_itab *it, unsigned needed)
{
    unsigned new_capacity;
    char *new_data;

    if (needed <= it->capacity) {
        return 1;
    }
    new_capacity = it->capacity ? it->capacity * 2 : 16;
    while (new_capacity < needed) {
        new_capacity *= 2;
    }
    new_data = (char *)realloc(it->data,
                               (size_t)new_capacity * it->line_length);
    if (!new_data) {
        return 0;
    }
    memset(new_data + (size_t)it->capacity * it->line_length, 0,
           (size_t)(new_capacity - it->capacity) * it->line_length);
    it->data = new_data;
    it->capacity = new_capacity;
    return 1;
}

void *ItGetLine(ITAB_H handle, int index)
{
    struct mock_itab *it = mock_itab(handle);
    if (!it || index < 1 || (unsigned)index > it->rows) {
        return NULL;
    }
    return mock_row(it, (unsigned)index - 1);
}

void *ItAppLine(ITAB_H handle)
{
    struct mock_itab *it = mock_itab(handle);
    char *line;

    if (!it || !mock_reserve(it, it->rows + 1)) {
        return NULL;
    }
    line = mock_row(it, it->rows);
    memset(line, 0, it->line_length);
    it->rows++;
    return line;
}

void *ItInsLine(ITAB_H handle, int index)
{
    struct mock_itab *it = mock_itab(handle);
    unsigned pos;

    if (!it || index < 1 || (unsigned)index > it->rows + 1) {
        return NULL;
    }
    if (!mock_reserve(it, it->rows + 1)) {
        return NULL;
    }
    pos = (unsigned)index - 1;
    memmove(mock_row(it, pos + 1), mock_row(it, pos),
            (size_t)(it->rows - pos) * it->line_length);
    it->rows++;
    memset(mock_row(it, pos), 0, it->line_length);
    return mock_row(it, pos);
}

int ItPutLine(ITAB_H handle, int index, void *line)
{
    struct mock_itab *it = mock_itab(handle);
    if (!it || !line || index < 1 || (unsigned)index > it->rows) {
        return -1;
    }
    memcpy(mock_row(it, (unsigned)index - 1), line, it->line_length);
    return 0;
}

int ItDelLine(ITAB_H handle, int index)
{
    struct mock_itab *it = mock_itab(handle);
    unsigned pos;

    if (!it || index < 1 || (unsigned)index > it->rows) {
        return -1;
    }
    pos = (unsigned)index - 1;
    memmove(mock_row(it, pos), mock_row(it, pos + 1),
            (size_t)(it->rows - pos - 1) * it->line_length);
    it->rows--;
    memset(mock_row(it, it->rows), 0, it->line_length);
    return 0;
}

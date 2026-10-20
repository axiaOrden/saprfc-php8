/*
 * saprfc.h - TEST DOUBLE for the classic SAP RFC SDK 6.20/6.40
 *
 * THIS IS NOT THE SAP RFCSDK. It is a small, deliberately incomplete
 * re-implementation of the public API surface that SAPRFC 1.4.1 uses, so the
 * PHP extension can be compiled and exercised on machines without the real
 * (EOL, licensed) SAP RFCSDK installed.
 *
 * It must never be used to connect to a real SAP system. Build production
 * extensions against the genuine SDK headers and librfccm.so.
 *
 * The mock is identified by SAPRFC_MOCK_SDK; config.m4 prints a warning when
 * it detects this file.
 */

#ifndef SAPRFC_H
#define SAPRFC_H

#define SAPRFC_MOCK_SDK 1

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
#  define DLLIMPORT __declspec(dllimport)
#  define DLL_EXPORT __declspec(dllexport)
#  define DLL_CALL_BACK_FUNCTION __stdcall
#else
#  define DLLIMPORT
#  define DLL_EXPORT
#  define DLL_CALL_BACK_FUNCTION
#endif

/* Elementary RFC types. Values are local to the mock and must stay consistent
 * with the extension object built against these headers. */
#define TYPC     0
#define TYPNUM   1
#define TYPINT   2
#define TYPINT2  3
#define TYPINT1  4
#define TYPFLOAT 5
#define TYPBCD   6
#define TYPDATE  7
#define TYPTIME  8
#define TYPX     9
#define TYPP     10

/* Return codes (classic SDK names). */
#define RFC_OK                 0
#define RFC_FAILURE            1
#define RFC_EXCEPTION          2
#define RFC_SYS_EXCEPTION      3
#define RFC_CALL               4
#define RFC_INTERNAL_COM       5
#define RFC_CLOSED             6
#define RFC_RETRY              7
#define RFC_NO_TID             8
#define RFC_EXECUTED           9
#define RFC_SYNCHRONIZE       10
#define RFC_MEMORY_INSUFFICIENT 11
#define RFC_VERSION_MISMATCH  12
#define RFC_NOT_FOUND         13
#define RFC_CALL_NOT_SUPPORTED 14
#define RFC_NOT_OWNER         15
#define RFC_NOT_INITIALIZED   16
#define RFC_SYSTEM_CALLED     17

#define RFC_TID_LN 24

#define RFC_ITMODE_BYREFERENCE 0
#define RFC_ITMODE_BYVALUE     1

typedef unsigned char  RFC_CHAR;
typedef unsigned char  RFC_NUM;
typedef unsigned char  RFC_BYTE;
typedef unsigned char  RFC_BCD;
typedef unsigned char  RFC_INT1;
typedef short          RFC_INT2;
typedef int            RFC_INT;
typedef double         RFC_FLOAT;

typedef char  RFC_TID[RFC_TID_LN + 1];
typedef char  RFC_FUNCTIONNAME[31];

typedef void *RFC_HANDLE;
#define RFC_HANDLE_NULL ((RFC_HANDLE)0)

typedef long RFC_RC;
typedef unsigned long RFC_TYPEHANDLE;

typedef struct {
    char     *name;
    unsigned  type;
    unsigned  length;
    unsigned  decimals;
} RFC_TYPE_ELEMENT;

typedef struct {
    char     *name;
    unsigned  nlen;
    unsigned  type;
    unsigned  leng;
    void     *addr;
} RFC_PARAMETER;

typedef struct {
    char     *name;
    unsigned  nlen;
    unsigned  type;
    void     *ithandle;
    unsigned  itmode;
} RFC_TABLE;

typedef struct {
    RFC_CHAR dest[65];
    RFC_CHAR own_host[65];
    RFC_CHAR partner_host[65];
    RFC_CHAR systnr[3];
    RFC_CHAR sysid[9];
    RFC_CHAR client[4];
    RFC_CHAR user[13];
    RFC_CHAR language[3];
    RFC_CHAR trace;
    RFC_CHAR ISO_language[3];
    RFC_CHAR own_codepage[5];
    RFC_CHAR partner_codepage[5];
    RFC_CHAR rfc_role;
    RFC_CHAR own_type;
    RFC_CHAR own_rel[5];
    RFC_CHAR partner_type;
    RFC_CHAR partner_rel[5];
    RFC_CHAR kernel_rel[5];
    RFC_CHAR CPIC_convid[9];
    RFC_CHAR password_sate;
} RFC_ATTRIBUTES;

typedef struct {
    int  group;
    char key[128];
    char message[512];
} RFC_ERROR_INFO_EX;

typedef struct {
    char key[128];
    char status[128];
    char message[512];
    char intstat[128];
} RFC_ERROR_INFO;

typedef struct {
    void *(*allocate)(void *old_ptr, unsigned long new_size);
    void  (*deallocate)(void *ptr);
    void *(*reallocate)(void *ptr, unsigned long new_size);
} RFC_ENV;

typedef RFC_RC (DLL_CALL_BACK_FUNCTION *RFC_ONCALL)(RFC_HANDLE);
typedef int    (DLL_CALL_BACK_FUNCTION *RFC_ON_CHECK_TID)(RFC_TID);
typedef void   (DLL_CALL_BACK_FUNCTION *RFC_ON_COMMIT)(RFC_TID);
typedef void   (DLL_CALL_BACK_FUNCTION *RFC_ON_ROLLBACK)(RFC_TID);
typedef void   (DLL_CALL_BACK_FUNCTION *RFC_ON_CONFIRM_TID)(RFC_TID);

RFC_HANDLE RfcOpenEx(char *connect_param, RFC_ERROR_INFO_EX *error_info);
RFC_RC     RfcClose(RFC_HANDLE rfc);
RFC_RC     RfcSetCodePage(RFC_HANDLE rfc, const char *codepage);
RFC_RC     RfcSetSystemCodePage(const char *codepage);
RFC_RC     RfcGetAttributes(RFC_HANDLE rfc, RFC_ATTRIBUTES *attributes);
RFC_RC     RfcCreateTransID(RFC_HANDLE rfc, RFC_TID tid);
RFC_RC     RfcSetTrace(RFC_HANDLE rfc, int level);
RFC_RC     RfcAllowStartProgram(const char *program_list);
RFC_RC     RfcGetTicket(RFC_HANDLE rfc, RFC_CHAR *ticket);
RFC_RC     RfcInstallStructure(const char *name, RFC_TYPE_ELEMENT *def,
                               unsigned count, RFC_TYPEHANDLE *handle);
RFC_RC     RfcCallReceive(RFC_HANDLE rfc, const char *function_name,
                          RFC_PARAMETER *importing, RFC_PARAMETER *exporting,
                          RFC_TABLE *tables, char **exception);
RFC_RC     RfcIndirectCallEx(RFC_HANDLE rfc, const char *function_name,
                             RFC_PARAMETER *importing, RFC_TABLE *tables,
                             const char *tid);
RFC_RC     RfcConfirmTransID(RFC_HANDLE rfc, const char *tid);
RFC_RC     RfcGetData(RFC_HANDLE rfc, RFC_PARAMETER *importing, RFC_TABLE *tables);
RFC_RC     RfcSendData(RFC_HANDLE rfc, RFC_PARAMETER *exporting, RFC_TABLE *tables);
RFC_RC     RfcRaise(RFC_HANDLE rfc, const char *exception);
RFC_RC     RfcRaiseTables(RFC_HANDLE rfc, const char *exception, RFC_TABLE *tables);
void       RfcLastError(RFC_ERROR_INFO *error_info);
void       RfcGetAllLibVersions(char *buffer, unsigned length);
void       RfcEnvironment(RFC_ENV *env);

void RfcConvertBcdToChar(void *bcd, unsigned len, unsigned decimals,
                         char *buffer, unsigned buffer_len);
void RfcConvertCharToBcd(char *str, unsigned len, unsigned *decimals,
                         void *bcd, unsigned bcd_len);

RFC_HANDLE RfcAccept(char **argv);
RFC_HANDLE RfcAcceptExt(char *cmdline);
RFC_RC     RfcWaitForRequest(RFC_HANDLE rfc, int timeout);
RFC_RC     RfcGetNameEx(RFC_HANDLE rfc, RFC_FUNCTIONNAME function_name);
RFC_RC     RfcAbort(RFC_HANDLE rfc, const char *text);
RFC_RC     RfcDispatch(RFC_HANDLE rfc);
RFC_RC     RfcInstallTransactionControl(RFC_ON_CHECK_TID tid_check,
                                        RFC_ON_COMMIT tid_commit,
                                        RFC_ON_ROLLBACK tid_rollback,
                                        RFC_ON_CONFIRM_TID tid_confirm);
RFC_RC     RfcInstallFunction(const char *name, RFC_ONCALL callback,
                              const char *docu);
RFC_RC     RfcCheckRegisterServer(const char *tpid, const char *gwhost,
                                  const char *gwserver, int *ntotal, int *ninit,
                                  int *nready, int *nbusy,
                                  RFC_ERROR_INFO_EX *error_info);
RFC_RC     RfcCancelRegisterServer(const char *tpid, const char *gwhost,
                                   const char *gwserver, int *ntotal,
                                   int *ncancel, RFC_ERROR_INFO_EX *error_info);

#ifdef __cplusplus
}
#endif

#endif /* SAPRFC_H */

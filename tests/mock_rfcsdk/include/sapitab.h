/*
 * sapitab.h - TEST DOUBLE for the classic SAP RFC SDK internal table API.
 * Not the SAP RFCSDK; see saprfc.h in this directory.
 */

#ifndef SAPITAB_H
#define SAPITAB_H

#include "saprfc.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void *ITAB_H;
#define ITAB_NULL ((ITAB_H)0)

ITAB_H ItCreate(const char *name, unsigned line_length,
                unsigned max_lines, unsigned mode);
void   ItDelete(ITAB_H handle);
void   ItFree(ITAB_H handle);
unsigned ItLeng(ITAB_H handle);
unsigned ItFill(ITAB_H handle);
void  *ItGetLine(ITAB_H handle, int index);
void  *ItAppLine(ITAB_H handle);
void  *ItInsLine(ITAB_H handle, int index);
int    ItPutLine(ITAB_H handle, int index, void *line);
int    ItDelLine(ITAB_H handle, int index);

#ifdef __cplusplus
}
#endif

#endif /* SAPITAB_H */

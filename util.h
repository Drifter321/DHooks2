#ifndef _INCLUDE_UTIL_FUNCTIONS_H_
#define _INCLUDE_UTIL_FUNCTIONS_H_

#include "vhook.h"
#include "convention.h"

size_t GetParamOffset(HookParamsStruct *params, unsigned int index);
void * GetObjectAddr(HookParamType type, unsigned int flags, void **params, size_t offset);
size_t GetParamTypeSize(HookParamType type);
size_t GetParamsSize(DHooksCallback *dg);

DataType_t DynamicHooks_ConvertParamTypeFrom(HookParamType type);
DataType_t DynamicHooks_ConvertReturnTypeFrom(ReturnType type);
#endif

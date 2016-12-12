#include "util.h"

void * GetObjectAddr(HookParamType type, unsigned int flags, void **params, size_t offset)
{
#ifdef  WIN32
	if (type == HookParamType_Object)
		return (void *)((intptr_t)params + offset);
#elif POSIX
	if (type == HookParamType_Object && !(flags & PASSFLAG_ODTOR)) //Objects are passed by rrefrence if they contain destructors.
		return (void *)((intptr_t)params + offset);
#endif
	return *(void **)((intptr_t)params + offset);

}

size_t GetParamOffset(HookParamsStruct *paramStruct, unsigned int index)
{
	size_t offset = 0;
	for (unsigned int i = 0; i < index; i++)
	{
#ifndef WIN32
		if (paramStruct->dg->params.at(i).type == HookParamType_Object && (paramStruct->dg->params.at(i).flags & PASSFLAG_ODTOR)) //Passed by refrence
		{
			offset += sizeof(void *);
			continue;
		}
#endif
		offset += paramStruct->dg->params.at(i).size;
	}
	return offset;
}

size_t GetParamTypeSize(HookParamType type)
{
	return sizeof(void *);
}

size_t GetParamsSize(DHooksCallback *dg)//Get the full size, this is for creating the STACK.
{
	size_t res = 0;

	for (int i = dg->params.size() - 1; i >= 0; i--)
	{
		res += dg->params.at(i).size;
	}

	return res;
}

DataType_t DynamicHooks_ConvertParamTypeFrom(HookParamType type)
{
	switch (type)
	{
	case HookParamType_Int:
		return DATA_TYPE_INT;
	case HookParamType_Bool:
		return DATA_TYPE_BOOL;
	case HookParamType_Float:
		return DATA_TYPE_FLOAT;
	case HookParamType_StringPtr:
	case HookParamType_CharPtr:
	case HookParamType_VectorPtr:
	case HookParamType_CBaseEntity:
	case HookParamType_ObjectPtr:
	case HookParamType_Edict:
		return DATA_TYPE_POINTER;
	case HookParamType_Object:
		return DATA_TYPE_OBJECT;
	default:
		smutils->LogError(myself, "Unhandled parameter type %d!", type);
	}

	return DATA_TYPE_POINTER;
}

DataType_t DynamicHooks_ConvertReturnTypeFrom(ReturnType type)
{
	switch (type)
	{
	case ReturnType_Void:
		return DATA_TYPE_VOID;
	case ReturnType_Int:
		return DATA_TYPE_INT;
	case ReturnType_Bool:
		return DATA_TYPE_BOOL;
	case ReturnType_Float:
		return DATA_TYPE_FLOAT;
	case ReturnType_StringPtr:
	case ReturnType_CharPtr:
	case ReturnType_VectorPtr:
	case ReturnType_CBaseEntity:
	case ReturnType_Edict:
		return DATA_TYPE_POINTER;
	case ReturnType_Vector:
		return DATA_TYPE_OBJECT;
	default:
		smutils->LogError(myself, "Unhandled return type %d!", type);
	}

	return DATA_TYPE_VOID;
}

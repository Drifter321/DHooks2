#ifndef _INCLUDE_VFUNC_CALL_H_
#define _INCLUDE_VFUNC_CALL_H_

#include "vhook.h"
#include "extension.h"

#define PARAMINFO_SWITCH(passType) \
		paramInfo[i].flags = dg->params.at(i).flag; \
		paramInfo[i].size = dg->params.at(i).size; \
		paramInfo[i].type = passType;

#define VSTK_PARAM_SWITCH(paramType) \
		if(paramStruct->isChanged[i]) \
		{ \
			*(paramType *)vptr = (paramType)(paramStruct->newParams[i]); \
		} \
		else \
		{ \
			*(paramType *)vptr = (paramType)(paramStruct->orgParams[i]); \
		} \
		if(i + 1 != dg->params.size()) \
		{ \
			vptr += dg->params.at(i).size; \
		} \
		break;
#define VSTK_PARAM_SWITCH_FLOAT() \
		if(paramStruct->isChanged[i]) \
		{ \
			*(float *)vptr = *(float *)(paramStruct->newParams[i]); \
		} \
		else \
		{ \
			*(float *)vptr = *(float *)(paramStruct->orgParams[i]); \
		} \
		if(i + 1 != dg->params.size()) \
		{ \
			vptr += dg->params.at(i).size; \
		} \
		break;

template <class T>
T CallVFunction(DHooksCallback *dg, HookParamsStruct *paramStruct, void *iface)
{
	SourceMod::PassInfo *paramInfo = NULL;
	SourceMod::PassInfo returnInfo;

	if(dg->returnType != ReturnType_Void)
	{
		returnInfo.flags = dg->returnFlag;
		returnInfo.size = sizeof(T);
		if( dg->returnType != ReturnType_Vector)
		{
			returnInfo.type = PassType_Basic;
		}
		else
		{
			returnInfo.type = PassType_Object;
		}
	}

	ICallWrapper *pCall;

	size_t size = GetStackArgsSize(dg);

	unsigned char *vstk = (unsigned char *)malloc(sizeof(void *) + size);
	unsigned char *vptr = vstk;

	*(void **)vptr = iface;

	if(paramStruct)
	{
		vptr += sizeof(void *);
		paramInfo = (SourceMod::PassInfo *)malloc(sizeof(SourceMod::PassInfo) * dg->params.size());
		for(int i = 0; i < (int)dg->params.size(); i++)
		{
			switch(dg->params.at(i).type)
			{
				case HookParamType_Int:
					PARAMINFO_SWITCH(PassType_Basic);
					VSTK_PARAM_SWITCH(int);
				case HookParamType_Bool:
					PARAMINFO_SWITCH(PassType_Basic);
					VSTK_PARAM_SWITCH(cell_t);
				case HookParamType_Float:
					PARAMINFO_SWITCH(PassType_Float);
					VSTK_PARAM_SWITCH_FLOAT();
				case HookParamType_String:
					PARAMINFO_SWITCH(PassType_Object);
					VSTK_PARAM_SWITCH(int);
				case HookParamType_StringPtr:
					PARAMINFO_SWITCH(PassType_Basic);
					VSTK_PARAM_SWITCH(string_t *);
				case HookParamType_CharPtr:
					PARAMINFO_SWITCH(PassType_Basic);
					VSTK_PARAM_SWITCH(char *);
				case HookParamType_VectorPtr:
					PARAMINFO_SWITCH(PassType_Basic);
					VSTK_PARAM_SWITCH(Vector *);
				case HookParamType_CBaseEntity:
					PARAMINFO_SWITCH(PassType_Basic);
					VSTK_PARAM_SWITCH(CBaseEntity *);
				case HookParamType_Edict:
					PARAMINFO_SWITCH(PassType_Basic);
					VSTK_PARAM_SWITCH(edict_t *);
				default:
					PARAMINFO_SWITCH(PassType_Basic);
					VSTK_PARAM_SWITCH(void *);
			}
		}
	}

	T ret = 0;
	
	if(dg->returnType == ReturnType_Void)
	{
		pCall = g_pBinTools->CreateVCall(dg->offset, 0, 0, NULL, paramInfo, dg->params.size());
		pCall->Execute(vstk, NULL);
	}
	else
	{
		pCall = g_pBinTools->CreateVCall(dg->offset, 0, 0, &returnInfo, paramInfo, dg->params.size());
		pCall->Execute(vstk, &ret);
	}

	pCall->Destroy();
	free(vstk);

	if(paramInfo != NULL)
	{
		free(paramInfo);
	}

	return ret;
}
template <>
Vector CallVFunction<Vector>(DHooksCallback *dg, HookParamsStruct *paramStruct, void *iface)
{
	SourceMod::PassInfo *paramInfo = NULL;
	SourceMod::PassInfo returnInfo;

	if(dg->returnType != ReturnType_Void)
	{
		returnInfo.flags = dg->returnFlag;
		returnInfo.size = sizeof(Vector);
		returnInfo.type = PassType_Object;
	}

	ICallWrapper *pCall;

	size_t size = GetStackArgsSize(dg);

	unsigned char *vstk = (unsigned char *)malloc(sizeof(void *) + size);
	unsigned char *vptr = vstk;

	*(void **)vptr = iface;

	if(paramStruct)
	{
		vptr += sizeof(void *);
		paramInfo = (SourceMod::PassInfo *)malloc(sizeof(SourceMod::PassInfo) * dg->params.size());
		for(int i = 0; i < (int)dg->params.size(); i++)
		{
			switch(dg->params.at(i).type)
			{
				case HookParamType_Int:
					PARAMINFO_SWITCH(PassType_Basic);
					VSTK_PARAM_SWITCH(int);
				case HookParamType_Bool:
					PARAMINFO_SWITCH(PassType_Basic);
					VSTK_PARAM_SWITCH(cell_t);
				case HookParamType_Float:
					PARAMINFO_SWITCH(PassType_Float);
					VSTK_PARAM_SWITCH_FLOAT();
				case HookParamType_String:
					PARAMINFO_SWITCH(PassType_Object);
					VSTK_PARAM_SWITCH(int);
				case HookParamType_StringPtr:
					PARAMINFO_SWITCH(PassType_Basic);
					VSTK_PARAM_SWITCH(string_t *);
				case HookParamType_CharPtr:
					PARAMINFO_SWITCH(PassType_Basic);
					VSTK_PARAM_SWITCH(char *);
				case HookParamType_VectorPtr:
					PARAMINFO_SWITCH(PassType_Basic);
					VSTK_PARAM_SWITCH(Vector *);
				case HookParamType_CBaseEntity:
					PARAMINFO_SWITCH(PassType_Basic);
					VSTK_PARAM_SWITCH(CBaseEntity *);
				case HookParamType_Edict:
					PARAMINFO_SWITCH(PassType_Basic);
					VSTK_PARAM_SWITCH(edict_t *);
				default:
					PARAMINFO_SWITCH(PassType_Basic);
					VSTK_PARAM_SWITCH(void *);
			}
		}
	}

	Vector ret;

	pCall = g_pBinTools->CreateVCall(dg->offset, 0, 0, &returnInfo, paramInfo, dg->params.size());
	pCall->Execute(vstk, &ret);

	pCall->Destroy();
	free(vstk);

	if(paramInfo != NULL)
	{
		free(paramInfo);
	}

	return ret;
}
#ifdef __linux__
template <>
string_t CallVFunction<string_t>(DHooksCallback *dg, HookParamsStruct *paramStruct, void *iface)
{
	SourceMod::PassInfo *paramInfo = NULL;
	SourceMod::PassInfo returnInfo;

	if(dg->returnType != ReturnType_Void)
	{
		returnInfo.flags = dg->returnFlag;
		returnInfo.size = sizeof(string_t);
		returnInfo.type = PassType_Object;
	}

	ICallWrapper *pCall;

	size_t size = GetStackArgsSize(dg);

	unsigned char *vstk = (unsigned char *)malloc(sizeof(void *) + size);
	unsigned char *vptr = vstk;

	*(void **)vptr = iface;

	if(paramStruct)
	{
		vptr += sizeof(void *);
		paramInfo = (SourceMod::PassInfo *)malloc(sizeof(SourceMod::PassInfo) * dg->params.size());
		for(int i = 0; i < dg->params.size(); i++)
		{
			switch(dg->params.at(i).type)
			{
				case HookParamType_Int:
					PARAMINFO_SWITCH(PassType_Basic);
					VSTK_PARAM_SWITCH(int);
				case HookParamType_Bool:
					PARAMINFO_SWITCH(PassType_Basic);
					VSTK_PARAM_SWITCH(cell_t);
				case HookParamType_Float:
					PARAMINFO_SWITCH(PassType_Float);
					VSTK_PARAM_SWITCH_FLOAT();
				case HookParamType_String:
					PARAMINFO_SWITCH(PassType_Object);
					VSTK_PARAM_SWITCH(int);
				case HookParamType_StringPtr:
					PARAMINFO_SWITCH(PassType_Basic);
					VSTK_PARAM_SWITCH(string_t *);
				case HookParamType_CharPtr:
					PARAMINFO_SWITCH(PassType_Basic);
					VSTK_PARAM_SWITCH(char *);
				case HookParamType_VectorPtr:
					PARAMINFO_SWITCH(PassType_Basic);
					VSTK_PARAM_SWITCH(Vector *);
				case HookParamType_CBaseEntity:
					PARAMINFO_SWITCH(PassType_Basic);
					VSTK_PARAM_SWITCH(CBaseEntity *);
				case HookParamType_Edict:
					PARAMINFO_SWITCH(PassType_Basic);
					VSTK_PARAM_SWITCH(edict_t *);
				default:
					PARAMINFO_SWITCH(PassType_Basic);
					VSTK_PARAM_SWITCH(void *);
			}
		}
	}

	string_t ret;

	pCall = g_pBinTools->CreateVCall(dg->offset, 0, 0, &returnInfo, paramInfo, dg->params.size());
	pCall->Execute(vstk, &ret);

	pCall->Destroy();
	free(vstk);

	if(paramInfo != NULL)
	{
		free(paramInfo);
	}

	return ret;
}
#endif
#endif

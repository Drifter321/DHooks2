#include "vfunc_call.h"

#define PARAMINFO_SWITCH(passType) \
		paramInfo[i].flags = dg->params.Element(i).flag; \
		paramInfo[i].size = dg->params.Element(i).size; \
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
		if(i + 1 != dg->params.Count()) \
		{ \
			vptr += dg->params.Element(i).size; \
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
		if(i + 1 != dg->params.Count()) \
		{ \
			vptr += dg->params.Element(i).size; \
		} \
		break;

void *CallVFunction(DHooksCallback *dg, HookParamsStruct *paramStruct, void *iface)
{
	PassInfo *paramInfo = NULL;
	PassInfo returnInfo;

	if(dg->returnType != ReturnType_Void)
	{
		returnInfo.flags = dg->returnFlag;
		returnInfo.size = sizeof(void *);
		returnInfo.type = PassType_Basic;
	}

	ICallWrapper *pCall;

	size_t size = GetStackArgsSize(dg);

	unsigned char *vstk = (unsigned char *)malloc(sizeof(void *) + size);
	unsigned char *vptr = vstk;

	*(void **)vptr = iface;

	if(paramStruct)
	{
		vptr += sizeof(void *);
		paramInfo = (PassInfo *)malloc(sizeof(PassInfo) * dg->params.Count());
		for(int i = 0; i < dg->params.Count(); i++)
		{
			switch(dg->params.Element(i).type)
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

	void *ret = 0;
	if(dg->returnType == ReturnType_Void)
	{
		pCall = g_pBinTools->CreateVCall(dg->offset, 0, 0, NULL, paramInfo, dg->params.Count());
		pCall->Execute(vstk, NULL);
	}
	else
	{
		pCall = g_pBinTools->CreateVCall(dg->offset, 0, 0, &returnInfo, paramInfo, dg->params.Count());
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

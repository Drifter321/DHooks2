#include "natives.h"

CBaseEntity *UTIL_GetCBaseEntity(int num)
{
	edict_t *pEdict = gamehelpers->EdictOfIndex(num);
	if (!pEdict || pEdict->IsFree())
	{
		return NULL;
	}
	IServerUnknown *pUnk;
	if ((pUnk=pEdict->GetUnknown()) == NULL)
	{
		return NULL;
	}

	return pUnk->GetBaseEntity();
}

//native Handle:DHookCreate(offset, HookType:hooktype, ReturnType:returntype, ThisPointerType:thistype, DHookCallback:callback);
cell_t Native_CreateHook(IPluginContext *pContext, const cell_t *params)
{
	if(!pContext->GetFunctionById(params[5]))
	{
		return pContext->ThrowNativeError("Failed to retrieve function by id");
	}

	HookSetup *setup = new HookSetup((ReturnType)params[3], PASSFLAG_BYVAL,(HookType)params[2], (ThisPointerType)params[4], params[1], pContext->GetFunctionById(params[5]));

	Handle_t hndl = handlesys->CreateHandle(g_HookSetupHandle, setup, pContext->GetIdentity(), myself->GetIdentity(), NULL);

	if(!hndl)
	{
		delete setup;
		return pContext->ThrowNativeError("Failed to create hook");
	}

	return hndl;
}
//native bool:DHookAddParam(Handle:setup, HookParamType:type);
cell_t Native_AddParam(IPluginContext *pContext, const cell_t *params)
{
	if(params[1] == BAD_HANDLE)
	{
		return pContext->ThrowNativeError("Invalid Handle %i", BAD_HANDLE);
	}

	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());
	HookSetup *setup;

	if((err = handlesys->ReadHandle(params[1], g_HookSetupHandle, &sec, (void **)&setup)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", params[1], err);
	}
	ParamInfo info;

	if(params[0] == 3)
	{
		info.flag = params[3];
	}
	else
	{
		info.flag = PASSFLAG_BYVAL;
	}
	
	info.type = (HookParamType)params[2];
	info.size = GetParamTypeSize(info.type);
	info.pass_type = GetParamTypePassType(info.type);
	setup->params.AddToTail(info);

	return 1;
}
cell_t Native_HookEntity(IPluginContext *pContext, const cell_t *params)
{
	if(params[1] == BAD_HANDLE)
	{
		return pContext->ThrowNativeError("Invalid Handle %i", BAD_HANDLE);
	}

	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());
	HookSetup *setup;

	if((err = handlesys->ReadHandle(params[1], g_HookSetupHandle, &sec, (void **)&setup)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", params[1], err);
	}
	if(setup->hookType != HookType_Entity)
	{
		return pContext->ThrowNativeError("Hook is not an entity hook");
	}
	bool post = params[2] != 0;

	for(int i = g_pHooks.Count() -1; i >= 0; i--)
	{
		DHooksManager *manager = g_pHooks.Element(i);
		if(manager->callback->hookType == HookType_Entity && manager->callback->entity == params[3] && manager->callback->offset == setup->offset && manager->callback->post == post && manager->remove_callback == pContext->GetFunctionById(params[4]) && manager->callback->plugin_callback == setup->callback)
		{
			return manager->hookid;
		}
	}
	CBaseEntity *pEnt = UTIL_GetCBaseEntity(params[3]);

	if(!pEnt)
	{
		return pContext->ThrowNativeError("Invalid entity passed %i", params[2]);
	}

	DHooksManager *manager = new DHooksManager(setup, pEnt, pContext->GetFunctionById(params[4]), post);

	if(!manager->hookid)
	{
		delete manager;
		return 0;
	}

	g_pHooks.AddToTail(manager);

	return manager->hookid;
}
/*cell_t Native_HookGamerules(IPluginContext *pContext, const cell_t *params);
cell_t Native_RemoveHookID(IPluginContext *pContext, const cell_t *params);*/
cell_t Native_GetParam(IPluginContext *pContext, const cell_t *params)
{
	if(params[1] == BAD_HANDLE)
	{
		return pContext->ThrowNativeError("Invalid Handle %i", BAD_HANDLE);
	}

	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());
	HookParamsStruct *paramStruct;
	if((err = handlesys->ReadHandle(params[1], g_HookParamsHandle, &sec, (void **)&paramStruct)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", params[1], err);
	}

	if(params[2] < 0 || params[2] > paramStruct->dg->params.Count())
	{
		return pContext->ThrowNativeError("Invalid param number %i max params is %i", params[2], paramStruct->dg->params.Count());
	}
	if(params[2] == 0)
	{
		return paramStruct->dg->params.Count();
	}

	int index = params[2] - 1;

	if(paramStruct->orgParams[index] == NULL && (paramStruct->dg->params.Element(index).type == HookParamType_CBaseEntity || paramStruct->dg->params.Element(index).type == HookParamType_Edict))
	{
		return pContext->ThrowNativeError("Trying to get value for null pointer.");
	}

	switch(paramStruct->dg->params.Element(index).type)
	{
		case HookParamType_Int:
			return (int)paramStruct->orgParams[index];
		case HookParamType_Bool:
			return (cell_t)paramStruct->orgParams[index] != 0;
		case HookParamType_CBaseEntity:
			return gamehelpers->EntityToBCompatRef((CBaseEntity *)paramStruct->orgParams[index]);
		case HookParamType_Edict:
			return gamehelpers->IndexOfEdict((edict_t *)paramStruct->orgParams[index]);
		case HookParamType_Float:
			return sp_ftoc(*(float *)paramStruct->orgParams[index]);
		default:
			return pContext->ThrowNativeError("Invalid param type (%i) to get", paramStruct->dg->params.Element(index).type);
	}

	return 1;
}
cell_t Native_GetReturn(IPluginContext *pContext, const cell_t *params)
{
	if(params[1] == BAD_HANDLE)
	{
		return pContext->ThrowNativeError("Invalid Handle %i", BAD_HANDLE);
	}

	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());
	HookReturnStruct *returnStruct;

	if((err = handlesys->ReadHandle(params[1], g_HookReturnHandle, &sec, (void **)&returnStruct)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", params[1], err);
	}

	switch(returnStruct->type)
	{
		case ReturnType_Int:
			return *(int *)returnStruct->orgResult;
		case ReturnType_Bool:
			return *(cell_t *)returnStruct->orgResult;
		case ReturnType_CBaseEntity:
			return gamehelpers->EntityToBCompatRef((CBaseEntity *)returnStruct->orgResult);
		case ReturnType_Edict:
			return gamehelpers->IndexOfEdict((edict_t *)returnStruct->orgResult);
		case ReturnType_Float:
			return sp_ftoc(*(float *)returnStruct->orgResult);
		default:
			return pContext->ThrowNativeError("Invalid param type (%i) to get",returnStruct->type);
	}
	return 1;
}
cell_t Native_SetReturn(IPluginContext *pContext, const cell_t *params)
{
	if(params[1] == BAD_HANDLE)
	{
		return pContext->ThrowNativeError("Invalid Handle %i", BAD_HANDLE);
	}

	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());
	HookReturnStruct *returnStruct;

	if((err = handlesys->ReadHandle(params[1], g_HookReturnHandle, &sec, (void **)&returnStruct)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", params[1], err);
	}

	CBaseEntity *pEnt;
	edict_t *pEdict;
	float *fpVal;
	switch(returnStruct->type)
	{
		case ReturnType_Int:
			returnStruct->newResult = (void *)((int)params[2]);
			break;
		case ReturnType_Bool:
			returnStruct->newResult = (void *)(params[2] != 0);
			break;
		case ReturnType_CBaseEntity:
			pEnt = UTIL_GetCBaseEntity(params[2]);
			if(!pEnt)
			{
				return pContext->ThrowNativeError("Invalid entity index passed for return value");
			}
			returnStruct->newResult = pEnt;
			break;
		case ReturnType_Edict:
			pEdict = gamehelpers->EdictOfIndex(params[2]);
			if(!pEdict || pEdict->IsFree())
			{
				pContext->ThrowNativeError("Invalid entity index passed for return value");
			}
			returnStruct->newResult = pEdict;
			break;
		case ReturnType_Float:
			fpVal = new float;
			*fpVal = sp_ctof(params[2]);
			returnStruct->newResult = fpVal;
			break;
		default:
			return pContext->ThrowNativeError("Invalid param type (%i) to get",returnStruct->type);
	}
	returnStruct->isChanged = true;
	return 1;
}
/*cell_t Native_SetParam(IPluginContext *pContext, const cell_t *params);
cell_t Native_GetParamVector(IPluginContext *pContext, const cell_t *params);
cell_t Native_GetReturnVector(IPluginContext *pContext, const cell_t *params);
cell_t Native_SetReturnVector(IPluginContext *pContext, const cell_t *params);
cell_t Native_SetParamVector(IPluginContext *pContext, const cell_t *params);*/
cell_t Native_GetParamString(IPluginContext *pContext, const cell_t *params)
{
	if(params[1] == BAD_HANDLE)
	{
		return pContext->ThrowNativeError("Invalid Handle %i", BAD_HANDLE);
	}

	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());
	HookParamsStruct *paramStruct;
	if((err = handlesys->ReadHandle(params[1], g_HookParamsHandle, &sec, (void **)&paramStruct)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", params[1], err);
	}

	if(params[2] <= 0 || params[2] > paramStruct->dg->params.Count())
	{
		return pContext->ThrowNativeError("Invalid param number %i max params is %i", params[2], paramStruct->dg->params.Count());
	}
	int index = params[2] - 1;

	if(paramStruct->orgParams[index] == NULL)
	{
		return pContext->ThrowNativeError("Trying to get value for null pointer.");
	}

	if(paramStruct->dg->params.Element(index).type == HookParamType_CharPtr)
	{
		pContext->StringToLocal(params[3], params[4], (const char *)paramStruct->orgParams[index]);
	}

	return 1;
}
//cell_t Native_GetReturnString(IPluginContext *pContext, const cell_t *params);
//cell_t Native_SetReturnString(IPluginContext *pContext, const cell_t *params);

//native DHookSetParamString(Handle:hParams, num, String:value[])
cell_t Native_SetParamString(IPluginContext *pContext, const cell_t *params)
{
	if(params[1] == BAD_HANDLE)
	{
		return pContext->ThrowNativeError("Invalid Handle %i", BAD_HANDLE);
	}

	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());
	HookParamsStruct *paramStruct;
	if((err = handlesys->ReadHandle(params[1], g_HookParamsHandle, &sec, (void **)&paramStruct)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", params[1], err);
	}

	if(params[2] <= 0 || params[2] > paramStruct->dg->params.Count())
	{
		return pContext->ThrowNativeError("Invalid param number %i max params is %i", params[2], paramStruct->dg->params.Count());
	}

	int index = params[2] - 1;

	char *value;
	pContext->LocalToString(params[3], &value);

	if(paramStruct->dg->params.Element(index).type == HookParamType_CharPtr)
	{
		if(paramStruct->newParams[index] != NULL)
			delete (char *)paramStruct->newParams[index];

		paramStruct->newParams[index] = new char[strlen(value)+1];
		strcpy((char *)paramStruct->newParams[index], value);
		paramStruct->isChanged[index] = true;
	}
	return 1;
}

sp_nativeinfo_t g_Natives[] = 
{
	{"DHookCreate",							Native_CreateHook},
	{"DHookAddParam",						Native_AddParam},
	{"DHookEntity",							Native_HookEntity},
	/*{"DHookGamerules",						Native_HookGamerules},
	{"DHookRaw",							Native_HookRaw},
	{"DHookRemoveHookID",					Native_RemoveHookID},*/
	{"DHookGetParam",						Native_GetParam},
	{"DHookGetReturn",						Native_GetReturn},
	{"DHookSetReturn",						Native_SetReturn},
	/*{"DHookSetParam",						Native_SetParam},
	{"DHookGetParamVector",					Native_GetParamVector},
	{"DHookGetReturnVector",				Native_GetReturnVector},
	{"DHookSetReturnVector",				Native_SetReturnVector},
	{"DHookSetParamVector",					Native_SetParamVector},*/
	{"DHookGetParamString",					Native_GetParamString},
	//{"DHookGetReturnString",				Native_GetReturnString},
	//{"DHookSetReturnString",				Native_SetReturnString},
	{"DHookSetParamString",					Native_SetParamString},
	/*{"DHookAddEntityListener",				Native_AddEntityListener},
	{"DHookRemoveEntityListener",			Native_RemoveEntityListener},
	{"DHookGetParamObjectPtrVar",			Native_GetParamObjectPtrVar},
	{"DHookSetParamObjectPtrVar",			Native_SetParamObjectPtrVar},
	{"DHookGetParamObjectPtrVarVector",		Native_GetParamObjectPtrVarVector},
	{"DHookSetParamObjectPtrVarVector",		Native_SetParamObjectPtrVarVector},
	{"DHookIsNullParam",					Native_IsNullParam},*/
	{NULL,							NULL}
};

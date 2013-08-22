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
//native bool:DHookAddParam(Handle:setup, HookParamType:type); OLD
//native bool:DHookAddParam(Handle:setup, HookParamType:type, size=-1, flag=-1);
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

	//Add support for setting this
	info.flag = PASSFLAG_BYVAL;

	info.type = (HookParamType)params[2];

	if(params[0] == 3 && params[3] != -1)
	{
		info.size = params[3];
	}
	else
	{
		info.size = GetParamTypeSize(info.type);
	}
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
// native DHookGamerules(Handle:setup, bool:post, DHookRemovalCB:removalcb);
cell_t Native_HookGamerules(IPluginContext *pContext, const cell_t *params)
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
	if(setup->hookType != HookType_GameRules)
	{
		return pContext->ThrowNativeError("Hook is not a gamerules hook");
	}

	bool post = params[2] != 0;

	for(int i = g_pHooks.Count() -1; i >= 0; i--)
	{
		DHooksManager *manager = g_pHooks.Element(i);
		if(manager->callback->hookType == HookType_GameRules && manager->callback->offset == setup->offset && manager->callback->post == post && manager->remove_callback == pContext->GetFunctionById(params[3]) && manager->callback->plugin_callback == setup->callback)
		{
			return manager->hookid;
		}
	}

	void *rules = g_pSDKTools->GetGameRules();

	if(!rules)
	{
		return pContext->ThrowNativeError("Could not get game rules pointer");
	}

	DHooksManager *manager = new DHooksManager(setup, rules, pContext->GetFunctionById(params[3]), post);

	if(!manager->hookid)
	{
		delete manager;
		return 0;
	}

	g_pHooks.AddToTail(manager);

	return manager->hookid;
}
// DHookRaw(Handle:setup, bool:post, Address:addr, DHookRemovalCB:removalcb);
cell_t Native_HookRaw(IPluginContext *pContext, const cell_t *params)
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
	if(setup->hookType != HookType_Raw)
	{
		return pContext->ThrowNativeError("Hook is not a gamerules hook");
	}

	bool post = params[2] != 0;

	for(int i = g_pHooks.Count() -1; i >= 0; i--)
	{
		DHooksManager *manager = g_pHooks.Element(i);
		if(manager->callback->hookType == HookType_Raw && manager->callback->offset == setup->offset && manager->callback->post == post && manager->remove_callback == pContext->GetFunctionById(params[3]) && manager->callback->plugin_callback == setup->callback)
		{
			return manager->hookid;
		}
	}

	void *iface = (void *)(params[3]);

	if(!iface)
	{
		return pContext->ThrowNativeError("Invalid address passed");
	}

	DHooksManager *manager = new DHooksManager(setup, iface, pContext->GetFunctionById(params[3]), post);

	if(!manager->hookid)
	{
		delete manager;
		return 0;
	}

	g_pHooks.AddToTail(manager);

	return manager->hookid;
}
// native bool:DHookRemoveHookID(hookid);
cell_t Native_RemoveHookID(IPluginContext *pContext, const cell_t *params)
{
	for(int i = g_pHooks.Count() -1; i >= 0; i--)
	{
		DHooksManager *manager = g_pHooks.Element(i);
		if(manager->hookid == params[1] && manager->callback->plugin_callback->GetParentRuntime()->GetDefaultContext() == pContext)
		{
			delete manager;
			g_pHooks.Remove(i);
			return 1;
		}
	}
	return 0;
}
// native any:DHookGetParam(Handle:hParams, num);
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

// native DHookSetParam(Handle:hParams, param, any:value)
cell_t Native_SetParam(IPluginContext *pContext, const cell_t *params)
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

	switch(paramStruct->dg->params.Element(index).type)
	{
		case HookParamType_Int:
			paramStruct->newParams[index] = (void *)params[3];
			break;
		case HookParamType_Bool:
			paramStruct->newParams[index] = (void *)(params[3] ? 1 : 0);
			break;
		case HookParamType_CBaseEntity:
		{
			CBaseEntity *pEnt = UTIL_GetCBaseEntity(params[2]);
			if(!pEnt)
			{
				return pContext->ThrowNativeError("Invalid entity index passed for param value");
			}
			paramStruct->newParams[index] = pEnt;
			break;
		}
		case HookParamType_Edict:
		{
			edict_t *pEdict = gamehelpers->EdictOfIndex(params[2]);
			if(!pEdict || pEdict->IsFree())
			{
				pContext->ThrowNativeError("Invalid entity index passed for param value");
			}
			paramStruct->newParams[index] = pEdict;
			break;
		}
		case HookParamType_Float:
			paramStruct->newParams[index] = new float;
			*(float *)paramStruct->newParams[index] = sp_ctof(params[3]);
			break;
		default:
			return pContext->ThrowNativeError("Invalid param type (%i) to set", paramStruct->dg->params.Element(index).type);
	}

	paramStruct->isChanged[index] = true;
	return 1;
}

// native any:DHookGetReturn(Handle:hReturn);
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
			return *(bool *)returnStruct->orgResult? 1 : 0;
		case ReturnType_CBaseEntity:
			return gamehelpers->EntityToBCompatRef(*(CBaseEntity **)returnStruct->orgResult);
		case ReturnType_Edict:
			return gamehelpers->IndexOfEdict(*(edict_t **)returnStruct->orgResult);
		case ReturnType_Float:
			return sp_ftoc(*(float *)returnStruct->orgResult);
		default:
			return pContext->ThrowNativeError("Invalid param type (%i) to get", returnStruct->type);
	}
	return 1;
}
// native DHookSetReturn(Handle:hReturn, any:value)
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

	switch(returnStruct->type)
	{
		case ReturnType_Int:
			*(int *)returnStruct->newResult = params[2];
			break;
		case ReturnType_Bool:
			*(bool *)returnStruct->newResult = params[2] != 0;
			break;
		case ReturnType_CBaseEntity:
		{
			CBaseEntity *pEnt = UTIL_GetCBaseEntity(params[2]);
			if(!pEnt)
			{
				return pContext->ThrowNativeError("Invalid entity index passed for return value");
			}
			*(CBaseEntity **)returnStruct->newResult = pEnt;
			break;
		}
		case ReturnType_Edict:
		{
			edict_t *pEdict = gamehelpers->EdictOfIndex(params[2]);
			if(!pEdict || pEdict->IsFree())
			{
				pContext->ThrowNativeError("Invalid entity index passed for return value");
			}
			*(edict_t **)returnStruct->newResult = pEdict;
			break;
		}
		case ReturnType_Float:
			*(float *)returnStruct->newResult = sp_ctof(params[2]);
			break;
		default:
			return pContext->ThrowNativeError("Invalid param type (%i) to get",returnStruct->type);
	}
	returnStruct->isChanged = true;
	return 1;
}
/*cell_t Native_GetParamVector(IPluginContext *pContext, const cell_t *params);
cell_t Native_GetReturnVector(IPluginContext *pContext, const cell_t *params);
cell_t Native_SetReturnVector(IPluginContext *pContext, const cell_t *params);
cell_t Native_SetParamVector(IPluginContext *pContext, const cell_t *params);*/

// native DHookGetParamString(Handle:hParams, num, String:buffer[], size)
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
cell_t Native_GetReturnString(IPluginContext *pContext, const cell_t *params)
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
		case ReturnType_String:
			pContext->StringToLocal(params[2], params[3], (*(string_t *)returnStruct->orgResult == NULL_STRING) ? "" : STRING(*(string_t *)returnStruct->orgResult));
			return 1;
		case ReturnType_StringPtr:
			pContext->StringToLocal(params[2], params[3], (*(string_t **)returnStruct->orgResult == NULL) ? "" : (*(string_t **)returnStruct->orgResult)->ToCStr());
			return 1;
		case ReturnType_CharPtr:
			pContext->StringToLocal(params[2], params[3], (*(char **)returnStruct->orgResult == NULL) ? "" : *(const char **)returnStruct->orgResult);
			return 1;
		default:
			return pContext->ThrowNativeError("Invalid param type to get. Param is not a string.");
	}
}
cell_t Native_SetReturnString(IPluginContext *pContext, const cell_t *params)
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

	char *value;
	pContext->LocalToString(params[2], &value);

	switch(returnStruct->type)
	{
		case ReturnType_String:
			*(string_t *)returnStruct->newResult = MAKE_STRING(value);
			returnStruct->isChanged = true;
			return 1;
		case ReturnType_StringPtr:
			*(string_t **)returnStruct->newResult = new string_t(MAKE_STRING(value));
			returnStruct->isChanged = true;
			return 1;
		case ReturnType_CharPtr:
			*(char **)returnStruct->newResult = new char[strlen(value)+1];
			strcpy(*(char **)returnStruct->newResult, value);
			returnStruct->isChanged = true;
			return 1;
		default:
			return pContext->ThrowNativeError("Invalid param type to get. Param is not a string.");
	}
}

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
		if((char *)paramStruct->newParams[index] != NULL && paramStruct->isChanged[index])
			delete (char *)paramStruct->newParams[index];

		paramStruct->newParams[index] = new char[strlen(value)+1];
		strcpy((char *)paramStruct->newParams[index], value);
		paramStruct->isChanged[index] = true;
	}
	return 1;
}

//native DHookAddEntityListener(ListenType:type, ListenCB:callback);
cell_t Native_AddEntityListener(IPluginContext *pContext, const cell_t *params)
{
	if(g_pEntityListener)
	{
		return g_pEntityListener->AddPluginEntityListener((ListenType)params[1], pContext->GetFunctionById(params[2]));;
	}
	return pContext->ThrowNativeError("Failed to get g_pEntityListener");
}

//native bool:DHookRemoveEntityListener(ListenType:type, ListenCB:callback);
cell_t Native_RemoveEntityListener(IPluginContext *pContext, const cell_t *params)
{
	if(g_pEntityListener)
	{
		return g_pEntityListener->RemovePluginEntityListener((ListenType)params[1], pContext->GetFunctionById(params[2]));;
	}
	return pContext->ThrowNativeError("Failed to get g_pEntityListener");
}

//native bool:DHookIsNullParam(Handle:hParams, num);
cell_t Native_IsNullParam(IPluginContext *pContext, const cell_t *params)
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

	HookParamType type = paramStruct->dg->params.Element(index).type;

	//Check that the type is ptr
	if(type == HookParamType_StringPtr || type == HookParamType_CharPtr || type == HookParamType_VectorPtr || type == HookParamType_CBaseEntity || type == HookParamType_ObjectPtr || type == HookParamType_Edict || type == HookParamType_Unknown)
		return (paramStruct->orgParams[index] == NULL);
	else
		return pContext->ThrowNativeError("Param is not a pointer!");
}

sp_nativeinfo_t g_Natives[] = 
{
	{"DHookCreate",							Native_CreateHook},
	{"DHookAddParam",						Native_AddParam},
	{"DHookEntity",							Native_HookEntity},
	{"DHookGamerules",						Native_HookGamerules},
	{"DHookRaw",							Native_HookRaw},
	{"DHookRemoveHookID",					Native_RemoveHookID},
	{"DHookGetParam",						Native_GetParam},
	{"DHookGetReturn",						Native_GetReturn},
	{"DHookSetReturn",						Native_SetReturn},
	{"DHookSetParam",						Native_SetParam},
	/*{"DHookGetParamVector",					Native_GetParamVector},
	{"DHookGetReturnVector",				Native_GetReturnVector},
	{"DHookSetReturnVector",				Native_SetReturnVector},
	{"DHookSetParamVector",					Native_SetParamVector},*/
	{"DHookGetParamString",					Native_GetParamString},
	{"DHookGetReturnString",				Native_GetReturnString},
	{"DHookSetReturnString",				Native_SetReturnString},
	{"DHookSetParamString",					Native_SetParamString},
	{"DHookAddEntityListener",				Native_AddEntityListener},
	{"DHookRemoveEntityListener",			Native_RemoveEntityListener},
	/*{"DHookGetParamObjectPtrVar",			Native_GetParamObjectPtrVar},
	{"DHookSetParamObjectPtrVar",			Native_SetParamObjectPtrVar},
	{"DHookGetParamObjectPtrVarVector",		Native_GetParamObjectPtrVarVector},
	{"DHookSetParamObjectPtrVarVector",		Native_SetParamObjectPtrVarVector},*/
	{"DHookIsNullParam",					Native_IsNullParam},
	{NULL,							NULL}
};

#include "natives.h"

bool GetHandleIfValidOrError(HandleType_t type, void **object, IPluginContext *pContext, cell_t param)
{
	if(param == BAD_HANDLE)
	{
		return pContext->ThrowNativeError("Invalid Handle %i", BAD_HANDLE) != 0;
	}

	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	if((err = handlesys->ReadHandle(param, type, &sec, object)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", param, err) != 0;
	}
	return true;
}

intptr_t GetObjectAddr(HookParamType type, void **params, int index)
{
#ifdef  WIN32
	if(type == HookParamType_Object)
		return (intptr_t)&params[index];
	else if(type == HookParamType_ObjectPtr)
		return (intptr_t)params[index];

	return 0;
#else
	return (intptr_t)params[index];
#endif
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
//native bool:DHookAddParam(Handle:setup, HookParamType:type, size=-1, DHookPassFlag:flag=DHookPass_ByVal);
cell_t Native_AddParam(IPluginContext *pContext, const cell_t *params)
{
	HookSetup *setup;

	if(!GetHandleIfValidOrError(g_HookSetupHandle, (void **)&setup, pContext, params[1]))
	{
		return 0;
	}

	ParamInfo info;

	info.type = (HookParamType)params[2];

	if(params[0] >= 4)
	{
		info.flag = params[4];
	}
	else
	{
		info.flag = PASSFLAG_BYVAL;
	}

	if(params[0] >= 3 && params[3] != -1)
	{
		info.size = params[3];
	}
	else if(info.type == HookParamType_Object)
	{
		return pContext->ThrowNativeError("Object param being set with no size");
	}
	else
	{
		info.size = GetParamTypeSize(info.type);
	}

	info.pass_type = GetParamTypePassType(info.type);
	setup->params.push_back(info);

	return 1;
}
// native DHookEntity(Handle:setup, bool:post, entity, DHookRemovalCB:removalcb);
cell_t Native_HookEntity(IPluginContext *pContext, const cell_t *params)
{
	HookSetup *setup;

	if(!GetHandleIfValidOrError(g_HookSetupHandle, (void **)&setup, pContext, params[1]))
	{
		return 0;
	}

	if(setup->hookType != HookType_Entity)
	{
		return pContext->ThrowNativeError("Hook is not an entity hook");
	}
	bool post = params[2] != 0;

	for(int i = g_pHooks.size() -1; i >= 0; i--)
	{
		DHooksManager *manager = g_pHooks.at(i);
		if(manager->callback->hookType == HookType_Entity && manager->callback->entity == gamehelpers->ReferenceToBCompatRef(params[3]) && manager->callback->offset == setup->offset && manager->callback->post == post && manager->remove_callback == pContext->GetFunctionById(params[4]) && manager->callback->plugin_callback == setup->callback)
		{
			return manager->hookid;
		}
	}
	CBaseEntity *pEnt = gamehelpers->ReferenceToEntity(params[3]);

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

	g_pHooks.push_back(manager);

	return manager->hookid;
}
// native DHookGamerules(Handle:setup, bool:post, DHookRemovalCB:removalcb);
cell_t Native_HookGamerules(IPluginContext *pContext, const cell_t *params)
{
	HookSetup *setup;

	if(!GetHandleIfValidOrError(g_HookSetupHandle, (void **)&setup, pContext, params[1]))
	{
		return 0;
	}

	if(setup->hookType != HookType_GameRules)
	{
		return pContext->ThrowNativeError("Hook is not a gamerules hook");
	}

	bool post = params[2] != 0;

	for(int i = g_pHooks.size() -1; i >= 0; i--)
	{
		DHooksManager *manager = g_pHooks.at(i);
		if(manager->callback->hookType == HookType_GameRules && manager->callback->offset == setup->offset && manager->callback->post == post && manager->remove_callback == pContext->GetFunctionById(params[3]) && manager->callback->plugin_callback == setup->callback)
		{
			return manager->hookid;
		}
	}

	void *rules = g_pSDKTools->GetGameRules();

	if(!rules)
	{
		return pContext->ThrowNativeError("Could not get gamerules pointer");
	}

	DHooksManager *manager = new DHooksManager(setup, rules, pContext->GetFunctionById(params[3]), post);

	if(!manager->hookid)
	{
		delete manager;
		return 0;
	}

	g_pHooks.push_back(manager);

	return manager->hookid;
}
// DHookRaw(Handle:setup, bool:post, Address:addr, DHookRemovalCB:removalcb);
cell_t Native_HookRaw(IPluginContext *pContext, const cell_t *params)
{
	HookSetup *setup;

	if(!GetHandleIfValidOrError(g_HookSetupHandle, (void **)&setup, pContext, params[1]))
	{
		return 0;
	}

	if(setup->hookType != HookType_Raw)
	{
		return pContext->ThrowNativeError("Hook is not a raw hook");
	}

	bool post = params[2] != 0;

	for(int i = g_pHooks.size() -1; i >= 0; i--)
	{
		DHooksManager *manager = g_pHooks.at(i);
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

	g_pHooks.push_back(manager);

	return manager->hookid;
}
// native bool:DHookRemoveHookID(hookid);
cell_t Native_RemoveHookID(IPluginContext *pContext, const cell_t *params)
{
	for(int i = g_pHooks.size() -1; i >= 0; i--)
	{
		DHooksManager *manager = g_pHooks.at(i);
		if(manager->hookid == params[1] && manager->callback->plugin_callback->GetParentRuntime()->GetDefaultContext() == pContext)
		{
			delete manager;
			g_pHooks.erase(g_pHooks.iterAt(i));
			return 1;
		}
	}
	return 0;
}
// native any:DHookGetParam(Handle:hParams, num);
cell_t Native_GetParam(IPluginContext *pContext, const cell_t *params)
{
	HookParamsStruct *paramStruct;

	if(!GetHandleIfValidOrError(g_HookParamsHandle, (void **)&paramStruct, pContext, params[1]))
	{
		return 0;
	}

	if(params[2] < 0 || params[2] > (int)paramStruct->dg->params.size())
	{
		return pContext->ThrowNativeError("Invalid param number %i max params is %i", params[2], paramStruct->dg->params.size());
	}
	if(params[2] == 0)
	{
		return paramStruct->dg->params.size();
	}

	int index = params[2] - 1;

	if(paramStruct->orgParams[index] == NULL && (paramStruct->dg->params.at(index).type == HookParamType_CBaseEntity || paramStruct->dg->params.at(index).type == HookParamType_Edict))
	{
		return pContext->ThrowNativeError("Trying to get value for null pointer.");
	}

	switch(paramStruct->dg->params.at(index).type)
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
			return pContext->ThrowNativeError("Invalid param type (%i) to get", paramStruct->dg->params.at(index).type);
	}

	return 1;
}

// native DHookSetParam(Handle:hParams, param, any:value)
cell_t Native_SetParam(IPluginContext *pContext, const cell_t *params)
{
	HookParamsStruct *paramStruct;

	if(!GetHandleIfValidOrError(g_HookParamsHandle, (void **)&paramStruct, pContext, params[1]))
	{
		return 0;
	}

	if(params[2] <= 0 || params[2] > (int)paramStruct->dg->params.size())
	{
		return pContext->ThrowNativeError("Invalid param number %i max params is %i", params[2], paramStruct->dg->params.size());
	}

	int index = params[2] - 1;

	switch(paramStruct->dg->params.at(index).type)
	{
		case HookParamType_Int:
			paramStruct->newParams[index] = (void *)params[3];
			break;
		case HookParamType_Bool:
			paramStruct->newParams[index] = (void *)(params[3] ? 1 : 0);
			break;
		case HookParamType_CBaseEntity:
		{
			CBaseEntity *pEnt = gamehelpers->ReferenceToEntity(params[2]);
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
			return pContext->ThrowNativeError("Invalid param type (%i) to set", paramStruct->dg->params.at(index).type);
	}

	paramStruct->isChanged[index] = true;
	return 1;
}

// native any:DHookGetReturn(Handle:hReturn);
cell_t Native_GetReturn(IPluginContext *pContext, const cell_t *params)
{
	HookReturnStruct *returnStruct;

	if(!GetHandleIfValidOrError(g_HookReturnHandle, (void **)&returnStruct, pContext, params[1]))
	{
		return 0;
	}

	switch(returnStruct->type)
	{
		case ReturnType_Int:
			return *(int *)returnStruct->orgResult;
		case ReturnType_Bool:
			return *(bool *)returnStruct->orgResult? 1 : 0;
		case ReturnType_CBaseEntity:
			return gamehelpers->EntityToBCompatRef((CBaseEntity *)returnStruct->orgResult);
		case ReturnType_Edict:
			return gamehelpers->IndexOfEdict((edict_t *)returnStruct->orgResult);
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
	HookReturnStruct *returnStruct;

	if(!GetHandleIfValidOrError(g_HookReturnHandle, (void **)&returnStruct, pContext, params[1]))
	{
		return 0;
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
			CBaseEntity *pEnt = gamehelpers->ReferenceToEntity(params[2]);
			if(!pEnt)
			{
				return pContext->ThrowNativeError("Invalid entity index passed for return value");
			}
			returnStruct->newResult = pEnt;
			break;
		}
		case ReturnType_Edict:
		{
			edict_t *pEdict = gamehelpers->EdictOfIndex(params[2]);
			if(!pEdict || pEdict->IsFree())
			{
				pContext->ThrowNativeError("Invalid entity index passed for return value");
			}
			returnStruct->newResult = pEdict;
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
// native DHookGetParamVector(Handle:hParams, num, Float:vec[3])
cell_t Native_GetParamVector(IPluginContext *pContext, const cell_t *params)
{
	HookParamsStruct *paramStruct;

	if(!GetHandleIfValidOrError(g_HookParamsHandle, (void **)&paramStruct, pContext, params[1]))
	{
		return 0;
	}

	if(params[2] <= 0 || params[2] > (int)paramStruct->dg->params.size())
	{
		return pContext->ThrowNativeError("Invalid param number %i max params is %i", params[2], paramStruct->dg->params.size());
	}

	int index = params[2] - 1;

	switch(paramStruct->dg->params.at(index).type)
	{
		case HookParamType_VectorPtr:
		{
			cell_t *buffer;
			pContext->LocalToPhysAddr(params[3], &buffer);
			buffer[0] = sp_ftoc(((Vector *)paramStruct->orgParams[index])->x);
			buffer[1] = sp_ftoc(((Vector *)paramStruct->orgParams[index])->y);
			buffer[2] = sp_ftoc(((Vector *)paramStruct->orgParams[index])->z);
			return 1;
		}	
	}

	return pContext->ThrowNativeError("Invalid param type to get. Param is not a vector.");
}
// native DHookSetParamVector(Handle:hParams, num, Float:vec[3])
cell_t Native_SetParamVector(IPluginContext *pContext, const cell_t *params)
{
	HookParamsStruct *paramStruct;

	if(!GetHandleIfValidOrError(g_HookParamsHandle, (void **)&paramStruct, pContext, params[1]))
	{
		return 0;
	}

	if(params[2] <= 0 || params[2] > (int)paramStruct->dg->params.size())
	{
		return pContext->ThrowNativeError("Invalid param number %i max params is %i", params[2], paramStruct->dg->params.size());
	}

	int index = params[2] - 1;

	switch(paramStruct->dg->params.at(index).type)
	{
		case HookParamType_VectorPtr:
		{
			if(paramStruct->newParams[index] != NULL)
				delete (Vector *)paramStruct->newParams[index];

			cell_t *buffer;
			pContext->LocalToPhysAddr(params[3], &buffer);

			paramStruct->newParams[index] = new Vector(sp_ctof(buffer[0]), sp_ctof(buffer[1]), sp_ctof(buffer[2]));
			paramStruct->isChanged[index] = true;
			return 1;
		}
	}
	return pContext->ThrowNativeError("Invalid param type to set. Param is not a vector.");
}

// native DHookGetParamString(Handle:hParams, num, String:buffer[], size)
cell_t Native_GetParamString(IPluginContext *pContext, const cell_t *params)
{
	HookParamsStruct *paramStruct;

	if(!GetHandleIfValidOrError(g_HookParamsHandle, (void **)&paramStruct, pContext, params[1]))
	{
		return 0;
	}

	if(params[2] <= 0 || params[2] > (int)paramStruct->dg->params.size())
	{
		return pContext->ThrowNativeError("Invalid param number %i max params is %i", params[2], paramStruct->dg->params.size());
	}
	int index = params[2] - 1;

	if(paramStruct->orgParams[index] == NULL)
	{
		return pContext->ThrowNativeError("Trying to get value for null pointer.");
	}

	if(paramStruct->dg->params.at(index).type == HookParamType_CharPtr)
	{
		pContext->StringToLocal(params[3], params[4], (const char *)paramStruct->orgParams[index]);
	}

	return 1;
}

// native DHookGetReturnString(Handle:hReturn, String:buffer[], size)
cell_t Native_GetReturnString(IPluginContext *pContext, const cell_t *params)
{
	HookReturnStruct *returnStruct;

	if(!GetHandleIfValidOrError(g_HookReturnHandle, (void **)&returnStruct, pContext, params[1]))
	{
		return 0;
	}

	switch(returnStruct->type)
	{
		case ReturnType_String:
			pContext->StringToLocal(params[2], params[3], (*(string_t *)returnStruct->orgResult == NULL_STRING) ? "" : STRING(*(string_t *)returnStruct->orgResult));
			return 1;
		case ReturnType_StringPtr:
			pContext->StringToLocal(params[2], params[3], ((string_t *)returnStruct->orgResult == NULL) ? "" : ((string_t *)returnStruct->orgResult)->ToCStr());
			return 1;
		case ReturnType_CharPtr:
			pContext->StringToLocal(params[2], params[3], ((char *)returnStruct->orgResult == NULL) ? "" : (const char *)returnStruct->orgResult);
			return 1;
		default:
			return pContext->ThrowNativeError("Invalid param type to get. Param is not a string.");
	}
}

//native DHookSetReturnString(Handle:hReturn, String:value[])
cell_t Native_SetReturnString(IPluginContext *pContext, const cell_t *params)
{
	HookReturnStruct *returnStruct;

	if(!GetHandleIfValidOrError(g_HookReturnHandle, (void **)&returnStruct, pContext, params[1]))
	{
		return 0;
	}

	char *value;
	pContext->LocalToString(params[2], &value);

	switch(returnStruct->type)
	{
		case ReturnType_CharPtr:
			returnStruct->newResult = new char[strlen(value)+1];
			strcpy((char *)returnStruct->newResult, value);
			returnStruct->isChanged = true;
			return 1;
		default:
			return pContext->ThrowNativeError("Invalid param type to get. Param is not a char pointer.");
	}
}

//native DHookSetParamString(Handle:hParams, num, String:value[])
cell_t Native_SetParamString(IPluginContext *pContext, const cell_t *params)
{
	HookParamsStruct *paramStruct;

	if(!GetHandleIfValidOrError(g_HookParamsHandle, (void **)&paramStruct, pContext, params[1]))
	{
		return 0;
	}

	if(params[2] <= 0 || params[2] > (int)paramStruct->dg->params.size())
	{
		return pContext->ThrowNativeError("Invalid param number %i max params is %i", params[2], paramStruct->dg->params.size());
	}

	int index = params[2] - 1;

	char *value;
	pContext->LocalToString(params[3], &value);

	if(paramStruct->dg->params.at(index).type == HookParamType_CharPtr)
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

//native any:DHookGetParamObjectPtrVar(Handle:hParams, num, offset, ObjectValueType:type);
cell_t Native_GetParamObjectPtrVar(IPluginContext *pContext, const cell_t *params)
{
	HookParamsStruct *paramStruct;

	if(!GetHandleIfValidOrError(g_HookParamsHandle, (void **)&paramStruct, pContext, params[1]))
	{
		return 0;
	}

	if(params[2] <= 0 || params[2] > (int)paramStruct->dg->params.size())
	{
		return pContext->ThrowNativeError("Invalid param number %i max params is %i", params[2], paramStruct->dg->params.size());
	}

	int index = params[2] - 1;

	if(paramStruct->dg->params.at(index).type != HookParamType_ObjectPtr && paramStruct->dg->params.at(index).type != HookParamType_Object)
	{
		return pContext->ThrowNativeError("Invalid object value type %i", paramStruct->dg->params.at(index).type);
	}
	
	intptr_t addr = GetObjectAddr(paramStruct->dg->params.at(index).type, paramStruct->orgParams, index);

	switch((ObjectValueType)params[4])
	{
		case ObjectValueType_Int:
		{
			return *(int *)(addr + params[3]);
		}
		case ObjectValueType_Bool:
			return *(bool *)(addr + params[3]) ? 1 : 0;
		case ObjectValueType_Ehandle:
		{
			edict_t *pEdict = gamehelpers->GetHandleEntity(*(CBaseHandle *)(addr + params[3]));

			if(!pEdict)
			{
				return -1;
			}

			return gamehelpers->IndexOfEdict(pEdict);
		}
		case ObjectValueType_Float:
			return sp_ftoc(*(float *)(addr + params[3]));
		case ObjectValueType_CBaseEntityPtr:
			return gamehelpers->EntityToBCompatRef((CBaseEntity *)(addr + params[3]));
		case ObjectValueType_IntPtr:
		{
			int *ptr = *(int **)(addr + params[3]);
			return *ptr;
		}
		case ObjectValueType_BoolPtr:
		{
			bool *ptr = *(bool **)(addr + params[3]);
			return *ptr ? 1 : 0;
		}
		case ObjectValueType_EhandlePtr://Im pretty sure this is never gonna happen
		{
			edict_t *pEdict = gamehelpers->GetHandleEntity(**(CBaseHandle **)(addr + params[3]));

			if(!pEdict)
			{
				return -1;
			}

			return gamehelpers->IndexOfEdict(pEdict);
		}
		case ObjectValueType_FloatPtr:
		{
			float *ptr = *(float **)(addr + params[3]);
			return sp_ftoc(*ptr);
		}
		default:
			return pContext->ThrowNativeError("Invalid Object value type");
	}
}

//native DHookSetParamObjectPtrVar(Handle:hParams, num, offset, ObjectValueType:type, value)
cell_t Native_SetParamObjectPtrVar(IPluginContext *pContext, const cell_t *params)
{
	HookParamsStruct *paramStruct;

	if(!GetHandleIfValidOrError(g_HookParamsHandle, (void **)&paramStruct, pContext, params[1]))
	{
		return 0;
	}

	if(params[2] <= 0 || params[2] > (int)paramStruct->dg->params.size())
	{
		return pContext->ThrowNativeError("Invalid param number %i max params is %i", params[2], paramStruct->dg->params.size());
	}

	int index = params[2] - 1;

	if(paramStruct->dg->params.at(index).type != HookParamType_ObjectPtr)
	{
		return pContext->ThrowNativeError("Invalid object value type %i", paramStruct->dg->params.at(index).type);
	}
	
	intptr_t addr = GetObjectAddr(paramStruct->dg->params.at(index).type, paramStruct->orgParams, index);

	switch((ObjectValueType)params[4])
	{
		case ObjectValueType_Int:
			*(int *)(addr + params[3]) = params[5];
			break;
		case ObjectValueType_Bool:
			*(bool *)(addr + params[3]) = params[5] != 0;
			break;
		case ObjectValueType_Ehandle:
		{
			edict_t *pEdict = gamehelpers->EdictOfIndex(params[5]);

			if(!pEdict || pEdict->IsFree())
			{
				return pContext->ThrowNativeError("Invalid edict passed");
			}
			gamehelpers->SetHandleEntity(*(CBaseHandle *)(addr + params[3]), pEdict);

			break;
		}
		case ObjectValueType_Float:
			*(float *)(addr + params[3]) = sp_ctof(params[5]);
			break;
		case ObjectValueType_CBaseEntityPtr:
		{
			CBaseEntity *pEnt = gamehelpers->ReferenceToEntity(params[5]);

			if(!pEnt)
			{
				return pContext->ThrowNativeError("Invalid entity passed");
			}

			*(CBaseEntity **)(addr + params[3]) = pEnt;
			break;
		}
		case ObjectValueType_IntPtr:
		{
			int *ptr = *(int **)(addr + params[3]);
			*ptr = params[5];
			break;
		}
		case ObjectValueType_BoolPtr:
		{
			bool *ptr = *(bool **)(addr + params[3]);
			*ptr = params[5] != 0;
			break;
		}
		case ObjectValueType_EhandlePtr:
		{
			edict_t *pEdict = gamehelpers->EdictOfIndex(params[5]);

			if(!pEdict || pEdict->IsFree())
			{
				return pContext->ThrowNativeError("Invalid edict passed");
			}
			gamehelpers->SetHandleEntity(**(CBaseHandle **)(addr + params[3]), pEdict);

			break;
		}
		case ObjectValueType_FloatPtr:
		{
			float *ptr = *(float **)(addr + params[3]);
			*ptr = sp_ctof(params[5]);
			break;
		}
		default:
			return pContext->ThrowNativeError("Invalid Object value type");
	}
	return 1;
}

//native DHookGetParamObjectPtrVarVector(Handle:hParams, num, offset, ObjectValueType:type, Float:buffer[3]);
cell_t Native_GetParamObjectPtrVarVector(IPluginContext *pContext, const cell_t *params)
{
	HookParamsStruct *paramStruct;

	if(!GetHandleIfValidOrError(g_HookParamsHandle, (void **)&paramStruct, pContext, params[1]))
	{
		return 0;
	}

	if(params[2] <= 0 || params[2] > (int)paramStruct->dg->params.size())
	{
		return pContext->ThrowNativeError("Invalid param number %i max params is %i", params[2], paramStruct->dg->params.size());
	}

	int index = params[2] - 1;

	if(paramStruct->dg->params.at(index).type != HookParamType_ObjectPtr && paramStruct->dg->params.at(index).type != HookParamType_Object)
	{
		return pContext->ThrowNativeError("Invalid object value type %i", paramStruct->dg->params.at(index).type);
	}

	intptr_t addr = GetObjectAddr(paramStruct->dg->params.at(index).type, paramStruct->orgParams, index);

	cell_t *buffer;
	pContext->LocalToPhysAddr(params[5], &buffer);

	if((ObjectValueType)params[4] == ObjectValueType_VectorPtr || (ObjectValueType)params[4] == ObjectValueType_Vector)
	{
		Vector *vec;

		if((ObjectValueType)params[4] == ObjectValueType_VectorPtr)
		{
			vec = *(Vector **)(addr + params[3]);
			if(vec == NULL)
			{
				return pContext->ThrowNativeError("Trying to get value for null pointer.");
			}
		}
		else
		{
			vec = (Vector *)(addr + params[3]);
		}

		buffer[0] = sp_ftoc(vec->x);
		buffer[1] = sp_ftoc(vec->y);
		buffer[2] = sp_ftoc(vec->z);
		return 1;
	}

	return pContext->ThrowNativeError("Invalid Object value type (not a type of vector)");
}

//native DHookSetParamObjectPtrVarVector(Handle:hParams, num, offset, ObjectValueType:type, Float:value[3]);
cell_t Native_SetParamObjectPtrVarVector(IPluginContext *pContext, const cell_t *params)
{
	HookParamsStruct *paramStruct;

	if(!GetHandleIfValidOrError(g_HookParamsHandle, (void **)&paramStruct, pContext, params[1]))
	{
		return 0;
	}

	if(params[2] <= 0 || params[2] > (int)paramStruct->dg->params.size())
	{
		return pContext->ThrowNativeError("Invalid param number %i max params is %i", params[2], paramStruct->dg->params.size());
	}

	int index = params[2] - 1;

	if(paramStruct->dg->params.at(index).type != HookParamType_ObjectPtr)
	{
		return pContext->ThrowNativeError("Invalid object value type %i", paramStruct->dg->params.at(index).type);
	}

	intptr_t addr = GetObjectAddr(paramStruct->dg->params.at(index).type, paramStruct->orgParams, index);

	cell_t *buffer;
	pContext->LocalToPhysAddr(params[5], &buffer);

	if((ObjectValueType)params[4] == ObjectValueType_VectorPtr)
	{
		Vector *vec;

		if((ObjectValueType)params[4] == ObjectValueType_VectorPtr)
		{
			vec = *(Vector **)(addr + params[3]);
			if(vec == NULL)
			{
				return pContext->ThrowNativeError("Trying to set value for null pointer.");
			}
		}
		else
		{
			vec = (Vector *)(addr + params[3]);
		}

		vec->x = sp_ctof(buffer[0]);
		vec->y = sp_ctof(buffer[1]);
		vec->z = sp_ctof(buffer[2]);
		return 1;
	}
	return pContext->ThrowNativeError("Invalid Object value type (not a type of vector)");
}

//native DHookGetParamObjectPtrString(Handle:hParams, num, offset, ObjectValueType:type, String:buffer[], size)
cell_t Native_GetParamObjectPtrString(IPluginContext *pContext, const cell_t *params)
{
	HookParamsStruct *paramStruct;

	if(!GetHandleIfValidOrError(g_HookParamsHandle, (void **)&paramStruct, pContext, params[1]))
	{
		return 0;
	}

	if(params[2] <= 0 || params[2] > (int)paramStruct->dg->params.size())
	{
		return pContext->ThrowNativeError("Invalid param number %i max params is %i", params[2], paramStruct->dg->params.size());
	}

	int index = params[2] - 1;

	if(paramStruct->dg->params.at(index).type != HookParamType_ObjectPtr && paramStruct->dg->params.at(index).type != HookParamType_Object)
	{
		return pContext->ThrowNativeError("Invalid object value type %i", paramStruct->dg->params.at(index).type);
	}

	intptr_t addr = GetObjectAddr(paramStruct->dg->params.at(index).type, paramStruct->orgParams, index);

	switch((ObjectValueType)params[4])
	{
		case ObjectValueType_CharPtr:
		{
			char *ptr = *(char **)(addr + params[3]);
			pContext->StringToLocal(params[5], params[6], ptr == NULL ? "" : (const char *)ptr);
			break;
		}
		case ObjectValueType_String:
		{
			string_t string = *(string_t *)(addr + params[3]);
			pContext->StringToLocal(params[5], params[6], string == NULL_STRING ? "" : STRING(string));
			break;
		}
		default:
			return pContext->ThrowNativeError("Invalid Object value type (not a type of string)");
	}
	return 1;
}

// DHookGetReturnVector(Handle:hReturn, Float:vec[3])
cell_t Native_GetReturnVector(IPluginContext *pContext, const cell_t *params)
{
	HookReturnStruct *returnStruct;

	if(!GetHandleIfValidOrError(g_HookReturnHandle, (void **)&returnStruct, pContext, params[1]))
	{
		return 0;
	}

	cell_t *buffer;
	pContext->LocalToPhysAddr(params[2], &buffer);

	if(returnStruct->type == ReturnType_Vector)
	{
		buffer[0] = sp_ftoc((*(Vector *)returnStruct->orgResult).x);
		buffer[1] = sp_ftoc((*(Vector *)returnStruct->orgResult).y);
		buffer[2] = sp_ftoc((*(Vector *)returnStruct->orgResult).z);

		return 1;
	}
	else if(returnStruct->type == ReturnType_VectorPtr)
	{
		buffer[0] = sp_ftoc(((Vector *)returnStruct->orgResult)->x);
		buffer[1] = sp_ftoc(((Vector *)returnStruct->orgResult)->y);
		buffer[2] = sp_ftoc(((Vector *)returnStruct->orgResult)->z);

		return 1;
	}
	return pContext->ThrowNativeError("Return type is not a vector type");
}

//DHookSetReturnVector(Handle:hReturn, Float:vec[3])
cell_t Native_SetReturnVector(IPluginContext *pContext, const cell_t *params)
{
	HookReturnStruct *returnStruct;

	if(!GetHandleIfValidOrError(g_HookReturnHandle, (void **)&returnStruct, pContext, params[1]))
	{
		return 0;
	}

	cell_t *buffer;
	pContext->LocalToPhysAddr(params[2], &buffer);

	if(returnStruct->type == ReturnType_Vector)
	{
		*(Vector *)returnStruct->newResult = Vector(sp_ctof(buffer[0]), sp_ctof(buffer[1]), sp_ctof(buffer[2]));

		return 1;
	}
	else if(returnStruct->type == ReturnType_VectorPtr)
	{
		returnStruct->newResult = new Vector(sp_ctof(buffer[0]), sp_ctof(buffer[1]), sp_ctof(buffer[2]));
	}
	return pContext->ThrowNativeError("Return type is not a vector type");
}

//native bool:DHookIsNullParam(Handle:hParams, num);
cell_t Native_IsNullParam(IPluginContext *pContext, const cell_t *params)
{
	HookParamsStruct *paramStruct;

	if(!GetHandleIfValidOrError(g_HookParamsHandle, (void **)&paramStruct, pContext, params[1]))
	{
		return 0;
	}

	if(params[2] <= 0 || params[2] > (int)paramStruct->dg->params.size())
	{
		return pContext->ThrowNativeError("Invalid param number %i max params is %i", params[2], paramStruct->dg->params.size());
	}

	int index = params[2] - 1;

	HookParamType type = paramStruct->dg->params.at(index).type;

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
	{"DHookGetParamVector",					Native_GetParamVector},
	{"DHookGetReturnVector",				Native_GetReturnVector},
	{"DHookSetReturnVector",				Native_SetReturnVector},
	{"DHookSetParamVector",					Native_SetParamVector},
	{"DHookGetParamString",					Native_GetParamString},
	{"DHookGetReturnString",				Native_GetReturnString},
	{"DHookSetReturnString",				Native_SetReturnString},
	{"DHookSetParamString",					Native_SetParamString},
	{"DHookAddEntityListener",				Native_AddEntityListener},
	{"DHookRemoveEntityListener",			Native_RemoveEntityListener},
	{"DHookGetParamObjectPtrVar",			Native_GetParamObjectPtrVar},
	{"DHookSetParamObjectPtrVar",			Native_SetParamObjectPtrVar},
	{"DHookGetParamObjectPtrVarVector",		Native_GetParamObjectPtrVarVector},
	{"DHookSetParamObjectPtrVarVector",		Native_SetParamObjectPtrVarVector},
	{"DHookGetParamObjectPtrString",		Native_GetParamObjectPtrString},
	{"DHookIsNullParam",					Native_IsNullParam},
	{NULL,									NULL}
};

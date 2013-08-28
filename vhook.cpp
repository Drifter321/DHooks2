#include "vhook.h"
#include "vfunc_call.h"

SourceHook::IHookManagerAutoGen *g_pHookManager = NULL;

CUtlVector<DHooksManager *> g_pHooks;

using namespace SourceHook;

DHooksManager::DHooksManager(HookSetup *setup, void *iface, IPluginFunction *remove_callback, bool post)
{
	this->callback = MakeHandler(setup->returnType);
	this->hookid = 0;
	this->remove_callback = remove_callback;
	this->callback->offset = setup->offset;
	this->callback->plugin_callback = setup->callback;
	this->callback->returnFlag = setup->returnFlag;
	this->callback->thisType = setup->thisType;
	this->callback->post = post;
	this->callback->hookType = setup->hookType;
	this->callback->params = setup->params;
	this->addr = 0;

	if(this->callback->hookType == HookType_Entity)
	{
		this->callback->entity = gamehelpers->EntityToBCompatRef((CBaseEntity *)iface);
	}
	else
	{
		if(this->callback->hookType == HookType_Raw)
		{
			this->addr = (intptr_t)iface;
		}
		this->callback->entity = -1;
	}

	CProtoInfoBuilder protoInfo(ProtoInfo::CallConv_ThisCall);

	for(int i = this->callback->params.Count() -1; i >= 0; i--)
	{
		protoInfo.AddParam(this->callback->params.Element(i).size, this->callback->params.Element(i).pass_type, this->callback->params.Element(i).flag, NULL, NULL, NULL, NULL);
	}

	if(this->callback->returnType == ReturnType_Void)
	{
		protoInfo.SetReturnType(0, SourceHook::PassInfo::PassType_Unknown, 0, NULL, NULL, NULL, NULL);
	}
	else if(this->callback->returnType == ReturnType_Float)
	{
		protoInfo.SetReturnType(sizeof(float), SourceHook::PassInfo::PassType_Float, setup->returnFlag, NULL, NULL, NULL, NULL);
	}
	else if(this->callback->returnType == ReturnType_String)
	{
		protoInfo.SetReturnType(sizeof(string_t), SourceHook::PassInfo::PassType_Object, setup->returnFlag, NULL, NULL, NULL, NULL);//We have to be 4 really... or else RIP
	}
	else if(this->callback->returnType == ReturnType_Vector)
	{
		protoInfo.SetReturnType(sizeof(Vector), SourceHook::PassInfo::PassType_Object, setup->returnFlag, NULL, NULL, NULL, NULL);
	}
	else
	{
		protoInfo.SetReturnType(sizeof(void *), SourceHook::PassInfo::PassType_Basic, setup->returnFlag, NULL, NULL, NULL, NULL);
	}
	HookManagerPubFunc hook = g_pHookManager->MakeHookMan(protoInfo, 0, this->callback->offset);

	this->hookid = g_SHPtr->AddHook(g_PLID,ISourceHook::Hook_Normal, iface, 0, hook, this->callback, this->callback->post);
}

void CleanupHooks(IPluginContext *pContext)
{
	for(int i = g_pHooks.Count() -1; i >= 0; i--)
	{
		DHooksManager *manager = g_pHooks.Element(i);

		if(pContext == NULL || pContext == manager->callback->plugin_callback->GetParentRuntime()->GetDefaultContext())
		{
			delete manager;
			g_pHooks.Remove(i);
		}
	}
}

bool SetupHookManager(ISmmAPI *ismm)
{
	g_pHookManager = static_cast<SourceHook::IHookManagerAutoGen *>(ismm->MetaFactory(MMIFACE_SH_HOOKMANAUTOGEN, NULL, NULL));
	
	return g_pHookManager != NULL;
}

size_t GetParamTypeSize(HookParamType type)
{
	return sizeof(void *);
}
SourceHook::PassInfo::PassType GetParamTypePassType(HookParamType type)
{
	switch(type)
	{
		case HookParamType_Float:
			return SourceHook::PassInfo::PassType_Float;
		case HookParamType_Object:
			return SourceHook::PassInfo::PassType_Object;
	}
	return SourceHook::PassInfo::PassType_Basic;
}
size_t GetStackArgsSize(DHooksCallback *dg)
{
	size_t res = 0;
	for(int i = dg->params.Count() - 1; i >= 0; i--)
	{
		res += dg->params.Element(i).size;
	}

	if(dg->returnType == ReturnType_Vector)//Account for result vector ptr.
	{
		res += sizeof(void *);
	}
	return res;
}
HookParamsStruct *GetParamStruct(DHooksCallback *dg, void **argStack, size_t argStackSize)
{
	HookParamsStruct *params = new HookParamsStruct();
	params->dg = dg;
	if(dg->returnType != ReturnType_Vector)
	{
		params->orgParams = (void **)malloc(argStackSize);
		memcpy(params->orgParams, argStack, argStackSize);
	}
	else //Offset result ptr
	{
		params->orgParams = (void **)malloc(argStackSize-sizeof(void *));
		memcpy(params->orgParams, argStack+sizeof(void *), argStackSize);
	}
	params->newParams = (void **)malloc(dg->params.Count() * sizeof(void *));
	params->isChanged = (bool *)malloc(dg->params.Count() * sizeof(bool));
	for(int i = 0; i < dg->params.Count(); i++)
	{
		params->newParams[i] = NULL;
		params->isChanged[i] = false;
	}
	return params;
}
HookReturnStruct *GetReturnStruct(DHooksCallback *dg)
{
	HookReturnStruct *res = new HookReturnStruct();
	res->isChanged = false;
	res->type = dg->returnType;
	res->newResult = malloc(sizeof(void *));
	*(void **)res->newResult = NULL;
	res->orgResult = malloc(sizeof(void *));

	if(g_SHPtr->GetOrigRet() && dg->post)
	{
		switch(dg->returnType)
		{
			case ReturnType_String:
				*(string_t *)res->orgResult = META_RESULT_ORIG_RET(string_t);
				break;
			case ReturnType_Int:
				*(int *)res->orgResult = META_RESULT_ORIG_RET(int);
				break;
			case ReturnType_Bool:
				*(bool *)res->orgResult = META_RESULT_ORIG_RET(bool);
				break;
			case ReturnType_Float:
				*(float *)res->orgResult = META_RESULT_ORIG_RET(float);
				break;
			case ReturnType_Vector:
			{
				Vector vec = META_RESULT_ORIG_RET(Vector);
				*(Vector **)res->orgResult = new Vector(vec);
				break;
			}
			default:
				*(void **)res->orgResult = malloc(sizeof(void **));
				*(void **)res->orgResult = META_RESULT_ORIG_RET(void *);
				break;
		}
	}
	else
	{
		switch(dg->returnType)
		{
			case ReturnType_String:
				*(string_t *)res->orgResult = NULL_STRING;
				break;
			//ReturnType_Vector,
			case ReturnType_Int:
				*(int *)res->orgResult = 0;
				break;
			case ReturnType_Bool:
				*(bool *)res->orgResult = false;
				break;
			case ReturnType_Float:
				*(float *)res->orgResult = 0.0;
				break;
			default:
				*(void **)res->orgResult = NULL;
				break;
		}
	}

	return res;
}
cell_t GetThisPtr(void *iface, ThisPointerType type)
{
	if(ThisPointer_CBaseEntity)
	{
		return gamehelpers->EntityToBCompatRef((CBaseEntity *)iface);
	}

	return (cell_t)iface;
}

#ifndef __linux__
void *Callback(DHooksCallback *dg, void **argStack, size_t *argsizep)
#else
void *Callback(DHooksCallback *dg, void **argStack)
#endif
{
	HookReturnStruct *returnStruct = NULL;
	HookParamsStruct *paramStruct = NULL;
	Handle_t rHndl;
	Handle_t pHndl;

	#ifndef __linux__
	*argsizep = GetStackArgsSize(dg);
	#else
	size_t argsize = GetStackArgsSize(dg);
	#endif

	if(dg->thisType == ThisPointer_CBaseEntity || dg->thisType == ThisPointer_Address)
	{
		dg->plugin_callback->PushCell(GetThisPtr(g_SHPtr->GetIfacePtr(), dg->thisType));
	}
	if(dg->returnType != ReturnType_Void)
	{
		returnStruct = GetReturnStruct(dg);
		rHndl = handlesys->CreateHandle(g_HookReturnHandle, returnStruct, dg->plugin_callback->GetParentRuntime()->GetDefaultContext()->GetIdentity(), myself->GetIdentity(), NULL);
		if(!rHndl)
		{
			dg->plugin_callback->Cancel();
			if(returnStruct)
			{
				delete returnStruct;
			}
			g_SHPtr->SetRes(MRES_IGNORED);
			return NULL;
		}
		dg->plugin_callback->PushCell(rHndl);
	}

	#ifndef __linux__
	if(*argsizep > 0)
	{
		paramStruct = GetParamStruct(dg, argStack, *argsizep);
	#else
	if(argsize > 0)
	{
		paramStruct = GetParamStruct(dg, argStack, argsize);
	#endif
		pHndl = handlesys->CreateHandle(g_HookParamsHandle, paramStruct, dg->plugin_callback->GetParentRuntime()->GetDefaultContext()->GetIdentity(), myself->GetIdentity(), NULL);
		if(!pHndl)
		{
			dg->plugin_callback->Cancel();
			if(returnStruct)
			{
				delete returnStruct;
			}
			if(paramStruct)
			{
				delete paramStruct;
			}
			g_SHPtr->SetRes(MRES_IGNORED);
			return NULL;
		}
		dg->plugin_callback->PushCell(pHndl);
	}
	cell_t result = (cell_t)MRES_Ignored;
	META_RES mres = MRES_IGNORED;

	dg->plugin_callback->Execute(&result);

	void *ret = g_SHPtr->GetOverrideRetPtr();
	switch((MRESReturn)result)
	{
		case MRES_Handled:
		case MRES_ChangedHandled:
			g_SHPtr->DoRecall();
			g_SHPtr->SetRes(MRES_SUPERCEDE);
			mres = MRES_SUPERCEDE;
			ret = CallVFunction<void *>(dg, paramStruct, g_SHPtr->GetIfacePtr());
			break;
		case MRES_ChangedOverride:
			if(dg->returnType != ReturnType_Void)
			{
				if(returnStruct->isChanged)
				{
					ret = *(void **)returnStruct->newResult;
				}
				else //Throw an error if no override was set
				{
					g_SHPtr->SetRes(MRES_IGNORED);
					mres = MRES_IGNORED;
					dg->plugin_callback->GetParentRuntime()->GetDefaultContext()->ThrowNativeError("Tried to override return value without return value being set");
					break;
				}
			}
			g_SHPtr->DoRecall();
			g_SHPtr->SetRes(MRES_SUPERCEDE);
			mres = MRES_SUPERCEDE;
			CallVFunction<void *>(dg, paramStruct, g_SHPtr->GetIfacePtr());
			break;
		case MRES_Override:
			if(dg->returnType != ReturnType_Void)
			{
				if(returnStruct->isChanged)
				{
					g_SHPtr->SetRes(MRES_OVERRIDE);
					mres = MRES_OVERRIDE;
					ret = *(void **)returnStruct->newResult;
				}
				else //Throw an error if no override was set
				{
					g_SHPtr->SetRes(MRES_IGNORED);
					mres = MRES_IGNORED;
					dg->plugin_callback->GetParentRuntime()->GetDefaultContext()->ThrowNativeError("Tried to override return value without return value being set");
				}
			}
			break;
		case MRES_Supercede:
			if(dg->returnType != ReturnType_Void)
			{
				if(returnStruct->isChanged)
				{
					g_SHPtr->SetRes(MRES_SUPERCEDE);
					mres = MRES_SUPERCEDE;
					ret = *(void **)returnStruct->newResult;
				}
				else //Throw an error if no override was set
				{
					g_SHPtr->SetRes(MRES_IGNORED);
					mres = MRES_IGNORED;
					dg->plugin_callback->GetParentRuntime()->GetDefaultContext()->ThrowNativeError("Tried to override return value without return value being set");
				}
			}
			break;
		default:
			g_SHPtr->SetRes(MRES_IGNORED);
			mres = MRES_IGNORED;
			break;
	}

	HandleSecurity sec(dg->plugin_callback->GetParentRuntime()->GetDefaultContext()->GetIdentity(), myself->GetIdentity());

	if(returnStruct)
	{
		handlesys->FreeHandle(rHndl, &sec);
	}
	if(paramStruct)
	{
		handlesys->FreeHandle(pHndl, &sec);
	}

	if(dg->returnType == ReturnType_Void || mres <= MRES_HANDLED)
	{
		return NULL;
	}
	return ret;
}
#ifndef __linux__
float Callback_float(DHooksCallback *dg, void **argStack, size_t *argsizep)
#else
void *Callback_float(DHooksCallback *dg, void **argStack)
#endif
{
	HookReturnStruct *returnStruct = NULL;
	HookParamsStruct *paramStruct = NULL;
	Handle_t rHndl;
	Handle_t pHndl;

	#ifndef __linux__
	*argsizep = GetStackArgsSize(dg);
	#else
	size_t argsize = GetStackArgsSize(dg);
	#endif

	if(dg->thisType == ThisPointer_CBaseEntity || dg->thisType == ThisPointer_Address)
	{
		dg->plugin_callback->PushCell(GetThisPtr(g_SHPtr->GetIfacePtr(), dg->thisType));
	}

	returnStruct = GetReturnStruct(dg);
	rHndl = handlesys->CreateHandle(g_HookReturnHandle, returnStruct, dg->plugin_callback->GetParentRuntime()->GetDefaultContext()->GetIdentity(), myself->GetIdentity(), NULL);

	if(!rHndl)
	{
		dg->plugin_callback->Cancel();
		if(returnStruct)
		{
			delete returnStruct;
		}
		g_SHPtr->SetRes(MRES_IGNORED);
		return NULL;
	}
	dg->plugin_callback->PushCell(rHndl);

	#ifndef __linux__
	if(*argsizep > 0)
	{
		paramStruct = GetParamStruct(dg, argStack, *argsizep);
	#else
	if(argsize > 0)
	{
		paramStruct = GetParamStruct(dg, argStack, argsize);
	#endif
		pHndl = handlesys->CreateHandle(g_HookParamsHandle, paramStruct, dg->plugin_callback->GetParentRuntime()->GetDefaultContext()->GetIdentity(), myself->GetIdentity(), NULL);
		if(!pHndl)
		{
			dg->plugin_callback->Cancel();
			if(returnStruct)
			{
				delete returnStruct;
			}
			if(paramStruct)
			{
				delete paramStruct;
			}
			g_SHPtr->SetRes(MRES_IGNORED);
			return NULL;
		}
		dg->plugin_callback->PushCell(pHndl);
	}
	cell_t result = (cell_t)MRES_Ignored;
	META_RES mres = MRES_IGNORED;
	dg->plugin_callback->Execute(&result);

	void *ret = g_SHPtr->GetOverrideRetPtr();
	switch((MRESReturn)result)
	{
		case MRES_Handled:
		case MRES_ChangedHandled:
			g_SHPtr->DoRecall();
			g_SHPtr->SetRes(MRES_SUPERCEDE);
			mres = MRES_SUPERCEDE;
			*(float *)ret = CallVFunction<float>(dg, paramStruct, g_SHPtr->GetIfacePtr());
			break;
		case MRES_ChangedOverride:
			if(dg->returnType != ReturnType_Void)
			{
				if(returnStruct->isChanged)
				{
					*(float *)ret = *(float *)returnStruct->newResult;
				}
				else //Throw an error if no override was set
				{
					g_SHPtr->SetRes(MRES_IGNORED);
					mres = MRES_IGNORED;
					dg->plugin_callback->GetParentRuntime()->GetDefaultContext()->ThrowNativeError("Tried to override return value without return value being set");
					break;
				}
			}
			g_SHPtr->DoRecall();
			g_SHPtr->SetRes(MRES_SUPERCEDE);
			mres = MRES_SUPERCEDE;
			CallVFunction<float>(dg, paramStruct, g_SHPtr->GetIfacePtr());
			break;
		case MRES_Override:
			if(dg->returnType != ReturnType_Void)
			{
				if(returnStruct->isChanged)
				{
					g_SHPtr->SetRes(MRES_OVERRIDE);
					mres = MRES_OVERRIDE;
					*(float *)ret = *(float *)returnStruct->newResult;
				}
				else //Throw an error if no override was set
				{
					g_SHPtr->SetRes(MRES_IGNORED);
					mres = MRES_IGNORED;
					dg->plugin_callback->GetParentRuntime()->GetDefaultContext()->ThrowNativeError("Tried to override return value without return value being set");
				}
			}
			break;
		case MRES_Supercede:
			if(dg->returnType != ReturnType_Void)
			{
				if(returnStruct->isChanged)
				{
					g_SHPtr->SetRes(MRES_SUPERCEDE);
					mres = MRES_SUPERCEDE;
					*(float *)ret = *(float *)returnStruct->newResult;
				}
				else //Throw an error if no override was set
				{
					g_SHPtr->SetRes(MRES_IGNORED);
					mres = MRES_IGNORED;
					dg->plugin_callback->GetParentRuntime()->GetDefaultContext()->ThrowNativeError("Tried to override return value without return value being set");
				}
			}
			break;
		default:
			g_SHPtr->SetRes(MRES_IGNORED);
			mres = MRES_IGNORED;
			break;
	}

	HandleSecurity sec(dg->plugin_callback->GetParentRuntime()->GetDefaultContext()->GetIdentity(), myself->GetIdentity());

	if(returnStruct)
	{
		handlesys->FreeHandle(rHndl, &sec);
	}
	if(paramStruct)
	{
		handlesys->FreeHandle(pHndl, &sec);
	}

	if(dg->returnType == ReturnType_Void || mres <= MRES_HANDLED)
	{
		return NULL;
	}
	return *(float *)ret;
}
Vector *Callback_vector(DHooksCallback *dg, void **argStack, size_t *argsizep)
{
	Vector *vec_result = (Vector *)argStack[0]; // Save the result

	HookReturnStruct *returnStruct = NULL;
	HookParamsStruct *paramStruct = NULL;
	Handle_t rHndl;
	Handle_t pHndl;

	#ifndef __linux__
	*argsizep = GetStackArgsSize(dg);
	#else
	size_t argsize = GetStackArgsSize(dg);
	#endif

	if(dg->thisType == ThisPointer_CBaseEntity || dg->thisType == ThisPointer_Address)
	{
		dg->plugin_callback->PushCell(GetThisPtr(g_SHPtr->GetIfacePtr(), dg->thisType));
	}

	returnStruct = GetReturnStruct(dg);
	rHndl = handlesys->CreateHandle(g_HookReturnHandle, returnStruct, dg->plugin_callback->GetParentRuntime()->GetDefaultContext()->GetIdentity(), myself->GetIdentity(), NULL);

	if(!rHndl)
	{
		dg->plugin_callback->Cancel();
		if(returnStruct)
		{
			delete returnStruct;
		}
		g_SHPtr->SetRes(MRES_IGNORED);
		return NULL;
	}
	dg->plugin_callback->PushCell(rHndl);

	#ifndef __linux__
	if(*argsizep > 0)
	{
		paramStruct = GetParamStruct(dg, argStack, *argsizep);
	#else
	if(argsize > 0)
	{
		paramStruct = GetParamStruct(dg, argStack, argsize);
	#endif
		pHndl = handlesys->CreateHandle(g_HookParamsHandle, paramStruct, dg->plugin_callback->GetParentRuntime()->GetDefaultContext()->GetIdentity(), myself->GetIdentity(), NULL);
		if(!pHndl)
		{
			dg->plugin_callback->Cancel();
			if(returnStruct)
			{
				delete returnStruct;
			}
			if(paramStruct)
			{
				delete paramStruct;
			}
			g_SHPtr->SetRes(MRES_IGNORED);
			return NULL;
		}
		dg->plugin_callback->PushCell(pHndl);
	}
	cell_t result = (cell_t)MRES_Ignored;
	META_RES mres = MRES_IGNORED;
	dg->plugin_callback->Execute(&result);

	void *ret = g_SHPtr->GetOverrideRetPtr();
	ret = vec_result;
	switch((MRESReturn)result)
	{
		case MRES_Handled:
		case MRES_ChangedHandled:
			g_SHPtr->DoRecall();
			g_SHPtr->SetRes(MRES_SUPERCEDE);
			mres = MRES_SUPERCEDE;
			*vec_result = CallVFunction<Vector>(dg, paramStruct, g_SHPtr->GetIfacePtr());
			break;
		case MRES_ChangedOverride:
			if(dg->returnType != ReturnType_Void)
			{
				if(returnStruct->isChanged)
				{
					*vec_result = **(Vector **)returnStruct->newResult;
				}
				else //Throw an error if no override was set
				{
					g_SHPtr->SetRes(MRES_IGNORED);
					mres = MRES_IGNORED;
					dg->plugin_callback->GetParentRuntime()->GetDefaultContext()->ThrowNativeError("Tried to override return value without return value being set");
					break;
				}
			}
			g_SHPtr->DoRecall();
			g_SHPtr->SetRes(MRES_SUPERCEDE);
			mres = MRES_SUPERCEDE;
			CallVFunction<Vector>(dg, paramStruct, g_SHPtr->GetIfacePtr());
			break;
		case MRES_Override:
			if(dg->returnType != ReturnType_Void)
			{
				if(returnStruct->isChanged)
				{
					g_SHPtr->SetRes(MRES_OVERRIDE);
					mres = MRES_OVERRIDE;
					*vec_result = **(Vector **)returnStruct->newResult;
				}
				else //Throw an error if no override was set
				{
					g_SHPtr->SetRes(MRES_IGNORED);
					mres = MRES_IGNORED;
					dg->plugin_callback->GetParentRuntime()->GetDefaultContext()->ThrowNativeError("Tried to override return value without return value being set");
				}
			}
			break;
		case MRES_Supercede:
			if(dg->returnType != ReturnType_Void)
			{
				if(returnStruct->isChanged)
				{
					g_SHPtr->SetRes(MRES_SUPERCEDE);
					mres = MRES_SUPERCEDE;
					*vec_result = **(Vector **)returnStruct->newResult;
				}
				else //Throw an error if no override was set
				{
					g_SHPtr->SetRes(MRES_IGNORED);
					mres = MRES_IGNORED;
					dg->plugin_callback->GetParentRuntime()->GetDefaultContext()->ThrowNativeError("Tried to override return value without return value being set");
				}
			}
			break;
		default:
			g_SHPtr->SetRes(MRES_IGNORED);
			mres = MRES_IGNORED;
			break;
	}

	HandleSecurity sec(dg->plugin_callback->GetParentRuntime()->GetDefaultContext()->GetIdentity(), myself->GetIdentity());

	if(returnStruct)
	{
		handlesys->FreeHandle(rHndl, &sec);
	}
	if(paramStruct)
	{
		handlesys->FreeHandle(pHndl, &sec);
	}

	if(dg->returnType == ReturnType_Void || mres <= MRES_HANDLED)
	{
		META_CONPRINT("Returning now\n");
		vec_result->x = 0;
		vec_result->y = 0;
		vec_result->z = 0;
		return vec_result;
	}
	return vec_result;
}

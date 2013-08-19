#include "vhook.h"

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

	if(this->callback->hookType == HookType_Entity)
	{
		this->callback->entity = gamehelpers->EntityToBCompatRef((CBaseEntity *)iface);
	}
	else
	{
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
		protoInfo.SetReturnType(sizeof(float), SourceHook::PassInfo::PassType_Float, 0, NULL, NULL, NULL, NULL);
	}
	else
	{
		protoInfo.SetReturnType(sizeof(void *), SourceHook::PassInfo::PassType_Basic, 0, NULL, NULL, NULL, NULL);
	}
	HookManagerPubFunc hook = g_pHookManager->MakeHookMan(protoInfo, 0, this->callback->offset);

	this->hookid = g_SHPtr->AddHook(g_PLID,ISourceHook::Hook_Normal, iface, 0, hook, this->callback, this->callback->post);
}

void CleanupHooks(IPluginContext *pContext)
{
	for(int i = g_pHooks.Count() -1; i >= 0; i--)
	{
		DHooksManager *manager = g_pHooks.Element(i);

		if(pContext == NULL || pContext == manager->callback->plugin_callback->GetParentContext())
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
			return SourceHook::PassInfo::PassType::PassType_Float;
		default:
			return SourceHook::PassInfo::PassType::PassType_Basic;
	}
}
size_t GetStackArgsSize(DHooksCallback *dg)
{
	size_t res = 0;
	for(int i = dg->params.Count() - 1; i >= 0; i--)
	{
		res += dg->params.Element(i).size;
	}
	return res;
}
HookParamsStruct *GetParamStruct(DHooksCallback *dg, void **argStack, size_t argStackSize)
{
	HookParamsStruct *res = new HookParamsStruct();
	res->dg = dg;
	res->orgParams = (void **)malloc(argStackSize);
	res->newParams = (void **)malloc(argStackSize);
	memcpy(res->orgParams, argStack, argStackSize);
	for(int i = 0; i < dg->params.Count(); i++)
	{
		res->newParams[i] = NULL;
	}
	return res;
}
HookReturnStruct *GetReturnStruct(DHooksCallback *dg, const void *result)
{
	HookReturnStruct *res = new HookReturnStruct();
	res->isChanged = false;
	res->type = dg->returnType;
	res->orgResult = &result;
	res->newResult = 0;

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
{
	HookReturnStruct *returnStruct = NULL;
	HookParamsStruct *paramStruct = NULL;
	Handle_t rHndl;
	Handle_t pHndl;

	*argsizep = GetStackArgsSize(dg);

	if(dg->thisType == ThisPointer_CBaseEntity || dg->thisType == ThisPointer_Address)
	{
		dg->plugin_callback->PushCell(GetThisPtr(g_SHPtr->GetIfacePtr(), dg->thisType));
	}
	if(dg->returnType != ReturnType_Void)
	{
		returnStruct = GetReturnStruct(dg, g_SHPtr->GetOrigRet());
		rHndl = handlesys->CreateHandle(g_HookReturnHandle, returnStruct, dg->plugin_callback->GetParentRuntime()->GetDefaultContext()->GetIdentity(), myself->GetIdentity(), NULL);
		if(!rHndl)
		{
			if(returnStruct)
			{
				delete returnStruct;
			}
			g_SHPtr->SetRes(MRES_IGNORED);
			return 0;
		}
		dg->plugin_callback->PushCell(rHndl);
	}
	if(*argsizep > 0)
	{
		paramStruct = GetParamStruct(dg, argStack, *argsizep);
		pHndl = handlesys->CreateHandle(g_HookParamsHandle, paramStruct, dg->plugin_callback->GetParentRuntime()->GetDefaultContext()->GetIdentity(), myself->GetIdentity(), NULL);
		if(!pHndl)
		{
			if(returnStruct)
			{
				delete returnStruct;
			}
			if(paramStruct)
			{
				delete paramStruct;
			}
			g_SHPtr->SetRes(MRES_IGNORED);
			return 0;
		}
		dg->plugin_callback->PushCell(pHndl);
	}
	cell_t result = (cell_t)MRES_Ignored;
	dg->plugin_callback->Execute(&result);

	void *ret = g_SHPtr->GetOverrideRetPtr();
	void *res = 0;

	switch((MRESReturn)result)
	{
		case MRES_Handled:
		case MRES_ChangedHandled:
			g_SHPtr->DoRecall();
			g_SHPtr->SetRes(MRES_SUPERCEDE);
			//ret = CallVFunction(dg, argStack, g_SHPtr->GetIfacePtr());
			memcpy(res, ret, sizeof(void *));
			break;
		case MRES_ChangedOverride:
			g_SHPtr->DoRecall();
			g_SHPtr->SetRes(MRES_SUPERCEDE);
			//CallVFunction(dg, argStack, g_SHPtr->GetIfacePtr());
			if(dg->returnType != ReturnType_Void)
			{
				if(returnStruct->isChanged)
				{
					memcpy(ret, returnStruct->newResult, sizeof(void *));
					memcpy(res, returnStruct->newResult, sizeof(void *));
				}
				else if(dg->post)
				{
					memcpy(ret, returnStruct->orgResult, sizeof(void *));
					memcpy(res, returnStruct->orgResult, sizeof(void *));
				}
			}
			break;
		case MRES_Override:
			g_SHPtr->SetRes(MRES_OVERRIDE);
			if(dg->returnType != ReturnType_Void)
			{
				if(returnStruct->isChanged)
				{
					memcpy(ret, returnStruct->newResult, sizeof(void *));
					memcpy(res, returnStruct->newResult, sizeof(void *));
				}
				else if(dg->post)
				{
					memcpy(ret, returnStruct->orgResult, sizeof(void *));
					memcpy(res, returnStruct->orgResult, sizeof(void *));
				}
			}
			break;
		case MRES_Supercede:
			g_SHPtr->SetRes(MRES_SUPERCEDE);
			if(dg->returnType != ReturnType_Void)
			{
				if(returnStruct->isChanged)
				{
					memcpy(ret, returnStruct->newResult, sizeof(void *));
					memcpy(res, returnStruct->newResult, sizeof(void *));
				}
				else if(dg->post)
				{
					memcpy(ret, returnStruct->orgResult, sizeof(void *));
					memcpy(res, returnStruct->orgResult, sizeof(void *));
				}
			}
			break;
		default:
			g_SHPtr->SetRes(MRES_IGNORED);
			break;
	}

	HandleSecurity sec(dg->plugin_callback->GetParentContext()->GetIdentity(), myself->GetIdentity());

	if(returnStruct)
	{
		handlesys->FreeHandle(rHndl, &sec);
	}
	if(paramStruct)
	{
		handlesys->FreeHandle(pHndl, &sec);
	}

	return res;
}
#else
void *Callback(DHooksCallback *dg, void **argStack)
{
	if(dg->returnType == ReturnType_Void)
	{
		META_CONPRINTF("String is %s\n", argStack[0]);
		g_SHPtr->SetRes(MRES_IGNORED);
	}
	else
	{
		//META_CONPRINTF("Entity %i\n", gamehelpers->ReferenceToIndex(gamehelpers->EntityToBCompatRef((CBaseEntity *)g_SHPtr->GetIfacePtr())));
		g_SHPtr->SetRes(MRES_SUPERCEDE);
		void *ret = g_SHPtr->GetOverrideRetPtr();
		ret = (void *)false;
		return false;
	}
	return 0;
}
#endif

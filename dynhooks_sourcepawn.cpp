#include "dynhooks_sourcepawn.h"
#include "util.h"
#include <am-autoptr.h>

#include "conventions/x86MsCdecl.h"
#include "conventions/x86MsThiscall.h"
#include "conventions/x86MsStdcall.h"
#include "conventions/x86GccCdecl.h"
#include "conventions/x86GccThiscall.h"

#ifdef WIN32
typedef x86MsCdecl x86DetourCdecl;
typedef x86MsThiscall x86DetourThisCall;
typedef x86MsStdcall x86DetourStdCall;
#else
typedef x86GccCdecl x86DetourCdecl;
typedef x86GccThiscall x86DetourThisCall;
// Uhm
typedef x86MsStdcall x86DetourStdCall;
#endif

//ke::Vector<CHook *> g_pDetours;
//CallbackMap g_pPluginPreDetours;
//CallbackMap g_pPluginPostDetours;
DetourMap g_pPreDetours;
DetourMap g_pPostDetours;

void UnhookFunction(HookType_t hookType, CHook *pDetour)
{
	CHookManager *pDetourManager = GetHookManager();
	pDetour->RemoveCallback(hookType, (HookHandlerFn *)(void *)&HandleDetour);
	if (!pDetour->AreCallbacksRegistered())
		pDetourManager->UnhookFunction(pDetour->m_pFunc);
}

bool AddDetourPluginHook(HookType_t hookType, CHook *pDetour, HookSetup *setup, IPluginFunction *pCallback)
{
	DetourMap *map;
	if (hookType == HOOKTYPE_PRE)
		map = &g_pPreDetours;
	else
		map = &g_pPostDetours;

	// See if we already have this detour in our list.
	PluginCallbackList *wrappers;
	DetourMap::Insert f = map->findForAdd(pDetour);
	if (f.found())
	{
		wrappers = f->value;
	}
	else
	{
		// Create a vector to store all the plugin callbacks in.
		wrappers = new PluginCallbackList;
		if (!map->add(f, pDetour, wrappers))
		{
			delete wrappers;
			UnhookFunction(hookType, pDetour);
			return false;
		}
	}

	CDynamicHooksSourcePawn *pWrapper = new CDynamicHooksSourcePawn(setup, pDetour, pCallback, hookType == HOOKTYPE_POST);
	if (!wrappers->append(pWrapper))
	{
		if (wrappers->empty())
		{
			delete wrappers;
			UnhookFunction(hookType, pDetour);
			map->remove(f);
		}
		delete pWrapper;
		return false;
	}

	return true;
}

bool RemoveDetourPluginHook(HookType_t hookType, CHook *pDetour, IPluginFunction *pCallback)
{
	DetourMap *map;
	if (hookType == HOOKTYPE_PRE)
		map = &g_pPreDetours;
	else
		map = &g_pPostDetours;

	DetourMap::Result res = map->find(pDetour);
	if (!res.found())
		return false;

	// Remove the plugin's callback
	bool bRemoved = false;
	PluginCallbackList *wrappers = res->value;
	for (int i = wrappers->length()-1; i >= 0 ; i--)
	{
		CDynamicHooksSourcePawn *pWrapper = wrappers->at(i);
		if (pWrapper->plugin_callback == pCallback)
		{
			bRemoved = true;
			delete pWrapper;
			wrappers->remove(i--);
		}
	}

	// No more plugin hooks on this callback. Free our structures.
	if (wrappers->empty())
	{
		delete wrappers;
		UnhookFunction(hookType, pDetour);
		map->remove(res);
	}

	return bRemoved;
}

void RemoveAllCallbacksForContext(HookType_t hookType, DetourMap *map, IPluginContext *pContext)
{
	PluginCallbackList *wrappers;
	CDynamicHooksSourcePawn *pWrapper;
	DetourMap::iterator it = map->iter();
	// Run through all active detours we added.
	for (; !it.empty(); it.next())
	{
		wrappers = it->value;
		// See if there are callbacks of this plugin context registered
		// and remove them.
		for (int i = wrappers->length() - 1; i >= 0; i--)
		{
			pWrapper = wrappers->at(i);
			if (pWrapper->plugin_callback->GetParentContext() != pContext)
				continue;

			delete pWrapper;
			wrappers->remove(i--);
		}

		// No plugin interested in this hook anymore. unhook.
		if (wrappers->empty())
		{
			delete wrappers;
			UnhookFunction(hookType, it->key);
			it.erase();
		}
	}
}

void RemoveAllCallbacksForContext(IPluginContext *pContext)
{
	RemoveAllCallbacksForContext(HOOKTYPE_PRE, &g_pPreDetours, pContext);
	RemoveAllCallbacksForContext(HOOKTYPE_POST, &g_pPostDetours, pContext);
}

ICallingConvention *ConstructCallingConvention(HookSetup *setup)
{
	std::vector<DataTypeSized_t> vecArgTypes;
	for (size_t i = 0; i < setup->params.size(); i++)
	{
		ParamInfo &info = setup->params[i];
		DataTypeSized_t type;
		type.type = DynamicHooks_ConvertParamTypeFrom(info.type);
		type.size = info.size;
		vecArgTypes.push_back(type);
	}

	DataTypeSized_t returnType;
	returnType.type = DynamicHooks_ConvertReturnTypeFrom(setup->returnType);
	returnType.size = 0;

	ICallingConvention *pCallConv = nullptr;
	switch (setup->callConv)
	{
	case CallConv_CDECL:
		pCallConv = new x86DetourCdecl(vecArgTypes, returnType);
		break;
	case CallConv_THISCALL:
		pCallConv = new x86DetourThisCall(vecArgTypes, returnType);
		break;
	case CallConv_STDCALL:
		pCallConv = new x86DetourStdCall(vecArgTypes, returnType);
		break;
	}

	return pCallConv;
}

bool HandleDetour(HookType_t hookType, CHook* pDetour)
{
	DetourMap *map;
	if (hookType == HOOKTYPE_PRE)
		map = &g_pPreDetours;
	else
		map = &g_pPostDetours;

	// Find the callback list for this detour.
	DetourMap::Result r = map->find(pDetour);
	if (!r.found())
		return false;

	// List of all callbacks.
	PluginCallbackList *wrappers = r->value;

	HookReturnStruct *returnStruct = NULL;
	Handle_t rHndl = BAD_HANDLE;

	HookParamsStruct *paramStruct = NULL;
	Handle_t pHndl = BAD_HANDLE;

	int argNum = pDetour->m_pCallingConvention->m_vecArgTypes.size();
	MRESReturn finalRet = MRES_Ignored;
	ke::AutoPtr<void> finalRetBuf(new uint8_t[pDetour->m_pCallingConvention->m_returnType.size]);

	// Call all the plugin functions..
	for (size_t i = 0; i < wrappers->length(); i++)
	{
		CDynamicHooksSourcePawn *pWrapper = wrappers->at(i);
		IPluginFunction *pCallback = pWrapper->plugin_callback;
		MRESReturn tempRet = MRES_Ignored;
		ke::AutoPtr<void> tempRetBuf(new uint8_t[pDetour->m_pCallingConvention->m_returnType.size]);

		// Find the this pointer.
		if (pWrapper->callConv == CallConv_THISCALL)
		{
			void *thisPtr = pDetour->GetArgument<void *>(0);
			cell_t thisAddr = GetThisPtr(thisPtr, pWrapper->thisType);
			pCallback->PushCell(thisAddr);
		}

		if (pWrapper->returnType != ReturnType_Void)
		{
			returnStruct = pWrapper->GetReturnStruct();
			HandleError err;
			rHndl = handlesys->CreateHandle(g_HookReturnHandle, returnStruct, pCallback->GetParentRuntime()->GetDefaultContext()->GetIdentity(), myself->GetIdentity(), &err);
			if (!rHndl)
			{
				pCallback->Cancel();
				pCallback->GetParentRuntime()->GetDefaultContext()->BlamePluginError(pCallback, "Error creating ReturnHandle in preparation to call hook callback. (error %d)", err);

				if (returnStruct)
					delete returnStruct;

				// Don't call more callbacks. They will probably fail too.
				break;
			}
			pCallback->PushCell(rHndl);
		}

		if (argNum > 0)
		{
			paramStruct = pWrapper->GetParamStruct();
			HandleError err;
			pHndl = handlesys->CreateHandle(g_HookParamsHandle, paramStruct, pCallback->GetParentRuntime()->GetDefaultContext()->GetIdentity(), myself->GetIdentity(), &err);
			if (!pHndl)
			{
				pCallback->Cancel();
				pCallback->GetParentRuntime()->GetDefaultContext()->BlamePluginError(pCallback, "Error creating ThisHandle in preparation to call hook callback. (error %d)", err);

				// Don't leak our own handles here!
				if (rHndl)
				{
					HandleSecurity sec(pCallback->GetParentRuntime()->GetDefaultContext()->GetIdentity(), myself->GetIdentity());
					handlesys->FreeHandle(rHndl, &sec);
					rHndl = BAD_HANDLE;
				}

				if (paramStruct)
					delete paramStruct;

				// Don't call more callbacks. They will probably fail too.
				break;
			}
			pCallback->PushCell(pHndl);
		}

		cell_t result = (cell_t)MRES_Ignored;
		pCallback->Execute(&result);

		switch ((MRESReturn)result)
		{
		case MRES_Handled:
			tempRet = MRES_Handled;
			break;
		case MRES_ChangedHandled:
			tempRet = MRES_Handled;
			pWrapper->UpdateParamsFromStruct(paramStruct);
			break;
		case MRES_ChangedOverride:
			if (pWrapper->returnType != ReturnType_Void)
			{
				if (returnStruct->isChanged)
				{
					if (pWrapper->returnType == ReturnType_String || pWrapper->returnType == ReturnType_Int || pWrapper->returnType == ReturnType_Bool)
					{
						tempRetBuf = *(void **)returnStruct->newResult;
					}
					else if (pWrapper->returnType == ReturnType_Float)
					{
						*(float *)tempRetBuf.get() = *(float *)returnStruct->newResult;
					}
					else
					{
						tempRetBuf = returnStruct->newResult;
					}
				}
				else //Throw an error if no override was set
				{
					tempRet = MRES_Ignored;
					pCallback->GetParentRuntime()->GetDefaultContext()->BlamePluginError(pCallback, "Tried to override return value without return value being set");
					break;
				}
			}
			// TODO: Introduce that override concept in dyndetours.
			// This doesn't call the original function at the moment, but just returns the given return value.
			tempRet = MRES_Override;
			pWrapper->UpdateParamsFromStruct(paramStruct);
			break;
		case MRES_Override:
			if (pWrapper->returnType != ReturnType_Void)
			{
				if (returnStruct->isChanged)
				{
					// TODO: Introduce that override concept in dyndetours.
					// This doesn't call the original function at the moment, but just returns the given return value.
					tempRet = MRES_Override;
					if (pWrapper->returnType == ReturnType_String || pWrapper->returnType == ReturnType_Int || pWrapper->returnType == ReturnType_Bool)
					{
						tempRetBuf = *(void **)returnStruct->newResult;
					}
					else if (pWrapper->returnType == ReturnType_Float)
					{
						*(float *)tempRetBuf.get() = *(float *)returnStruct->newResult;
					}
					else
					{
						tempRetBuf = returnStruct->newResult;
					}
				}
				else //Throw an error if no override was set
				{
					tempRet = MRES_Ignored;
					pCallback->GetParentRuntime()->GetDefaultContext()->BlamePluginError(pCallback, "Tried to override return value without return value being set");
				}
			}
			break;
		case MRES_Supercede:
			if (pWrapper->returnType != ReturnType_Void)
			{
				if (returnStruct->isChanged)
				{
					tempRet = MRES_Supercede;
					if (pWrapper->returnType == ReturnType_String || pWrapper->returnType == ReturnType_Int || pWrapper->returnType == ReturnType_Bool)
					{
						tempRetBuf = *(void **)returnStruct->newResult;
					}
					else if (pWrapper->returnType == ReturnType_Float)
					{
						*(float *)tempRetBuf.get() = *(float *)returnStruct->newResult;
					}
					else
					{
						tempRetBuf = returnStruct->newResult;
					}
				}
				else //Throw an error if no override was set
				{
					tempRet = MRES_Ignored;
					pCallback->GetParentRuntime()->GetDefaultContext()->BlamePluginError(pCallback, "Tried to override return value without return value being set");
				}
			}
			else
			{
				tempRet = MRES_Supercede;
			}
			break;
		default:
			tempRet = MRES_Ignored;
			break;
		}

		// Prioritize the actions. 
		if (finalRet <= tempRet) {

			// ------------------------------------		
			// Copy the action and return value.
			// ------------------------------------
			finalRet = tempRet;
			memcpy(*finalRetBuf, *tempRetBuf, pDetour->m_pCallingConvention->m_returnType.size);
		}

		// Free the handles again.
		HandleSecurity sec(pCallback->GetParentRuntime()->GetDefaultContext()->GetIdentity(), myself->GetIdentity());

		if (returnStruct)
		{
			handlesys->FreeHandle(rHndl, &sec);
		}
		if (paramStruct)
		{
			handlesys->FreeHandle(pHndl, &sec);
		}
	}

	if (finalRet >= MRES_Override)
	{
		void* pPtr = pDetour->m_pCallingConvention->GetReturnPtr(pDetour->m_pRegisters);
		memcpy(pPtr, *finalRetBuf, pDetour->m_pCallingConvention->m_returnType.size);
		pDetour->m_pCallingConvention->ReturnPtrChanged(pDetour->m_pRegisters, pPtr);
	}

	return finalRet == MRES_Supercede;
}

CDynamicHooksSourcePawn::CDynamicHooksSourcePawn(HookSetup *setup, CHook *pDetour, IPluginFunction *pCallback, bool post)
{
	this->params = setup->params;
	this->offset = -1;
	this->returnFlag = setup->returnFlag;
	this->returnType = setup->returnType;
	this->post = post;
	this->plugin_callback = pCallback;
	this->entity = -1;
	this->thisType = setup->thisType;
	this->hookType = setup->hookType;
	this->m_pDetour = pDetour;
	this->callConv = setup->callConv;
}

HookReturnStruct *CDynamicHooksSourcePawn::GetReturnStruct()
{
	HookReturnStruct *res = new HookReturnStruct();
	res->isChanged = false;
	res->type = this->returnType;
	res->orgResult = NULL;
	res->newResult = NULL;

	if (this->post)
	{
		switch (this->returnType)
		{
		case ReturnType_String:
			res->orgResult = malloc(sizeof(string_t));
			res->newResult = malloc(sizeof(string_t));
			*(string_t *)res->orgResult = m_pDetour->GetReturnValue<string_t>();
			break;
		case ReturnType_Int:
			res->orgResult = malloc(sizeof(int));
			res->newResult = malloc(sizeof(int));
			*(int *)res->orgResult = m_pDetour->GetReturnValue<int>();
			break;
		case ReturnType_Bool:
			res->orgResult = malloc(sizeof(bool));
			res->newResult = malloc(sizeof(bool));
			*(bool *)res->orgResult = m_pDetour->GetReturnValue<bool>();
			break;
		case ReturnType_Float:
			res->orgResult = malloc(sizeof(float));
			res->newResult = malloc(sizeof(float));
			*(float *)res->orgResult = m_pDetour->GetReturnValue<float>();
			break;
		case ReturnType_Vector:
		{
			res->orgResult = malloc(sizeof(SDKVector));
			res->newResult = malloc(sizeof(SDKVector));
			SDKVector vec = m_pDetour->GetReturnValue<SDKVector>();
			*(SDKVector *)res->orgResult = vec;
			break;
		}
		default:
			res->orgResult = m_pDetour->GetReturnValue<void *>();
			break;
		}
	}
	else
	{
		switch (this->returnType)
		{
		case ReturnType_String:
			res->orgResult = malloc(sizeof(string_t));
			res->newResult = malloc(sizeof(string_t));
			*(string_t *)res->orgResult = NULL_STRING;
			break;
		case ReturnType_Vector:
			res->orgResult = malloc(sizeof(SDKVector));
			res->newResult = malloc(sizeof(SDKVector));
			*(SDKVector *)res->orgResult = SDKVector();
			break;
		case ReturnType_Int:
			res->orgResult = malloc(sizeof(int));
			res->newResult = malloc(sizeof(int));
			*(int *)res->orgResult = 0;
			break;
		case ReturnType_Bool:
			res->orgResult = malloc(sizeof(bool));
			res->newResult = malloc(sizeof(bool));
			*(bool *)res->orgResult = false;
			break;
		case ReturnType_Float:
			res->orgResult = malloc(sizeof(float));
			res->newResult = malloc(sizeof(float));
			*(float *)res->orgResult = 0.0;
			break;
		}
	}

	return res;
}

HookParamsStruct *CDynamicHooksSourcePawn::GetParamStruct()
{
	HookParamsStruct *params = new HookParamsStruct();
	params->dg = this;
	
	size_t paramsSize = this->m_pDetour->m_pCallingConvention->GetArgStackSize();
	std::vector<DataTypeSized_t> &argTypes = m_pDetour->m_pCallingConvention->m_vecArgTypes;
	int numArgs = argTypes.size();

	params->orgParams = (void **)malloc(paramsSize);
	params->newParams = (void **)malloc(paramsSize);
	params->isChanged = (bool *)malloc(numArgs * sizeof(bool));

	size_t offset = 0;
	for (int i = 0; i < numArgs; i++)
	{
		void *pArgPtr = m_pDetour->m_pCallingConvention->GetStackArgumentPtr(m_pDetour->m_pRegisters);
		memcpy(params->orgParams, pArgPtr, paramsSize);
		
		*(void **)((intptr_t)params->newParams + offset) = NULL;
		params->isChanged[i] = false;

		offset += argTypes[i].size;
	}

	return params;
}

void CDynamicHooksSourcePawn::UpdateParamsFromStruct(HookParamsStruct *params)
{
	// Function had no params to update now.
	if (!params)
		return;

	std::vector<DataTypeSized_t> &argTypes = m_pDetour->m_pCallingConvention->m_vecArgTypes;
	int numArgs = argTypes.size();

	int firstArg = 0;
	if (callConv == CallConv_THISCALL)
		firstArg = 1;

	size_t offset = 0;
	for (int i = 0; i < numArgs; i++)
	{
		int size = argTypes[i].size;;
		if (params->isChanged[i])
		{
			void *paramAddr = (void *)((intptr_t)params->newParams + offset);
			void *stackAddr = m_pDetour->m_pCallingConvention->GetArgumentPtr(i + firstArg, m_pDetour->m_pRegisters);
			memcpy(stackAddr, paramAddr, size);
		}
		offset += size;
	}
}
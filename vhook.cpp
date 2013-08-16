#include "vhook.h"
#include "vhook_macros.h"

SourceHook::IHookManagerAutoGen *g_pHookManager = NULL;

bool SetupHookManager(ISmmAPI *ismm)
{
	g_pHookManager = static_cast<SourceHook::IHookManagerAutoGen *>(ismm->MetaFactory(MMIFACE_SH_HOOKMANAUTOGEN, NULL, NULL));
	
	return g_pHookManager != NULL;
}

void CallFunction_Void(DHooksCallback<void> *info, unsigned long stack, void *iface)
{
	PassInfo *paramInfo = NULL;
	ICallWrapper *call;
	size_t size = 0;

	if(info->paramcount > 0)
	{
		paramInfo = (PassInfo *)malloc(sizeof(PassInfo) * info->paramcount);
		for(int i = 0; i < info->paramcount; i++)
		{
			switch(info->params[i].type)
			{
				PARAMINFO_SWITCH_CASE(int, HookParamType_Int ,PassType_Basic);
				PARAMINFO_SWITCH_CASE(bool, HookParamType_Bool ,PassType_Basic);
				PARAMINFO_SWITCH_CASE(float, HookParamType_Float ,PassType_Float);
				PARAMINFO_SWITCH_CASE(string_t, HookParamType_String ,PassType_Object);
				PARAMINFO_SWITCH_CASE(string_t *, HookParamType_StringPtr ,PassType_Basic);
				PARAMINFO_SWITCH_CASE(char *, HookParamType_CharPtr ,PassType_Basic);
				PARAMINFO_SWITCH_CASE(Vector *, HookParamType_VectorPtr ,PassType_Basic);
				PARAMINFO_SWITCH_CASE(CBaseEntity *, HookParamType_CBaseEntity ,PassType_Basic);
				PARAMINFO_SWITCH_CASE(edict_t *, HookParamType_Edict ,PassType_Basic);
				default:
					paramInfo[i].flags = info->params[i].flag;
					paramInfo[i].size = sizeof(void *);
					paramInfo[i].type = PassType_Basic;
					size += sizeof(void *);
					break;
			}
		}
		call = g_pBinTools->CreateVCall(info->offset, 0, 0, NULL, paramInfo, info->paramcount);
	}
	else
	{
		call = g_pBinTools->CreateVCall(info->offset, 0, 0, NULL, NULL, 0);
	}

	unsigned char *vstk = (unsigned char *)malloc(sizeof(void *) + size);
	unsigned char *vptr = vstk;

    *(void **)vptr = iface;
    vptr += sizeof(void *);

	size_t offset = STACK_OFFSET;

	if(info->paramcount > 0)
	{
		for(int i = 0; i < info->paramcount; i++)
		{
			switch(info->params[i].type)
			{
				VSTK_PARAM_SWITCH(int, HookParamType_Int);
				VSTK_PARAM_SWITCH(bool, HookParamType_Bool);
				VSTK_PARAM_SWITCH(float, HookParamType_Float);
				VSTK_PARAM_SWITCH(string_t, HookParamType_String);
				VSTK_PARAM_SWITCH(string_t *, HookParamType_StringPtr);
				VSTK_PARAM_SWITCH(char *, HookParamType_CharPtr);
				VSTK_PARAM_SWITCH(Vector *, HookParamType_VectorPtr);
				VSTK_PARAM_SWITCH(CBaseEntity *, HookParamType_CBaseEntity);
				VSTK_PARAM_SWITCH(edict_t *, HookParamType_Edict);
				default:
					*(void **)vptr = *(void **)(stack+offset);
					if(i + 1 != info->paramcount)
					{
						vptr += sizeof(void *);
					}
					offset += sizeof(void *);
					break;
			}
		}
	}
	call->Execute(vstk, NULL);
	call->Destroy();

	free(vstk);

	if(paramInfo)
		free(paramInfo);
}
template<class T>
T CallFunction(DHooksCallback<T> *info, unsigned long stack, void *iface)
{
	PassInfo *paramInfo = NULL;
	ICallWrapper *call;
	size_t size = 0;

	PassInfo returnInfo;
	returnInfo.flags = PASSFLAG_BYVAL;
	returnInfo.size = sizeof(T);
	returnInfo.type = PassType_Basic;

	if(info->paramcount > 0)
	{
		paramInfo = (PassInfo *)malloc(sizeof(PassInfo) * info->paramcount);
		for(int i = 0; i < info->paramcount; i++)
		{
			switch(info->params[i].type)
			{
				PARAMINFO_SWITCH_CASE(int, HookParamType_Int ,PassType_Basic);
				PARAMINFO_SWITCH_CASE(bool, HookParamType_Bool ,PassType_Basic);
				PARAMINFO_SWITCH_CASE(float, HookParamType_Float ,PassType_Float);
				PARAMINFO_SWITCH_CASE(string_t, HookParamType_String ,PassType_Object);
				PARAMINFO_SWITCH_CASE(string_t *, HookParamType_StringPtr ,PassType_Basic);
				PARAMINFO_SWITCH_CASE(char *, HookParamType_CharPtr ,PassType_Basic);
				PARAMINFO_SWITCH_CASE(Vector *, HookParamType_VectorPtr ,PassType_Basic);
				PARAMINFO_SWITCH_CASE(CBaseEntity *, HookParamType_CBaseEntity ,PassType_Basic);
				PARAMINFO_SWITCH_CASE(edict_t *, HookParamType_Edict ,PassType_Basic);
				default:
					paramInfo[i].flags = info->params[i].flag;
					paramInfo[i].size = sizeof(void *);
					paramInfo[i].type = PassType_Basic;
					size += sizeof(void *);
					break;
			}
		}
		call = g_pBinTools->CreateVCall(info->offset, 0, 0, &returnInfo, paramInfo, info->paramcount);
	}
	else
	{
		call = g_pBinTools->CreateVCall(info->offset, 0, 0, &returnInfo, NULL, 0);
	}

	unsigned char *vstk = (unsigned char *)malloc(sizeof(void *) + size);
	unsigned char *vptr = vstk;

    *(void **)vptr = iface;
    vptr += sizeof(void *);

	size_t offset = STACK_OFFSET;

	if(info->paramcount > 0)
	{
		for(int i = 0; i < info->paramcount; i++)
		{
			switch(info->params[i].type)
			{
				VSTK_PARAM_SWITCH(int, HookParamType_Int);
				VSTK_PARAM_SWITCH(bool, HookParamType_Bool);
				VSTK_PARAM_SWITCH(float, HookParamType_Float);
				VSTK_PARAM_SWITCH(string_t, HookParamType_String);
				VSTK_PARAM_SWITCH(string_t *, HookParamType_StringPtr);
				VSTK_PARAM_SWITCH(char *, HookParamType_CharPtr);
				VSTK_PARAM_SWITCH(Vector *, HookParamType_VectorPtr);
				VSTK_PARAM_SWITCH(CBaseEntity *, HookParamType_CBaseEntity);
				VSTK_PARAM_SWITCH(edict_t *, HookParamType_Edict);
				default:
					*(void **)vptr = *(void **)(stack+offset);
					if(i + 1 != info->paramcount)
					{
						vptr += sizeof(void *);
					}
					offset += sizeof(void *);
					break;
			}
		}
	}
	T ret;
	call->Execute(vstk, &ret);
	call->Destroy();

	free(vstk);

	if(paramInfo)
		free(paramInfo);

	return ret;
}
template <>
void DHooksCallback<void>::Call()
{
	GET_STACK;
	META_CONPRINTF("String %s\n", *(char **)(stack+STACK_OFFSET));
	strcpy(*(char **)(stack+STACK_OFFSET), "models/player/t_phoenix.mdl");
	
	SH_GLOB_SHPTR->DoRecall();
	CallFunction_Void(this, stack, g_SHPtr->GetIfacePtr());
	RETURN_META(MRES_IGNORED);
}
template <>
bool DHooksCallback<bool>::Call()
{
	GET_STACK;
	META_CONPRINTF("Entity %i\n", gamehelpers->ReferenceToIndex(gamehelpers->EntityToBCompatRef(*(CBaseEntity **)(stack+STACK_OFFSET))));
	/*__asm
	{
		mov ebp, stack
	};*/
	META_RES result = MRES_IGNORED;
	RETURN_META_VALUE(MRES_IGNORED, true);
	/*do
	{
		SH_GLOB_SHPTR->DoRecall();
		if ((result) >= MRES_OVERRIDE)
		{
			void *pOverride = g_SHPtr->GetOverrideRetPtr();
			pOverride = (void *)false;
		}
		if(result != MRES_SUPERCEDE)
		{
			RETURN_META_VALUE(MRES_SUPERCEDE, CallFunction<bool>(this, stack, g_SHPtr->GetIfacePtr()));
		}
		else
		{
			RETURN_META_VALUE(MRES_SUPERCEDE, true);
		}
	} while (0);*/
}
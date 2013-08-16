#include "extension.h"
#include "vhook.h"

DHooks g_Sample;		/**< Global singleton for extension's main interface */
SMEXT_LINK(&g_Sample);

IBinTools *g_pBinTools;
ISDKHooks *g_pSDKHooks;
ISDKTools *g_pSDKTools;

CBitVec<NUM_ENT_ENTRIES> m_EntityExists;

bool DHooks::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	sharesys->AddDependency(myself, "bintools.ext", true, true);
	sharesys->AddDependency(myself, "sdktools.ext", true, true);
	sharesys->AddDependency(myself, "sdkhooks.ext", true, true);

	sharesys->RegisterLibrary(myself, "dhooks");

	return true;
}
void DHooks::SDK_OnAllLoaded()
{
	SM_GET_LATE_IFACE(SDKTOOLS, g_pSDKTools);
	if(g_pSDKTools == NULL)
	{
		smutils->LogError(myself, "SDKTools interface not found. DHookGamerules native disabled.");
	}
	else if(g_pSDKTools->GetInterfaceVersion() < 2)
	{
		//<psychonic> THIS ISN'T DA LIMBO STICK. LOW IS BAD
		smutils->LogError(myself, "SDKTools interface is outdated. DHookGamerules native disabled.");
	}
	SM_GET_LATE_IFACE(BINTOOLS, g_pBinTools);
	SM_GET_LATE_IFACE(SDKHOOKS, g_pSDKHooks);
	g_pSDKHooks->AddEntityListener(this);
}
bool DHooks::SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlength, bool late)
{
	if(!SetupHookManager(ismm))
	{
		snprintf(error, maxlength, "Failed to get IHookManagerAutoGen iface");	
		return false;
	}
	return true;
}
DHooksManager *manager1;
DHooksManager *manager2;

#ifndef __linux__
#define WEAPON_CANUSE 258
#define SET_MODEL 24
#else
#define WEAPON_CANUSE 259
#define SET_MODEL 25
#endif

void DHooks::OnEntityCreated(CBaseEntity *pEntity, const char *classname)
{
	int entity = gamehelpers->ReferenceToIndex(gamehelpers->EntityToBCompatRef(pEntity));

	if(entity <= playerhelpers->GetMaxClients() && entity > 0)
	{			
		SourceHook::HookManagerPubFunc hook;
		SourceHook::CProtoInfoBuilder protoInfo(SourceHook::ProtoInfo::CallConv_ThisCall);
		protoInfo.AddParam(sizeof(CBaseEntity *), SourceHook::PassInfo::PassType_Basic, SourceHook::PassInfo::PassFlag_ByVal, NULL, NULL, NULL, NULL);
		protoInfo.SetReturnType(sizeof(bool), SourceHook::PassInfo::PassType_Basic, SourceHook::PassInfo::PassFlag_ByVal, NULL, NULL, NULL, NULL);
		hook = g_pHookManager->MakeHookMan(protoInfo, 0, WEAPON_CANUSE);
		manager1 = new DHooksManager();
		manager1->callback_bool = new DHooksCallback<bool>();
		manager1->callback_bool->paramcount = 1;
		manager1->callback_bool->params[0].size = sizeof(CBaseEntity *);
		manager1->callback_bool->params[0].type = HookParamType_CBaseEntity;
		manager1->callback_bool->params[0].flag = PASSFLAG_BYVAL;
		manager1->callback_bool->offset = WEAPON_CANUSE;
		manager1->hookid = g_SHPtr->AddHook(g_PLID, SourceHook::ISourceHook::Hook_Normal, pEntity, 0, hook, manager1->callback_bool, false);

		SourceHook::CProtoInfoBuilder protoInfo2(SourceHook::ProtoInfo::CallConv_ThisCall);
		protoInfo2.AddParam(sizeof(char *), SourceHook::PassInfo::PassType_Basic, SourceHook::PassInfo::PassFlag_ByVal, NULL, NULL, NULL, NULL);
		protoInfo2.SetReturnType(0, SourceHook::PassInfo::PassType_Unknown, 0, NULL, NULL, NULL, NULL);
		hook = g_pHookManager->MakeHookMan(protoInfo2, 0, SET_MODEL);
		manager2 = new DHooksManager();
		manager2->callback_void = new DHooksCallback<void>();
		manager2->callback_void->paramcount = 1;
		manager2->callback_void->params[0].size = sizeof(char *);
		manager2->callback_void->params[0].type = HookParamType_CharPtr;
		manager2->callback_void->params[0].flag = PASSFLAG_BYVAL;
		manager2->callback_void->offset = SET_MODEL;
		manager2->hookid = g_SHPtr->AddHook(g_PLID, SourceHook::ISourceHook::Hook_Normal, pEntity, 0, hook, manager2->callback_void, false);

		META_CONPRINTF("Hooked entity %i\n", entity);
	}
}
void DHooks::OnEntityDestroyed(CBaseEntity *pEntity)
{
	int entity = gamehelpers->ReferenceToIndex(gamehelpers->EntityToBCompatRef(pEntity));

	if(entity <= playerhelpers->GetMaxClients() && entity > 0)
	{
		META_CONPRINTF("Cleaning\n");
		delete manager1;
		delete manager2;
	}
}
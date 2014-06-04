#include "extension.h"
#include "vhook.h"
#include "listeners.h"
#include <macro-assembler-x86.h>

DHooks g_DHooksIface;		/**< Global singleton for extension's main interface */
SMEXT_LINK(&g_DHooksIface);

IBinTools *g_pBinTools;
ISDKHooks *g_pSDKHooks;
ISDKTools *g_pSDKTools;
DHooksEntityListener *g_pEntityListener = NULL;

HandleType_t g_HookSetupHandle = 0;
HandleType_t g_HookParamsHandle = 0;
HandleType_t g_HookReturnHandle = 0;

SH_DECL_HOOK0_void(IServerGameDLL, LevelShutdown, SH_NOATTRIB, false);

bool DHooks::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	HandleError err;
	g_HookSetupHandle = handlesys->CreateType("HookSetup", this, 0, NULL, NULL, myself->GetIdentity(), &err);
	if(g_HookSetupHandle == 0)
	{
		snprintf(error, maxlength, "Could not create hook setup handle type (err: %d)", err);	
		return false;
	}
	g_HookParamsHandle = handlesys->CreateType("HookParams", this, 0, NULL, NULL, myself->GetIdentity(), &err);
	if(g_HookParamsHandle == 0)
	{
		snprintf(error, maxlength, "Could not create hook params handle type (err: %d)", err);	
		return false;
	}
	g_HookReturnHandle = handlesys->CreateType("HookReturn", this, 0, NULL, NULL, myself->GetIdentity(), &err);
	if(g_HookReturnHandle == 0)
	{
		snprintf(error, maxlength, "Could not create hook return handle type (err: %d)", err);	
		return false;
	}

	sharesys->AddDependency(myself, "bintools.ext", true, true);
	sharesys->AddDependency(myself, "sdktools.ext", true, true);
	sharesys->AddDependency(myself, "sdkhooks.ext", true, true);

	sharesys->RegisterLibrary(myself, "dhooks");
	plsys->AddPluginsListener(this);
	sharesys->AddNatives(myself, g_Natives);

	g_pEntityListener = new DHooksEntityListener();

	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, LevelShutdown, gamedll, g_pEntityListener, &DHooksEntityListener::LevelShutdown, false);

	return true;
}

void DHooks::OnHandleDestroy(HandleType_t type, void *object)
{
	if(type == g_HookSetupHandle)
	{
		delete (HookSetup *)object;
	}
	else if(type == g_HookParamsHandle)
	{
		delete (HookParamsStruct *)object;
	}
	else if(type == g_HookReturnHandle)
	{
		delete (HookReturnStruct *)object;
	}
}

void DHooks::SDK_OnAllLoaded()
{
	SM_GET_LATE_IFACE(SDKTOOLS, g_pSDKTools);
	SM_GET_LATE_IFACE(BINTOOLS, g_pBinTools);
	SM_GET_LATE_IFACE(SDKHOOKS, g_pSDKHooks);

	g_pSDKHooks->AddEntityListener(g_pEntityListener);
}

void DHooks::SDK_OnUnload()
{
	CleanupHooks(NULL);
	if(g_pEntityListener)
	{
		g_pEntityListener->CleanupListeners(NULL);
		g_pSDKHooks->RemoveEntityListener(g_pEntityListener);
		SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, LevelShutdown, gamedll, g_pEntityListener, &DHooksEntityListener::LevelShutdown, false);
		delete g_pEntityListener;
	}
	plsys->RemovePluginsListener(this);
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

void DHooks::OnPluginUnloaded(IPlugin *plugin)
{
	CleanupHooks(plugin->GetBaseContext());
	if(g_pEntityListener)
	{
		g_pEntityListener->CleanupListeners(plugin->GetBaseContext());
	}
}
// The next 3 functions handle cleanup if our interfaces are going to be unloaded
bool DHooks::QueryRunning(char *error, size_t maxlength)
{
	SM_CHECK_IFACE(SDKTOOLS, g_pSDKTools);
	SM_CHECK_IFACE(BINTOOLS, g_pBinTools);
	SM_CHECK_IFACE(SDKHOOKS, g_pSDKHooks);
	return true;
}
/*
// The default for this one is *supposed* to be false already
bool DHooks::QueryInterfaceDrop(SMInterface *pInterface)
{
	if (strcmp(pInterface->GetInterfaceName(), SMINTERFACE_SDKTOOLS_NAME) == 0
		|| strcmp(pInterface->GetInterfaceName(), SMINTERFACE_BINTOOL_NAME) == 0)
		|| strcmp(pInterface->GetInterfaceName(), SMINTERFACE_SDKHOOKS_NAME) == 0)
	{
		return false;
	}

	return true;
}
*/
void DHooks::NotifyInterfaceDrop(SMInterface *pInterface)
{
	if (strcmp(pInterface->GetInterfaceName(), SMINTERFACE_SDKHOOKS_NAME) == 0)
	{
		if (g_pEntityListener)
		{
			// If this fails, remove this line and just delete the ent listener instead
			g_pSDKHooks->RemoveEntityListener(g_pEntityListener); 
			delete g_pEntityListener;
			g_pEntityListener = NULL;
		}
		g_pSDKHooks = NULL;
	}
	else if (strcmp(pInterface->GetInterfaceName(), SMINTERFACE_BINTOOLS_NAME) == 0)
	{
		g_pBinTools = NULL;
	}
	else if (strcmp(pInterface->GetInterfaceName(), SMINTERFACE_SDKTOOLS_NAME) == 0)
	{
		g_pSDKTools = NULL;
	}
}

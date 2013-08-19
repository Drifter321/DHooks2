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

bool g_bAllowGamerules = true;

HandleType_t g_HookSetupHandle = 0;
HandleType_t g_HookParamsHandle = 0;
HandleType_t g_HookReturnHandle = 0;

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

	if(g_pSDKTools->GetInterfaceVersion() < 2)
	{
		//<psychonic> THIS ISN'T DA LIMBO STICK. LOW IS BAD
		g_bAllowGamerules = false;
		smutils->LogError(myself, "SDKTools interface is outdated. DHookGamerules native disabled.");
	}

	SM_GET_LATE_IFACE(BINTOOLS, g_pBinTools);
	SM_GET_LATE_IFACE(SDKHOOKS, g_pSDKHooks);

	g_pEntityListener = new DHooksEntityListener();
	g_pSDKHooks->AddEntityListener(g_pEntityListener);
}

void DHooks::SDK_OnUnload()
{
	CleanupHooks(NULL);
	if(g_pEntityListener)
	{
		g_pEntityListener->CleanupListeners(NULL);
		g_pSDKHooks->RemoveEntityListener(g_pEntityListener);
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

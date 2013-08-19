#include "listeners.h"
#include "vhook.h"

using namespace SourceHook;

CUtlVector<EntityListener> g_EntityListeners;

void DHooksEntityListener::CleanupListeners(IPluginContext *pContext)
{
	for(int i = g_EntityListeners.Count() -1; i >= 0; i--)
	{
		if(pContext == NULL || pContext == g_EntityListeners.Element(i).callback->GetParentContext())
		{
			g_EntityListeners.Remove(i);
		}
	}
}

void DHooksEntityListener::OnEntityCreated(CBaseEntity *pEntity, const char *classname)
{
	int entity = gamehelpers->EntityToBCompatRef(pEntity);

	for(int i = g_EntityListeners.Count() -1; i >= 0; i--)
	{
		EntityListener listerner = g_EntityListeners.Element(i);
		if(listerner.type == ListenType_Created)
		{
			IPluginFunction *callback = listerner.callback;
			callback->PushCell(entity);
			callback->PushString(classname);
			callback->Execute(NULL);
		}
	}
}

void DHooksEntityListener::OnEntityDestroyed(CBaseEntity *pEntity)
{
	int entity = gamehelpers->EntityToBCompatRef(pEntity);

	for(int i = g_EntityListeners.Count() -1; i >= 0; i--)
	{
		EntityListener listerner = g_EntityListeners.Element(i);
		if(listerner.type == ListenType_Deleted)
		{
			IPluginFunction *callback = listerner.callback;
			callback->PushCell(gamehelpers->EntityToBCompatRef(pEntity));
			callback->Execute(NULL);
		}
	}

	for(int i = g_pHooks.Count() -1; i >= 0; i--)
	{
		DHooksManager *manager = g_pHooks.Element(i);
		if(manager->callback->entity == entity)
		{
			META_CONPRINT("Removing 1\n");
			delete manager;
			g_pHooks.Remove(i);
		}
	}
}
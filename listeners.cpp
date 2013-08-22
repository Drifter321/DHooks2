#include "listeners.h"
#include "vhook.h"

using namespace SourceHook;

CUtlVector<EntityListener> g_EntityListeners;

void DHooksEntityListener::LevelShutdown()
{
	for(int i = g_pHooks.Count() -1; i >= 0; i--)
	{
		DHooksManager *manager = g_pHooks.Element(i);
		if(manager->callback->hookType == HookType_GameRules)
		{
			delete manager;
			g_pHooks.Remove(i);
		}
	}
}

void DHooksEntityListener::CleanupListeners(IPluginContext *pContext)
{
	for(int i = g_EntityListeners.Count() -1; i >= 0; i--)
	{
		if(pContext == NULL || pContext == g_EntityListeners.Element(i).callback->GetParentRuntime()->GetDefaultContext())
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
			delete manager;
			g_pHooks.Remove(i);
		}
	}
}
bool DHooksEntityListener::AddPluginEntityListener(ListenType type, IPluginFunction *callback)
{
	for(int i = g_EntityListeners.Count() -1; i >= 0; i--)
	{
		EntityListener listerner = g_EntityListeners.Element(i);
		if(listerner.callback == callback && listerner.type == type)
		{
			return true;
		}
	}
	EntityListener listener;
	listener.callback = callback;
	listener.type = type;
	g_EntityListeners.AddToTail(listener);
	return true;
}
bool DHooksEntityListener::RemovePluginEntityListener(ListenType type, IPluginFunction *callback)
{
	for(int i = g_EntityListeners.Count() -1; i >= 0; i--)
	{
		EntityListener listerner = g_EntityListeners.Element(i);
		if(listerner.callback == callback && listerner.type == type)
		{
			g_EntityListeners.Remove(i);
			return true;
		}
	}
	return false;
}

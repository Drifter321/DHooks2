#ifndef _INCLUDE_FORWARDS_H_
#define _INCLUDE_FORWARDS_H_

#include "extension.h"
#include "vhook.h"

class DHooksEntityListener : public ISMEntityListener
{
public:
	void OnEntityCreated(CBaseEntity *pEntity, const char *classname);
	void OnEntityDestroyed(CBaseEntity *pEntity);
	void CleanupListeners(IPluginContext *func);
};

enum ListenType
{
	ListenType_Created,
	ListenType_Deleted
};

struct EntityListener
{
	ListenType type;
	IPluginFunction *callback;
};

extern CUtlVector<DHooksManager *> g_pHooks;
#endif

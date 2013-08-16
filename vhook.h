#ifndef _INCLUDE_VHOOK_H_
#define _INCLUDE_VHOOK_H_

#include "extension.h"
#include <sourcehook.h>

#define MAX_PARAMS 10
enum HookParamType
{
	HookParamType_Unknown,
	HookParamType_Int,
	HookParamType_Bool,
	HookParamType_Float,
	HookParamType_String,
	HookParamType_StringPtr,
	HookParamType_CharPtr,
	HookParamType_VectorPtr,
	HookParamType_CBaseEntity,
	HookParamType_ObjectPtr,
	HookParamType_Edict
};
enum ReturnType
{
	ReturnType_Unknown,
	ReturnType_Void,
	ReturnType_Int,
	ReturnType_Bool,
	ReturnType_Float,
	ReturnType_String,
	ReturnType_StringPtr,
	ReturnType_CharPtr,
	ReturnType_Vector,
	ReturnType_VectorPtr,
	ReturnType_CBaseEntity,
	ReturnType_Edict
};

class ParamInfo
{
public:
	HookParamType type;
	size_t size;
	unsigned int flag;
};

class DHooksInfo
{
public:
	int paramcount;
	ParamInfo params[MAX_PARAMS];
	int offset;
};

template <class T>
class DHooksCallback : public SourceHook::ISHDelegate, public DHooksInfo
{
public:
    virtual bool IsEqual(ISHDelegate *pOtherDeleg){return false;};
    virtual void DeleteThis(){delete this;};
    virtual T Call();
};
class DHooksManager
{
public:
	DHooksManager()
	{
		this->callback_bool = NULL; 
		this->callback_void = NULL;
		this->hookid = 0;
	};
	~DHooksManager()
	{
		if(this->hookid != 0)
		{
			SH_REMOVE_HOOK_ID(this->hookid);
		}
	}
	DHooksCallback<bool> *callback_bool;
	DHooksCallback<void> *callback_void;
	int hookid;
	unsigned int returnFlag;
};

bool SetupHookManager(ISmmAPI *ismm);
extern IBinTools *g_pBinTools;
#endif
#ifndef _INCLUDE_SIGNATURES_H_
#define _INCLUDE_SIGNATURES_H_

#include "extension.h"
#include "util.h"
#include <am-string.h>
#include <am-vector.h>
#include <sm_stringhashmap.h>

struct ArgumentInfo {
	ArgumentInfo() : name(nullptr)
	{ }

	ArgumentInfo(ke::AString name, ParamInfo info) : name(name), info(info)
	{ }

	ke::AString name;
	ParamInfo info;
};

class SignatureWrapper {
public:
	ke::AString signature;
	ke::AString address;
	ke::AString offset;
	ke::Vector<ArgumentInfo> args;
	CallingConvention callConv;
	HookType hookType;
	ReturnType retType;
	ThisPointerType thisType;
};

class SignatureGameConfig : public ITextListener_SMC {
public:
	SignatureWrapper *GetFunctionSignature(const char *function);
public:
	//ITextListener_SMC
	SMCResult ReadSMC_NewSection(const SMCStates *states, const char *name);
	SMCResult ReadSMC_KeyValue(const SMCStates *states, const char *key, const char *value);
	SMCResult ReadSMC_LeavingSection(const SMCStates *states);
	void ReadSMC_ParseStart();

private:
	ReturnType GetReturnTypeFromString(const char *str);
	HookParamType GetHookParamTypeFromString(const char *str);
	Register_t GetCustomRegisterFromString(const char *str);

private:
	StringHashMap<SignatureWrapper *> signatures_;
};

extern SignatureGameConfig *g_pSignatures;
#endif

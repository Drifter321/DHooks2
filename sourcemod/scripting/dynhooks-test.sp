#include <dhooks>
#include <sdktools>

// Uses Flashbang Tools gamedata
// https://forums.alliedmods.net/showthread.php?t=159876

Handle hFlashbangDetonateDetour;
Handle hFlashbangDeafen;
Handle hPercentageOfFlashForPlayer;

public void OnPluginStart()
{
	Handle temp = LoadGameConfigFile("fbtools.games");
	
	if(temp == INVALID_HANDLE)
	{
		SetFailState("Why you no has gamedata?");
	}
	
	hFlashbangDetonateDetour = DHookCreateDetour(Address_Null, CallConv_THISCALL, ReturnType_Void, ThisPointer_CBaseEntity);
	if (!hFlashbangDetonateDetour)
		SetFailState("Failed to setup detour for Detonate");
	
	if (!DHookSetFromConf(hFlashbangDetonateDetour, temp, SDKConf_Signature, "CFlashbangProjectile::Detonate"))
		SetFailState("Failed to load Detonate signature from gamedata");
	
	if (!DHookEnableDetour(hFlashbangDetonateDetour, false, Detour_OnFlashbangDetonate))
		SetFailState("Failed to detour Detonate.");
		
	if (!DHookEnableDetour(hFlashbangDetonateDetour, true, Detour_OnFlashbangDetonate_Post))
		SetFailState("Failed to detour Detonate post.");
	
	PrintToServer("CFlashbangProjectile::Detonate detoured!");
	

	hFlashbangDeafen = DHookCreateDetour(Address_Null, CallConv_THISCALL, ReturnType_Void, ThisPointer_CBaseEntity);
	if (!hFlashbangDeafen)
		SetFailState("Failed to setup detour for Deafen");
	
	if (!DHookSetFromConf(hFlashbangDeafen, temp, SDKConf_Signature, "Deafen"))
		SetFailState("Failed to load Deafen signature from gamedata");
	
	DHookAddParam(hFlashbangDeafen, HookParamType_Float);
	
	if (!DHookEnableDetour(hFlashbangDeafen, false, Detour_OnDeafen))
		SetFailState("Failed to detour Deafen.");
		
	if (!DHookEnableDetour(hFlashbangDeafen, true, Detour_OnDeafen_Post))
		SetFailState("Failed to detour Deafen post.");
	
	PrintToServer("CCSPlayer::Deafen detoured!");
	
	
	hPercentageOfFlashForPlayer = DHookCreateDetour(Address_Null, CallConv_CDECL, ReturnType_Float, ThisPointer_Ignore);
	if (!hPercentageOfFlashForPlayer)
		SetFailState("Failed to setup detour for PercentageOfFlashForPlayer");
	
	if (!DHookSetFromConf(hPercentageOfFlashForPlayer, temp, SDKConf_Signature, "PercentageOfFlashForPlayer"))
		SetFailState("Failed to load PercentageOfFlashForPlayer signature from gamedata");
	
	DHookAddParam(hPercentageOfFlashForPlayer, HookParamType_CBaseEntity);
	DHookAddParam(hPercentageOfFlashForPlayer, HookParamType_Object, 12);
	DHookAddParam(hPercentageOfFlashForPlayer, HookParamType_CBaseEntity);
	
	if (!DHookEnableDetour(hPercentageOfFlashForPlayer, false, Detour_OnPercentageOfFlashForPlayer))
		SetFailState("Failed to detour PercentageOfFlashForPlayer.");
	
	if (!DHookEnableDetour(hPercentageOfFlashForPlayer, true, Detour_OnPercentageOfFlashForPlayer_Post))
		SetFailState("Failed to detour PercentageOfFlashForPlayer post.");
	
	PrintToServer("PercentageOfFlashForPlayer detoured!");
}

public MRESReturn Detour_OnFlashbangDetonate(int pThis)
{
	PrintToServer("Detonate called on entity %d!", pThis);
	return MRES_Handled;
}

public MRESReturn Detour_OnFlashbangDetonate_Post(int pThis)
{
	PrintToServer("Detonate post called on entity %d!", pThis);
	return MRES_Handled;
}

public MRESReturn Detour_OnDeafen(int pThis, Handle hParams)
{
	PrintToServer("Deafen called on entity %d! distance: %f", pThis, DHookGetParam(hParams, 1));
	DHookSetParam(hParams, 1, 999.0);
	return MRES_ChangedHandled;
}

public MRESReturn Detour_OnDeafen_Post(int pThis, Handle hParams)
{
	PrintToServer("Deafen post called on entity %d! distance: %f", pThis, DHookGetParam(hParams, 1));
}

public MRESReturn Detour_OnPercentageOfFlashForPlayer(Handle hReturn, Handle hParams)
{
	float pos[3];
	DHookGetParamObjectPtrVarVector(hParams, 2, 0, ObjectValueType_Vector, pos);
	PrintToServer("PercentageOfFlashForPlayer called! entity1 %d, pos [%f %f %f], entity2 %d", DHookGetParam(hParams, 1), pos[0], pos[1], pos[2], DHookGetParam(hParams, 3));
	DHookSetReturn(hReturn, 0.3);
	return MRES_Supercede;
}

public MRESReturn Detour_OnPercentageOfFlashForPlayer_Post(Handle hReturn, Handle hParams)
{
	float pos[3];
	DHookGetParamObjectPtrVarVector(hParams, 2, 0, ObjectValueType_Vector, pos);
	PrintToServer("PercentageOfFlashForPlayer_Post called! entity1 %d, pos [%f %f %f], entity2 %d: %f", DHookGetParam(hParams, 1), pos[0], pos[1], pos[2], DHookGetParam(hParams, 3), DHookGetReturn(hReturn));
}
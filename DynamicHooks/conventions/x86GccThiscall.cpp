/**
* =============================================================================
* DynamicHooks
* Copyright (C) 2015 Robin Gohmert. All rights reserved.
* =============================================================================
*
* This software is provided 'as-is', without any express or implied warranty.
* In no event will the authors be held liable for any damages arising from 
* the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose, 
* including commercial applications, and to alter it and redistribute it 
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not 
* claim that you wrote the original software. If you use this software in a 
* product, an acknowledgment in the product documentation would be 
* appreciated but is not required.
*
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
*
* 3. This notice may not be removed or altered from any source distribution.
*
* asm.h/cpp from devmaster.net (thanks cybermind) edited by pRED* to handle gcc
* -fPIC thunks correctly
*
* Idea and trampoline code taken from DynDetours (thanks your-name-here).
*/

// ============================================================================
// >> INCLUDES
// ============================================================================
#include "x86GccThiscall.h"


// ============================================================================
// >> CLASSES
// ============================================================================

x86GccThiscall::x86GccThiscall(ke::Vector<DataTypeSized_t> &vecArgTypes, DataTypeSized_t returnType, int iAlignment) :
	x86GccCdecl(vecArgTypes, returnType, iAlignment)
{
	// Always add the |this| pointer.
	DataTypeSized_t type;
	type.type = DATA_TYPE_POINTER;
	type.size = GetDataTypeSize(type, iAlignment);
	type.custom_register = None;
	m_vecArgTypes.insert(0, type);
}

x86GccThiscall::~x86GccThiscall()
{
}

int x86GccThiscall::GetArgStackSize()
{
	// Remove the this pointer from the arguments size.
	DataTypeSized_t type;
	type.type = DATA_TYPE_POINTER;
	return x86GccCdecl::GetArgStackSize() - GetDataTypeSize(type, m_iAlignment);
}

void** x86GccThiscall::GetStackArgumentPtr(CRegisters* pRegisters)
{
	// Skip return address and this pointer.
	DataTypeSized_t type;
	type.type = DATA_TYPE_POINTER;
	return (void **)(pRegisters->m_esp->GetValue<unsigned long>() + 4 + GetDataTypeSize(type, m_iAlignment));
}

void x86GccThiscall::SavePostCallRegisters(CRegisters* pRegisters)
{
	uint8_t* pSavedThisPointer = new uint8_t[sizeof(size_t)];
	memcpy(pSavedThisPointer, GetArgumentPtr(0, pRegisters), sizeof(size_t));
	m_pSavedThisPointers.append(pSavedThisPointer);
}

void x86GccThiscall::RestorePostCallRegisters(CRegisters* pRegisters)
{
	uint8_t* pSavedThisPointer = m_pSavedThisPointers.back();
	memcpy(GetArgumentPtr(0, pRegisters), pSavedThisPointer, sizeof(size_t));
	m_pSavedThisPointers.pop();
}

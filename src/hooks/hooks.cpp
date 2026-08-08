#include "Hooks/hooks.h"

#include "LightArmor.h"
#include "Unarmed.h"
#include "Pickpocket.h"
#include "Alteration.h"
#include "OneHanded.h"
#include "TwoHanded.h"
#include "Block.h"

#include "RE/Offset.h"
#include <xbyak/xbyak.h>
#include <Windows.h>

namespace Hooks {
	bool Install() {
		logger::info("Installing hooks..."sv);
		//static constexpr std::size_t requiredHookAllocation = 14u; // 1 * 14 bytes for CALL.
		//std::size_t additionalAlloc = 0u;
		//SKSE::AllocTrampoline(requiredHookAllocation, additionalAlloc);
		SKSE::AllocTrampoline(249);

		bool result = true;

		InstallUpdateHook();
		InstallCombatHitHook();
		result &= LightArmor::InstallHooks();
		result &= Unarmed::InstallHooks();
		result &= Pickpocket::InstallHooks();
		result &= Alteration::InstallHooks();
		result &= Block::InstallHooks();

		logger::info("Finished installing hooks."sv);
		return result;
	}

	void InstallUpdateHook()
	{
		auto vtbl = REL::Relocation<std::uintptr_t>(RE::Offset::PlayerCharacter::Vtbl);
		_Update = vtbl.write_vfunc(173, &Update);
	}

	void Update(RE::PlayerCharacter* a_player, float a_delta)
	{
		_Update(a_player, a_delta);
		LightArmor::ProcessUpdate(a_player, a_delta);
		Alteration::ProcessUpdate(a_player, a_delta);
	}

	void InstallCombatHitHook()
	{
		auto& trampoline = SKSE::GetTrampoline();
		REL::Relocation<std::uintptr_t> target{ RE::Offset::Actor::CombatHit, 0x4A8 };
		_CombatHit = trampoline.write_call<5>(target.address(), &ProcessCombatHit);
	}

	void ProcessCombatHit(RE::Actor* a_this, RE::HitData* a_hitData)
	{
		_CombatHit(a_this, a_hitData);
		Unarmed::ProcessCombatHit(a_this, a_hitData);
		OneHanded::ProcessCombatHit(a_this, a_hitData);
		TwoHanded::ProcessCombatHit(a_this, a_hitData);
		Block::ProcessCombatHit(a_this, a_hitData);
	}

	
}
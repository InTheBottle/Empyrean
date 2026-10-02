#include "LightArmor.h"
#include "Utils/ArmorUtils.h"
#include "Data/Lookup.h"
#include "Data/ModObjectManager.h"
#include "RE/Offset.h"
#include <xbyak/xbyak.h>
#include <Windows.h>
#include "Settings/INI/INISettings.h"

namespace Hooks
{
	bool LightArmor::InstallHooks() {
		logger::info("  Installing LightArmor Hooks..."sv);
		InstallSprintingCostHook();
		return true;
	}

	void LightArmor::ProcessUpdate(RE::PlayerCharacter* a_player, float a_delta)
	{
		auto givePassiveXP = Settings::INI::GetSetting<bool>(Settings::INI::ENABLE_LIGHTARMOR_PASSIVE_XP).value_or(false);
		if (!givePassiveXP)
			return;

		//Give player LightArmor XP if moving while wearing at least 2 pieces of light armor; more XP during combat
		if (a_player->IsOnMount()) { return; }
		int armorCount = Utils::ArmorUtils::GetCountEquippedLightArmor(a_player, false);
		if (armorCount < 2) { return; }

		auto SkillXPLightAmorBaseRate = Settings::INI::GetSetting<float>(Settings::INI::SKILL_XP_LIGHTARMOR_BASE_RATE).value_or(0.0);
		auto SkillXPLightAmorGearCountAdd = Settings::INI::GetSetting<float>(Settings::INI::SKILL_XP_LIGHTARMOR_GEAR_COUNT_ADD).value_or(0.0);
		auto SkillXPLightAmorCombatMult = Settings::INI::GetSetting<float>(Settings::INI::SKILL_XP_LIGHTARMOR_COMBAT_MULT).value_or(0.0);
		auto SkillXPLightAmorSprintMult = Settings::INI::GetSetting<float>(Settings::INI::SKILL_XP_LIGHTARMOR_SPRINT_MULT).value_or(0.0);
		//logger::info("  {}, {}, {}, {}"sv, SkillXPLightAmorBaseRate, SkillXPLightAmorGearCountAdd, SkillXPLightAmorCombatMult, SkillXPLightAmorSprintMult);

		if (a_player->IsSprinting() || a_player->IsRunning()) {
			float baseRate = SkillXPLightAmorBaseRate;
			float armorCountAdd = SkillXPLightAmorGearCountAdd * (armorCount - 2);
			float combatMult = a_player->IsInCombat() ? SkillXPLightAmorCombatMult : 0.0f;
			float sprintMult = a_player->IsSprinting() ? SkillXPLightAmorSprintMult : 1.0f;
			float skillUse = a_delta * (baseRate + armorCountAdd) * combatMult * sprintMult;
			a_player->AddSkillExperience(RE::ActorValue::kLightArmor, skillUse);
		}
	}

	void LightArmor::InstallSprintingCostHook()
	{
		auto hook = REL::Relocation<std::uintptr_t>(RE::Offset::Actor::UpdateSprinting, 0xCE);
		if (!REL::make_pattern<"0F 57 05">().match(hook.address())) {
			logger::error("    > Sprinting cost hook did not find the expected instruction at {:#x}, skipping"sv, hook.address());
			return;
		}

		// TRAMPOLINE: 17
		struct Patch : Xbyak::CodeGenerator
		{
			Patch() : Xbyak::CodeGenerator(17, SKSE::GetTrampoline().allocate(17))
			{
				mov(rdx, rdi);
				jmp(ptr[rip]);
				dq(std::bit_cast<std::uintptr_t>(&LightArmor::CalcSprintingStaminaMod));
			}
		};

		auto& trampoline = SKSE::GetTrampoline();
		REL::safe_fill(hook.address(), REL::NOP, 0x7);
		trampoline.write_call<5>(hook.address(), Patch().getCode());
	}

	float LightArmor::CalcSprintingStaminaMod(float a_cost, const RE::Actor* a_actor)
	{
		float cost = a_cost;

		if (const auto perk = Data::ModObject<RE::BGSPerk>("PerkSprintReduceCost"sv);
			perk && a_actor->HasPerk(perk) && Utils::ArmorUtils::GetCountEquippedLightArmor(a_actor, true) >= 4) {
			float costMult = "SprintReduceCostMult"_gv.value_or(0.5f);
			cost *= costMult;
		}

		cost = (std::max)(cost, 0.0f);

		// xorps xmm0, -1.0
		return -cost;
	}
}
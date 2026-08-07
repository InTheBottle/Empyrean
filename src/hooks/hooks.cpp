#include "Hooks/hooks.h"

#include "LightArmor.h"
#include "Unarmed.h"
#include "Pickpocket.h"
#include "Alteration.h"
#include "OneHanded.h"
#include "TwoHanded.h"
#include "Destruction.h"
#include "Block.h"

#include "RE/Offset.h"
#include <xbyak/xbyak.h>
#include <Windows.h>

namespace Hooks {
	bool Install() {
		logger::info("Installing hooks..."sv);
		static constexpr std::size_t requiredHookAllocation = 14u; // 1 * 14 bytes for CALL.
		std::size_t additionalAlloc = 0u;

		//SKSE::AllocTrampoline(requiredHookAllocation, additionalAlloc);
		SKSE::AllocTrampoline(249);

		bool result = true;

		InstallUpdateHook();
		InstallCombatHitHook();
		//InstallMagicEffectAddedHooks();
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

	/*
	void ProcessSpellsForPatching()
	{
		//Destruction::LoadData();

		RE::TESDataHandler* dataHandler = RE::TESDataHandler::GetSingleton();
		if (!dataHandler) return;

		auto spells = dataHandler->GetFormArray<RE::SpellItem>();
		for (auto* spell : spells)
		{
			if (!spell) continue;
			auto effs = spell->effects;
			for (auto* eff : effs)
			{
				if (!eff) continue;

				auto magEff = eff->baseEffect;
				if (!magEff) continue;

				Destruction::PatchSpell(spell, eff, magEff);
			}
		}
	}
	*/

	void InstallMagicEffectAddedHooks()
	{
		auto target = REL::Relocation<std::uintptr_t>(RE::Character::VTABLE[4]);
		_MagicEffectAddedActor = target.write_vfunc(0x08, &ProcessMagicEffectAddedActor);

		target = REL::Relocation<std::uintptr_t>(RE::PlayerCharacter::VTABLE[4]);
		_MagicEffectAddedPlayer = target.write_vfunc(0x08, &ProcessMagicEffectAddedPlayer);
		
	}

	void ProcessMagicEffectAddedActor(RE::MagicTarget* a_target, RE::ActiveEffect* a_effect)
	{
		_MagicEffectAddedActor(a_target, a_effect);
		ProcessMagicEffectAdded(a_target, a_effect);
	}
	void ProcessMagicEffectAddedPlayer(RE::MagicTarget* a_target, RE::ActiveEffect* a_effect)
	{
		
		_MagicEffectAddedPlayer(a_target, a_effect);
		ProcessMagicEffectAdded(a_target, a_effect);
	}

	void ProcessMagicEffectAdded(RE::MagicTarget* a_target, RE::ActiveEffect* a_effect)
	{
		//Skip if no target, target isn't an actor, or if we can't fetch the base magEff for some reason
		if (!a_target || !a_effect) return;
		if (!a_target->MagicTargetIsActor()) return;
		auto targetActor = a_target->GetTargetAsActor();
		if (!targetActor) return;
		auto magEff = a_effect->GetBaseObject();
		if (!magEff) return;

		//if (targetActor->IsPlayerRef())
		//	logger::info("ProcessMagicEffectAdded({}) for Player"sv, magEff->GetFormID());
		//else
		//	logger::info("ProcessMagicEffectAdded({}) for {}"sv, magEff->GetFormID(), targetActor->GetFormID());

		//Do we need to check for effects that were absorbed? Or do they just not get added to begin with?

		//Skip if no caster, or if caster isn't an actor
		auto caster = a_effect->GetCasterActor() ? a_effect->GetCasterActor().get() : nullptr;
		if (!caster)
		{
			//logger::info("     > Skipped: Caster is missing or not an actor"sv);
		//	return;
		}

		auto magItem = a_effect->spell;
		if (!magItem)
		{
			//logger::info("     > Skipped: MagEff is not from a MagicItem (spell, enchantment, etc)"sv);
			return;
		}

		Destruction::ProcessMagicEffectAdded_DES(targetActor, caster, a_effect, magItem);
		return;
		
		/*
		//Exclude everything that isn't a spell - SpellMastery lesser power just casts the spell remotely
		//TODO If SpellMastery is changed so that the spell itself is altered via DLL to become a lesser power, this check will need to update
		auto spell = magItem->As<RE::SpellItem>();
		if (!spell)
		{
			//logger::info("     > Skipped: MagEff is not from a SpellItem"sv);
			return;
		}
		auto spellType = spell->GetSpellType();
		if (spellType != RE::MagicSystem::SpellType::kSpell)
		{
			//logger::info("     > Skipped: SpellItem is not 'Spell' type"sv);
			return;
		}

	
		//Only trigger for highest cost effect
		auto highestCostMagEff = spell->GetCostliestEffectItem();
		if (!highestCostMagEff)
		{
			//logger::info("     > Skipped: Could not find highest cost effect for spell...?"sv);
			return;
		}
		if (magEff != highestCostMagEff->baseEffect)
		{
			//logger::info("     > Skipped: MagEff is not the highest cost effect for this spell"sv);
			return;
		}

		Destruction::ProcessMagicEffectAdded_DES(targetActor, caster, a_effect, spell);
		return;
		*/

		/*
		//Check for spell casting perk
		//TODO Handle hazard spells like WallOfFire -> grab their enchantment from their hazard
		if (!spell->data.castingPerk)
		{
			logger::info("     > Skipped: Parent spell has no casting perk"sv);
			return;
		}
		
		//Check for empty name, description, or set HideInUI
		auto magEffName = magEff->GetName();
		if (!magEffName || magEffName == "")
		{
			logger::info("     > Skipped: MagEff is the highest cost effect but has no name, probably isn't from a spell we should patch"sv);
			return;
		}
		auto magEffDesc = magEff->magicItemDescription;
		if (magEffDesc.empty())
		{
			logger::info("     > Skipped: MagEff is the highest cost effect but has no description, probably isn't from a spell we should patch"sv);
			return;
		}
		if (magEff->data.flags.all(RE::EffectSetting::EffectSettingData::Flag::kHideInUI))
		{
			logger::info("     > Skipped: MagEff is the highest cost effect but is HideInUI, probably isn't from a spell we should patch"sv);
			return;
		}

		
		*/
	}
}
#include "Alteration.h"
#include "Data/Lookup.h"
#include "Data/ModObjectManager.h"
#include "RE/Offset.h"
#include "Settings/INI/INISettings.h"

namespace
{
	thread_local RE::Actor* s_castingActor = nullptr;

	struct CastingScope
	{
		explicit CastingScope(RE::ActorMagicCaster* a_caster) :
			previous(s_castingActor)
		{
			s_castingActor = a_caster ? a_caster->actor : nullptr;
		}

		~CastingScope()
		{
			s_castingActor = previous;
		}

		RE::Actor* previous;
	};
}

namespace Hooks
{
	bool Alteration::InstallHooks()
	{
		logger::info("  Installing Alteration Hooks..."sv);

		auto enableBloodRitualHealthCasting = Settings::INI::GetSetting<bool>(Settings::INI::ENABLE_BLOOD_RITUAL_HEALTH_CASTING).value_or(true);
		if (enableBloodRitualHealthCasting)
		{
			REL::Relocation<std::uintptr_t> casterVtbl{ RE::VTABLE_ActorMagicCaster[0] };
			_InterruptCastImpl = casterVtbl.write_vfunc(0x08, &InterruptCastImpl);
			_SpellCast = casterVtbl.write_vfunc(0x09, &SpellCast);
			_CheckCast = casterVtbl.write_vfunc(0x0A, &CheckCast);
			_Update = casterVtbl.write_vfunc(REL::Relocate(0x1D, 0x1D, 0x1F), &Update);

			REL::Relocation<std::uintptr_t> getAssociatedResource{ RELOCATION_ID(33817, 34609) };
			SKSE::GetTrampoline().write_branch<5>(getAssociatedResource.address(), &GetAssociatedResource);
			logger::info("    > Installed hooks for BloodRitual"sv);
		}

		return true;
	}

	bool Alteration::UsesHealthForMagicka(RE::Actor* a_actor, RE::MagicItem* a_spell)
	{
		if (!a_actor || !a_spell || !a_actor->IsPlayerRef()) return false;
		if (a_spell->GetSpellType() != RE::MagicSystem::SpellType::kSpell) return false;
		return "PoE_GLO_ALT_BloodRitualActive"_gv.value_or(0.0f) > 0.0f;
	}

	RE::ActorValue Alteration::GetAssociatedResource(RE::MagicItem* a_item, RE::MagicSystem::CastingSource a_source)
	{
		using SpellType = RE::MagicSystem::SpellType;

		if (!a_item) return RE::ActorValue::kNone;

		switch (a_item->GetSpellType()) {
		case SpellType::kSpell:
			return UsesHealthForMagicka(s_castingActor, a_item) ? RE::ActorValue::kHealth : RE::ActorValue::kMagicka;
		case SpellType::kLesserPower:
		case SpellType::kPoison:
			return RE::ActorValue::kMagicka;
		case SpellType::kEnchantment:
		case SpellType::kStaffEnchantment:
			if (a_item->GetCastingType() == RE::MagicSystem::CastingType::kConstantEffect) return RE::ActorValue::kNone;
			return a_source == RE::MagicSystem::CastingSource::kLeftHand ? RE::ActorValue::kLeftItemCharge : RE::ActorValue::kRightItemCharge;
		default:
			return RE::ActorValue::kNone;
		}
	}

	bool Alteration::CheckCast(RE::ActorMagicCaster* a_caster, RE::MagicItem* a_spell, bool a_dualCast, float* a_effectStrength, RE::MagicSystem::CannotCastReason* a_reason, bool a_useBaseValueForCost)
	{
		CastingScope scope(a_caster);
		const bool result = _CheckCast(a_caster, a_spell, a_dualCast, a_effectStrength, a_reason, a_useBaseValueForCost);
		if (!result || !a_caster || !a_caster->actor) return result;

		auto* spell = a_spell ? a_spell : a_caster->currentSpell;
		if (!UsesHealthForMagicka(a_caster->actor, spell)) return result;
		if (spell->GetCastingType() != RE::MagicSystem::CastingType::kConcentration) return result;
		if (a_caster->flags.any(RE::ActorMagicCaster::Flags::kSkipCheckCast) || RE::PlayerCharacter::IsGodMode()) return result;

		const float health = a_caster->actor->AsActorValueOwner()->GetActorValue(RE::ActorValue::kHealth);
		if (health > MIN_HEALTH_WHILE_CONCENTRATING) return result;

		if (a_reason) *a_reason = RE::MagicSystem::CannotCastReason::kMagicka;
		return false;
	}

	void Alteration::SpellCast(RE::ActorMagicCaster* a_caster, bool a_doCast, std::uint32_t a_arg2, RE::MagicItem* a_spell)
	{
		CastingScope scope(a_caster);
		_SpellCast(a_caster, a_doCast, a_arg2, a_spell);
	}

	void Alteration::InterruptCastImpl(RE::ActorMagicCaster* a_caster, bool a_refund)
	{
		CastingScope scope(a_caster);
		_InterruptCastImpl(a_caster, a_refund);
	}

	void Alteration::Update(RE::ActorMagicCaster* a_caster, float a_delta)
	{
		CastingScope scope(a_caster);
		_Update(a_caster, a_delta);
	}

	void Alteration::ProcessUpdate(RE::PlayerCharacter* a_player, float a_delta)
	{
		//============== BLOOD MAGE ==============
		const auto perkBloodMage1 = Data::ModObject<RE::BGSPerk>("PerkBloodMage1"sv);
		const auto perkBloodMage2 = Data::ModObject<RE::BGSPerk>("PerkBloodMage2"sv);
		const auto gloPrevHPModAmt = Data::ModObject<RE::TESGlobal>("GlobalBloodMagePrevHPAmt"sv);
		bool isBloodRitualActive = "PoE_GLO_ALT_BloodRitualActive"_gv.value_or(false);

		float prevHPModAmt = gloPrevHPModAmt->value;

		if (isBloodRitualActive)
		{
			float curHP = a_player->AsActorValueOwner()->GetActorValue(RE::ActorValue::kHealth);
			float maxHP = a_player->As<RE::Actor>()->GetActorValueMax(RE::ActorValue::kHealth);
			float maxMag = a_player->As<RE::Actor>()->GetActorValueMax(RE::ActorValue::kMagicka);

			//Used for Blood to Power scaling
			float missingHP = maxHP - curHP;
			a_player->AsActorValueOwner()->SetActorValue(RE::ActorValue::kFame, missingHP); 

			//Update player HP based on max magicka
			float hpModAmt = a_player->HasPerk(perkBloodMage2) ? maxMag : maxMag / 2.0;
			if (hpModAmt > 0.0)
			{
				float delta = hpModAmt - prevHPModAmt;
				if (delta > 0.01 || delta < -0.01)
				{
					a_player->AsActorValueOwner()->ModActorValue(RE::ACTOR_VALUE_MODIFIER::kPermanent, RE::ActorValue::kHealth, delta);
					gloPrevHPModAmt->value = prevHPModAmt + delta;
					//logger::info("BloodMage - Updated HP"sv);
					//logger::info("     > maxHP = {}, maxMag = {}"sv, maxHP, maxMag);
					//logger::info("     > prevHPModAmt = {}, deltaHP = {}"sv, prevHPModAmt, delta);
				}
			}
		}
		else
		{
			a_player->AsActorValueOwner()->ModActorValue(RE::ACTOR_VALUE_MODIFIER::kPermanent, RE::ActorValue::kHealth, -prevHPModAmt);
			gloPrevHPModAmt->value = 0.0;
			a_player->AsActorValueOwner()->SetActorValue(RE::ActorValue::kFame, 0.0); 
		}
	}
}

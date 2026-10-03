#include "Alteration.h"
#include "Data/Lookup.h"
#include "Data/ModObjectManager.h"
#include "RE/Offset.h"

namespace Hooks
{
	bool Alteration::InstallHooks()
	{
		return true;
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
#include "OneHanded.h"
#include "Data/Lookup.h"
#include "Data/ModObjectManager.h"
#include "RE/Offset.h"
#include "Settings/INI/INISettings.h"

namespace Hooks
{
	void OneHanded::ProcessCombatHit(RE::Actor* a_this, RE::HitData* a_hitData)
	{
		const auto aggressor = a_hitData->aggressor.get();
		const auto victim = a_hitData->target.get();

		if (!aggressor)
			return;
		if (!victim || victim->IsDead())
			return;

		if (!a_hitData->weapon)
			return;
		
		//if (a_hitData->flags.all(RE::HitData::Flag::kBlocked))
		//	return;

		//Not a 1H bladed weapon
		const auto KeywordWeapBladed1H = Data::ModObject<RE::BGSKeyword>("KeywordWeapBladed1H"sv);
		if (!a_hitData->weapon->HasKeyword(KeywordWeapBladed1H))
		{
			//logger::info("AThousandCuts - Weapon is not 1H bladed, aborting..."sv);
			return;
		}
		
		const auto perkAThousandCuts = Data::ModObject<RE::BGSPerk>("PerkAThousandCuts"sv);
		const auto PerkBleedOut = Data::ModObject<RE::BGSPerk>("PerkBleedOut"sv);
		const auto KeywordWeapBleed = Data::ModObject<RE::BGSKeyword>("KeywordWeapBleed"sv);
		//auto maxBleedDur = Settings::INI::GetSetting<float>(Settings::INI::THOUSAND_CUTS_MAX_BLEED_DUR).value_or(0.0);

		/*
		//Perk check
		if (!aggressor->HasPerk(perkAThousandCuts))
		{
			//logger::info("AThousandCuts - Aggressor does not have perk AThousandCuts, aborting..."sv);
			return;
		}
		*/
		
		int bleedStackCount = 0;
		bool wasPowerAttack = a_hitData->flags.all(RE::HitData::Flag::kPowerAttack);
		auto activeBleedEffs = std::vector<RE::ActiveEffect*>{};

		const auto activeEffects = victim->AsMagicTarget()->GetActiveEffectList();
		if (!activeEffects)
		{
			//logger::info("AThousandCuts - No active magicEffects found, aborting..."sv);
			return;
		}

		//logger::info("AThousandCuts - Trying to increase bleed duration (max {})"sv, maxBleedDur);
		//int index = 0;
		float maxElapsedTime = 0.0;
		RE::ActiveEffect* oldestBleedEff = nullptr;

		for (const auto& activeEffect : *activeEffects) {
			if (auto mgef = activeEffect ? activeEffect->GetBaseObject() : nullptr; mgef) {
				if (activeEffect->flags.all(RE::ActiveEffect::Flag::kDispelled))
				{
					//logger::info("     > Active effect is dispelled?"sv);
					continue;
				}
				if (mgef->HasKeyword(KeywordWeapBleed))
				{
					//logger::info("     > Active effect has bleed keyword!"sv);
					bleedStackCount += 1;
					activeBleedEffs.push_back(activeEffect);

					float time = activeEffect->elapsedSeconds;
					if (time >= maxElapsedTime)
					{
						maxElapsedTime = time;
						oldestBleedEff = activeEffect;
					}

					/*
					float newDur = activeEffect->duration + 1.0;
					if (newDur > maxBleedDur && maxBleedDur > activeEffect->duration)
						newDur = maxBleedDur;

					activeEffect->duration = newDur;
					//logger::info("     > Found active bleed effect #{} -> newDur = {}"sv, index, activeEffect->duration);
					
					activeBleedEffs.push_back(activeEffect);
					//index += 1;
					*/
				}
			}
		}

		//Don't stack up bleeding unless you have the AThousandCuts perk
		if (!aggressor->HasPerk(perkAThousandCuts) && activeBleedEffs.size() > 1 && oldestBleedEff)
		{
			oldestBleedEff->Dispel(true);
			//logger::info("     > OneHanded: Dispelled oldeset bleed stack"sv);
		}
		
		//Was power attack, try to apply BleedOut perk
		if (aggressor->HasPerk(PerkBleedOut) && wasPowerAttack)
		{
			int minBleedStacks = "BleedOutMinStacks"_gv.value_or(5);

			logger::info("BleedOut - Hit with power attack (req. {})!"sv, minBleedStacks);
			if (bleedStackCount <= 0)
			{
				//logger::info("     > Bleed stack count is 0, aborting..."sv);
				return;
			}
			
			if (bleedStackCount >= minBleedStacks)
			{
				logger::info("     > Min bleed stack count reached, doing combust"sv);
				int index = 0;
				float totalDmg = 0.0;
				for (const auto& bleedEffect : activeBleedEffs)
				{
					float remainingDmg = bleedEffect->magnitude * (bleedEffect->duration - bleedEffect->elapsedSeconds);
					//logger::info("     > Bleed stack #{}: Duration = {}, ElapsedSeconds = {}, Magnitude = {}, RemainingDmg = {}"sv, index, bleedEffect->duration, bleedEffect->elapsedSeconds, bleedEffect->magnitude, remainingDmg);
					totalDmg += remainingDmg;

					bleedEffect->Dispel(true);
					index += 1;
				}
				//logger::info("     > Total damage from all bleed stacks = {}"sv, totalDmg);
				
				//VFX spell, which will also trigger Battlemaster on death
				const auto SpellBleedOut = Data::ModObject<RE::SpellItem>("SpellBleedOut"sv);
				aggressor->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)->CastSpellImmediate(SpellBleedOut, false, victim->As<RE::Actor>(), 1.0f, false, 0.0f, aggressor->As<RE::Actor>());
			
				//Deal remaning bleeding damage to HP
				victim->AsActorValueOwner()->DamageActorValue(RE::ActorValue::kHealth, totalDmg);
			}
			
		}
	}
}
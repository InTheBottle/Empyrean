#include "Pickpocket.h"
#include "Data/Lookup.h"
#include "Data/ModObjectManager.h"
#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"
#include <unordered_set>
using namespace RE;

namespace Hooks
{
    inline std::unordered_set<RE::TESRace*> g_modifiedRaces;
    using ActivateSignature = bool(TESObjectREFR*, TESObjectREFR*, std::uint8_t, TESBoundObject*, std::int32_t, bool);
    REL::Relocation<ActivateSignature> activate_original;

    TESRace* GetRefRace(TESObjectREFR* ref) {
        if (!ref) {
            return nullptr;
        }
        auto actor = ref->As<RE::Actor>();
        if (!actor) {
            return nullptr;
        }
        return actor->GetRace();
    }

    bool IsRaceModified(TESRace* race) {
        return race && g_modifiedRaces.contains(race);
    }

    void Pickpocket::SetRacesAllowPickpocket() {
        TESDataHandler* dataHandler = TESDataHandler::GetSingleton();
        if (dataHandler) {
            auto player = PlayerCharacter::GetSingleton();
            const auto perkAllowPP1 = Data::ModObject<RE::BGSPerk>("PerkDaringFingersmith1"sv);
            const auto perkAllowPP2 = Data::ModObject<RE::BGSPerk>("PerkDaringFingersmith2"sv);
            const auto listRacesAllowPP1 = Data::ModObject<RE::BGSListForm>("ListRacesAllowPickpocketRank1"sv);
            const auto kRaceAllowPP1 = Data::ModObject<RE::BGSKeyword>("KeywordRaceAllowPickpocketRank1"sv);

            bool hasRank1 = player->HasPerk(perkAllowPP1);
            bool hasRank2 = player->HasPerk(perkAllowPP2);

            auto races = dataHandler->GetFormArray<TESRace>();
            for (auto* race : races) {
                if (!race) continue;
                if (race->IsChildRace()) continue;

                if (!hasRank1 && !hasRank2)
                {
                    if (IsRaceModified(race)) {
                        race->data.flags.reset(RACE_DATA::Flag::kAllowPickpocket);
                    }
                }
                else if (!race->data.flags.all(RE::RACE_DATA::Flag::kAllowPickpocket))
                {
                    if (hasRank2 || listRacesAllowPP1->HasForm(race) || race->HasKeyword(kRaceAllowPP1))
                    {
                        g_modifiedRaces.insert(race);
                        race->data.flags.set(RACE_DATA::Flag::kAllowPickpocket);
                    }
                }
                
            }
            if (!hasRank1 && !hasRank2)
            {
                g_modifiedRaces.clear();
                logger::info("Disabled pickpocketing for modified races.");
            }
            else
                logger::info("Enabled pickpocketing for {} races.", g_modifiedRaces.size());
           
        }
        
    }

    //Bugfix copied from Pickpocket Everyone SKSE
    bool ActivateHook(TESObjectREFR* a_targetRef, TESObjectREFR* a_activatorRef, std::uint8_t a_arg3,
        TESBoundObject* a_object, std::int32_t a_targetCount, bool a_defaultProcessingOnly) {
        auto player = PlayerCharacter::GetSingleton();
        if (player && a_activatorRef == player && player->IsSneaking()) {
            auto targetRace = GetRefRace(a_targetRef);
            if (IsRaceModified(targetRace)) {
                logger::info("Prevented script activation from triggering pickpocketing for {} without player input.", a_targetRef->GetDisplayFullName());
                targetRace->data.flags.reset(RACE_DATA::Flag::kAllowPickpocket);
                auto returnValue = activate_original(a_targetRef, a_activatorRef, a_arg3, a_object, a_targetCount, a_defaultProcessingOnly);
                targetRace->data.flags.set(RACE_DATA::Flag::kAllowPickpocket);
                return returnValue;
            }
        }
        return activate_original(a_targetRef, a_activatorRef, a_arg3, a_object, a_targetCount, a_defaultProcessingOnly);
    }

    //---------------------------------------------------------------
	bool Pickpocket::InstallHooks()
	{
		//logger::info("  Installing Pickpocket Hooks..."sv);
		return true;
	}

	void Pickpocket::InstallActivateHook()
	{
		logger::info("  Installing Pickpocket Activate Hook..."sv);

        if (REL::Module::IsVR()) {
            logger::info("    > Skipped on Skyrim VR"sv);
            return;
        }

        auto& trampoline = SKSE::GetTrampoline();
        REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(55610, 56139), 0x4A };
        if (!util::is_call_site(target.address())) {
            logger::error("    > Activate hook did not find a call at {:#x}, skipping"sv, target.address());
            return;
        }
        activate_original = trampoline.write_call<5>(target.address(), &ActivateHook);

    }

}
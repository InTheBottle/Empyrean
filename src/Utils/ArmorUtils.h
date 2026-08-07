#pragma once

namespace Utils
{
	struct ArmorUtils
	{
		static std::vector<RE::TESObjectARMO*> GetEquippedArmor(const RE::Actor* a_actor);
		static int GetCountEquippedLightArmor(const RE::Actor* a_actor, bool allowSubstituteHelmet);
		static bool HasEquppedLightArmorGauntlets(const RE::Actor* a_actor);
		static bool HasEquppedHeavyArmorGauntlets(const RE::Actor* a_actor);
		static bool HasEquippedLightShield(const RE::Actor* a_actor);
	};
}
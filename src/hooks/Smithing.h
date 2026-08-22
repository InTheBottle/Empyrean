#pragma once

namespace Hooks
{
	class Smithing final
	{
	public:
		static void LoadData();
		static bool CreateStasisCubeFromAutomaton(RE::Actor* automaton);
		static void FixAutomatonPotionsInContainer(RE::TESObjectREFR* contRef);
	private:
		static void CreatePotionFromEffects(RE::EffectSetting* mainSpawnMagEff, RE::BSTArray<RE::Effect> effects, std::string modelName, float weight);
		static inline RE::BGSKeyword* KeywordCraftingAutomaton;
		static inline RE::BGSKeyword* KeywordCraftingSmithingForge;
		static inline RE::BGSKeyword* KeywordVendorItemClutter;
		static inline RE::BGSKeyword* KeywordVendorItemTool;
		static inline RE::BGSKeyword* KeywordAutomaton;
		static inline RE::BGSKeyword* KeywordFabricant;
		static inline RE::BGSKeyword* KeywordAether;
		static inline RE::BGSListForm* ListRaceKeywords;
		static inline RE::BGSListForm* ListRaceWeights;
		static inline RE::BGSListForm* ListFabricantActors;
		static inline RE::BGSListForm* ListFabricantWeights;
		static inline RE::BGSListForm* ListActiveUpgradeMagEffs;
		static inline RE::BGSListForm* ListSpawnRaceMagEffs;
		static inline RE::BGSListForm* ListSpawnRaceMagEffsAether;
		static inline RE::BGSListForm* ListSpawnRaceMagEffsFabricant;
		static inline RE::BGSListForm* ListSpawnUpgradeMagEffs;
		static inline RE::EffectSetting* MagEffSpawnUpgradeStart;
		static inline RE::EffectSetting* MagEffSpawnUpgradeSpacer;
		static inline RE::EffectSetting* MagEffSpawnUpgradeEnd;
		static inline bool loaded;
	};
}

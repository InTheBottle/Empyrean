#pragma once

namespace Settings
{
	namespace INI
	{
		bool Read();

		class Holder :
			public REX::Singleton<Holder>
		{
		public:
			bool StoreSettings();
			void DumpSettings();

			template <typename T>
			std::optional<T> GetStoredSetting(const std::string& a_settingName) {
				if constexpr (std::is_same_v<T, float>) {
					auto it = floatSettings.find(a_settingName);
					if (it != floatSettings.end()) return it->second;
				}
				else if constexpr (std::is_same_v<T, std::string>) {
					auto it = stringSettings.find(a_settingName);
					if (it != stringSettings.end()) return it->second;
				}
				else if constexpr (std::is_same_v<T, long>) {
					auto it = longSettings.find(a_settingName);
					if (it != longSettings.end()) return it->second;
				}
				else if constexpr (std::is_same_v<T, bool>) {
					auto it = boolSettings.find(a_settingName);
					if (it != boolSettings.end()) return it->second;
				}
				else {
					static_assert(always_false<T>, "Called GetStoredSetting with unsupported type.");
				}
				return std::nullopt;
			}

		private:
			std::map<std::string, long>        longSettings;
			std::map<std::string, bool>        boolSettings;
			std::map<std::string, float>       floatSettings;
			std::map<std::string, std::string> stringSettings;

			bool OverrideSettings();
		};

		inline static constexpr const char* ENABLE_LIGHTARMOR_PASSIVE_XP = "LightArmor|bGainPassiveLightArmorXP";
		inline static constexpr const char* SKILL_XP_LIGHTARMOR_BASE_RATE = "LightArmor|fSkillXPLightAmorBaseRate";
		inline static constexpr const char* SKILL_XP_LIGHTARMOR_GEAR_COUNT_ADD = "LightArmor|fSkillXPLightAmorGearCountAdd";
		inline static constexpr const char* SKILL_XP_LIGHTARMOR_SPRINT_MULT = "LightArmor|fSkillXPLightAmorSprintMult";
		inline static constexpr const char* SKILL_XP_LIGHTARMOR_COMBAT_MULT = "LightArmor|fSkillXPLightAmorCombatMult";
		inline static constexpr const char* ENABLE_LIGHTARMOR_XP_FROM_UNARMED = "Unarmed|bGetLightArmorXPFromUnarmedAttacks";
		inline static constexpr const char* ENABLE_HEAVYARMOR_XP_FROM_UNARMED = "Unarmed|bGetHeavyArmorXPFromUnarmedAttacks";
		inline static constexpr const char* SKILL_XP_FROM_UNARMED_MULT = "Unarmed|fSkillXPFromUnarmedMult";
		inline static constexpr const char* ENABLE_UNARMED_SPEED_FIX = "Unarmed|bEnableUnarmedWeapSpeedMultFix";
		inline static constexpr const char* ENABLE_BASH_STAMINA_REDUCTION = "Block|bEnableBashStaminaReduction";
		inline static constexpr const char* ENABLE_BASH_SPELL_REFLECTION = "Block|bEnableBashSpellReflection";
		inline static constexpr const char* ENABLE_BASH_DESTROY_ARROW = "Block|bEnableBashDestroyArrow";
		inline static constexpr const char* SKILL_XP_BLOCK_REFLECT_SPELL = "Block|fSkillXPBlockReflectSpell";
		inline static constexpr const char* SKILL_XP_BLOCK_DESTROY_ARROW = "Block|fSkillXPBlockDestroyArrow";

		inline static constexpr const std::uint8_t EXPECTED_COUNT = 14;

		inline static constexpr const std::array<const char*, EXPECTED_COUNT> EXPECTED_SETTINGS = {
			ENABLE_LIGHTARMOR_PASSIVE_XP,
			SKILL_XP_LIGHTARMOR_BASE_RATE,
			SKILL_XP_LIGHTARMOR_GEAR_COUNT_ADD,
			SKILL_XP_LIGHTARMOR_SPRINT_MULT,
			SKILL_XP_LIGHTARMOR_COMBAT_MULT,
			ENABLE_LIGHTARMOR_XP_FROM_UNARMED,
			ENABLE_HEAVYARMOR_XP_FROM_UNARMED,
			SKILL_XP_FROM_UNARMED_MULT,
			ENABLE_UNARMED_SPEED_FIX,
			ENABLE_BASH_STAMINA_REDUCTION,
			ENABLE_BASH_SPELL_REFLECTION,
			SKILL_XP_BLOCK_REFLECT_SPELL,
			ENABLE_BASH_DESTROY_ARROW,
			SKILL_XP_BLOCK_DESTROY_ARROW
		};

		template <typename T>
		std::optional<T> GetSetting(const std::string& a_settingName) {
			auto* holder = Holder::GetSingleton();
			return holder ? holder->GetStoredSetting<T>(a_settingName) : std::nullopt;
		}
	}
}
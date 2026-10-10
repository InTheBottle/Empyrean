#pragma once

namespace Hooks
{
	class Alteration final
	{
	public:
		static bool InstallHooks();
		static void ProcessUpdate(RE::PlayerCharacter* a_player, float a_delta);

	private:
		static bool UsesHealthForMagicka(RE::Actor* a_actor, RE::MagicItem* a_spell);

		static RE::ActorValue GetAssociatedResource(RE::MagicItem* a_item, RE::MagicSystem::CastingSource a_source);
		static RE::MagicSystem::CannotCastReason GetAssociatedResourceReason(RE::MagicItem* a_item, RE::MagicSystem::CastingSource a_source);

		static bool CheckCast(RE::ActorMagicCaster* a_caster, RE::MagicItem* a_spell, bool a_dualCast, float* a_effectStrength, RE::MagicSystem::CannotCastReason* a_reason, bool a_useBaseValueForCost);
		static void SpellCast(RE::ActorMagicCaster* a_caster, bool a_doCast, std::uint32_t a_arg2, RE::MagicItem* a_spell);
		static void InterruptCastImpl(RE::ActorMagicCaster* a_caster, bool a_refund);
		static void Update(RE::ActorMagicCaster* a_caster, float a_delta);

		static void InstallCastFailureHook(REL::RelocationID a_function, std::ptrdiff_t a_offset);
		static void CastFailure(RE::ActorMagicCaster* a_caster, RE::MagicSystem::CannotCastReason a_reason);
		static std::string NotEnoughHealthText();

		static inline REL::Relocation<decltype(&CheckCast)> _CheckCast;
		static inline REL::Relocation<decltype(&SpellCast)> _SpellCast;
		static inline REL::Relocation<decltype(&InterruptCastImpl)> _InterruptCastImpl;
		static inline REL::Relocation<decltype(&Update)> _Update;
		static inline REL::Relocation<decltype(&CastFailure)> _CastFailure;

		static inline constexpr float MIN_HEALTH_WHILE_CONCENTRATING = 5.0f;
	};
}

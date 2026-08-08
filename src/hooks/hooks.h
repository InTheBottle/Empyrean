#pragma once

namespace Hooks {
	bool Install();

	static void InstallUpdateHook();
	static void Update(RE::PlayerCharacter* a_player, float a_delta);
	inline static REL::Relocation<decltype(&Update)> _Update;

	static void InstallCombatHitHook();
	static void ProcessCombatHit(RE::Actor* a_this, RE::HitData* a_hitData);
	static inline REL::Relocation<decltype(&ProcessCombatHit)> _CombatHit;

}
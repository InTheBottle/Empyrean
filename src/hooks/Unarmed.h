#pragma once

namespace Hooks
{
	class Unarmed final
	{
	public:
		static bool InstallHooks();

		static void ProcessCombatHit(RE::Actor* a_this, RE::HitData* a_hitData);

	private:
		// Use unarmed weapon as equipped weapon for speed mult
		static void WeaponSpeedMultPatch();
	};
}
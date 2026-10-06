#pragma once

namespace Hooks
{
	class Lockpicking final
	{
	public:
		static bool InstallHooks();
		static void LoadData();

	private:
		static void DamagePick(RE::LockpickingMenu* a_menu);
		static inline REL::Relocation<decltype(&DamagePick)> _damagePick;

		static inline RE::BGSPerk* perkTrialAndError;
	};
}

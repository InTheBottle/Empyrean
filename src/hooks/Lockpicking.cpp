#include "Lockpicking.h"
#include "Data/ModObjectManager.h"

namespace Hooks
{
	bool Lockpicking::InstallHooks()
	{
		logger::info("  Installing Lockpicking Hooks..."sv);

		// The call offset differs between SE/AE/VR, so find RotateLock's call to DamagePick instead of hardcoding it
		const auto rotateLock = RELOCATION_ID(51096, 51978).address();
		const auto damagePick = RELOCATION_ID(51093, 51975).address();
		for (auto addr = rotateLock; addr < rotateLock + 0x400; ++addr) {
			if (util::is_call_site(addr) && addr + 5 + *reinterpret_cast<std::int32_t*>(addr + 1) == damagePick) {
				_damagePick = SKSE::GetTrampoline().write_call<5>(addr, &DamagePick);
				logger::info("    > Installed hook for RememberLockpickAngle"sv);
				return true;
			}
		}

		logger::error("    > RememberLockpickAngle hook did not find the DamagePick call, skipping"sv);
		return true;
	}

	void Lockpicking::LoadData()
	{
		perkRememberLockpickAngle = Data::ModObject<RE::BGSPerk>("PerkRememberLockpickAngle"sv);
	}

	void Lockpicking::DamagePick(RE::LockpickingMenu* a_menu)
	{
		// Breaking the pick is the only thing in DamagePick that writes pickAngle (resets it to 0)
		const auto pickAngle = a_menu->GetRuntimeData().pickAngle;
		_damagePick(a_menu);

		const auto player = RE::PlayerCharacter::GetSingleton();
		if (perkRememberLockpickAngle && player && player->HasPerk(perkRememberLockpickAngle)) {
			a_menu->GetRuntimeData().pickAngle = pickAngle;
		}
	}
}

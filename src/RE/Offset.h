#pragma once

namespace RE::Offset
{
	namespace Actor
	{
		inline constexpr REL::RelocationID CombatHit(37673, 38627);
		inline constexpr REL::RelocationID Jump(36271, 37257);
		inline constexpr REL::RelocationID UpdateSprinting(36994, 38022);
	}

	namespace PlayerCharacter
	{
		inline constexpr REL::VariantID Vtbl = RE::VTABLE_PlayerCharacter[0];
	}

	inline constexpr REL::RelocationID HandleWeaponSpeedChannel(37386, 42779);
	inline constexpr REL::RelocationID HandleLeftWeaponSpeedChannel(37378, 42780);

	typedef RE::TESObjectREFR* (_fastcall* _getEquippedShield)(RE::Actor* a_actor);
	inline static REL::Relocation<_getEquippedShield> getEquippedShield{ RELOCATION_ID(37624, 38577) };

	typedef void(_fastcall* _destroyProjectile)(RE::Projectile* a_projectile);
	inline static REL::Relocation<_destroyProjectile> destroyProjectile{ RELOCATION_ID(42930, 44110) };
}
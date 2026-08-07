#pragma once

namespace RE::Offset
{
	namespace Example
	{
		constexpr auto FunctionName = REL::ID(55976);
	}

	namespace Actor
	{
		constexpr auto ActorValueModifiedCallbacks = REL::ID(403905);
		constexpr auto CheckAbsorb = REL::ID(38741);
		constexpr auto CombatHit = REL::ID(38627);
		constexpr auto ComputeMovementType = REL::ID(37943);
		constexpr auto ForceUpdateCachedMovementType = REL::ID(37941);
		constexpr auto Jump = REL::ID(37257);
		constexpr auto UpdateCommandedActor = REL::ID(38799);
		constexpr auto UpdateSprinting = REL::ID(38022);
	}

	namespace PlayerCharacter
	{
		inline constexpr REL::ID Vtbl(static_cast<std::uint64_t>(208040));
	}

	constexpr auto HandleWeaponSpeedChannel = REL::ID(42779);
	constexpr auto HandleLeftWeaponSpeedChannel = REL::ID(42780);

	typedef RE::TESObjectREFR* (_fastcall* _getEquippedShield)(RE::Actor* a_actor);
	inline static REL::Relocation<_getEquippedShield> getEquippedShield{ RELOCATION_ID(37624, 38577) };

	typedef void(_fastcall* _destroyProjectile)(RE::Projectile* a_projectile);
	inline static REL::Relocation<_destroyProjectile> destroyProjectile{ RELOCATION_ID(42930, 44110) };
}
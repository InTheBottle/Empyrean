#pragma once

namespace Hooks
{
	class Block final
	{
	public:
		static bool InstallHooks();
		static void LoadData();
		static void ProcessCombatHit(RE::Actor* a_this, RE::HitData* a_hitData);
		

	private:
		static float GetAttackStaminaCost(RE::ActorValueOwner* avOwner, RE::BGSAttackData* atkData);
		static inline REL::Relocation<decltype(&GetAttackStaminaCost)> _getAttackStaminaCost;

		static void OnArrowCollision(RE::Projectile* a_this, RE::hkpAllCdPointCollector* a_AllCdPointCollector);
		static void OnMissileCollision(RE::Projectile* a_this, RE::hkpAllCdPointCollector* a_AllCdPointCollector);
		static inline REL::Relocation<decltype(OnArrowCollision)> _arrowCollision;
		static inline REL::Relocation<decltype(OnMissileCollision)> _missileCollision;
		static bool TryBashReflectSpell(RE::Projectile* a_projectile, RE::hkpAllCdPointCollector* a_AllCdPointCollector);
		static bool TryBashDestroyArrow(RE::Projectile* a_projectile, RE::hkpAllCdPointCollector* a_AllCdPointCollector);
		static bool processProjectileParry(RE::Actor* a_blocker, RE::Projectile* a_projectile, RE::hkpCollidable* a_projectile_collidable);
		static bool DoTryBashDestroyArrow(RE::Actor* a_actor, RE::Projectile* a_projectile);

		static void SetRotationMatrix(RE::NiMatrix3& a_matrix, float sacb, float cacb, float sb);
		static void resetProjectileOwner(RE::Projectile* a_projectile, RE::Actor* a_actor, RE::hkpCollidable* a_projectile_collidable);
		static bool ApproximatelyEqual(float A, float B);
		static bool PredictAimProjectile(RE::NiPoint3 a_projectilePos, RE::NiPoint3 a_targetPosition, RE::NiPoint3 a_targetVelocity, float a_gravity, RE::NiPoint3& a_projectileVelocity);
		static void ReflectProjectile(RE::Projectile* a_projectile);
		static void getBodyPos(RE::Actor* a_actor, RE::NiPoint3& pos);
		static void RetargetProjectile(RE::Projectile* a_projectile, RE::TESObjectREFR* a_target);

		static inline float _parryAngle;
		static inline float _skillXPReflectSpell;
		static inline float _skillXPDestroyArrow;
		static inline RE::BGSPerk* perkMirrorWall;
		static inline RE::BGSPerk* perkRebound;
		static inline RE::BGSPerk* perkDeflectArrows;
		static inline RE::BGSPerk* perkSkirmishersTarge;
		static inline RE::SpellItem* spellDestroyArrowVFX;
		static inline RE::SpellItem* spellBashReflectSpellVFX;
#define PI 3.1415926535897932384626f

	};
}

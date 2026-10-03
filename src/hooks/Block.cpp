#include "Block.h"
#include "Utils/ArmorUtils.h"
#include "Data/ModObjectManager.h"
#include "Settings/INI/INISettings.h"
#include "RE/Offset.h"

namespace Hooks
{
	bool Block::InstallHooks()
	{
		logger::info("  Installing Block Hooks..."sv);

		auto enableBashStaminaReduction = Settings::INI::GetSetting<bool>(Settings::INI::ENABLE_BASH_STAMINA_REDUCTION).value_or(false);
		if (enableBashStaminaReduction)
		{
			auto& trampoline = SKSE::GetTrampoline();
			REL::Relocation<std::uintptr_t> hook{ RELOCATION_ID(37650, 38603), REL::VariantOffset(0x16E, 0x171, 0x16E) };
			if (util::is_call_site(hook.address())) {
				_getAttackStaminaCost = trampoline.write_call<5>(hook.address(), &GetAttackStaminaCost);
				logger::info("    > Installed hook for SkirmishersTarge"sv);
			}
			else {
				logger::error("    > SkirmishersTarge hook did not find a call at {:#x}, skipping"sv, hook.address());
			}
		}

		auto enableBashSpellReflection = Settings::INI::GetSetting<bool>(Settings::INI::ENABLE_BASH_SPELL_REFLECTION).value_or(false);
		if (enableBashSpellReflection)
		{
			REL::Relocation<std::uintptr_t> missileProjectileVtbl{ RE::VTABLE_MissileProjectile[0] };
			_missileCollision = missileProjectileVtbl.write_vfunc(REL::Relocate(190, 190, 191), OnMissileCollision);
			logger::info("    > Installed hook for MirrorWall"sv);
		}

		auto enableBashDestroyArrow = Settings::INI::GetSetting<bool>(Settings::INI::ENABLE_BASH_DESTROY_ARROW).value_or(false);
		if (enableBashDestroyArrow)
		{
			REL::Relocation<std::uintptr_t> arrowProjectileVtbl{ RE::VTABLE_ArrowProjectile[0] };
			_arrowCollision = arrowProjectileVtbl.write_vfunc(REL::Relocate(190, 190, 191), OnArrowCollision);
			logger::info("    > Installed hook for ReflectArrows"sv);
		}
		
		return true;
	}

	void Block::LoadData()
	{
		_parryAngle = RE::GameSettingCollection::GetSingleton()->GetSetting("fCombatHitConeAngle")->GetFloat();
		_skillXPReflectSpell = Settings::INI::GetSetting<float>(Settings::INI::SKILL_XP_BLOCK_REFLECT_SPELL).value_or(0.0);
		_skillXPDestroyArrow = Settings::INI::GetSetting<float>(Settings::INI::SKILL_XP_BLOCK_DESTROY_ARROW).value_or(0.0);

		perkMirrorWall = Data::ModObject<RE::BGSPerk>("PerkMirrorWall"sv);
		perkRebound = Data::ModObject<RE::BGSPerk>("PerkRebound"sv);
		perkDeflectArrows = Data::ModObject<RE::BGSPerk>("DeflectArrows"sv);
		perkSkirmishersTarge = Data::ModObject<RE::BGSPerk>("PerkSkirmishersTarge"sv);

		spellBashReflectSpellVFX = Data::ModObject<RE::SpellItem>("SpellVFXBashReflectSpell"sv);
		spellDestroyArrowVFX = Data::ModObject<RE::SpellItem>("SpellVFXDestroyArrow"sv);
	}

	//----------------------------------------------------------------------------------------------------------------
	void Block::ProcessCombatHit(RE::Actor* a_this, RE::HitData* a_hitData)
	{
		const auto aggressor = a_hitData->aggressor.get().get();
		const auto victim = a_hitData->target.get().get();

		if (!aggressor)
			return;
		if (!victim || victim->IsDead())
			return;

		if (!a_hitData->weapon)
			return;

		if (!a_hitData->flags.all(RE::HitData::Flag::kBlocked))
			return;

		if (!RE::Offset::getEquippedShield(victim))
			return;

		if (!victim->HasPerk(perkRebound)) return;

		//logger::info("Rebound: physicalDamage={}, totalDamage={} ", a_hitData->physicalDamage, a_hitData->totalDamage);
		aggressor->AsActorValueOwner()->DamageActorValue(RE::ActorValue::kHealth, a_hitData->physicalDamage * 0.1);
	}

	//----------------------------------------------------------------------------------------------------------------
	float Block::GetAttackStaminaCost(RE::ActorValueOwner* avOwner, RE::BGSAttackData* atkData)
	{
		if (atkData->data.flags.any(RE::AttackData::AttackFlag::kBashAttack)) {
			//logger::info("Bash attack!");

			const auto* a_actor = skyrim_cast<const RE::Actor*>(avOwner);

			if (a_actor && a_actor->HasPerk(perkSkirmishersTarge) && Utils::ArmorUtils::HasEquippedLightShield(a_actor)) {
				float baseStamina = _getAttackStaminaCost(avOwner, atkData);
				//logger::info(" > Base stamina cost = {}, New stamina cost = {}", baseStamina, baseStamina / 2.0);
				return baseStamina / 2.0;
			}
			
		}
		return _getAttackStaminaCost(avOwner, atkData);
	}

	//----------------------------------------------------------------------------------------------------------------
	void Block::OnArrowCollision(RE::Projectile* a_this, RE::hkpAllCdPointCollector* a_AllCdPointCollector)
	{
		//logger::info("Arrow collision!");
		if (TryBashDestroyArrow(a_this, a_AllCdPointCollector)) {
			return;
		};
		_arrowCollision(a_this, a_AllCdPointCollector);
	}

	bool Block::TryBashDestroyArrow(RE::Projectile* a_projectile, RE::hkpAllCdPointCollector* a_AllCdPointCollector)
	{
		if (!a_AllCdPointCollector || !a_projectile) return false;
		
		for (auto& hit : a_AllCdPointCollector->hits) {
			auto refrA = RE::TESHavokUtilities::FindCollidableRef(*hit.rootCollidableA);
			auto refrB = RE::TESHavokUtilities::FindCollidableRef(*hit.rootCollidableB);

			if (refrA && refrA->formType == RE::FormType::ActorCharacter)
			{
				auto* actorA = refrA->As<RE::Actor>();
				if (actorA && DoTryBashDestroyArrow(actorA, a_projectile)) {
					return true;
				}
			}
			if (refrB && refrB->formType == RE::FormType::ActorCharacter)
			{
				auto* actorB = refrB->As<RE::Actor>();
				if (actorB && DoTryBashDestroyArrow(actorB, a_projectile)) {
					return true;
				}
			}
		}

		return false;
	}

	bool Block::DoTryBashDestroyArrow(RE::Actor* a_actor, RE::Projectile* a_projectile)
	{
		if (!a_actor) return false;

		if (a_actor && (a_actor->AsActorState()->GetAttackState() == RE::ATTACK_STATE_ENUM::kBash ||
			a_actor->AsActorState()->GetAttackState() == RE::ATTACK_STATE_ENUM::kSwing ||
			a_actor->AsActorState()->GetAttackState() == RE::ATTACK_STATE_ENUM::kHit ||
			a_actor->AsActorState()->GetAttackState() == RE::ATTACK_STATE_ENUM::kFollowThrough)) {
			//logger::info("  > In bash/attack state...");
			if (a_actor->IsPlayerRef() && a_actor->HasPerk(perkDeflectArrows) && !RE::Offset::getEquippedShield(a_actor)) {
				//logger::info("  > Trying to destroy arrow...");
				RE::Offset::destroyProjectile(a_projectile);
				a_actor->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)->CastSpellImmediate(spellDestroyArrowVFX, false, a_actor->As<RE::Actor>(), 1.0f, false, 0.0f, a_actor->As<RE::Actor>());
				RE::PlayerCharacter::GetSingleton()->AddSkillExperience(RE::ActorValue::kBlock, _skillXPDestroyArrow);
				return true;
			}
		}

		return false;
	}

	//----------------------------------------------------------------------------------------------------------------
	void Block::OnMissileCollision(RE::Projectile* a_this, RE::hkpAllCdPointCollector* a_AllCdPointCollector)
	{
		//logger::info("Missile collision!");
		if (TryBashReflectSpell(a_this, a_AllCdPointCollector)) {
			return;
		};
		_missileCollision(a_this, a_AllCdPointCollector);
	}

	bool Block::TryBashReflectSpell(RE::Projectile* a_projectile, RE::hkpAllCdPointCollector* a_AllCdPointCollector)
	{
		if (!a_AllCdPointCollector || !a_projectile) return false;
		if (!a_projectile->GetProjectileRuntimeData().spell) return false;

		for (auto& hit : a_AllCdPointCollector->hits) {
			auto refrA = RE::TESHavokUtilities::FindCollidableRef(*hit.rootCollidableA);
			auto refrB = RE::TESHavokUtilities::FindCollidableRef(*hit.rootCollidableB);

			if (refrA && refrA->formType == RE::FormType::ActorCharacter)
			{
				auto* actorA = refrA->As<RE::Actor>();
				if (actorA && actorA->AsActorState()->GetAttackState() == RE::ATTACK_STATE_ENUM::kBash) {
					if (refrA->IsPlayerRef() && actorA->HasPerk(perkMirrorWall) && Utils::ArmorUtils::HasEquippedLightShield(actorA)) {
						//logger::info(" > A: Trying to parry projectile...");
						return processProjectileParry(actorA, a_projectile, const_cast<RE::hkpCollidable*>(hit.rootCollidableB));
					}
				}
			}

			if (refrB && refrB->formType == RE::FormType::ActorCharacter)
			{
				auto* actorB = refrB->As<RE::Actor>();
				if (actorB && actorB->AsActorState()->GetAttackState() == RE::ATTACK_STATE_ENUM::kBash) {
					if (refrB->IsPlayerRef() && actorB->HasPerk(perkMirrorWall) && Utils::ArmorUtils::HasEquippedLightShield(actorB)) {
						//logger::info(" > B: Trying to parry projectile...");
						return processProjectileParry(actorB, a_projectile, const_cast<RE::hkpCollidable*>(hit.rootCollidableA));
					}
				}
			}
		}
		
		return false;
	}

	
	bool Block::processProjectileParry(RE::Actor* a_parrier, RE::Projectile* a_projectile, RE::hkpCollidable* a_projectile_collidable)
	{
		if (!a_parrier || !a_projectile || !a_projectile_collidable) return false;

		auto angle = a_parrier->GetHeadingAngle(a_projectile->GetPosition(), false);
		if (angle <= _parryAngle && angle >= -_parryAngle) {
			RE::TESObjectREFR* shooter = nullptr;
			if (a_projectile->GetProjectileRuntimeData().shooter && a_projectile->GetProjectileRuntimeData().shooter.get()) {
				shooter = a_projectile->GetProjectileRuntimeData().shooter.get().get();
			}

			resetProjectileOwner(a_projectile, a_parrier, a_projectile_collidable);

			if (shooter && shooter->Is3DLoaded()) {
				RetargetProjectile(a_projectile, shooter);
			}
			else {
				ReflectProjectile(a_projectile);
			}

			a_parrier->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)->CastSpellImmediate(spellBashReflectSpellVFX, false, a_parrier->As<RE::Actor>(), 1.0f, false, 0.0f, a_parrier->As<RE::Actor>());

			if (a_parrier->IsPlayerRef()) {
				RE::PlayerCharacter::GetSingleton()->AddSkillExperience(RE::ActorValue::kBlock, _skillXPReflectSpell);
			}
			return true;
		}
		return false;

	}

	//------------------------------------
	void Block::SetRotationMatrix(RE::NiMatrix3& a_matrix, float sacb, float cacb, float sb)
	{
		float cb = std::sqrtf(1 - sb * sb);
		float ca = cacb / cb;
		float sa = sacb / cb;
		a_matrix.entry[0][0] = ca;
		a_matrix.entry[0][1] = -sacb;
		a_matrix.entry[0][2] = sa * sb;
		a_matrix.entry[1][0] = sa;
		a_matrix.entry[1][1] = cacb;
		a_matrix.entry[1][2] = -ca * sb;
		a_matrix.entry[2][0] = 0.0;
		a_matrix.entry[2][1] = sb;
		a_matrix.entry[2][2] = cb;
	}

	void Block::resetProjectileOwner(RE::Projectile* a_projectile, RE::Actor* a_actor, RE::hkpCollidable* a_projectile_collidable)
	{
		a_projectile->SetActorCause(a_actor->GetActorCause());
		a_projectile->GetProjectileRuntimeData().shooter = a_actor->GetHandle();
		RE::CFilter a_collisionFilterInfo;
		a_actor->GetCollisionFilterInfo(a_collisionFilterInfo);
		a_projectile_collidable->broadPhaseHandle.collisionFilterInfo.SetSystemGroup(a_collisionFilterInfo.filter);
	}

	bool Block::ApproximatelyEqual(float A, float B)
	{
		return ((A - B) < FLT_EPSILON) && ((B - A) < FLT_EPSILON);
	}

	bool Block::PredictAimProjectile(RE::NiPoint3 a_projectilePos, RE::NiPoint3 a_targetPosition, RE::NiPoint3 a_targetVelocity, float a_gravity, RE::NiPoint3& a_projectileVelocity)
	{
		// http://ringofblades.com/Blades/Code/PredictiveAim.cs

		float projectileSpeedSquared = a_projectileVelocity.SqrLength();
		float projectileSpeed = std::sqrtf(projectileSpeedSquared);

		if (projectileSpeed <= 0.f || a_projectilePos == a_targetPosition) {
			return false;
		}

		float targetSpeedSquared = a_targetVelocity.SqrLength();
		float targetSpeed = std::sqrtf(targetSpeedSquared);
		RE::NiPoint3 targetToProjectile = a_projectilePos - a_targetPosition;
		float distanceSquared = targetToProjectile.SqrLength();
		float distance = std::sqrtf(distanceSquared);
		RE::NiPoint3 direction = targetToProjectile;
		direction.Unitize();
		RE::NiPoint3 targetVelocityDirection = a_targetVelocity;
		targetVelocityDirection.Unitize();

		float cosTheta = (targetSpeedSquared > 0)
			? direction.Dot(targetVelocityDirection)
			: 1.0f;

		bool bValidSolutionFound = true;
		float t;

		if (ApproximatelyEqual(projectileSpeedSquared, targetSpeedSquared)) {
			// We want to avoid div/0 that can result from target and projectile traveling at the same speed
			//We know that cos(theta) of zero or less means there is no solution, since that would mean B goes backwards or leads to div/0 (infinity)
			if (cosTheta > 0) {
				t = 0.5f * distance / (targetSpeed * cosTheta);
			}
			else {
				bValidSolutionFound = false;
				t = 1;
			}
		}
		else {
			float a = projectileSpeedSquared - targetSpeedSquared;
			float b = 2.0f * distance * targetSpeed * cosTheta;
			float c = -distanceSquared;
			float discriminant = b * b - 4.0f * a * c;

			if (discriminant < 0) {
				// NaN
				bValidSolutionFound = false;
				t = 1;
			}
			else {
				// a will never be zero
				float uglyNumber = sqrtf(discriminant);
				float t0 = 0.5f * (-b + uglyNumber) / a;
				float t1 = 0.5f * (-b - uglyNumber) / a;

				// Assign the lowest positive time to t to aim at the earliest hit
				t = std::min(t0, t1);
				if (t < FLT_EPSILON) {
					t = std::max(t0, t1);
				}

				if (t < FLT_EPSILON) {
					// Time can't flow backwards when it comes to aiming.
					// No real solution was found, take a wild shot at the target's future location
					bValidSolutionFound = false;
					t = 1;
				}
			}
		}

		a_projectileVelocity = a_targetVelocity + (-targetToProjectile / t);

		if (!bValidSolutionFound)
		{
			a_projectileVelocity.Unitize();
			a_projectileVelocity *= projectileSpeed;
		}

		if (!ApproximatelyEqual(a_gravity, 0.f))
		{
			float netFallDistance = (a_projectileVelocity * t).z;
			float gravityCompensationSpeed = (netFallDistance + 0.5f * a_gravity * t * t) / t;
			a_projectileVelocity.z = gravityCompensationSpeed;
		}

		return bValidSolutionFound;
	}

	void Block::ReflectProjectile(RE::Projectile* a_projectile)
	{
		a_projectile->GetProjectileRuntimeData().linearVelocity *= -1.f;

		// rotate model
		auto projectileNode = a_projectile->Get3D2();
		if (projectileNode)
		{
			RE::NiPoint3 direction = a_projectile->GetProjectileRuntimeData().linearVelocity;
			direction.Unitize();

			a_projectile->data.angle.x = asin(direction.z);
			a_projectile->data.angle.z = atan2(direction.x, direction.y);

			if (a_projectile->data.angle.z < 0.0) {
				a_projectile->data.angle.z += PI;
			}

			if (direction.x < 0.0) {
				a_projectile->data.angle.z += PI;
			}

			SetRotationMatrix(projectileNode->local.rotate, -direction.x, direction.y, direction.z);
		}
	}

	/*Get the body position of this actor.*/
	void Block::getBodyPos(RE::Actor* a_actor, RE::NiPoint3& pos)
	{
		if (!a_actor->GetActorRuntimeData().race) {
			return;
		}
		RE::BGSBodyPart* bodyPart = a_actor->GetActorRuntimeData().race->bodyPartData->parts[0];
		if (!bodyPart) {
			return;
		}
		auto targetPoint = a_actor->GetNodeByName(bodyPart->targetName.c_str());
		if (!targetPoint) {
			return;
		}

		pos = targetPoint->world.translate;
	}

	void Block::RetargetProjectile(RE::Projectile* a_projectile, RE::TESObjectREFR* a_target)
	{
		a_projectile->GetProjectileRuntimeData().desiredTarget = a_target;

		auto projectileNode = a_projectile->Get3D2();
		auto targetHandle = a_target->GetHandle();

		RE::NiPoint3 targetPos = a_target->GetPosition();
		if (a_target->GetFormType() == RE::FormType::ActorCharacter) {
			getBodyPos(a_target->As<RE::Actor>(), targetPos);
		}

		RE::NiPoint3 targetVelocity;
		targetHandle.get()->GetLinearVelocity(targetVelocity);

		float projectileGravity = 0.f;
		if (auto ammo = a_projectile->GetProjectileRuntimeData().ammoSource) {
			if (auto bgsProjectile = ammo->GetRuntimeData().data.projectile) {
				projectileGravity = bgsProjectile->data.gravity;
				if (auto bhkWorld = a_projectile->parentCell->GetbhkWorld()) {
					if (auto hkpWorld = bhkWorld->GetWorld1()) {
						auto vec4 = hkpWorld->gravity;
						float quad[4];
						_mm_store_ps(quad, vec4.quad);
						float gravity = -quad[2] * RE::bhkWorld::GetWorldScaleInverse();
						projectileGravity *= gravity;
					}
				}
			}
		}

		PredictAimProjectile(a_projectile->data.location, targetPos, targetVelocity, projectileGravity, a_projectile->GetProjectileRuntimeData().linearVelocity);

		// rotate
		RE::NiPoint3 direction = a_projectile->GetProjectileRuntimeData().linearVelocity;
		direction.Unitize();

		a_projectile->data.angle.x = asin(direction.z);
		a_projectile->data.angle.z = atan2(direction.x, direction.y);

		if (a_projectile->data.angle.z < 0.0) {
			a_projectile->data.angle.z += PI;
		}

		if (direction.x < 0.0) {
			a_projectile->data.angle.z += PI;
		}

		SetRotationMatrix(projectileNode->local.rotate, -direction.x, direction.y, direction.z);
	}

}
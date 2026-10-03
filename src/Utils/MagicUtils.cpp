#include "MagicUtils.h"

namespace Utils
{
	void MagicUtils::RemoveOldestEffectStackWithKeyword(RE::Actor* target, RE::BGSKeyword* keyword)
	{
		if (!target || !keyword) return;

		const auto activeEffects = target->AsMagicTarget()->GetActiveEffectList();
		if (activeEffects)
		{
			float maxElapsedTime = 0.0;
			RE::ActiveEffect* oldestEff = nullptr;
			int stackCount = 0;

			for (const auto& activeEffect : *activeEffects) {
				if (auto mgef = activeEffect ? activeEffect->GetBaseObject() : nullptr; mgef) {
					if (activeEffect->flags.all(RE::ActiveEffect::Flag::kDispelled))
						continue;

					if (mgef->HasKeyword(keyword))
					{
						stackCount += 1;

						float time = activeEffect->elapsedSeconds;
						if (time >= maxElapsedTime)
						{
							maxElapsedTime = time;
							oldestEff = activeEffect;
						}
					}
				}
			}

			if (stackCount > 1 && oldestEff)
			{
				oldestEff->Dispel(true);
			}
		}
	}
}
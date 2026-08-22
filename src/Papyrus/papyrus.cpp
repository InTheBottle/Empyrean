#include "papyrus.h"
#include "Hooks/Pickpocket.h"
#include "hooks/Smithing.h"
#include "Utils/ArmorUtils.h"

namespace Papyrus
{
	std::vector<int> GetVersion(STATIC_ARGS) {
		return { Plugin::VERSION[0], Plugin::VERSION[1], Plugin::VERSION[2] };
	}

	//----------------------------------------------------------------------------------------------------
	static void UpdateRacesAllowPickpocket(STATIC_ARGS)
	{
		Hooks::Pickpocket::SetRacesAllowPickpocket();
	}

	//----------------------------------------------------------------------------------------------------
	std::vector< RE::TESObjectARMO*> GetAllEquippedArmor(STATIC_ARGS, RE::Actor* a_actor)
	{
		return Utils::ArmorUtils::GetEquippedArmor(a_actor);
	}

	//----------------------------------------------------------------------------------------------------
	static bool CreateStasisCubeFromAutomaton(STATIC_ARGS, RE::Actor* automaton)
	{
		return Hooks::Smithing::CreateStasisCubeFromAutomaton(automaton);
	}

	//----------------------------------------------------------------------------------------------------
	static void FixAutomatonPotionsInContainer(STATIC_ARGS, RE::TESObjectREFR* contRef)
	{
		return Hooks::Smithing::FixAutomatonPotionsInContainer(contRef);
	}

	//----------------------------------------------------------------------------------------------------
	static void SetMagicEffectDescription(STATIC_ARGS, RE::EffectSetting* a_mgef, std::string_view a_descr)
	{
		if (!a_mgef) return;

		a_mgef->magicItemDescription = a_descr;
		//logger::info("  > SetMagicEffectDescription() -> {}"sv, a_mgef->magicItemDescription.c_str());
	}

	//----------------------------------------------------------------------------------------------------
	/*
	static auto GetExtraHealthList(RE::BSSimpleList<RE::ExtraDataList*>* a_lists) -> RE::ExtraDataList*
	{
		if (a_lists) {
			for (const auto& xList : *a_lists) {
				if (xList && xList->GetByType<RE::ExtraHealth>()) {
					return xList;
				}
			}
		}
		return nullptr;
	}

	static auto ConstructExtraDataList(void* a_this) -> RE::ExtraDataList*
	{
		using func_t = decltype(&ConstructExtraDataList);
		REL::Relocation<func_t> func{ RELOCATION_ID(11437, 11583) };
		return func(a_this);
	}

	static auto GetExtraHealth(RE::ExtraDataList* a_extra) -> float
	{
		using func_t = decltype(&GetExtraHealth);
		REL::Relocation<func_t> func{ RELOCATION_ID(11557, 11703) };
		return func(a_extra);
	}

	static void SetExtraHealth(RE::ExtraDataList* a_extra, float a_health)
	{
		using func_t = decltype(&SetExtraHealth);
		REL::Relocation<func_t> func{ RELOCATION_ID(11470, 11616) };
		return func(a_extra, a_health);
	}

	static void RemoveEnchantment(RE::InventoryEntryData* a_entry)
	{
		if (!a_entry) return;

		logger::info("RemoveEnchantment()"sv);

		auto item = a_entry->object;

		if (a_entry->extraLists) {
			for (const auto& xList : *a_entry->extraLists) {
				if (xList) {
					auto xEnchantment = xList->GetByType<RE::ExtraEnchantment>();

					if (xEnchantment) {
						xList->Remove(RE::ExtraDataType::kEnchantment, xEnchantment);
					}

					auto xCharge = xList->GetByType<RE::ExtraCharge>();

					if (xCharge) {
						xList->Remove(RE::ExtraDataType::kCharge, xCharge);
					}
				}
			}
		}

		RE::TESBoundObject* templateItem = nullptr;

		if (item && item->IsArmor()) {
			templateItem = item->As<RE::TESObjectARMO>()->templateArmor;
		}

		if (item && item->IsWeapon()) {
			templateItem = item->As<RE::TESObjectWEAP>()->templateWeapon;
		}

		if (templateItem) {
			logger::info("  > Has template"sv);
			auto xListOld = GetExtraHealthList(a_entry->extraLists);
			const auto player = RE::PlayerCharacter::GetSingleton();

			if (xListOld) {
				auto xListNew = ConstructExtraDataList(RE::MemoryManager::GetSingleton()->Allocate(0x20, 0, false));
				SetExtraHealth(xListNew, GetExtraHealth(xListOld));

				player->RemoveItem(item, 1, RE::ITEM_REMOVE_REASON::kRemove, xListOld, nullptr);
				player->AddObjectToContainer(templateItem, xListNew, 1, nullptr);
			}
			else {
				player->RemoveItem(item, 1, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
				player->AddObjectToContainer(templateItem, nullptr, 1, nullptr);
			}
		}
			
		
	}

	static void UpdateUI()
	{
		const auto queue = RE::UIMessageQueue::GetSingleton();
		const auto strings = RE::InterfaceStrings::GetSingleton();
		const auto tasks = SKSE::GetTaskInterface();

		tasks->AddUITask([queue, strings]() {
			queue->AddMessage(strings->craftingMenu, RE::UI_MESSAGE_TYPE::kHide, nullptr);
			queue->AddMessage(strings->craftingMenu, RE::UI_MESSAGE_TYPE::kShow, nullptr);
			});
	}

	//----------------------------------------------------------------------------------------------------
	static void AddAllEnchantedItemsWithKeywordToListAndRemoveEnch(STATIC_ARGS, RE::TESObjectREFR* contRef, RE::BGSListForm* a_list, RE::BGSKeyword* a_keyword)
	{
		if (!contRef || !a_list || !a_keyword) return;
		//logger::info("  >GetNumEnchantedFormsWithKeyword() - keyword EDID = {}"sv, keywordEDID);

		auto* invChanges = contRef->GetInventoryChanges(true);
		if (!invChanges) {
			return;
		}

		auto* invLists = invChanges->entryList;
		if (!invLists || invLists->empty()) {
			return;
		}

		for (auto& entry : *invChanges->entryList) {
			auto* obj = entry ? entry->GetObject() : nullptr;
			if (!obj) {
				continue;
			}

			bool hasTargetEnch = false;
			bool hasOtherEnch = false;

			//Non-player made enchantment
			auto ench = obj->As<RE::TESEnchantableForm>();
			if (ench && ench->formEnchanting) {
				for (auto& effect : ench->formEnchanting->effects) {
					if (effect->baseEffect->HasKeyword(a_keyword)) 
						hasTargetEnch = true;
					else 
						hasOtherEnch = true;
				}
			}

			auto* xLists = entry->extraLists;
			if (xLists) {
				//Player-made enchantment
				for (auto* xList : *xLists) {
					auto xEnch = xList->GetByType<RE::ExtraEnchantment>();
					if (xEnch && xEnch->enchantment) {
						for (auto& effect : xEnch->enchantment->effects) {
							if (effect->baseEffect->HasKeyword(a_keyword))
								hasTargetEnch = true;
							else
								hasOtherEnch = true;
						}
					}
				}
			}

			if (hasTargetEnch && !hasOtherEnch)
			{
				a_list->AddForm(obj);
				RemoveEnchantment(entry);
				UpdateUI();
			}
		}
	}
	*/

	//----------------------------------------------------------------------------------------------------
	/*
	static void FindAllReferencesOfTypeAndShowVFX(STATIC_ARGS, RE::TESObjectREFR* a_ref, const RE::TESForm* a_formOrList, float a_radius, RE::BGSReferenceEffect* a_vfx)
	{
		if (!a_formOrList) {
			logger::error("FindAllReferencesOfTypeAndShowFX() failed, a_formOrList is NONE");
			return;
		}

		if (const auto TES = RE::TES::GetSingleton(); TES) {
			const auto list = a_formOrList->As<RE::BGSListForm>();

			TES->ForEachReferenceInRange(a_ref, a_radius, [&](RE::TESObjectREFR* b_ref) {
				if (const auto base = b_ref->GetBaseObject(); base && b_ref->Is3DLoaded()) {
					if (list && list->HasForm(base) || a_formOrList == base) {
						b_ref->ApplyArtObject(a_vfx->data.artObject);
						b_ref->ApplyEffectShader(a_vfx->data.effectShader);
					}
				}
				return RE::BSContainer::ForEachResult::kContinue;
			});
		}
	}
	

	void StopVisualEffect(RE::BGSReferenceEffect* effect, RE::TESObjectREFR* target) {
		if (effect && target) {
			if (static auto processLists = RE::ProcessLists::GetSingleton(); processLists) {
				if (auto artObject = effect->data.artObject; artObject) {
					using StopArtObject_t = void(*)(RE::ProcessLists*, RE::TESObjectREFR*, RE::BGSArtObject*);
					static REL::Relocation<StopArtObject_t> StopArtObject{ REL::ID(41396) }; //REL::VariantID(40382, 41396, 0x7048E0)
					StopArtObject(processLists, target, artObject);
				}
				if (auto effectShader = effect->data.effectShader; effectShader) {
					using StopEffectShader_t = void(*)(RE::ProcessLists*, RE::TESObjectREFR*, RE::TESEffectShader*);
					static REL::Relocation<StopEffectShader_t> StopEffectShader{ REL::ID(41395) }; //REL::VariantID(40381, 41395, 0x7047D0)
					StopEffectShader(processLists, target, effectShader);
				}
			}
		}
	}
	*/

	//----------------------------------------------------------------------------------------------------
	void Bind(VM& a_vm) {
		logger::info("  >Binding GetVersion..."sv);
		BIND(GetVersion);
		logger::info("  >Binding UpdateRacesAllowPickpocket..."sv);
		BIND(UpdateRacesAllowPickpocket);
		logger::info("  >Binding GetAllEquippedArmor..."sv);
		BIND(GetAllEquippedArmor);
		logger::info("  >Binding CreateStasisCubeFromAutomaton..."sv);
		BIND(CreateStasisCubeFromAutomaton);
		logger::info("  >Binding FixAutomatonPotionsInContainer..."sv);
		BIND(FixAutomatonPotionsInContainer);
		logger::info("  >Binding SetMagicEffectDescription..."sv);
		BIND(SetMagicEffectDescription);
	}

	bool RegisterFunctions(VM* a_vm) {
		logger::info("Binding papyrus functions in utility script {}..."sv, script);
		Bind(*a_vm);
		logger::info("Finished binding functions."sv);
		return true;
	}
}

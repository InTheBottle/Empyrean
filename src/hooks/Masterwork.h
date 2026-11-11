#pragma once

#include "Serialization/SerializationManager/SerializationManager.h"

namespace Hooks
{
    class Masterwork final : public Serialization::SerializationManager::Serializable, public RE::BSTEventSink<RE::TESUniqueIDChangeEvent>
    {
    public:
        enum class Type : std::uint8_t
        {
            kNone = 0,
            kMasterwork = 1,
            kQuality = 2
        };

        static Masterwork* GetSingleton();
        static bool InstallHooks();
        static void LoadData();
        static void OnPostLoadGame();

        bool Save(SKSE::SerializationInterface* a_intfc) override;
        bool Load(SKSE::SerializationInterface* a_intfc) override;
        void Revert(SKSE::SerializationInterface* a_intfc) override;

        RE::BSEventNotifyControl ProcessEvent(const RE::TESUniqueIDChangeEvent* a_event, RE::BSTEventSource<RE::TESUniqueIDChangeEvent>* a_source) override;

        inline static constexpr std::uint32_t RecordType = 'MWRK';
        inline static constexpr std::uint32_t RecordVersion = 1;

    private:
        struct Key
        {
            RE::FormID    owner;
            std::uint16_t uid;

            auto operator<=>(const Key&) const = default;
        };

        struct Mark
        {
            RE::FormID base;
            Type       type;
        };

        static Type GetMark(RE::ExtraDataList* a_list, const RE::TESForm* a_base);
        static Type GetMark(RE::TESObjectREFR* a_ref);
        static Type PreviewMark(RE::ExtraDataList* a_list, RE::TESBoundObject* a_obj);
        static Type Qualify(RE::TESBoundObject* a_item, RE::ExtraDataList* a_list);
        static void SetMark(RE::ExtraDataList* a_list, RE::TESBoundObject* a_item, Type a_type);
        static void BoostEnchantment(RE::ExtraDataList* a_list, RE::TESBoundObject* a_item, Type a_type);
        static float EnchantMult(Type a_type);
        static void ScaleEffect(RE::Effect& a_effect, float a_mult);
        static const char* BuildName(RE::ExtraTextDisplayData* a_this, RE::TESBoundObject* a_obj, float a_temper, Type a_type, REL::Relocation<const char* (*)(RE::ExtraTextDisplayData*, RE::TESBoundObject*, float)>& a_original);
        static const std::string& Label(Type a_type);

        static bool CanTemper(RE::BGSConstructibleObject* a_recipe, RE::TESFurniture* a_bench, bool a_checkConditions);
        static void TemperBonus(RE::BGSEntryPoint::ENTRY_POINT a_entryPoint, RE::Actor* a_perkOwner, RE::TESBoundObject* a_item, float* a_value);
        static void SmithWrite(RE::InventoryChanges* a_changes, RE::TESBoundObject* a_item, RE::ExtraDataList* a_list, float a_health);
        static void SetHealth(RE::ExtraDataList* a_list, float a_health);
        static void EnchantTraverse(RE::MagicItem* a_item, RE::MagicItemTraversalFunctor* a_functor);
        static RE::CraftingSubMenus::EnchantConstructMenu* EnchantMenu();
        static void EnchantPower(RE::BGSEntryPoint::ENTRY_POINT a_entryPoint, RE::Actor* a_perkOwner, RE::EnchantmentItem* a_enchantment, RE::TESForm* a_item, float* a_value);
        static float NameItemTemper(RE::ExtraDataList* a_list);
        static const char* NameItem(RE::ExtraTextDisplayData* a_this, RE::TESBoundObject* a_obj, float a_temper);
        static float NameRefTemper(RE::ExtraDataList* a_list);
        static const char* NameRef(RE::ExtraTextDisplayData* a_this, RE::TESBoundObject* a_obj, float a_temper);
        static float ValueTemper(RE::ExtraDataList* a_list);
        static float TemperedValue(float a_value, float a_temper);

        inline static REL::Relocation<decltype(&CanTemper)>     _CanTemper;
        inline static REL::Relocation<decltype(&TemperBonus)>   _TemperBonus;
        inline static REL::Relocation<decltype(&SmithWrite)>    _SmithWrite;
        inline static REL::Relocation<decltype(&SetHealth)>     _SetHealth;
        inline static REL::Relocation<decltype(&EnchantTraverse)> _EnchantTraverse;
        inline static REL::Relocation<decltype(&EnchantPower)>  _EnchantPower;
        inline static REL::Relocation<decltype(&NameItemTemper)> _NameItemTemper;
        inline static REL::Relocation<const char* (*)(RE::ExtraTextDisplayData*, RE::TESBoundObject*, float)> _NameItem;
        inline static REL::Relocation<decltype(&NameRefTemper)> _NameRefTemper;
        inline static REL::Relocation<const char* (*)(RE::ExtraTextDisplayData*, RE::TESBoundObject*, float)> _NameRef;
        inline static REL::Relocation<decltype(&ValueTemper)>   _ValueTemper;
        inline static REL::Relocation<decltype(&TemperedValue)> _TemperedValue;

        inline static constexpr float MasterworkTemperMult = 1.5f;
        inline static constexpr float MasterworkEnchantMult = 1.25f;
        inline static constexpr float MasterworkValueMult = 1.15f;
        inline static constexpr float MasterworkValueAdd = 50.0f;
        inline static constexpr float QualityEnchantMult = 1.2f;
        inline static constexpr float QualityValueMult = 1.35f;
        inline static constexpr float QualityValueAdd = 22.0f;

        inline static RE::BGSKeyword* MasterworkExclude;
        inline static RE::BGSKeyword* TailorExclude;
        inline static RE::TESGlobal*  MasterworkActive;
        inline static RE::BGSPerk*    Tailor;
        inline static bool            loaded;

        inline static std::mutex          lock;
        inline static std::map<Key, Mark> marks;

        inline static Type                pendingType{ Type::kNone };
        inline static RE::TESBoundObject* pendingItem;
        inline static RE::ExtraDataList*  pendingSource;
        inline static RE::ExtraDataList*  pendingMarked;
        inline static bool                pendingNew;

        inline static RE::CraftingSubMenus::EnchantConstructMenu* activeMenu;
        inline static bool                                         powerHooked;

        inline static thread_local RE::ExtraDataList* nameItemList;
        inline static thread_local RE::ExtraDataList* nameRefList;
        inline static thread_local RE::ExtraDataList* valueList;
    };
}

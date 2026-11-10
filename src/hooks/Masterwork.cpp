#include "Masterwork.h"

namespace Hooks
{
    // engine layout the enchant and preview hooks read (51449 / 51414)
    static_assert(offsetof(RE::CraftingSubMenus::EnchantConstructMenu, selected) == 0x170);
    static_assert(offsetof(RE::CraftingSubMenus::EnchantConstructMenu, createEffectFunctor) == 0x1A8);
    static_assert(offsetof(RE::CraftingSubMenus::SmithingMenu, recipes) == 0x100);
    static_assert(offsetof(RE::CraftingSubMenus::SmithingMenu, selectedIndex) == 0x154);
    static_assert(offsetof(RE::CraftingSubMenus::SmithingMenu, unk160) == 0x160);

    namespace
    {
        std::uintptr_t CallSite(REL::RelocationID a_id, REL::VariantOffset a_offset, std::string_view a_name)
        {
            REL::Relocation<std::uintptr_t> target{ a_id, a_offset };
            if (!util::is_call_site(target.address())) {
                logger::error("  {} hook did not find a call at {:#x}"sv, a_name, target.address());
                return 0;
            }
            return target.address();
        }

        RE::ExtraDataList* FrontList(RE::InventoryEntryData* a_entry)
        {
            if (!a_entry || !a_entry->extraLists || a_entry->extraLists->empty()) {
                return nullptr;
            }
            return a_entry->extraLists->front();
        }

        std::string Translate(const char* a_key, const char* a_fallback)
        {
            std::string text;
            if (!SKSE::Translation::Translate(a_key, text) || text.empty() || text.starts_with('$')) {
                text = a_fallback;
            }
            return text;
        }
    }

    Masterwork* Masterwork::GetSingleton()
    {
        static Masterwork singleton;
        return &singleton;
    }

    bool Masterwork::InstallHooks()
    {
        logger::info("  Installing Masterwork hooks..."sv);

        const std::array sites{
            CallSite(RELOCATION_ID(50296, 51360), REL::VariantOffset(0x153, 0x325, 0x153), "TemperBonus"sv),      // HandleEntryPoint(kModTemperingHealth)
            CallSite(RELOCATION_ID(50477, 51370), REL::VariantOffset(0xB5, 0xEA, 0xB5), "SmithWrite"sv),          // temper writer
            CallSite(RELOCATION_ID(15905, 16145), REL::VariantOffset(0x152, 0x146, 0x152), "SetHealth"sv),        // ExtraHealth setter
            CallSite(RELOCATION_ID(50366, 51283), REL::VariantOffset(0xE4, 0xD7, 0xE4), "EnchantPower"sv),        // HandleEntryPoint(kModEnchantmentPower)
            CallSite(RELOCATION_ID(15780, 16018), REL::VariantOffset(0x6B, 0x66, 0x6B), "NameItemTemper"sv),      // GetTemper
            CallSite(RELOCATION_ID(15780, 16018), REL::VariantOffset(0x10A, 0xFE, 0x10A), "NameItem"sv),          // ExtraTextDisplayData::GetDisplayName
            CallSite(RELOCATION_ID(19354, 19781), REL::VariantOffset(0x171, 0x16B, 0x171), "NameRefTemper"sv),    // GetTemper
            CallSite(RELOCATION_ID(19354, 19781), REL::VariantOffset(0x247, 0x23D, 0x247), "NameRef"sv),          // ExtraTextDisplayData::GetDisplayName
            CallSite(RELOCATION_ID(15757, 15995), REL::VariantOffset(0x50, 0x50, 0x50), "ValueTemper"sv),         // GetTemper
            CallSite(RELOCATION_ID(15757, 15995), REL::VariantOffset(0x84, 0x83, 0x84), "TemperedValue"sv),       // tempered gold value
            CallSite(RELOCATION_ID(50376, 51386), REL::VariantOffset(0x3D, 0x162, 0x3D), "CanTemper"sv),          // CanBeCreatedOnWorkbench in the temper recipe map
            CallSite(RELOCATION_ID(50557, 51449), REL::VariantOffset(0x28D, 0x1ED, 0x28D), "EnchantTraverse"sv)   // MagicItem::Traverse(enchantment, &menu->createEffectFunctor)
        };

        if (std::ranges::find(sites, std::uintptr_t{ 0 }) != sites.end()) {
            return false;
        }

        auto& trampoline = SKSE::GetTrampoline();
        _TemperBonus = trampoline.write_call<5>(sites[0], &TemperBonus);
        _SmithWrite = trampoline.write_call<5>(sites[1], &SmithWrite);
        _SetHealth = trampoline.write_call<5>(sites[2], &SetHealth);
        _EnchantPower = trampoline.write_call<5>(sites[3], &EnchantPower);
        _NameItemTemper = trampoline.write_call<5>(sites[4], &NameItemTemper);
        _NameItem = trampoline.write_call<5>(sites[5], &NameItem);
        _NameRefTemper = trampoline.write_call<5>(sites[6], &NameRefTemper);
        _NameRef = trampoline.write_call<5>(sites[7], &NameRef);
        _ValueTemper = trampoline.write_call<5>(sites[8], &ValueTemper);
        _TemperedValue = trampoline.write_call<5>(sites[9], &TemperedValue);
        _CanTemper = trampoline.write_call<5>(sites[10], &CanTemper);
        _EnchantTraverse = trampoline.write_call<5>(sites[11], &EnchantTraverse);

        return true;
    }

    void Masterwork::LoadData()
    {
        const auto dataHandler = RE::TESDataHandler::GetSingleton();
        MasterworkExclude = dataHandler->LookupForm<RE::BGSKeyword>(0x5DBADA, "PathsOfEmpyrean.esp"sv);  // PoE_K_MasterworkExclude
        TailorExclude = dataHandler->LookupForm<RE::BGSKeyword>(0x5DBADB, "PathsOfEmpyrean.esp"sv);      // PoE_K_TailorExclude
        MasterworkActive = dataHandler->LookupForm<RE::TESGlobal>(0x014C67, "PathsOfEmpyrean.esp"sv);    // IWP_GLO_SMITH_DragonbloodMasterworkActive
        Tailor = dataHandler->LookupForm<RE::BGSPerk>(0x00FB50, "PathsOfEmpyrean.esp"sv);                // IWP_PERK_SMITH_Tailor

        if (!MasterworkExclude || !TailorExclude || !MasterworkActive || !Tailor) {
            logger::info("ERROR: Failed to fetch necessary data for Masterwork and Tailor!"sv);
            loaded = false;
            return;
        }

        RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink<RE::TESUniqueIDChangeEvent>(GetSingleton());
        loaded = true;
    }

    void Masterwork::OnPostLoadGame()
    {
        std::vector<RE::FormID> dropped;
        {
            std::scoped_lock guard(lock);
            for (const auto& [key, mark] : marks) {
                if (key.uid == 0) {
                    dropped.push_back(key.owner);
                }
            }
        }

        for (const auto id : dropped) {
            if (const auto ref = RE::TESForm::LookupByID<RE::TESObjectREFR>(id)) {
                ref->formFlags |= RE::TESForm::RecordFlags::kFormRetainsID;
            }
        }
    }

    bool Masterwork::Save(SKSE::SerializationInterface* a_intfc)
    {
        std::scoped_lock guard(lock);
        if (!a_intfc->OpenRecord(RecordType, RecordVersion)) {
            return false;
        }

        const auto count = static_cast<std::uint32_t>(marks.size());
        if (!a_intfc->WriteRecordData(count)) {
            return false;
        }

        for (const auto& [key, mark] : marks) {
            if (!a_intfc->WriteRecordData(key.owner) || !a_intfc->WriteRecordData(key.uid) || !a_intfc->WriteRecordData(mark.base) || !a_intfc->WriteRecordData(mark.type)) {
                return false;
            }
        }
        return true;
    }

    bool Masterwork::Load(SKSE::SerializationInterface* a_intfc)
    {
        std::scoped_lock guard(lock);
        marks.clear();

        std::uint32_t count = 0;
        if (!a_intfc->ReadRecordData(count)) {
            return false;
        }

        for (std::uint32_t i = 0; i < count; ++i) {
            Key  key{};
            Mark mark{};
            if (!a_intfc->ReadRecordData(key.owner) || !a_intfc->ReadRecordData(key.uid) || !a_intfc->ReadRecordData(mark.base) || !a_intfc->ReadRecordData(mark.type)) {
                return false;
            }
            if (!a_intfc->ResolveFormID(key.owner, key.owner) || !a_intfc->ResolveFormID(mark.base, mark.base)) {
                continue;
            }
            marks[key] = mark;
        }
        return true;
    }

    void Masterwork::Revert(SKSE::SerializationInterface*)
    {
        std::scoped_lock guard(lock);
        marks.clear();
    }

    RE::BSEventNotifyControl Masterwork::ProcessEvent(const RE::TESUniqueIDChangeEvent* a_event, RE::BSTEventSource<RE::TESUniqueIDChangeEvent>*)
    {
        if (!a_event) {
            return RE::BSEventNotifyControl::kContinue;
        }

        RE::FormID dropped = 0;
        {
            std::scoped_lock guard(lock);
            const auto it = marks.find(Key{ a_event->oldBaseID, a_event->oldUniqueID });
            if (it == marks.end()) {
                return RE::BSEventNotifyControl::kContinue;
            }

            const auto mark = it->second;
            marks.erase(it);
            if (a_event->newBaseID == 0) {
                return RE::BSEventNotifyControl::kContinue;
            }

            marks[Key{ a_event->newBaseID, a_event->newUniqueID }] = mark;
            if (a_event->newUniqueID == 0) {
                dropped = a_event->newBaseID;
            }
        }

        // dropped to the world: same flag papyrus sets, so pickup assigns a new uid
        if (dropped != 0) {
            if (const auto ref = RE::TESForm::LookupByID<RE::TESObjectREFR>(dropped)) {
                ref->formFlags |= RE::TESForm::RecordFlags::kFormRetainsID;
            }
        }
        return RE::BSEventNotifyControl::kContinue;
    }

    Masterwork::Type Masterwork::GetMark(RE::ExtraDataList* a_list, const RE::TESForm* a_base)
    {
        if (!a_list) {
            return Type::kNone;
        }

        const auto xUID = a_list->GetByType<RE::ExtraUniqueID>();
        if (!xUID) {
            return Type::kNone;
        }

        std::scoped_lock guard(lock);
        const auto it = marks.find(Key{ xUID->baseID, xUID->uniqueID });
        if (it == marks.end()) {
            return Type::kNone;
        }
        if (a_base && it->second.base != a_base->GetFormID()) {
            return Type::kNone;
        }
        return it->second.type;
    }

    Masterwork::Type Masterwork::GetMark(RE::TESObjectREFR* a_ref)
    {
        const auto base = a_ref->GetBaseObject();
        std::scoped_lock guard(lock);
        const auto it = marks.find(Key{ a_ref->GetFormID(), 0 });
        if (it == marks.end() || !base || it->second.base != base->GetFormID()) {
            return Type::kNone;
        }
        return it->second.type;
    }

    Masterwork::Type Masterwork::PreviewMark(RE::ExtraDataList* a_list, RE::TESBoundObject* a_obj)
    {
        // smithing preview is a deep copy of the selected entry with the new health (50529 / 51414)
        const auto ui = RE::UI::GetSingleton();
        if (!ui) {
            return Type::kNone;
        }

        const auto crafting = ui->GetMenu<RE::CraftingMenu>();
        if (!crafting) {
            return Type::kNone;
        }

        const auto smithing = skyrim_cast<RE::CraftingSubMenus::SmithingMenu*>(crafting->GetCraftingSubMenu());
        if (!smithing || !smithing->unk160 || FrontList(smithing->unk160) != a_list) {
            return Type::kNone;
        }

        const auto index = smithing->selectedIndex;
        if (index < 0 || static_cast<std::uint32_t>(index) >= smithing->recipes.size()) {
            return Type::kNone;
        }

        const auto source = reinterpret_cast<RE::InventoryEntryData*>(smithing->recipes[index].item);
        return Qualify(a_obj, FrontList(source));
    }

    Masterwork::Type Masterwork::Qualify(RE::TESBoundObject* a_item, RE::ExtraDataList* a_list)
    {
        if (!loaded || !a_item) {
            return Type::kNone;
        }

        if (const auto type = GetMark(a_list, a_item); type != Type::kNone) {
            return type;
        }

        const auto keywords = a_item->As<RE::BGSKeywordForm>();
        if (!keywords) {
            return Type::kNone;
        }

        if (const auto armor = a_item->As<RE::TESObjectARMO>(); armor && armor->IsClothing()) {
            if (keywords->HasKeyword(TailorExclude) || !RE::PlayerCharacter::GetSingleton()->HasPerk(Tailor)) {
                return Type::kNone;
            }
            return Type::kQuality;
        }

        if (!a_item->IsWeapon() && !a_item->IsArmor()) {
            return Type::kNone;
        }
        if (keywords->HasKeyword(MasterworkExclude) || MasterworkActive->value != 1.0f) {
            return Type::kNone;
        }
        return Type::kMasterwork;
    }

    void Masterwork::SetMark(RE::ExtraDataList* a_list, RE::TESBoundObject* a_item, Type a_type)
    {
        // a split-off copy carries the source's uid, so it always gets its own
        if (!a_list->GetByType<RE::ExtraUniqueID>() || a_list != pendingSource) {
            RE::PlayerCharacter::GetSingleton()->GetInventoryChanges()->SetUniqueID(a_list, nullptr, a_item);
        }

        const auto xUID = a_list->GetByType<RE::ExtraUniqueID>();
        if (!xUID) {
            return;
        }

        std::scoped_lock guard(lock);
        marks[Key{ xUID->baseID, xUID->uniqueID }] = Mark{ a_item->GetFormID(), a_type };
    }

    const std::string& Masterwork::Label(Type a_type)
    {
        static const std::string masterwork = Translate("$PoEMasterwork", "Masterwork");
        static const std::string quality = Translate("$PoEQuality", "Quality");
        if (a_type == Type::kQuality) {
            return quality;
        }
        return masterwork;
    }

    const char* Masterwork::BuildName(RE::ExtraTextDisplayData* a_this, RE::TESBoundObject* a_obj, float a_temper, Type a_type, REL::Relocation<const char* (*)(RE::ExtraTextDisplayData*, RE::TESBoundObject*, float)>& a_original)
    {
        if (a_type == Type::kNone) {
            return a_original(a_this, a_obj, a_temper);
        }

        // rebuild without the temper prefix, then put our label in its place
        a_this->temperFactor = -1.0f;
        const char* plain = a_original(a_this, a_obj, 1.0f);
        if (!plain) {
            plain = "";
        }

        const std::string name = std::format("{} ({})", plain, Label(a_type));
        a_this->displayName = name.c_str();
        a_this->temperFactor = a_temper;
        return a_this->displayName.c_str();
    }

    bool Masterwork::CanTemper(RE::BGSConstructibleObject* a_recipe, RE::TESFurniture* a_bench, bool a_checkConditions)
    {
        const bool result = _CanTemper(a_recipe, a_bench, a_checkConditions);
        if (!result || !loaded || !a_recipe->createdItem) {
            return result;
        }

        // clothing temper recipes stay out of the list until the player has Tailor
        const auto armor = a_recipe->createdItem->As<RE::TESObjectARMO>();
        if (armor && armor->IsClothing() && !RE::PlayerCharacter::GetSingleton()->HasPerk(Tailor)) {
            return false;
        }
        return result;
    }

    void Masterwork::TemperBonus(RE::BGSEntryPoint::ENTRY_POINT a_entryPoint, RE::Actor* a_perkOwner, RE::TESBoundObject* a_item, float* a_value)
    {
        _TemperBonus(a_entryPoint, a_perkOwner, a_item, a_value);

        // engine stores the InventoryEntryData being evaluated here
        const auto entry = reinterpret_cast<RE::InventoryEntryData*>(RE::PlayerCharacter::GetSingleton()->GetPlayerRuntimeData().temperingEntryPoint);
        if (Qualify(a_item, FrontList(entry)) == Type::kMasterwork) {
            *a_value *= MasterworkTemperMult;
        }
    }

    void Masterwork::SmithWrite(RE::InventoryChanges* a_changes, RE::TESBoundObject* a_item, RE::ExtraDataList* a_list, float a_health)
    {
        pendingType = Qualify(a_item, a_list);
        pendingItem = a_item;
        pendingSource = a_list;
        pendingNew = GetMark(a_list, a_item) == Type::kNone;

        _SmithWrite(a_changes, a_item, a_list, a_health);

        // an enchantment the item already had gets the same boost the enchanting table gives
        if (pendingNew && pendingType != Type::kNone && pendingMarked) {
            BoostEnchantment(pendingMarked, a_item, pendingType);
        }

        if (pendingType == Type::kMasterwork && pendingNew) {
            MasterworkActive->value = 0.0f;
        }

        pendingType = Type::kNone;
        pendingItem = nullptr;
        pendingSource = nullptr;
        pendingMarked = nullptr;
    }

    void Masterwork::SetHealth(RE::ExtraDataList* a_list, float a_health)
    {
        _SetHealth(a_list, a_health);
        if (pendingType != Type::kNone && a_list) {
            SetMark(a_list, pendingItem, pendingType);
            pendingMarked = a_list;
        }
    }

    void Masterwork::BoostEnchantment(RE::ExtraDataList* a_list, RE::TESBoundObject* a_item, Type a_type)
    {
        RE::EnchantmentItem* enchantment = nullptr;
        std::uint16_t        charge = 0;
        if (const auto xEnchantment = a_list->GetByType<RE::ExtraEnchantment>(); xEnchantment && xEnchantment->enchantment) {
            enchantment = xEnchantment->enchantment;
            charge = xEnchantment->charge;
        } else if (const auto enchantable = a_item->As<RE::TESEnchantableForm>(); enchantable && enchantable->formEnchanting) {
            enchantment = enchantable->formEnchanting;
            charge = enchantable->amountofEnchantment;
        }
        if (!enchantment || enchantment->effects.empty()) {
            return;
        }

        const float mult = EnchantMult(a_type);

        RE::BSTArray<RE::Effect> effects;
        effects.reserve(enchantment->effects.size());
        for (const auto source : enchantment->effects) {
            if (!source || !source->baseEffect) {
                continue;
            }
            auto& effect = effects.emplace_back();
            effect.Copy(source);
            ScaleEffect(effect, mult);
        }
        if (effects.empty()) {
            return;
        }

        const auto manager = RE::BGSCreatedObjectManager::GetSingleton();
        RE::EnchantmentItem* boosted = nullptr;
        if (a_item->IsWeapon()) {
            boosted = manager->AddWeaponEnchantment(effects);
        } else {
            boosted = manager->AddArmorEnchantment(effects);
        }
        if (!boosted) {
            return;
        }

        const auto player = RE::PlayerCharacter::GetSingleton();
        const auto list = player->GetInventoryChanges()->EnchantObject(a_item, a_list, boosted, charge);

        // what the enchant craft does for a worn item (50450 / 51355)
        if (list && (list->HasType(RE::ExtraDataType::kWorn) || list->HasType(RE::ExtraDataType::kWornLeft))) {
            player->UpdateWeaponAbility(a_item, list, list->HasType(RE::ExtraDataType::kWornLeft));
            player->UpdateArmorAbility(a_item, list);
        }
    }

    float Masterwork::EnchantMult(Type a_type)
    {
        if (a_type == Type::kMasterwork) {
            return MasterworkEnchantMult;
        }
        if (a_type == Type::kQuality) {
            return QualityEnchantMult;
        }
        return 1.0f;
    }

    void Masterwork::ScaleEffect(RE::Effect& a_effect, float a_mult)
    {
        // same rule CreateEffect uses: power scales magnitude, or duration when only that flag is set
        if (!a_effect.baseEffect) {
            return;
        }
        const auto flags = a_effect.baseEffect->data.flags;
        if (flags.all(RE::EffectSetting::EffectSettingData::Flag::kPowerAffectsMagnitude)) {
            a_effect.effectItem.magnitude *= a_mult;
        } else if (flags.all(RE::EffectSetting::EffectSettingData::Flag::kPowerAffectsDuration)) {
            a_effect.effectItem.duration = static_cast<std::uint32_t>(std::round(static_cast<float>(a_effect.effectItem.duration) * a_mult));
        }
    }

    void Masterwork::EnchantTraverse(RE::MagicItem* a_item, RE::MagicItemTraversalFunctor* a_functor)
    {
        // the updater passes &menu->createEffectFunctor (50557 / 51449)
        const auto menu = reinterpret_cast<RE::CraftingSubMenus::EnchantConstructMenu*>(reinterpret_cast<std::uintptr_t>(a_functor) - offsetof(RE::CraftingSubMenus::EnchantConstructMenu, createEffectFunctor));
        auto&      created = menu->createEffectFunctor.createdEffects;
        const auto start = created.size();

        activeMenu = menu;
        powerHooked = false;
        _EnchantTraverse(a_item, a_functor);
        activeMenu = nullptr;

        // a replaced functor (Scrambled Bugs improveMultipleEnchantmentEffects) never reaches the vanilla power call,
        // so scale what it created instead
        if (powerHooked) {
            powerHooked = false;
            return;
        }

        const auto selected = menu->selected.item.get();
        if (!selected || !selected->data) {
            return;
        }

        const float mult = EnchantMult(GetMark(FrontList(selected->data), selected->data->object));
        if (mult == 1.0f) {
            return;
        }

        for (auto i = start; i < created.size(); ++i) {
            ScaleEffect(created[i], mult);
        }
    }

    RE::CraftingSubMenus::EnchantConstructMenu* Masterwork::EnchantMenu()
    {
        if (activeMenu) {
            return activeMenu;
        }

        const auto ui = RE::UI::GetSingleton();
        if (!ui) {
            return nullptr;
        }

        const auto crafting = ui->GetMenu<RE::CraftingMenu>();
        if (!crafting) {
            return nullptr;
        }
        return skyrim_cast<RE::CraftingSubMenus::EnchantConstructMenu*>(crafting->GetCraftingSubMenu());
    }

    void Masterwork::EnchantPower(RE::BGSEntryPoint::ENTRY_POINT a_entryPoint, RE::Actor* a_perkOwner, RE::EnchantmentItem* a_enchantment, RE::TESForm* a_item, float* a_value)
    {
        _EnchantPower(a_entryPoint, a_perkOwner, a_enchantment, a_item, a_value);
        powerHooked = true;

        const auto menu = EnchantMenu();
        if (!menu) {
            return;
        }

        const auto selected = menu->selected.item.get();
        if (!selected || !selected->data) {
            return;
        }

        *a_value *= EnchantMult(GetMark(FrontList(selected->data), selected->data->object));
    }

    float Masterwork::NameItemTemper(RE::ExtraDataList* a_list)
    {
        nameItemList = a_list;
        return _NameItemTemper(a_list);
    }

    const char* Masterwork::NameItem(RE::ExtraTextDisplayData* a_this, RE::TESBoundObject* a_obj, float a_temper)
    {
        const auto list = nameItemList;
        nameItemList = nullptr;

        auto type = Type::kNone;
        if (list && list->GetByType<RE::ExtraTextDisplayData>() == a_this) {
            type = GetMark(list, a_obj);
            if (type == Type::kNone) {
                type = PreviewMark(list, a_obj);
            }
        }
        return BuildName(a_this, a_obj, a_temper, type, _NameItem);
    }

    float Masterwork::NameRefTemper(RE::ExtraDataList* a_list)
    {
        nameRefList = a_list;
        return _NameRefTemper(a_list);
    }

    const char* Masterwork::NameRef(RE::ExtraTextDisplayData* a_this, RE::TESBoundObject* a_obj, float a_temper)
    {
        const auto list = nameRefList;
        nameRefList = nullptr;

        auto type = Type::kNone;
        if (list && list->GetByType<RE::ExtraTextDisplayData>() == a_this) {
            // the call site passes &ref->extraList
            const auto ref = reinterpret_cast<RE::TESObjectREFR*>(reinterpret_cast<std::uintptr_t>(list) - offsetof(RE::TESObjectREFR, extraList));
            type = GetMark(ref);
        }
        return BuildName(a_this, a_obj, a_temper, type, _NameRef);
    }

    float Masterwork::ValueTemper(RE::ExtraDataList* a_list)
    {
        valueList = a_list;
        return _ValueTemper(a_list);
    }

    float Masterwork::TemperedValue(float a_value, float a_temper)
    {
        const auto list = valueList;
        valueList = nullptr;

        auto result = _TemperedValue(a_value, a_temper);
        if (GetMark(list, nullptr) == Type::kQuality) {
            result *= QualityValueMult;
        }
        return result;
    }
}

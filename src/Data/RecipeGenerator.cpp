#include "RecipeGenerator.h"

namespace Data::RecipeGenerator
{
    namespace
    {
        using Votes = std::unordered_map<RE::BGSKeyword*, std::unordered_map<RE::TESBoundObject*, std::uint32_t>>;
        using Ingots = std::unordered_map<RE::BGSKeyword*, RE::TESBoundObject*>;

        bool IsMaterial(const RE::BGSKeyword* a_keyword)
        {
            if (!a_keyword) {
                return false;
            }
            const char* editorID = a_keyword->GetFormEditorID();
            return editorID && std::string_view(editorID).find("Material"sv) != std::string_view::npos;
        }

        RE::TESBoundObject* FirstComponent(const RE::BGSConstructibleObject* a_recipe)
        {
            if (a_recipe->requiredItems.numContainerObjects == 0 || !a_recipe->requiredItems.containerObjects[0]) {
                return nullptr;
            }
            return a_recipe->requiredItems.containerObjects[0]->obj;
        }

        RE::TESBoundObject* IngotFor(const RE::TESBoundObject* a_item, const Ingots& a_ingots)
        {
            const auto keywords = a_item->As<RE::BGSKeywordForm>();
            if (!keywords) {
                return nullptr;
            }
            for (std::uint32_t i = 0; i < keywords->numKeywords; ++i) {
                if (const auto it = a_ingots.find(keywords->keywords[i]); it != a_ingots.end()) {
                    return it->second;
                }
            }
            return nullptr;
        }

        RE::TESConditionItem* MakeCondition(RE::FUNCTION_DATA::FunctionID a_function, void* a_param, float a_value, bool a_or)
        {
            const auto item = new RE::TESConditionItem();
            item->data.functionData.function = a_function;
            item->data.functionData.params[0] = a_param;
            item->data.comparisonValue.f = a_value;
            item->data.flags.opCode = RE::CONDITION_ITEM_DATA::OpCode::kEqualTo;
            item->data.flags.isOR = a_or;
            item->data.object = RE::CONDITIONITEMOBJECT::kSelf;
            return item;
        }

        bool AddRecipe(RE::TESBoundObject* a_item, RE::TESBoundObject* a_ingot, RE::BGSKeyword* a_bench, RE::BGSPerk* a_arcane, RE::BGSPerk* a_tailor)
        {
            const auto recipe = RE::IFormFactory::Create<RE::BGSConstructibleObject>();
            if (!recipe) {
                return false;
            }

            recipe->requiredItems.AddObjectToContainer(a_ingot, 1, nullptr);
            recipe->createdItem = a_item;
            recipe->benchKeyword = a_bench;
            recipe->data.numConstructed = 1;

            // vanilla temper rule: EPTemperingItemIsEnchanted == 0 OR HasPerk ArcaneBlacksmith == 1
            const auto enchanted = MakeCondition(RE::FUNCTION_DATA::FunctionID::kEPTemperingItemIsEnchanted, nullptr, 0.0f, true);
            const auto arcane = MakeCondition(RE::FUNCTION_DATA::FunctionID::kHasPerk, a_arcane, 1.0f, false);
            enchanted->next = arcane;
            if (a_tailor) {
                arcane->next = MakeCondition(RE::FUNCTION_DATA::FunctionID::kHasPerk, a_tailor, 1.0f, false);
            }
            recipe->conditions.head = enchanted;

            RE::TESDataHandler::GetSingleton()->GetFormArray<RE::BGSConstructibleObject>().push_back(recipe);
            return true;
        }
    }

    void Run()
    {
        logger::info("Generating missing temper recipes..."sv);

        const auto dataHandler = RE::TESDataHandler::GetSingleton();
        const auto wheel = dataHandler->LookupForm<RE::BGSKeyword>(0x088108, "Skyrim.esm"sv);               // CraftingSmithingSharpeningWheel
        const auto table = dataHandler->LookupForm<RE::BGSKeyword>(0x0ADB78, "Skyrim.esm"sv);               // CraftingSmithingArmorTable
        const auto arcane = dataHandler->LookupForm<RE::BGSPerk>(0x05218E, "Skyrim.esm"sv);                 // ArcaneBlacksmith
        const auto tailor = dataHandler->LookupForm<RE::BGSPerk>(0x00FB50, "PathsOfEmpyrean.esp"sv);        // IWP_PERK_SMITH_Tailor
        const auto tailorExclude = dataHandler->LookupForm<RE::BGSKeyword>(0x5DBADB, "PathsOfEmpyrean.esp"sv);  // PoE_K_TailorExclude
        const auto linen = dataHandler->LookupForm<RE::TESObjectMISC>(0x005913, "PathsOfEmpyrean.esp"sv);     // IWP_MISC_CleanLinen
        const auto jewelry = dataHandler->LookupForm<RE::BGSKeyword>(0x06BBE9, "Skyrim.esm"sv);              // ArmorJewelry
        const auto vendorJewelry = dataHandler->LookupForm<RE::BGSKeyword>(0x08F95A, "Skyrim.esm"sv);        // VendorItemJewelry
        if (!wheel || !table || !arcane || !tailor || !tailorExclude || !linen || !jewelry || !vendorJewelry) {
            logger::info("ERROR: Failed to fetch necessary data for the temper recipe generator!"sv);
            return;
        }

        // the load order's own temper recipes decide which ingot each material keyword uses
        std::unordered_set<const RE::TESForm*> tempered;
        Votes votes;
        for (const auto recipe : dataHandler->GetFormArray<RE::BGSConstructibleObject>()) {
            if (!recipe || !recipe->createdItem || (recipe->benchKeyword != wheel && recipe->benchKeyword != table)) {
                continue;
            }
            tempered.insert(recipe->createdItem);

            const auto ingot = FirstComponent(recipe);
            const auto keywords = recipe->createdItem->As<RE::BGSKeywordForm>();
            if (!ingot || !keywords) {
                continue;
            }
            for (std::uint32_t i = 0; i < keywords->numKeywords; ++i) {
                if (IsMaterial(keywords->keywords[i])) {
                    votes[keywords->keywords[i]][ingot] += 1;
                }
            }
        }

        Ingots ingots;
        for (const auto& [keyword, counts] : votes) {
            const auto best = std::ranges::max_element(counts, {}, &std::pair<RE::TESBoundObject* const, std::uint32_t>::second);
            ingots[keyword] = best->first;
        }

        std::uint32_t added = 0;
        std::uint32_t skipped = 0;

        for (const auto weapon : dataHandler->GetFormArray<RE::TESObjectWEAP>()) {
            if (!weapon || !weapon->GetPlayable() || weapon->IsStaff() || tempered.contains(weapon)) {
                continue;
            }
            if (weapon->templateWeapon && tempered.contains(weapon->templateWeapon)) {
                continue;
            }
            const auto ingot = IngotFor(weapon, ingots);
            if (!ingot) {
                skipped += 1;
                continue;
            }
            if (AddRecipe(weapon, ingot, wheel, arcane, nullptr)) {
                added += 1;
            }
        }

        for (const auto armor : dataHandler->GetFormArray<RE::TESObjectARMO>()) {
            if (!armor || !armor->GetPlayable() || tempered.contains(armor)) {
                continue;
            }
            if (armor->templateArmor && tempered.contains(armor->templateArmor)) {
                continue;
            }
            const bool clothing = armor->IsClothing();
            if (clothing && (armor->HasKeyword(tailorExclude) || armor->HasKeyword(jewelry) || armor->HasKeyword(vendorJewelry))) {
                continue;
            }
            auto ingot = IngotFor(armor, ingots);
            if (!ingot && clothing) {
                ingot = linen;
            }
            if (!ingot) {
                skipped += 1;
                continue;
            }
            RE::BGSPerk* requiredPerk = nullptr;
            if (clothing) {
                requiredPerk = tailor;
            }
            if (AddRecipe(armor, ingot, table, arcane, requiredPerk)) {
                added += 1;
            }
        }

        logger::info("  >Added {} temper recipes, skipped {} items with no known material."sv, added, skipped);
    }
}

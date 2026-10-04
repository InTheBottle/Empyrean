#include "ModObjectManager.h"

namespace Data
{
	namespace
	{
		class InitialPropertyValues
		{
		public:
			using Map = RE::BSTScrapHashMap<RE::BSFixedString, RE::BSScript::Variable>;

			InitialPropertyValues() :
				map()
			{}

			InitialPropertyValues(const InitialPropertyValues&) = delete;
			InitialPropertyValues& operator=(const InitialPropertyValues&) = delete;

			~InitialPropertyValues()
			{
				for (auto& property : map) {
					std::destroy_at(std::addressof(property));
				}

				const auto table = reinterpret_cast<const Layout*>(std::addressof(map));
				if (table->entries) {
					table->heap->Deallocate(table->entries);
				}
			}

			union
			{
				Map map;
			};

		private:
			struct Layout
			{
				std::uint64_t  pad00;
				std::uint32_t  pad08;
				std::uint32_t  capacity;
				std::uint32_t  free;
				std::uint32_t  good;
				const void*    sentinel;
				RE::ScrapHeap* heap;
				void*          entries;
			};
			static_assert(sizeof(Layout) == sizeof(Map));
			static_assert(offsetof(Layout, heap) == 0x20);
		};
	}

	bool ModObjectManager::PreLoad() {
		logger::info("  >Looking for script {} on quest {}..."sv, ScriptName, QuestName);
		const auto quest = RE::TESForm::LookupByEditorID<RE::TESQuest>(QuestName);
		if (!quest) {
			if (EXPECTED_OBJECTS.empty()) {
				logger::info("    >No need to preload mod objects."sv);
				return true;
			}
			logger::critical("  >Failed to lookup quest."sv);
			return false;
		}

		const auto vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
		if (!vm) {
			logger::critical("  >Failed to get VM"sv);
			return false;
		}

		const auto bindPolicy = vm->GetObjectBindPolicy();
		const auto handlePolicy = vm->GetObjectHandlePolicy();
		if (!bindPolicy || !handlePolicy) {
			logger::critical("  >Failed to get VM object policies"sv);
			return false;
		}

		const auto handle = handlePolicy->GetHandleForObject(RE::TESQuest::FORMTYPE, quest);
		InitialPropertyValues initialValues;
		auto& properties = initialValues.map;
		std::uint32_t nonConverted;
		bindPolicy->GetInitialPropertyValues(handle, ScriptName, properties, nonConverted);
		objects.clear();

		for (const auto& [name, var] : properties) {
			if (var.IsObjectArray()) {
				auto varArray = var.GetArray();

				RE::BSScript::Array::size_type index = 0;
				for (const auto& element : *varArray) {
					if (const auto foundForm = element.Unpack<RE::TESForm*>()) {
						auto keyName = fmt::format<std::string>("{}|{}", std::to_string(index), name.c_str());
						objects.emplace(keyName, foundForm);
					}
					else {
						logger::critical("  >Undefined element in array propery {}."sv, name.c_str());
						return false;
					}
					++index;
				}
			}
			else if (var.IsObject()) {
				const auto value = var.Unpack<RE::TESForm*>();
				if (!value) {
					logger::critical("  >Property {} is null."sv, name.c_str());
					return false;
				}
				objects.emplace(name, value);
			}
			else {
				logger::critical("  >Undefined property {}", name.c_str());
				return false;
			}
		}

		vm->ResetAllBoundObjects(handle);
		logger::info("  >Found {} Mod Objects."sv, properties.size());
		logger::info("Done."sv);
		return Verify();
	}

	RE::TESForm* ModObjectManager::Get(std::string_view a_key) const {
		if (const auto it = objects.find(a_key); it != objects.end()) {
			return it->second;
		}
		SKSE::stl::report_and_fail(fmt::format("Mod Object {} was requested, but was not found."sv, a_key));
	}

	bool ModObjectManager::Verify() {
		logger::info("Verifying discovered objects:"sv);
		bool foundAll = true;
		for (const auto* objectName : EXPECTED_OBJECTS) {
			if (!objects.contains(std::string(objectName))) {
				foundAll = false;
				logger::critical("  >Failed to find {}.", objectName);
			}
			else {
				logger::info("  >Found {}", objectName);
			}
		}
		return foundAll;
	}

	bool PreloadModObjects() {
		logger::info("Preloading Mod Objects..."sv);
		auto* objectManager = ModObjectManager::GetSingleton();
		if (!objectManager) {
			logger::critical("  >Failed to get Mod Object manager."sv);
			return false;
		}
		return objectManager->PreLoad();
	}
}
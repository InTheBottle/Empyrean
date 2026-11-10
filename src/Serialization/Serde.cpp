#include "Serde.h"

#include "SerializationManager/SerializationManager.h"

namespace Serialization {
	void SaveCallback(SKSE::SerializationInterface* a_intfc)
	{
		logger::info("Starting save..."sv);
		auto* serdeManager = SerializationManager::ObjectManager::GetSingleton();
		if (!serdeManager) {
			logger::critical("  >Failed to get internal serialization manager."sv);
			return;
		}
		if (!serdeManager->Save(a_intfc)) {
			logger::critical("  >Failed to save!"sv);
			return;
		}
		logger::info("  >Save successful."sv);
	}

	void LoadCallback(SKSE::SerializationInterface* a_intfc)
	{
		logger::info("Starting load..."sv);
		auto* serdeManager = SerializationManager::ObjectManager::GetSingleton();
		if (!serdeManager) {
			logger::critical("  >Failed to get internal serialization manager."sv);
			return;
		}
		if (!serdeManager->Load(a_intfc)) {
			logger::critical("  >Failed to load!"sv);
			return;
		}
		logger::info("  >Load successful."sv);
	}

	void RevertCallback(SKSE::SerializationInterface* a_intfc)
	{
		logger::info("Starting revert..."sv);
		auto* serdeManager = SerializationManager::ObjectManager::GetSingleton();
		if (!serdeManager) {
			logger::critical("  >Failed to get internal serialization manager."sv);
			return;
		}
		serdeManager->Revert(a_intfc);
		logger::info("  >Revert done."sv);
	}
}
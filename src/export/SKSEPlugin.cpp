#include "Hooks/Pickpocket.h"
#include "hooks/Destruction.h"
#include "Data/ModObjectManager.h"
#include "Hooks/Hooks.h"
#include "Papyrus/Papyrus.h"
#include "Serialization/Serde.h"
#include "Settings/INI/INISettings.h"
#include "Settings/JSON/JSONSettings.h"
#include "Hooks/Destruction.h"
#include "Hooks/Smithing.h"
#include "hooks/Block.h"
#include "Hooks/Lockpicking.h"

static void MessageEventCallback(SKSE::MessagingInterface::Message* a_msg)
{
	switch (a_msg->type) {
	case SKSE::MessagingInterface::kDataLoaded:
		SECTION_SEPARATOR;
		if (!Data::PreloadModObjects()) {
			SKSE::stl::report_and_fail("Failed to preload mod objects. Check the log for more information."sv);
		}

		SECTION_SEPARATOR;
		Hooks::Pickpocket::InstallActivateHook();
		//Hooks::Destruction::LoadData();
		//Hooks::ProcessSpellsForPatching();
		Hooks::Smithing::LoadData();
		Hooks::Block::LoadData();
		Hooks::Lockpicking::LoadData();

		SECTION_SEPARATOR;
		logger::info("Finished startup tasks, enjoy your game!"sv);
		break;
	default:
		break;
	}
}

SKSEPluginInfo(
	.Version = Plugin::VERSION,
	.Name = Plugin::NAME,
	.Author = "Steelfeathers & SeaSparrow"sv,
	.RuntimeCompatibility = SKSE::VersionIndependence::AddressLibrary
)

SKSE_PLUGIN_LOAD(const SKSE::LoadInterface* a_skse)
{
	SKSE::Init(a_skse, { .trampoline = true, .trampolineSize = 249 });
	logger::info("Author: Steelfeathers & SeaSparrow"sv);
	logger::info("Runtime: {} ({})"sv, REL::Module::get().version().string(), REL::Module::IsVR() ? "VR"sv : REL::Module::IsAE() ? "AE"sv : "SE"sv);
	SECTION_SEPARATOR;

	logger::info("Performing startup tasks..."sv);

	if (!Settings::INI::Read()) {
		SKSE::stl::report_and_fail("Failed to load INI settings. Check the log for more information."sv);
	}
	SECTION_SEPARATOR;
	if (!Settings::JSON::Read()) {
		SKSE::stl::report_and_fail("Failed to read JSON settings. Check the log for more information."sv);
	}
	SECTION_SEPARATOR;
	if (!Hooks::Install()) {
		SKSE::stl::report_and_fail("Failed to install hooks. Check the log for more information."sv);
	}
	SECTION_SEPARATOR;

	SKSE::GetPapyrusInterface()->Register(Papyrus::RegisterFunctions);

	const auto messaging = SKSE::GetMessagingInterface();
	messaging->RegisterListener(&MessageEventCallback);

	logger::info("Setting up serialization system..."sv);
	const auto serialization = SKSE::GetSerializationInterface();
	serialization->SetUniqueID(Serialization::ID);
	serialization->SetSaveCallback(&Serialization::SaveCallback);
	serialization->SetLoadCallback(&Serialization::LoadCallback);
	serialization->SetRevertCallback(&Serialization::RevertCallback);
	logger::info("  >Registered necessary functions."sv);
	SECTION_SEPARATOR;

	return true;
}
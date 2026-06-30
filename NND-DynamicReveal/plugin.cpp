#include "KeyCodes.h"
#include "NND_API.h"

#include <SimpleIni.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>

namespace logger = SKSE::log;

namespace
{
	// NND's modder API, obtained once NND has finished loading.
	NND_API::IVNND4* g_nnd = nullptr;

	// Pending auto-reveal from a SkyrimNet dialogue event.
	// Written on SkyrimNet's ThreadPool; consumed on the game's main thread in ProcessEvent.
	struct PendingReveal { RE::FormID formId{0}; std::string dialogueText; };
	std::mutex g_pendingMutex;
	PendingReveal g_pendingReveal;

	bool g_skyrimNetAutoReveal = true;

	// Optional SkyrimNet integration.
	namespace SkyrimNet
	{
		using RegisterDecoratorFn = bool (*)(const char* name, const char* description,
		                                     std::function<std::string(RE::Actor*)> callback);

		static bool TryRegisterNNDDecorator()
		{
			const auto hDLL = GetModuleHandleA("SkyrimNet.dll");
			if (!hDLL) return false;

			const auto fn = reinterpret_cast<RegisterDecoratorFn>(
				GetProcAddress(hDLL, "PublicRegisterDecorator"));
			if (!fn) return false;

			return fn(
				"nnd_name",
				"NND real name for this actor (remains hidden from the player's crosshair until "
				"they reveal it with the hotkey). Returns empty for actors without an NND-assigned name.",
				[](RE::Actor* actor) -> std::string {
					if (!actor || !g_nnd || actor->IsPlayerRef()) return "";

					const auto name = g_nnd->GetRawName(actor);
					if (name.empty()) return "";

					// Strip disambiguation suffix: "Olffik Jurgensen [Whiterun Guard]" → "Olffik Jurgensen"
					std::string result{name.substr(0, name.find('['))};
					while (!result.empty() && result.back() == ' ') result.pop_back();
					if (result.empty() || result == actor->GetName()) return "";

					logger::info("nnd_name decorator: actor={:08X} name='{}'", actor->GetFormID(), result);
					return result;
				});
		}

		// Extracts the value of a field from the escaped inner JSON in the "data" string.
		// In the outer C++ string inner quotes appear as \" (backslash + quote) and inner
		// backslashes appear as \\ (two backslashes).  The value ends at the first \" that
		// is NOT preceded by another \ (which would mean the \ itself is escaped).
		static std::string ExtractEscapedField(const std::string& json, const std::string& key)
		{
			const std::string prefix = "\\\"" + key + "\\\":\\\"";
			const auto pos = json.find(prefix);
			if (pos == std::string::npos) return {};
			const auto start = pos + prefix.size();
			std::size_t search = start;
			while (true) {
				const auto end = json.find("\\\"", search);
				if (end == std::string::npos) return {};
				// If the char before this \ is itself a \, then \\ is an escaped backslash
				// and the " after it is literal — not a closing delimiter.  Keep searching.
				if (end == 0 || json[end - 1] != '\\') {
					return json.substr(start, end - start);
				}
				search = end + 2;
			}
		}

		// Returns true if `text` contains any word from `nndName` (case-insensitive, min 4 chars).
		static bool DialogueMentionsName(const std::string& text, const std::string& nndName)
		{
			if (text.empty() || nndName.empty()) return false;
			auto toLower = [](std::string s) {
				std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
				return s;
			};
			const auto lowerText = toLower(text);
			std::istringstream ss(nndName);
			std::string word;
			while (std::getline(ss, word, ' ')) {
				if (word.size() < 4) continue;
				if (lowerText.find(toLower(word)) != std::string::npos) return true;
			}
			return false;
		}

		static bool TryRegisterDialogueCallback()
		{
			using RegisterEventCallbackFn =
				uint64_t (*)(const char* eventType, std::function<void(const char*)> callback);

			const auto hDLL = GetModuleHandleA("SkyrimNet.dll");
			if (!hDLL) return false;

			const auto fn = reinterpret_cast<RegisterEventCallbackFn>(
				GetProcAddress(hDLL, "PublicRegisterEventCallback"));
			if (!fn) return false;

			fn("dialogue", [](const char* jsonRaw) {
				if (!jsonRaw) return;
				const std::string json = jsonRaw;
				logger::info("SkyrimNet dialogue: {}",
					json.size() > 400 ? json.substr(0, 400) + "..." : json);

				// Only act on player-initiated conversations (not ambient NPC-to-NPC chatter).
				// The field lives inside the escaped "data" string, so the closing quote is \"
				// (backslash + quote) in the C++ string, not a bare ".
				if (json.find("isPlayerInitiated\\\":true") == std::string::npos) return;

				// Parse the originating actor's FormID (the NPC who is speaking).
				constexpr std::string_view idKey = "\"originatingActorFormId\":";
				const auto ip = json.find(idKey);
				if (ip == std::string::npos) return;
				const auto formId = static_cast<RE::FormID>(
					std::strtoul(json.c_str() + ip + idKey.size(), nullptr, 10));
				if (formId == 0 || formId == 0x14) return;

				const auto dialogueText = ExtractEscapedField(json, "dialogue");
				logger::info("SkyrimNet dialogue: actor {:08X}, text='{}'", formId,
					dialogueText.size() > 120 ? dialogueText.substr(0, 120) + "..." : dialogueText);

				// Queue for the game's main thread (InputHandler::ProcessEvent).
				{
					std::lock_guard<std::mutex> lock(g_pendingMutex);
					g_pendingReveal.formId = formId;
					g_pendingReveal.dialogueText = dialogueText;
				}
			});

			return true;
		}
	}

	// The active hotkey's scan code. Loaded from the INI file at startup
	// (default "H" if the file is missing or the value can't be parsed).
	std::uint32_t g_revealKeyCode = 0x23;  // DIK_H

	constexpr const char* kIniPath = "Data/SKSE/Plugins/NNDDynamicReveal.ini";
	constexpr const char* kDefaultKeyName = "H";

	void LoadSettings()
	{
		CSimpleIniA ini;
		ini.SetUnicode();

		const auto result = ini.LoadFile(kIniPath);
		if (result != SI_OK) {
			ini.SetValue("General", nullptr, nullptr);
			ini.SetValue("General", "sRevealKey", kDefaultKeyName,
				"; Key that reveals the name of whatever NPC your crosshair is on.\n"
				"; Single keys only (no modifiers). Examples: H, G, F5, Insert, Comma.");
			ini.SetValue("SkyrimNet", nullptr, nullptr);
			ini.SetValue("SkyrimNet", "bAutoReveal", "1",
				"; Set to 0 to disable automatic name reveal when SkyrimNet NPCs mention their name in dialogue.\n"
				"; When enabled, the reveal respects NND's MCM 'Reveal on greeting' toggle.");
			ini.SaveFile(kIniPath);
			logger::info("No INI found, created default at {} (sRevealKey = {})", kIniPath, kDefaultKeyName);
			return;
		}

		const char* keyName = ini.GetValue("General", "sRevealKey", kDefaultKeyName);
		if (const auto code = KeyCodes::Parse(keyName)) {
			g_revealKeyCode = *code;
			logger::info("Reveal hotkey set to '{}' from INI.", keyName);
		} else {
			logger::error("Unrecognized sRevealKey '{}' in INI, falling back to '{}'.", keyName, kDefaultKeyName);
		}

		g_skyrimNetAutoReveal = ini.GetBoolValue("SkyrimNet", "bAutoReveal", true);
		logger::info("SkyrimNet auto-reveal: {}", g_skyrimNetAutoReveal ? "enabled" : "disabled");

		// Add any keys that are missing from an older INI (e.g. after a plugin update).
		bool dirty = false;
		if (!ini.GetValue("SkyrimNet", "bAutoReveal")) {
			ini.SetValue("SkyrimNet", nullptr, nullptr);
			ini.SetValue("SkyrimNet", "bAutoReveal", g_skyrimNetAutoReveal ? "1" : "0",
				"; Set to 0 to disable automatic name reveal when SkyrimNet NPCs mention their name in dialogue.\n"
				"; When enabled, the reveal respects NND's MCM 'Reveal on greeting' toggle.");
			dirty = true;
		}
		if (dirty) {
			ini.SaveFile(kIniPath);
			logger::info("Updated INI with missing keys.");
		}
	}

	void InitializeLogging()
	{
		auto path = logger::log_directory();
		if (!path) {
			SKSE::stl::report_and_fail("Failed to find the SKSE log directory");
		}

		// Diagnostic: prove the resolved directory + raw file I/O work, independent of spdlog,
		// before attempting the real spdlog sink. Temporary — remove once NNDDynamicReveal.log
		// is confirmed to appear reliably.
		{
			std::ofstream marker((*path / "NNDDynamicReveal.init.txt").string(), std::ios::trunc);
			marker << "resolved log_directory: " << path->string() << "\n";
			marker << "cwd: " << std::filesystem::current_path().string() << "\n";
		}

		*path /= "NNDDynamicReveal.log";

		try {
			auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
			auto log = std::make_shared<spdlog::logger>("NNDDynamicReveal", std::move(sink));
			log->set_level(spdlog::level::info);
			log->flush_on(spdlog::level::info);
			spdlog::set_default_logger(std::move(log));
		} catch (const std::exception& e) {
			std::ofstream marker((path->parent_path() / "NNDDynamicReveal.init.txt").string(), std::ios::app);
			marker << "spdlog sink construction threw: " << e.what() << "\n";
		}
	}

	// Reveals the name of whatever NPC the player's crosshair is currently on.
	void RevealCrosshairTarget()
	{
		if (!g_nnd) {
			logger::warn("Reveal hotkey pressed, but NND API was never obtained.");
			return;
		}

		if (const auto ui = RE::UI::GetSingleton(); ui && ui->GameIsPaused()) {
			return;
		}

		const auto pickData = RE::CrosshairPickData::GetSingleton();
		if (!pickData) {
			return;
		}

		const auto targetRef = pickData->targetActor.get().get();
		if (!targetRef) {
			return;
		}

		const auto actor = targetRef->As<RE::Actor>();
		if (!actor) {
			return;
		}

		g_nnd->RevealName(actor, NND_API::RevealReason::kDialogue);
		logger::info("Requested name reveal for actor {:08X}", actor->GetFormID());
	}

	class InputHandler final : public RE::BSTEventSink<RE::InputEvent*>
	{
	public:
		static InputHandler* GetSingleton()
		{
			static InputHandler singleton;
			return &singleton;
		}

		RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* a_event, RE::BSTEventSource<RE::InputEvent*>*) override
		{
			// Consume any pending dialogue auto-reveal queued by the SkyrimNet callback.
			// This runs on the main game thread, making RE:: calls safe.
			PendingReveal pending;
			{
				std::lock_guard<std::mutex> lock(g_pendingMutex);
				pending = g_pendingReveal;
				g_pendingReveal.formId = 0;
				g_pendingReveal.dialogueText.clear();
			}
			if (pending.formId && g_nnd) {
				if (const auto actor = RE::TESForm::LookupByID<RE::Actor>(pending.formId)) {
					if (!actor->IsPlayerRef()) {
						// NND only stores a raw name for actors it generated or customized a
						// name/title for. NPCs that keep their vanilla name untouched (just
						// obscured) get no entry, so GetRawName returns empty for them — fall
						// back to the actor's own name, which is what NND itself falls back to
						// for display purposes (see NNDData::UpdateDisplayName).
						auto rawName = g_nnd->GetRawName(actor);
						if (rawName.empty()) {
							rawName = actor->GetName();
						}
						if (!rawName.empty()) {
							// Strip disambiguation suffix before matching against dialogue text.
							std::string stripped{rawName.substr(0, rawName.find('['))};
							while (!stripped.empty() && stripped.back() == ' ') stripped.pop_back();
							if (!stripped.empty() && SkyrimNet::DialogueMentionsName(pending.dialogueText, stripped)) {
								g_nnd->RevealName(actor, NND_API::RevealReason::kDialogue);
								logger::info("Auto-reveal: {:08X} ('{}') mentioned name in dialogue", pending.formId, stripped);
							} else {
								logger::info("Auto-reveal: skipped {:08X} — name '{}' not found in dialogue", pending.formId, stripped.empty() ? rawName : stripped);
							}
						}
					}
				}
			}
			if (!a_event) {
				return RE::BSEventNotifyControl::kContinue;
			}

			for (auto event = *a_event; event; event = event->next) {
				const auto button = event->AsButtonEvent();
				if (!button || !button->IsDown()) {
					continue;
				}
				if (button->GetDevice() == RE::INPUT_DEVICE::kKeyboard && button->GetIDCode() == g_revealKeyCode) {
					RevealCrosshairTarget();
				}
			}

			return RE::BSEventNotifyControl::kContinue;
		}

	private:
		InputHandler() = default;
	};
}

SKSEPluginLoad(const SKSE::LoadInterface* skse)
{
	SKSE::Init(skse);
	InitializeLogging();
	LoadSettings();

	logger::info("NND Reveal Hotkey loaded.");

	SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message* message) {
		switch (message->type) {
		case SKSE::MessagingInterface::kPostLoad:
			g_nnd = static_cast<NND_API::IVNND4*>(NND_API::RequestPluginAPI(NND_API::InterfaceVersion::kV4));
			if (g_nnd) {
				logger::info("Obtained NPCs Names Distributor API.");
			} else {
				logger::error("Could not obtain NPCs Names Distributor API - is NND installed and active?");
			}
			break;
		case SKSE::MessagingInterface::kInputLoaded:
			if (const auto inputManager = RE::BSInputDeviceManager::GetSingleton()) {
				inputManager->AddEventSink(InputHandler::GetSingleton());
				logger::info("Registered input handler.");
			}
			break;
		case SKSE::MessagingInterface::kDataLoaded:
			if (SkyrimNet::TryRegisterNNDDecorator()) {
				logger::info("Registered 'nnd_name' decorator with SkyrimNet.");
			} else {
				logger::info("SkyrimNet not found or does not export PublicRegisterDecorator — skipping decorator registration.");
			}
			if (g_skyrimNetAutoReveal) {
				if (SkyrimNet::TryRegisterDialogueCallback()) {
					logger::info("Registered 'dialogue' event callback with SkyrimNet.");
				} else {
					logger::info("SkyrimNet not found or does not export PublicRegisterEventCallback — skipping dialogue callback.");
				}
			} else {
				logger::info("SkyrimNet auto-reveal disabled by INI — skipping dialogue callback.");
			}
			break;
		default:
			break;
		}
	});

	return true;
}

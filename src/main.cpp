#include "SimpleIni.h"
#include "POISE/PoiseMod.h"
#include "POISE/TrueHUDControl.h"
#include "POISE/TrueHUDAPI.h"
#include "Loki_PluginTools.h"

const SKSE::MessagingInterface* g_messaging2 = nullptr;


namespace PoiseMod {  // Papyrus Functions

    inline auto DamagePoise(RE::StaticFunctionTag*, RE::Actor* a_actor, float a_amount) -> void {

        if (!a_actor) {
            return;
        } else {
            int poise = (int)a_actor->pad0EC;
            poise -= (int)a_amount;
            a_actor->pad0EC = poise;
            if (a_actor->pad0EC > 100000) { a_actor->pad0EC = (uint32_t)0.00f; }
        }

    }

    inline auto RestorePoise(RE::StaticFunctionTag*, RE::Actor* a_actor, float a_amount) -> void {

        if (!a_actor) {
            return;
        } else {
            int poise = (int)a_actor->pad0EC;
            poise += (int)a_amount;
            a_actor->pad0EC = poise;
			if (a_actor->pad0EC > 100000) {
				a_actor->pad0EC = (uint32_t)0.00f;
			}
        }

    }

    inline auto GetPoise(RE::StaticFunctionTag*, RE::Actor* a_actor) -> float {

        if (!a_actor) {
            return -1.00f;
        } else {
            return (float)a_actor->pad0EC;
        }

    }

    inline auto GetMaxPoise(RE::StaticFunctionTag*, RE::Actor* a_actor) -> float {

        if (!a_actor) {
            return -1.00f;
        } else {
            auto ptr = Loki::PoiseMod::GetSingleton();

            float a_result = (a_actor->equippedWeight + (a_actor->GetBaseActorValue(RE::ActorValue::kHeavyArmor) * 0.20f));

            for (auto idx : ptr->poiseRaceMap) {
                if (a_actor) {
                    RE::TESRace* a_actorRace = a_actor->race;
                    RE::TESRace* a_mapRace = idx.first;
                    if (a_actorRace && a_mapRace) {
                        if (a_actorRace->formID == a_mapRace->formID) {
                            a_result = idx.second[1];
                            /*if (a_actor->HasKeyword(ptr->kCreature) || a_actor->HasKeyword(ptr->kDwarven)) {
                                a_result = idx.second[1];
                            } else {
                                a_result *= idx.second[1];
                            }
                            */
                            break;
                        }
                    }
                }
            }

            auto hasBuff = Loki::PluginTools::ActorHasEffectWithKeyword(a_actor, ptr->PoiseHPBuff->formID);
            if (hasBuff) {
                logger::info("health buff keyword detected");
                auto buffPercent = hasBuff->effectItem.magnitude / 100.00f; // convert to percentage
                auto resultingBuff = (a_result * buffPercent);
                a_result += resultingBuff;
            }
            auto hasNerf = Loki::PluginTools::ActorHasEffectWithKeyword(a_actor, ptr->PoiseHPNerf->formID);
            if (hasNerf) {
                logger::info("health nerf keyword detected");
                auto nerfPercent = hasNerf->effectItem.magnitude / 100.00f;
                auto resultingNerf = (a_result * nerfPercent);
                a_result -= resultingNerf;
            }

            return a_result;
        }

    }

    inline auto SetPoise(RE::StaticFunctionTag*, RE::Actor* a_actor, float a_amount) -> void {

        if (!a_actor) {
            return;
        } else {
            a_actor->pad0EC = (int)a_amount;
			if (a_actor->pad0EC > 100000) {
				a_actor->pad0EC = (uint32_t)0.00f;
			}
        }

    }

    bool RegisterFuncsForSKSE(RE::BSScript::IVirtualMachine* a_vm) {

        if (!a_vm) {
            return false;
        }

        a_vm->RegisterFunction("DamagePoise", "Loki_PoiseMod", DamagePoise, false);
        a_vm->RegisterFunction("RestorePoise", "Loki_PoiseMod", RestorePoise, false);
        a_vm->RegisterFunction("GetPoise", "Loki_PoiseMod", GetPoise, false);
        a_vm->RegisterFunction("SetPoise", "Loki_PoiseMod", SetPoise, false);

        return true;

    }

}

static void MessageHandler(SKSE::MessagingInterface::Message* message) {

    switch (message->type) {
    case SKSE::MessagingInterface::kDataLoaded: {
        auto ptr = Loki::TrueHUDControl::GetSingleton();
        if (ptr->TrueHUDBars) {
            if (ptr->g_trueHUD) {
                if (ptr->g_trueHUD->RequestSpecialResourceBarsControl(SKSE::GetPluginHandle()) == TRUEHUD_API::APIResult::OK) {
                    ptr->g_trueHUD->RegisterSpecialResourceFunctions(SKSE::GetPluginHandle(), Loki::TrueHUDControl::GetCurrentSpecial, Loki::TrueHUDControl::GetMaxSpecial, true);
                }
            }
        }
        break;
    }
    case SKSE::MessagingInterface::kNewGame:
    case SKSE::MessagingInterface::kPostLoadGame: {
        break;
    }
    case SKSE::MessagingInterface::kPostLoad: {
        Loki::TrueHUDControl::GetSingleton()->g_trueHUD = reinterpret_cast<TRUEHUD_API::IVTrueHUD3*>(TRUEHUD_API::RequestPluginAPI(TRUEHUD_API::InterfaceVersion::V3));
        if (Loki::TrueHUDControl::GetSingleton()->g_trueHUD) {
            logger::info("Obtained TrueHUD API -> {0:x}", (uintptr_t)Loki::TrueHUDControl::GetSingleton()->g_trueHUD);
        }
        else {
            logger::warn("Failed to obtain TrueHUD API");
        }
        break;
    }
    case SKSE::MessagingInterface::kPostPostLoad: {
    }
    default:
        break;
    }

}

void InitializeLog()
{
#ifndef NDEBUG
	auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
	auto path = logger::log_directory();
	if (!path) {
		util::report_and_fail("Failed to find standard logging directory"sv);
	}

	*path /= fmt::format("{}.log"sv, Plugin::NAME);
	auto       sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

#ifndef NDEBUG
	const auto level = spdlog::level::trace;
#else
	const auto level = spdlog::level::info;
#endif

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));
	log->set_level(level);
	log->flush_on(level);

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%l] %v"s);
}

EXTERN_C [[maybe_unused]] __declspec(dllexport) bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
#ifndef NDEBUG
	while (!IsDebuggerPresent()) {};
#endif

	InitializeLog();

	logger::info("POISE loaded");

	SKSE::Init(a_skse);
	SKSE::AllocTrampoline(64);

	auto messaging = SKSE::GetMessagingInterface();
	if (!messaging->RegisterListener("SKSE", MessageHandler)) {  // add callbacks for TrueHUD
		return false;
	}
	SKSE::GetPapyrusInterface()->Register(PoiseMod::RegisterFuncsForSKSE);  // register papyrus functions

	Loki::PoiseMod::InstallStaggerHook();
	Loki::PoiseMod::InstallWaterHook();
	Loki::PoiseMod::InstallIsActorKnockdownHook();
	Loki::PoiseMod::InstallMagicEventSink();

	return true;
}

EXTERN_C [[maybe_unused]] __declspec(dllexport) constinit auto SKSEPlugin_Version = []() noexcept {
	SKSE::PluginVersionData v;
	v.PluginName("PluginName");
	v.PluginVersion({ 1, 0, 0, 0 });
	v.UsesAddressLibrary(true);
	return v;
}();

EXTERN_C [[maybe_unused]] __declspec(dllexport) bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface*, SKSE::PluginInfo* pluginInfo)
{
	pluginInfo->name = SKSEPlugin_Version.pluginName;
	pluginInfo->infoVersion = SKSE::PluginInfo::kVersion;
	pluginInfo->version = SKSEPlugin_Version.pluginVersion;
	return true;
}

#include "Loki_PluginTools.h"
#include "POISE/PoiseMod.h"
#include "POISE/TrueHUDAPI.h"
#include "POISE/TrueHUDControl.h"

#include "NG/ActorCache.h"

const SKSE::MessagingInterface* g_messaging2 = nullptr;

namespace PoiseMod
{  // Papyrus Functions
	inline auto DamagePoise(RE::StaticFunctionTag*, RE::Actor* a_actor, float a_amount) -> void
	{
		if (!a_actor) {
			return;
		} else {
			int poise = (int)a_actor->GetActorRuntimeData().pad0EC;
			poise -= (int)a_amount;
			a_actor->GetActorRuntimeData().pad0EC = poise;
			if (a_actor->GetActorRuntimeData().pad0EC > 100000) {
				a_actor->GetActorRuntimeData().pad0EC = (uint32_t)0.00f;
			}
		}
	}

	inline auto RestorePoise(RE::StaticFunctionTag*, RE::Actor* a_actor, float a_amount) -> void
	{
		if (!a_actor) {
			return;
		} else {
			int poise = (int)a_actor->GetActorRuntimeData().pad0EC;
			poise += (int)a_amount;
			a_actor->GetActorRuntimeData().pad0EC = poise;
			if (a_actor->GetActorRuntimeData().pad0EC > 100000) {
				a_actor->GetActorRuntimeData().pad0EC = (uint32_t)0.00f;
			}
		}
	}

	inline auto GetPoise(RE::StaticFunctionTag*, RE::Actor* a_actor) -> float
	{
		if (!a_actor) {
			return -1.00f;
		} else {
			return (float)a_actor->GetActorRuntimeData().pad0EC;
		}
	}

	inline auto GetMaxPoise(RE::StaticFunctionTag*, RE::Actor* a_actor) -> float
	{
		if (!a_actor) {
			return -1.00f;
		} else {
			auto ptr = Loki::PoiseMod::GetSingleton();

			float a_result = (ActorCache::GetSingleton()->GetOrCreateCachedWeight(a_actor) + (a_actor->AsActorValueOwner()->GetBaseActorValue(RE::ActorValue::kHeavyArmor) * 0.20f));

			for (auto idx : ptr->poiseRaceMap) {
				if (a_actor) {
					RE::TESRace* a_actorRace = a_actor->GetActorRuntimeData().race;
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
				auto buffPercent = hasBuff->effectItem.magnitude / 100.00f;  // convert to percentage
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

	inline auto SetPoise(RE::StaticFunctionTag*, RE::Actor* a_actor, float a_amount) -> void
	{
		if (!a_actor) {
			return;
		} else {
			a_actor->GetActorRuntimeData().pad0EC = (int)a_amount;
			if (a_actor->GetActorRuntimeData().pad0EC > 100000) {
				a_actor->GetActorRuntimeData().pad0EC = (uint32_t)0.00f;
			}
		}
	}

	bool RegisterFuncsForSKSE(RE::BSScript::IVirtualMachine* a_vm)
	{
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

static void MessageHandler(SKSE::MessagingInterface::Message* message)
{
	switch (message->type) {
	case SKSE::MessagingInterface::kDataLoaded:
		{
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
		ActorCache::GetSingleton()->Revert();
	case SKSE::MessagingInterface::kPostLoadGame:
		{
			ActorCache::GetSingleton()->Revert();
			break;
		}
	case SKSE::MessagingInterface::kPostLoad:
		{
			Loki::TrueHUDControl::GetSingleton()->g_trueHUD = reinterpret_cast<TRUEHUD_API::IVTrueHUD3*>(TRUEHUD_API::RequestPluginAPI(TRUEHUD_API::InterfaceVersion::V3));
			if (Loki::TrueHUDControl::GetSingleton()->g_trueHUD) {
				logger::info("Obtained TrueHUD API -> {0:x}", (uintptr_t)Loki::TrueHUDControl::GetSingleton()->g_trueHUD);
			} else {
				logger::warn("Failed to obtain TrueHUD API");
			}
			break;
		}
	case SKSE::MessagingInterface::kPostPostLoad:
		{
		}
	default:
		break;
	}
}

void Load()
{
	SKSE::AllocTrampoline(64);

	auto messaging = SKSE::GetMessagingInterface();
	messaging->RegisterListener("SKSE", MessageHandler);                    // add callbacks for TrueHUD
	SKSE::GetPapyrusInterface()->Register(PoiseMod::RegisterFuncsForSKSE);  // register papyrus functions

	Loki::PoiseMod::InstallStaggerHook();
	Loki::PoiseMod::InstallWaterHook();
	Loki::PoiseMod::InstallIsActorKnockdownHook();
	Loki::PoiseMod::InstallMagicEventSink();

	ActorCache::RegisterEvents();
}

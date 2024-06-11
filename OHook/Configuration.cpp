#include "Configuration.h"
#include "PaliaOverlay.h"

#include <json.hpp>

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <type_traits>

using json = nlohmann::json;
namespace fs = std::filesystem;

// Current config version.
// WARNING: Only bump this version in case of
// non-backwards compatible config changes.
// When this gets bumped, an migration is needed.
int Configuration::Version = 2;

WindowSize Configuration::WindowSize;
GameModifiers Configuration::GameModifiers;
Fishing Configuration::Fishing;
Teleport Configuration::Teleport;
Housing Configuration::Housing;
Aimbot Configuration::Aimbot;
Misc Configuration::Misc;
ESPConfig Configuration::ESP;

bool Configuration::ConfigLoaded = false;

// Path for the configuration file
static const std::string configDirectory = "C:\\ProgramData\\OriginPalia\\config";
static const std::string configFileName = "overlay_config.json";
static const std::string configFilePath = configDirectory + "\\" + configFileName;

// Makes sure that the folder containing the Configuration
// Json file exists.
static void EnsureDirectoryExists(const std::string& path) {
    fs::path dir(path);
    if (!fs::exists(dir)) {
        fs::create_directories(dir);
    }
}

#pragma region ESPItems
void to_json(json& j, const ESPStarItem& p) {
	j = json{ {"Normal", p.Normal}, {"Star", p.Star}, {"Color", p.Color} };
}

void from_json(const json& j, ESPStarItem& p) {
	j.at("Normal").get_to(p.Normal);
	j.at("Star").get_to(p.Star);
	j.at("Color").get_to(p.Color);
}

void to_json(json& j, const ESPSingleItem& p) {
	j = json{ {"Enabled", p.Enabled}, {"Color", p.Color} };
}

void from_json(const json& j, ESPSingleItem& p) {
	j.at("Enabled").get_to(p.Enabled);
	j.at("Color").get_to(p.Color);
}

void to_json(json& j, const ESPSizeItem& p) {
	j = json{ {"Small", p.Small}, {"Medium", p.Medium}, {"Large", p.Large}, {"Color", p.Color} };
}

void from_json(const json& j, ESPSizeItem& p) {
	j.at("Small").get_to(p.Small);
	j.at("Medium").get_to(p.Medium);
	j.at("Large").get_to(p.Large);
	j.at("Color").get_to(p.Color);
}
#pragma endregion

#pragma region ESPConfig
void to_json(json& j, const struct ESPConfig::PlayerEntities& p) {
	j = json{
		{"FishHook", p.FishHook},
		{"FishPool", p.FishPool},
		{"Loot", p.Loot},
		{"NPC", p.NPC},
		{"Others", p.Others},
		{"Player", p.Player},
		{"Quest", p.Quest},
		{"RummagePiles", p.RummagePiles},
		{"Stables", p.Stables}
	};
}

void from_json(const json& j, struct ESPConfig::PlayerEntities& p) {
	j.at("FishHook").get_to(p.FishHook);
	j.at("FishPool").get_to(p.FishPool);
	j.at("Loot").get_to(p.Loot);
	j.at("NPC").get_to(p.NPC);
	j.at("Others").get_to(p.Others);
	j.at("Player").get_to(p.Player);
	j.at("Quest").get_to(p.Quest);
	j.at("RummagePiles").get_to(p.RummagePiles);
	j.at("Stables").get_to(p.Stables);
}

void to_json(json& j, const ESPConfig& p) {
	j = json{
		{"Enabled", p.Enabled},
		{"Culling", p.Culling},
		{"DrawFOVCircle", p.DrawFOVCircle},
		{"DespawnTimer", p.DespawnTimer},
		{"CullDistance", p.CullDistance},
		{"FOVRadius", p.FOVRadius},
		{"TextScale", p.TextScale},
		{"PlayerEntities", p.PlayerEntities},
		{"Ores", p.Ores},
		{"Trees", p.Trees},
		{"Animals", p.Animals},
		{"Forageables", p.Forageables},
		{"Bugs", p.Bugs}
	};
}

void from_json(const json& j, ESPConfig& p) {
	j.at("Enabled").get_to(p.Enabled);
	j.at("Culling").get_to(p.Culling);
	j.at("DrawFOVCircle").get_to(p.DrawFOVCircle);
	j.at("DespawnTimer").get_to(p.DespawnTimer);
	j.at("CullDistance").get_to(p.CullDistance);
	j.at("FOVRadius").get_to(p.FOVRadius);
	j.at("TextScale").get_to(p.TextScale);
	j.at("PlayerEntities").get_to(p.PlayerEntities);
	j.at("Ores").get_to(p.Ores);
	j.at("Trees").get_to(p.Trees);
	j.at("Animals").get_to(p.Animals);
	j.at("Forageables").get_to(p.Forageables);
	j.at("Bugs").get_to(p.Bugs);
}
#pragma endregion

#pragma region Config
void to_json(json& j, const struct WindowSize& p) {
	j = json{ {"X", p.X}, {"Y", p.Y} };
}

void from_json(const json& j, struct WindowSize& p) {
	j.at("X").get_to(p.X);
	j.at("Y").get_to(p.Y);
}

void to_json(json& j, const struct GameModifiers& p) {
	j = json{
		{"CustomWalkSpeed", p.CustomWalkSpeed},
		{"CustomSprintSpeedMultiplier", p.CustomSprintSpeedMultiplier},
		{"CustomClimbingSpeed", p.CustomClimbingSpeed},
		{"CustomGlidingSpeed", p.CustomGlidingSpeed},
		{"CustomGlidingFallSpeed", p.CustomGlidingFallSpeed},
		{"CustomJumpVelocity", p.CustomJumpVelocity},
		{"CustomMaxStepHeight", p.CustomMaxStepHeight}
	};
}

void from_json(const json& j, struct GameModifiers& p) {
	j.at("CustomWalkSpeed").get_to(p.CustomWalkSpeed);
	j.at("CustomSprintSpeedMultiplier").get_to(p.CustomSprintSpeedMultiplier);
	j.at("CustomClimbingSpeed").get_to(p.CustomClimbingSpeed);
	j.at("CustomGlidingSpeed").get_to(p.CustomGlidingSpeed);
	j.at("CustomGlidingFallSpeed").get_to(p.CustomGlidingFallSpeed);
	j.at("CustomJumpVelocity").get_to(p.CustomJumpVelocity);
	j.at("CustomMaxStepHeight").get_to(p.CustomMaxStepHeight);
}

void to_json(json& j, const struct Fishing& p) {
	j = json{ 
		{"NoDurability", p.NoDurability}, 
		{"MultiplayerHelp", p.MultiplayerHelp},
		{"InstantCatch", p.InstantCatch}, 
		{"PerfectCatch", p.PerfectCatch},
		{"SellFish", p.SellFish},
		{"DiscardTrash", p.DiscardTrash},
		{"OpenStoreWaterlogged", p.OpenStoreWaterlogged},
		{"RequireClickFishing", p.RequireClickFishing}
	};
}

void from_json(const json& j, struct Fishing& p) {
	j.at("NoDurability").get_to(p.NoDurability);
	j.at("MultiplayerHelp").get_to(p.MultiplayerHelp);
	j.at("InstantCatch").get_to(p.InstantCatch);
	j.at("PerfectCatch").get_to(p.PerfectCatch);
	j.at("SellFish").get_to(p.SellFish);
	j.at("DiscardTrash").get_to(p.DiscardTrash);
	j.at("OpenStoreWaterlogged").get_to(p.OpenStoreWaterlogged);
	j.at("RequireClickFishing").get_to(p.RequireClickFishing);
}

void to_json(json& j, const struct Teleport& p) {
	j = json{
		{"WaypointTeleport", p.WaypointTeleport}, 
		{"TeleportToTargeted", p.TeleportToTargeted},
		{"AvoidTeleportingToPlayers", p.AvoidTeleportingToPlayers},
		{"RadiusPlayersAvoidance", p.RadiusPlayersAvoidance},
		{"LootbagTeleportation", p.LootbagTeleportation},
		{"AvoidanceRadius", p.AvoidanceRadius}
	};
}

void from_json(const json& j, struct Teleport& p) {
	j.at("WaypointTeleport").get_to(p.WaypointTeleport);
	j.at("TeleportToTargeted").get_to(p.TeleportToTargeted);
	j.at("AvoidTeleportingToPlayers").get_to(p.AvoidTeleportingToPlayers);
	j.at("RadiusPlayersAvoidance").get_to(p.RadiusPlayersAvoidance);
	j.at("LootbagTeleportation").get_to(p.LootbagTeleportation);
	j.at("AvoidanceRadius").get_to(p.AvoidanceRadius);
}

void to_json(json& j, const struct Housing& p) {
	j = json{ 
		{"PlaceAnywhere", p.PlaceAnywhere}, 
		{"ManualPositionAdjustment", p.ManualPositionAdjustment},
		{"MaxUpAngle", p.MaxUpAngle}
	};
}

void from_json(const json& j, struct Housing& p) {
	j.at("PlaceAnywhere").get_to(p.PlaceAnywhere);
	j.at("ManualPositionAdjustment").get_to(p.ManualPositionAdjustment);
	j.at("MaxUpAngle").get_to(p.MaxUpAngle);
}

void to_json(json& j, const struct Aimbot& p) {
	j = json{ 
		{"LegacyAimbot", p.LegacyAimbot}, 
		{"SilentAimbot", p.SilentAimbot}
	};
}

void from_json(const json& j, struct Aimbot& p) {
	j.at("LegacyAimbot").get_to(p.LegacyAimbot);
	j.at("SilentAimbot").get_to(p.SilentAimbot);
}

void to_json(json& j, const struct Misc& p) {
	j = json{
		{"AntiAfk", p.AntiAfk},
		{"MinigameSkip", p.MinigameSkip},
		{"QuicksellHotkeys", p.QuicksellHotkeys},
		{"ChallengeEasyMode", p.ChallengeEasyMode},
		{"ShowWatermark", p.ShowWatermark} 
	};
}

void from_json(const json& j, struct Misc& p) {
	j.at("AntiAfk").get_to(p.AntiAfk);
	j.at("MinigameSkip").get_to(p.MinigameSkip);
	j.at("QuicksellHotkeys").get_to(p.QuicksellHotkeys);
	j.at("ChallengeEasyMode").get_to(p.ChallengeEasyMode);
	j.at("ShowWatermark").get_to(p.ShowWatermark);
}
#pragma endregion

// Loads configs from an Json File.
//
void Configuration::Load() {
	if (ConfigLoaded) return;

	InitializeDefaultColors();

	std::ifstream configFile(configFilePath);
	if (!configFile.is_open()) return;

	json j;
	configFile >> j;
	Configuration::WindowSize = j.at("WindowSize").template get<struct WindowSize>();
	Configuration::GameModifiers = j.at("GameModifiers").template get<struct GameModifiers>();
	Configuration::Fishing = j.at("Fishing").template get<struct Fishing>();
	Configuration::Teleport = j.at("Teleport").template get<struct Teleport>();
	Configuration::Housing = j.at("Housing").template get<struct Housing>();
	Configuration::Aimbot = j.at("Aimbot").template get<struct Aimbot>();
	Configuration::Misc = j.at("Misc").template get<struct Misc>();
	Configuration::ESP = j.at("ESP").template get<struct ESPConfig>();

	ConfigLoaded = true;
}

// Saves the current config to an Json File.
//
void Configuration::Save() {
	EnsureDirectoryExists(configDirectory);

	std::ofstream configFile(configFilePath);
	if (!configFile.is_open()) return;

	json j;
	j["Version"] = Configuration::Version;
	j["WindowSize"] = Configuration::WindowSize;
	j["GameModifiers"] = Configuration::GameModifiers;
	j["Fishing"] = Configuration::Fishing;
	j["Teleport"] = Configuration::Teleport;
	j["Housing"] = Configuration::Housing;
	j["Aimbot"] = Configuration::Aimbot;
	j["Misc"] = Configuration::Misc;
	j["ESP"] = Configuration::ESP;

	// Write prettified JSON to the file
	configFile << j.dump(4);
}

// Initializes the default colors for ESP
//
void Configuration::InitializeDefaultColors() {

	ESP.PlayerEntities.FishHook.Color = IM_COL32(0xFF, 0xFF, 0xFF, 0xFF);
	ESP.PlayerEntities.FishPool.Color = IM_COL32(0xFF, 0xFF, 0xFF, 0xFF);
	ESP.PlayerEntities.Loot.Color = IM_COL32(0xEE, 0x82, 0xEE, 0xFF);
	ESP.PlayerEntities.NPC.Color = IM_COL32(0xDE, 0xB8, 0x87, 0xFF);
	ESP.PlayerEntities.Others.Color = IM_COL32(0xFF, 0xFF, 0xFF, 0xFF);
	ESP.PlayerEntities.Player.Color = IM_COL32(0xFF, 0x63, 0x47, 0xFF);
	ESP.PlayerEntities.Quest.Color = IM_COL32(0xFF, 0xA5, 0x00, 0xFF);
	ESP.PlayerEntities.RummagePiles.Color = IM_COL32(0xFF, 0x45, 0x00, 0xFF);
	ESP.PlayerEntities.Stables.Color = IM_COL32(0x8B, 0x45, 0x13, 0xFF);

	ESP.Ores[EOreType::Clay].Color = IM_COL32(0xAD, 0x50, 0x49, 0xFF);
	ESP.Ores[EOreType::Stone].Color = IM_COL32(0x88, 0x8C, 0x8D, 0xFF);
	ESP.Ores[EOreType::Copper].Color = IM_COL32(0xB8, 0x73, 0x33, 0xFF);
	ESP.Ores[EOreType::Iron].Color = IM_COL32(0xA1, 0x9D, 0x94, 0xFF);
	ESP.Ores[EOreType::Palium].Color = IM_COL32(0x94, 0xA0, 0xE2, 0xFF);

	ESP.Trees[ETreeType::Bush].Color = IM_COL32(0xFF, 0xFF, 0xFF, 0xFF);
	ESP.Trees[ETreeType::Sapwood].Color = IM_COL32(0x00, 0xFF, 0x00, 0xFF);
	ESP.Trees[ETreeType::Heartwood].Color = IM_COL32(0x00, 0xFF, 0x00, 0xFF);
	ESP.Trees[ETreeType::Flow].Color = IM_COL32(0x67, 0x00, 0xEA, 0xFF);

	unsigned int AnimalTier1 = IM_COL32(0xCD, 0xCD, 0xCD, 0xFF);
	unsigned int AnimalTier2 = IM_COL32(0x32, 0xCD, 0x32, 0xFF);
	unsigned int AnimalTier3 = IM_COL32(0x1E, 0x90, 0xFF, 0xFF);
	unsigned int AnimalChase = IM_COL32(0xFF, 0xD7, 0x00, 0xFF);
	ESP.Animals[ECreatureKind::Chapaa][ECreatureQuality::Tier1].Color = AnimalTier1;
	ESP.Animals[ECreatureKind::Chapaa][ECreatureQuality::Tier2].Color = AnimalTier2;
	ESP.Animals[ECreatureKind::Chapaa][ECreatureQuality::Tier3].Color = AnimalTier3;
	ESP.Animals[ECreatureKind::Chapaa][ECreatureQuality::Chase].Color = AnimalChase;
	ESP.Animals[ECreatureKind::TreeClimber][ECreatureQuality::Tier1].Color = AnimalTier1;
	ESP.Animals[ECreatureKind::TreeClimber][ECreatureQuality::Tier2].Color = AnimalTier2;
	ESP.Animals[ECreatureKind::TreeClimber][ECreatureQuality::Tier3].Color = AnimalTier3;
	ESP.Animals[ECreatureKind::Cearnuk][ECreatureQuality::Tier1].Color = AnimalTier1;
	ESP.Animals[ECreatureKind::Cearnuk][ECreatureQuality::Tier2].Color = AnimalTier2;
	ESP.Animals[ECreatureKind::Cearnuk][ECreatureQuality::Tier3].Color = AnimalTier3;

	unsigned int BugCommon = IM_COL32(0xCD, 0xCD, 0xCD, 0xFF);
	unsigned int BugUncommon = IM_COL32(0x32, 0xCD, 0x32, 0xFF);
	unsigned int BugRare = IM_COL32(0x1E, 0x90, 0xFF, 0xFF);
	unsigned int BugRare2 = IM_COL32(0x00, 0xBF, 0xFF, 0xFF);
	unsigned int BugEpic = IM_COL32(0xFF, 0xD7, 0x00, 0xFF);
	ESP.Bugs[EBugKind::Bee][EBugQuality::Uncommon].Color = BugUncommon;
	ESP.Bugs[EBugKind::Bee][EBugQuality::Rare].Color = BugRare;
	ESP.Bugs[EBugKind::Beetle][EBugQuality::Common].Color = BugCommon;
	ESP.Bugs[EBugKind::Beetle][EBugQuality::Uncommon].Color = BugUncommon;
	ESP.Bugs[EBugKind::Beetle][EBugQuality::Rare].Color = BugRare;
	ESP.Bugs[EBugKind::Beetle][EBugQuality::Epic].Color = BugEpic;
	ESP.Bugs[EBugKind::Butterfly][EBugQuality::Common].Color = BugCommon;
	ESP.Bugs[EBugKind::Butterfly][EBugQuality::Uncommon].Color = BugUncommon;
	ESP.Bugs[EBugKind::Butterfly][EBugQuality::Rare].Color = BugRare;
	ESP.Bugs[EBugKind::Butterfly][EBugQuality::Epic].Color = BugEpic;
	ESP.Bugs[EBugKind::Cicada][EBugQuality::Common].Color = BugCommon;
	ESP.Bugs[EBugKind::Cicada][EBugQuality::Uncommon].Color = BugUncommon;
	ESP.Bugs[EBugKind::Cicada][EBugQuality::Rare].Color = BugRare;
	ESP.Bugs[EBugKind::Crab][EBugQuality::Common].Color = BugCommon;
	ESP.Bugs[EBugKind::Crab][EBugQuality::Uncommon].Color = BugUncommon;
	ESP.Bugs[EBugKind::Crab][EBugQuality::Rare].Color = BugRare;
	ESP.Bugs[EBugKind::Cricket][EBugQuality::Common].Color = BugCommon;
	ESP.Bugs[EBugKind::Cricket][EBugQuality::Uncommon].Color = BugUncommon;
	ESP.Bugs[EBugKind::Cricket][EBugQuality::Rare].Color = BugRare;
	ESP.Bugs[EBugKind::Dragonfly][EBugQuality::Common].Color = BugCommon;
	ESP.Bugs[EBugKind::Dragonfly][EBugQuality::Uncommon].Color = BugUncommon;
	ESP.Bugs[EBugKind::Dragonfly][EBugQuality::Rare].Color = BugRare;
	ESP.Bugs[EBugKind::Dragonfly][EBugQuality::Epic].Color = BugEpic;
	ESP.Bugs[EBugKind::Glowbug][EBugQuality::Common].Color = BugCommon;
	ESP.Bugs[EBugKind::Glowbug][EBugQuality::Uncommon].Color = BugUncommon;
	ESP.Bugs[EBugKind::Ladybug][EBugQuality::Common].Color = BugCommon;
	ESP.Bugs[EBugKind::Ladybug][EBugQuality::Uncommon].Color = BugUncommon;
	ESP.Bugs[EBugKind::Mantis][EBugQuality::Uncommon].Color = BugUncommon;
	ESP.Bugs[EBugKind::Mantis][EBugQuality::Rare].Color = BugRare;
	ESP.Bugs[EBugKind::Mantis][EBugQuality::Rare2].Color = BugRare2;
	ESP.Bugs[EBugKind::Mantis][EBugQuality::Epic].Color = BugEpic;
	ESP.Bugs[EBugKind::Moth][EBugQuality::Common].Color = BugCommon;
	ESP.Bugs[EBugKind::Moth][EBugQuality::Uncommon].Color = BugUncommon;
	ESP.Bugs[EBugKind::Moth][EBugQuality::Rare].Color = BugRare;
	ESP.Bugs[EBugKind::Pede][EBugQuality::Uncommon].Color = BugUncommon;
	ESP.Bugs[EBugKind::Pede][EBugQuality::Rare].Color = BugRare;
	ESP.Bugs[EBugKind::Pede][EBugQuality::Rare2].Color = BugRare2;

	unsigned int ForageableCommon = IM_COL32(0xCD, 0xCD, 0xCD, 0xFF);
	unsigned int ForageableUncommon = IM_COL32(0x32, 0xCD, 0x32, 0xFF);
	unsigned int ForageableRare = IM_COL32(0x1E, 0x90, 0xFF, 0xFF);
	unsigned int ForageableEpic = IM_COL32(0xFF, 0xD7, 0x00, 0xFF);
	ESP.Forageables[EForageableType::Coral].Color = ForageableUncommon;
	ESP.Forageables[EForageableType::Oyster].Color = ForageableCommon;
	ESP.Forageables[EForageableType::Shell].Color = ForageableCommon;
	ESP.Forageables[EForageableType::PoisonFlower].Color = ForageableUncommon;
	ESP.Forageables[EForageableType::WaterFlower].Color = ForageableUncommon;
	ESP.Forageables[EForageableType::Heartdrop].Color = ForageableEpic;
	ESP.Forageables[EForageableType::Sundrop].Color = ForageableCommon;
	ESP.Forageables[EForageableType::DragonsBeard].Color = ForageableRare;
	ESP.Forageables[EForageableType::EmeraldCarpet].Color = ForageableUncommon;
	ESP.Forageables[EForageableType::MushroomBlue].Color = ForageableRare;
	ESP.Forageables[EForageableType::MushroomRed].Color = ForageableCommon;
	ESP.Forageables[EForageableType::DariCloves].Color = ForageableEpic;
	ESP.Forageables[EForageableType::HeatRoot].Color = ForageableRare;
	ESP.Forageables[EForageableType::SpicedSprouts].Color = ForageableUncommon;
	ESP.Forageables[EForageableType::SweetLeaves].Color = ForageableUncommon;
	ESP.Forageables[EForageableType::Garlic].Color = ForageableUncommon;
	ESP.Forageables[EForageableType::Ginger].Color = ForageableUncommon;
	ESP.Forageables[EForageableType::GreenOnion].Color = ForageableUncommon;
}

// I hate this so much
// All of the functions below have to go eventually
// This is just so Configuration can work with the ESP code we have now.
bool Configuration::IsUnknown(const TypeEnum& type) {
	return std::visit([](auto&& arg) -> bool {
		using T = std::decay_t<decltype(arg)>;
		if constexpr (std::is_same_v<T, EOreType>) {
			return arg == EOreType::Unknown;
		}
		else if constexpr (std::is_same_v<T, ETreeType>) {
			return arg == ETreeType::Unknown;
		}
		else if constexpr (std::is_same_v<T, EForageableType>) {
			return arg == EForageableType::Unknown;
		}
		else if constexpr (std::is_same_v<T, EBugKind>) {
			return arg == EBugKind::Unknown;
		}
		else if constexpr (std::is_same_v<T, ECreatureKind>) {
			return arg == ECreatureKind::Unknown;
		}
		return false;
		}, type);
}

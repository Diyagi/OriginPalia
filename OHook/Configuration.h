#pragma once
#include <json.hpp>
#include "PaliaOverlay.h"
using json = nlohmann::json;

#pragma region ESPConfig
struct ESPStarItem {
	bool Normal = false;
	bool Star = false;
	unsigned int Color;

	bool Enabled(int quality) {
		if (quality == 1) return Star;
		else return Normal;
	}
};

struct ESPSingleItem {
	bool Enabled = false;
	unsigned int Color;
};

struct ESPSizeItem {
	bool Default = false;
	bool Small = false;
	bool Medium = false;
	bool Large = false;
	unsigned int Color;

	bool* Enabled(EGatherableSize size) {
		switch (size) {
		case EGatherableSize::Bush:
		case EGatherableSize::Small:
			return &Small;
			break;
		case EGatherableSize::Medium:
			return &Medium;
			break;
		case EGatherableSize::Large:
			return &Large;
			break;
		default:
			return &Default;
			break;
		}
	}
};

typedef std::map<ECreatureQuality, ESPSingleItem> AnimalQuality;
typedef std::map<ECreatureKind, AnimalQuality> AnimalConfig;

typedef std::map<EBugQuality, ESPStarItem> BugQuality;
typedef std::map<EBugKind, BugQuality> BugConfig;

typedef std::map<EOreType, ESPSizeItem> OresConfig;
typedef std::map<ETreeType, ESPSizeItem> TreeConfig;
typedef std::map<EForageableType, ESPStarItem> ForageablesConfig;

NLOHMANN_JSON_SERIALIZE_ENUM(ECreatureQuality, {
	{ECreatureQuality::Tier1, "Tier1"},
	{ECreatureQuality::Tier2, "Tier2"},
	{ECreatureQuality::Tier3, "Tier3"},
	{ECreatureQuality::Chase, "Chase"}
	});
NLOHMANN_JSON_SERIALIZE_ENUM(ECreatureKind, {
	{ECreatureKind::Chapaa, "Chapaa"},
	{ECreatureKind::Cearnuk, "Cearnuk"},
	{ECreatureKind::TreeClimber, "TreeClimber"}
	});
NLOHMANN_JSON_SERIALIZE_ENUM(EBugQuality, {
	{EBugQuality::Common, "Common"},
	{EBugQuality::Uncommon, "Uncommon"},
	{EBugQuality::Rare, "Rare"},
	{EBugQuality::Rare2, "Rare2"},
	{EBugQuality::Epic, "Epic"}
	});
NLOHMANN_JSON_SERIALIZE_ENUM(EBugKind, {
	{EBugKind::Bee, "Bee"},
	{EBugKind::Beetle, "Beetle"},
	{EBugKind::Butterfly, "Butterfly"},
	{EBugKind::Cicada, "Cicada"},
	{EBugKind::Crab, "Crab"},
	{EBugKind::Cricket, "Cricket"},
	{EBugKind::Dragonfly, "Dragonfly"},
	{EBugKind::Glowbug, "Glowbug"},
	{EBugKind::Ladybug, "Ladybug"},
	{EBugKind::Mantis, "Mantis"},
	{EBugKind::Moth, "Moth"},
	{EBugKind::Pede, "Pede"},
	{EBugKind::Snail, "Snail"}
	});
NLOHMANN_JSON_SERIALIZE_ENUM(EOreType, {
	{EOreType::Stone, "Stone"},
	{EOreType::Copper, "Copper"},
	{EOreType::Clay, "Clay"},
	{EOreType::Iron, "Iron"},
	{EOreType::Palium, "Palium"}
	});
NLOHMANN_JSON_SERIALIZE_ENUM(ETreeType, {
	{ETreeType::Flow, "Flow"},
	{ETreeType::Heartwood, "Heartwood"},
	{ETreeType::Sapwood, "Sapwood"},
	{ETreeType::Bush, "Bush"}
	});
NLOHMANN_JSON_SERIALIZE_ENUM(EForageableType, {
	{EForageableType::Oyster, "Oyster"},
	{EForageableType::Coral, "Coral"},
	{EForageableType::MushroomBlue, "MushroomBlue"},
	{EForageableType::MushroomRed, "MushroomRed"},
	{EForageableType::Heartdrop, "Heartdrop"},
	{EForageableType::DragonsBeard, "DragonsBeard"},
	{EForageableType::EmeraldCarpet, "EmeraldCarpet"},
	{EForageableType::PoisonFlower, "PoisonFlower"},
	{EForageableType::Shell, "Shell"},
	{EForageableType::DariCloves, "DariCloves"},
	{EForageableType::HeatRoot, "HeatRoot"},
	{EForageableType::SpicedSprouts, "SpicedSprouts"},
	{EForageableType::Sundrop, "Sundrop"},
	{EForageableType::SweetLeaves, "SweetLeaves"},
	{EForageableType::WaterFlower, "WaterFlower"},
	{EForageableType::Garlic, "Garlic"},
	{EForageableType::Ginger, "Ginger"},
	{EForageableType::GreenOnion, "GreenOnion"}
	});

struct ESPConfig {
	bool Enabled = true;
	bool Culling = true;
	bool DrawFOVCircle = true;
	bool DespawnTimer = true;
	int CullDistance = 200;
	float FOVRadius = 185.0f;
	float TextScale = 1.0f;

	struct PlayerEntities {
		ESPSingleItem Player;
		ESPSingleItem NPC;
		ESPSingleItem FishHook;
		ESPSingleItem FishPool;
		ESPSingleItem Loot;
		ESPSingleItem Quest;
		ESPSingleItem RummagePiles;
		ESPSingleItem Stables;
		ESPSingleItem Others;
	} PlayerEntities;

	OresConfig Ores = {};
	TreeConfig Trees = {};
	AnimalConfig Animals = {};
	ForageablesConfig Forageables = {};
	BugConfig Bugs = {};
};
#pragma endregion

#pragma region Config
struct WindowSize {
	float X = 1450.0f;
	float Y = 950.0f;
};

struct GameModifiers {
	float CustomWalkSpeed = 565.0f;
	float CustomSprintSpeedMultiplier = 1.65f;
	float CustomClimbingSpeed = 80.0f;
	float CustomGlidingSpeed = 900.0f;
	float CustomGlidingFallSpeed = 250.0f;
	float CustomJumpVelocity = 700.0f;
	float CustomMaxStepHeight = 45.0f;
};

struct Fishing {
	bool NoDurability = true;
	bool MultiplayerHelp = false;
	bool InstantCatch = false;
	bool PerfectCatch = true;
	bool SellFish = false;
	bool DiscardTrash = false;
	bool OpenStoreWaterlogged = false;
	bool RequireClickFishing = true;
};

struct Teleport {
	bool WaypointTeleport = false;
	bool TeleportToTargeted = false;
	bool AvoidTeleportingToPlayers = true;
	bool RadiusPlayersAvoidance = true;
	bool LootbagTeleportation = false;
	int AvoidanceRadius = 30;
};

struct Housing {
	bool PlaceAnywhere = false;
	bool ManualPositionAdjustment = false;
	float MaxUpAngle = 360.0f;
};

struct Aimbot {
	bool LegacyAimbot = false;
	bool SilentAimbot = false;
};

struct Misc {
	bool AntiAfk = false;
	bool MinigameSkip = false;
	bool QuicksellHotkeys = false;
	bool ChallengeEasyMode = false;
	bool ShowWatermark = true;
};
#pragma endregion

// Forward declaration of PaliaOverlay class
class PaliaOverlay;

class Configuration final {
private:
	static void InitializeDefaultColors();
	static bool ConfigLoaded;
	static int Version;

public:
	static void Load();
	static void Save();
	static bool IsUnknown(const TypeEnum& type);

	template <typename T>
	static T GetConfig(const TypeEnum& type, const VariantEnum& variant) {
		return std::visit([&](auto&& argType) -> T {
			using Type = std::decay_t<decltype(argType)>;
			if constexpr (std::is_same_v<Type, EBugKind>) {
				if (std::holds_alternative<EBugQuality>(variant)) {
					return Configuration::ESP.Bugs[argType][std::get<EBugQuality>(variant)];
				}
			}
			else if constexpr (std::is_same_v<Type, ECreatureKind>) {
				if (std::holds_alternative<ECreatureQuality>(variant)) {
					return Configuration::ESP.Animals[argType][std::get<ECreatureQuality>(variant)];
				}
			}
			return T{};
			}, type);
	};

	template <typename T>
	static T GetConfig(const TypeEnum& type) {
		return std::visit([](auto&& arg) -> T {
			using Type = std::decay_t<decltype(arg)>;
			if constexpr (std::is_same_v<Type, EOreType>) {
				return Configuration::ESP.Ores[arg];
			}
			else if constexpr (std::is_same_v<Type, ETreeType>) {
				return Configuration::ESP.Trees[arg];
			}
			else if constexpr (std::is_same_v<Type, EForageableType>) {
				return Configuration::ESP.Forageables[arg];
			}
			else if constexpr (std::is_same_v<Type, EFishType>) {
				if (arg == EFishType::Hook) return Configuration::ESP.PlayerEntities.FishHook;
				else if (arg == EFishType::Node) return Configuration::ESP.PlayerEntities.FishPool;
			}
			return T{};
			}, type);
	};

	static WindowSize WindowSize;
	static GameModifiers GameModifiers;
	static Fishing Fishing;
	static Teleport Teleport;
	static Housing Housing;
	static Aimbot Aimbot;
	static Misc Misc;
	static ESPConfig ESP;
};
#include "PaliaOverlay.h"
#include "DetourManager.h"
#include <SDK/Palia_parameters.hpp>

#include "ImGuiExt.h"
#include "SDKExt.h"
#include "Utils.h"

#include <algorithm>
#include <imgui_internal.h>
#include "Configuration.h"

using namespace SDK;

std::vector<std::string> debugger;
DetourManager gDetourManager;

std::map<int, std::string> PaliaOverlay::CreatureQualityNames = {
    {0, "Unknown"},
    {1, "T1"},
    {2, "T2"},
    {3, "T3"},
    {4, "Chase"}
};
std::map<int, std::string> PaliaOverlay::BugQualityNames = {
    {0, "Unknown"},
    {1, "Common"},
    {2, "Uncommon"},
    {3, "Rare"},
    {4, "Rare2"},
    {5, "Epic"}
};
std::map<int, std::string> PaliaOverlay::GatherableSizeNames = {
    {0, "Unknown"},
    {1, "Sm"},
    {2, "Md"},
    {3, "Lg"},
    {4, "Bush"}
};

AValeriaCharacter* GetValeriaData() {
    AValeriaCharacter* ValeriaCharacter = GetValeriaCharacter();
    if (UWorld* World = GetWorld()) {
        if (UGameInstance* GameInstance = World->OwningGameInstance; GameInstance && GameInstance->LocalPlayers.Num() > 0) {
            if (ULocalPlayer* LocalPlayer = GameInstance->LocalPlayers[0]) {
                if (APlayerController* PlayerController = LocalPlayer->PlayerController) {
                    if (PlayerController && PlayerController->Pawn) {
                        ValeriaCharacter = static_cast<AValeriaPlayerController*>(PlayerController)->GetValeriaCharacter();
                    }
                }
            }
        }
    }
    return ValeriaCharacter;
}

void PaliaOverlay::DrawHUD() {
    PaliaOverlay* Overlay = static_cast<PaliaOverlay*>(OverlayBase::Instance);

    Configuration::Load();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable keyboard navigation once at initialization if possible.

    ImGui::SetNextWindowBgAlpha(0.35f);
    ImGuiStyle& style = ImGui::GetStyle();
    const float prevWindowRounding = style.WindowRounding;
    style.WindowRounding = 5.0f; // Temporary change of style.

    std::string watermarkText = "OriginPalia By Wimberton, Void, & The UnknownCheats Community";
    bool showWatermark = false;
    if ((CurrentLevel && (CurrentMap == "MAP_PreGame" || CurrentMap == "Unknown")) || Configuration::Misc.ShowWatermark) {
        if (CurrentMap == "MAP_PreGame" || CurrentMap == "Unknown") {
            watermarkText = "Waiting for the game to load...";
        }
        showWatermark = true;
    }

    if (showWatermark) {
        ImGui::SetNextWindowPos(ImVec2((io.DisplaySize.x - ImGui::CalcTextSize(watermarkText.c_str()).x) * 0.5f, 10.0f));
        ImGui::Begin("Watermark", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);
        ImGui::Text("%s", watermarkText.c_str());
        ImGui::End();
    }

    style.WindowRounding = prevWindowRounding;

    APlayerController* PlayerController = nullptr;
    AValeriaPlayerController* ValeriaController = nullptr;
    AValeriaCharacter* ValeriaCharacter = nullptr;

    if (UWorld* World = GetWorld()) {
        if (UGameInstance* GameInstance = World->OwningGameInstance; GameInstance && GameInstance->LocalPlayers.Num() > 0) {
            if (ULocalPlayer* LocalPlayer = GameInstance->LocalPlayers[0]) {
                if (PlayerController = LocalPlayer->PlayerController; PlayerController && PlayerController->Pawn) {
                    ValeriaController = static_cast<AValeriaPlayerController*>(PlayerController);
                    if (ValeriaController) {
                        ValeriaCharacter = static_cast<AValeriaPlayerController*>(PlayerController)->GetValeriaCharacter();
                    }
                }
            }
        }
    }

    if (ValeriaController) {
        if (UTrackingComponent* TrackingComponent = ValeriaController->GetTrackingComponent(); TrackingComponent != nullptr) {
            gDetourManager.SetupDetour(TrackingComponent);
        }
    }

    // HOOKS
    if (ValeriaCharacter) {

        // INVENTORY COMPONENT
        if (UInventoryComponent* InventoryComponent = ValeriaCharacter->GetInventory(); InventoryComponent != nullptr) {
            gDetourManager.SetupDetour(InventoryComponent);
        }

        // FISHING COMPONENT
        if (UFishingComponent* FishingComponent = ValeriaCharacter->GetFishing(); FishingComponent != nullptr) {
            gDetourManager.SetupDetour(FishingComponent);
        }

        // FIRING COMPONENT
        if (UProjectileFiringComponent* FiringComponent = ValeriaCharacter->GetFiringComponent(); FiringComponent != nullptr) {
            gDetourManager.SetupDetour(FiringComponent);
        }

        // MOVEMENT COMPONENT
        if (UValeriaCharacterMoveComponent* ValeriaMovementComponent = ValeriaCharacter->GetValeriaCharacterMovementComponent(); ValeriaMovementComponent != nullptr) {
            gDetourManager.SetupDetour(ValeriaMovementComponent);
        }

        // PLACEMENT COMPONENT
        if (UPlacementComponent* PlacementComponent = ValeriaCharacter->GetPlacement(); PlacementComponent != nullptr) {
            gDetourManager.SetupDetour(PlacementComponent);
        }
    }

    // HOOKING PROCESSEVENT IN AHUD
    if (PlayerController && HookedClient != PlayerController->MyHUD && PlayerController->MyHUD != nullptr) {
        gDetourManager.SetupDetour(PlayerController->MyHUD);
    }
}

void PaliaOverlay::DrawOverlay() {
    bool show = true;
    const ImGuiIO& io = ImGui::GetIO();
    constexpr ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse;

    // Calculate the center position for the window
    const auto center_pos = ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
    const auto window_size = ImVec2(Configuration::WindowSize.X, Configuration::WindowSize.Y);
    const auto window_pos = ImVec2(center_pos.x - window_size.x * 0.5f, center_pos.y - window_size.y * 0.5f);

    // Set the initial window position to the center of the screen
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(window_size, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.98f);

    const auto WindowTitle = std::string("OriginPalia V2.7.1 - Game Version 0.180.1");
    PaliaOverlay* Overlay = static_cast<PaliaOverlay*>(OverlayBase::Instance);

    if (ImGui::Begin(WindowTitle.data(), &show, window_flags)) {
        static int OpenTab = 0;

        if (ImGui::IsMouseDragging(0)) {
            Configuration::WindowSize.X = static_cast<float>(ImGui::GetWindowSize().x);
            Configuration::WindowSize.Y = static_cast<float>(ImGui::GetWindowSize().y);
            Configuration::Save();
        }

        // Draw tabs
        if (ImGui::BeginTabBar("OverlayTabs")) {
            if (ImGui::BeginTabItem("ESP & Visuals")) {
                OpenTab = 0;
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Aimbots & Fun")) {
                OpenTab = 1;
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Move & Teleport")) {
                OpenTab = 2;
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Selling & Items")) {
                OpenTab = 3;
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Skills & Tools")) {
                OpenTab = 4;
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Housing & Decor")) {
                OpenTab = 5;
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        // ==================================== 0 Visuals & ESPs TAB
        if (OpenTab == 0) {
            ImGui::Columns(3, nullptr, false);

            // Base ESP controls
            if (ImGui::CollapsingHeader("Visual Settings - General##VisualSettingsGeneralHeader", ImGuiTreeNodeFlags_DefaultOpen)) {
                if (ImGui::Checkbox("Show Watermark", &Configuration::Misc.ShowWatermark)) {
                    Configuration::Save();
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Toggle display of the custom watermark on the screen.");

                if (ImGui::Checkbox("Enable ESP", &Configuration::ESP.Enabled)) {
                    Configuration::Save();
                }
                if (ImGui::Checkbox("Enable Despawn Timer", &Configuration::ESP.DespawnTimer)) {
                    Configuration::Save();
                }
                if (ImGui::SliderFloat("ESP Text Scale", &Configuration::ESP.TextScale, 0.5f, 3.0f, "%.1f")) {
					Configuration::Save();
                }
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) 
                    ImGui::SetTooltip("Adjust the scale of ESP text size.");

                Configuration::ESP.CullDistance = std::clamp(Configuration::ESP.CullDistance, 10, 999);
                if (ImGui::InputInt("ESP Distance", &Configuration::ESP.CullDistance)) {
                    Configuration::ESP.CullDistance = std::clamp(Configuration::ESP.CullDistance, 10, 999);
                    Configuration::Save();
                }

                if (ImGui::Checkbox("Enable InteliAim Circle", &Configuration::ESP.DrawFOVCircle)) {
                    Configuration::Save();
                }
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                    ImGui::SetTooltip("Enable the smart FOV targeting system. Teleport to actors, enable aimbots, and more.");

                if (Configuration::ESP.DrawFOVCircle && ImGui::SliderFloat("InteliAim Radius", &Configuration::ESP.FOVRadius, 10.0f, 600.0f, "%1.0f")) {
                    Configuration::Save();
                }
            }

            ImGui::NextColumn();

            if (ImGui::CollapsingHeader("Animals##AnimalsSettingsHeader")) {

                if (ImGui::Button("Sernuk##SernukBtn")) {
                    Configuration::ESP.Animals[ECreatureKind::Cearnuk][ECreatureQuality::Tier1].Enabled =
						!Configuration::ESP.Animals[ECreatureKind::Cearnuk][ECreatureQuality::Tier1].Enabled;

                    Configuration::ESP.Animals[ECreatureKind::Cearnuk][ECreatureQuality::Tier2].Enabled =
                        !Configuration::ESP.Animals[ECreatureKind::Cearnuk][ECreatureQuality::Tier2].Enabled;

                    Configuration::ESP.Animals[ECreatureKind::Cearnuk][ECreatureQuality::Tier3].Enabled =
                        !Configuration::ESP.Animals[ECreatureKind::Cearnuk][ECreatureQuality::Tier3].Enabled;

                    Configuration::Save();
                }
                ImGui::SameLine();
                if (ImGui::Button("Chapaa##ChapaaBtn")) {
                    Configuration::ESP.Animals[ECreatureKind::Chapaa][ECreatureQuality::Tier1].Enabled =
						!Configuration::ESP.Animals[ECreatureKind::Chapaa][ECreatureQuality::Tier1].Enabled;

                    Configuration::ESP.Animals[ECreatureKind::Chapaa][ECreatureQuality::Tier2].Enabled =
						!Configuration::ESP.Animals[ECreatureKind::Chapaa][ECreatureQuality::Tier2].Enabled;

                    Configuration::ESP.Animals[ECreatureKind::Chapaa][ECreatureQuality::Tier3].Enabled =
                        !Configuration::ESP.Animals[ECreatureKind::Chapaa][ECreatureQuality::Tier3].Enabled;

                    Configuration::ESP.Animals[ECreatureKind::Chapaa][ECreatureQuality::Chase].Enabled =
                        !Configuration::ESP.Animals[ECreatureKind::Chapaa][ECreatureQuality::Chase].Enabled;

                    Configuration::Save();
                }
                ImGui::SameLine();
                if (ImGui::Button("Muujin##MuujinBtn")) {
                    Configuration::ESP.Animals[ECreatureKind::TreeClimber][ECreatureQuality::Tier1].Enabled =
						!Configuration::ESP.Animals[ECreatureKind::TreeClimber][ECreatureQuality::Tier1].Enabled;

                    Configuration::ESP.Animals[ECreatureKind::TreeClimber][ECreatureQuality::Tier2].Enabled =
                        !Configuration::ESP.Animals[ECreatureKind::TreeClimber][ECreatureQuality::Tier2].Enabled;

                    Configuration::ESP.Animals[ECreatureKind::TreeClimber][ECreatureQuality::Tier3].Enabled =
                        !Configuration::ESP.Animals[ECreatureKind::TreeClimber][ECreatureQuality::Tier3].Enabled;

                    Configuration::Save();
                }

                ImGui::BeginTable("Animals", 3);
                {
                    ImGui::TableSetupColumn("Name");
                    ImGui::TableSetupColumn("Show", ImGuiTableColumnFlags_WidthFixed, 40);
                    ImGui::TableSetupColumn("Color", ImGuiTableColumnFlags_WidthFixed, 40);
                    ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
                    ImGui::TableNextColumn();
                    ImGui::TableNextColumn();
                    ImGui::Text("Show");
                    ImGui::TableNextColumn();
                    ImGui::Text("Color");
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Sernuk");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##Sernuk", &Configuration::ESP.Animals[ECreatureKind::Cearnuk][ECreatureQuality::Tier1].Enabled)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##Sernuk", &Configuration::ESP.Animals[ECreatureKind::Cearnuk][ECreatureQuality::Tier1].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Elder Sernuk");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##ElderSernuk", &Configuration::ESP.Animals[ECreatureKind::Cearnuk][ECreatureQuality::Tier2].Enabled)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##ElderSernuk", &Configuration::ESP.Animals[ECreatureKind::Cearnuk][ECreatureQuality::Tier2].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Proudhorn Sernuk");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##ProudhornSernuk", &Configuration::ESP.Animals[ECreatureKind::Cearnuk][ECreatureQuality::Tier3].Enabled)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##ProudhornSernuk", &Configuration::ESP.Animals[ECreatureKind::Cearnuk][ECreatureQuality::Tier3].Color);
                    ImGui::TableNextColumn();
                    ImGui::TableNextColumn();
                    ImGui::Text("Show");
                    ImGui::TableNextColumn();
                    ImGui::Text("Color");
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Chapaa");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##Chapaa", &Configuration::ESP.Animals[ECreatureKind::Chapaa][ECreatureQuality::Tier1].Enabled)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##Chapaa", &Configuration::ESP.Animals[ECreatureKind::Chapaa][ECreatureQuality::Tier1].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Striped Chapaa");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##StripedChapaa", &Configuration::ESP.Animals[ECreatureKind::Chapaa][ECreatureQuality::Tier2].Enabled)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##StripedChapaa", &Configuration::ESP.Animals[ECreatureKind::Chapaa][ECreatureQuality::Tier2].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Azure Chapaa");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##AzureChapaa", &Configuration::ESP.Animals[ECreatureKind::Chapaa][ECreatureQuality::Tier3].Enabled)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##AzureChapaa", &Configuration::ESP.Animals[ECreatureKind::Chapaa][ECreatureQuality::Tier3].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Minigame Chapaa");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##MinigameChapaa", &Configuration::ESP.Animals[ECreatureKind::Chapaa][ECreatureQuality::Chase].Enabled)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##MinigameChapaa", &Configuration::ESP.Animals[ECreatureKind::Chapaa][ECreatureQuality::Chase].Color);
                    ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
                    ImGui::TableNextColumn();
                    ImGui::TableNextColumn();
                    ImGui::Text("Show");
                    ImGui::TableNextColumn();
                    ImGui::Text("Color");
                    ImGui::TableNextRow();

                    ImGui::TableNextColumn();
                    ImGui::Text("Muujin");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##Muujin", &Configuration::ESP.Animals[ECreatureKind::TreeClimber][ECreatureQuality::Tier1].Enabled)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##Muujin", &Configuration::ESP.Animals[ECreatureKind::TreeClimber][ECreatureQuality::Tier1].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Banded Muujin");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##BandedMuujin", &Configuration::ESP.Animals[ECreatureKind::TreeClimber][ECreatureQuality::Tier2].Enabled)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##BandedMuujin", &Configuration::ESP.Animals[ECreatureKind::TreeClimber][ECreatureQuality::Tier2].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Bluebristle Muujin");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##BluebristleMuujin", &Configuration::ESP.Animals[ECreatureKind::TreeClimber][ECreatureQuality::Tier3].Enabled)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##BluebristleMuujin", &Configuration::ESP.Animals[ECreatureKind::TreeClimber][ECreatureQuality::Tier3].Color);
                }
                ImGui::EndTable();
            }
            if (ImGui::CollapsingHeader("Ores##OresSettingsHeader")) {

                if (ImGui::Button("Clay##ClayBtn")) {
                    bool newState = !Configuration::ESP.Ores[EOreType::Clay].Large;
                    Configuration::ESP.Ores[EOreType::Clay].Large = newState;
                    Configuration::Save();
                }
                ImGui::SameLine();
                if (ImGui::Button("Stone##StoneBtn")) {
                    bool newState = !Configuration::ESP.Ores[EOreType::Stone].Large;
                    Configuration::ESP.Ores[EOreType::Stone].Small = newState;
                    Configuration::ESP.Ores[EOreType::Stone].Medium = newState;
                    Configuration::ESP.Ores[EOreType::Stone].Large = newState;
                    Configuration::Save();
                }
                ImGui::SameLine();
                if (ImGui::Button("Copper##CopperBtn")) {
                    bool newState = !Configuration::ESP.Ores[EOreType::Copper].Large;
                    Configuration::ESP.Ores[EOreType::Copper].Small = newState;
                    Configuration::ESP.Ores[EOreType::Copper].Medium = newState;
                    Configuration::ESP.Ores[EOreType::Copper].Large = newState;
                    Configuration::Save();
                }
                ImGui::SameLine();
                if (ImGui::Button("Iron##IronBtn")) {
                    bool newState = !Configuration::ESP.Ores[EOreType::Iron].Large;
                    Configuration::ESP.Ores[EOreType::Iron].Small = newState;
                    Configuration::ESP.Ores[EOreType::Iron].Medium = newState;
                    Configuration::ESP.Ores[EOreType::Iron].Large = newState;
                    Configuration::Save();
                }
                ImGui::SameLine();
                if (ImGui::Button("Palium##PaliumBtn")) {
                    bool newState = !Configuration::ESP.Ores[EOreType::Palium].Large;
                    Configuration::ESP.Ores[EOreType::Palium].Small = newState;
                    Configuration::ESP.Ores[EOreType::Palium].Medium = newState;
                    Configuration::ESP.Ores[EOreType::Palium].Large = newState;
                    Configuration::Save();
                }

                ImGui::BeginTable("Ores", 5);
                {
                    ImGui::TableSetupColumn("Name");
                    ImGui::TableSetupColumn("Sm", ImGuiTableColumnFlags_WidthFixed, 40);
                    ImGui::TableSetupColumn("Med", ImGuiTableColumnFlags_WidthFixed, 40);
                    ImGui::TableSetupColumn("Lg", ImGuiTableColumnFlags_WidthFixed, 40);
                    ImGui::TableSetupColumn("Color", ImGuiTableColumnFlags_WidthFixed, 40);
                    ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
                    ImGui::TableNextColumn();
                    ImGui::TableNextColumn();
                    ImGui::Text("Sm");
                    ImGui::TableNextColumn();
                    ImGui::Text("Med");
                    ImGui::TableNextColumn();
                    ImGui::Text("Lg");
                    ImGui::TableNextColumn();
                    ImGui::Text("Color");
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Clay");
                    ImGui::TableNextColumn();
                    ImGui::TableNextColumn();
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##ClayLg", Configuration::ESP.Ores[EOreType::Clay].Enabled(EGatherableSize::Large))) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##Clay", &Configuration::ESP.Ores[EOreType::Clay].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Stone");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##StoneSm", Configuration::ESP.Ores[EOreType::Stone].Enabled(EGatherableSize::Small))) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##StoneMed", Configuration::ESP.Ores[EOreType::Stone].Enabled(EGatherableSize::Medium))) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##StoneLg", Configuration::ESP.Ores[EOreType::Stone].Enabled(EGatherableSize::Large))) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##Stone", &Configuration::ESP.Ores[EOreType::Stone].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Copper");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##CopperSm", Configuration::ESP.Ores[EOreType::Copper].Enabled(EGatherableSize::Small))) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##CopperMed", Configuration::ESP.Ores[EOreType::Copper].Enabled(EGatherableSize::Medium))) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##CopperLg", Configuration::ESP.Ores[EOreType::Copper].Enabled(EGatherableSize::Large))) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##Copper", &Configuration::ESP.Ores[EOreType::Copper].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Iron");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##IronSm", Configuration::ESP.Ores[EOreType::Iron].Enabled(EGatherableSize::Small))) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##IronMed", Configuration::ESP.Ores[EOreType::Iron].Enabled(EGatherableSize::Medium))) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##IronLg", Configuration::ESP.Ores[EOreType::Iron].Enabled(EGatherableSize::Large))) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##Iron", &Configuration::ESP.Ores[EOreType::Iron].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Palium");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##PaliumSm", Configuration::ESP.Ores[EOreType::Palium].Enabled(EGatherableSize::Small))) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##PaliumMed", Configuration::ESP.Ores[EOreType::Palium].Enabled(EGatherableSize::Medium))) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##PaliumLg", Configuration::ESP.Ores[EOreType::Palium].Enabled(EGatherableSize::Large))) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##Palium", &Configuration::ESP.Ores[EOreType::Palium].Color);
                }
                ImGui::EndTable();
            }
            if (ImGui::CollapsingHeader("Forageables##ForageablesSettingsHeader")) {

                if (ImGui::Button("Common##Forage")) {
                    for (EForageableType pos : ForageableCommon) {
                        Configuration::ESP.Forageables[pos].Normal = !Configuration::ESP.Forageables[pos].Normal;
                        Configuration::ESP.Forageables[pos].Star = Configuration::ESP.Forageables[pos].Normal;
                    }
                    Configuration::Save();
                }
                ImGui::SameLine();
                if (ImGui::Button("Uncommon##Forage")) {
                    for (EForageableType pos : ForageableUncommon) {
                        Configuration::ESP.Forageables[pos].Normal = !Configuration::ESP.Forageables[pos].Normal;
                        Configuration::ESP.Forageables[pos].Star = Configuration::ESP.Forageables[pos].Normal;
                    }
                    Configuration::Save();
                }
                ImGui::SameLine();
                if (ImGui::Button("Rare##Forage")) {
                    for (EForageableType pos : ForageableRare) {
                        Configuration::ESP.Forageables[pos].Normal = !Configuration::ESP.Forageables[pos].Normal;
                        Configuration::ESP.Forageables[pos].Star = Configuration::ESP.Forageables[pos].Normal;
                    }
                    Configuration::Save();
                }
                ImGui::SameLine();
                if (ImGui::Button("Epic##Forage")) {
                    for (EForageableType pos : ForageableEpic) {
                        Configuration::ESP.Forageables[pos].Normal = !Configuration::ESP.Forageables[pos].Normal;
                        Configuration::ESP.Forageables[pos].Star = Configuration::ESP.Forageables[pos].Normal;
                    }
                    Configuration::Save();
                }
                ImGui::SameLine();
                if (ImGui::Button("Star##Forage")) {
                    for (int i = static_cast<int>(EForageableType::Oyster); i <= static_cast<int>(EForageableType::GreenOnion); ++i) {
                        EForageableType pos = static_cast<EForageableType>(i);
                        Configuration::ESP.Forageables[pos].Star = !Configuration::ESP.Forageables[pos].Star;
                    }
                    Configuration::Save();
                }

                ImGui::BeginTable("Forageables", 4);
                {
                    ImGui::TableSetupColumn("Name");
                    ImGui::TableSetupColumn("Normal", ImGuiTableColumnFlags_WidthFixed, 40);
                    ImGui::TableSetupColumn("Star", ImGuiTableColumnFlags_WidthFixed, 40);
                    ImGui::TableSetupColumn("Color", ImGuiTableColumnFlags_WidthFixed, 40);
                    ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
                    ImGui::TableNextColumn();
                    ImGui::Text("Beach");
                    ImGui::TableNextColumn();
                    ImGui::Text("Normal");
                    ImGui::TableNextColumn();
                    ImGui::TableNextColumn();
                    ImGui::Text("Color");
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Coral");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##Coral", &Configuration::ESP.Forageables[EForageableType::Coral].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##Coral", &Configuration::ESP.Forageables[EForageableType::Coral].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Oyster");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##Oyster", &Configuration::ESP.Forageables[EForageableType::Oyster].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##Oyster", &Configuration::ESP.Forageables[EForageableType::Oyster].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Shell");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##Shell", &Configuration::ESP.Forageables[EForageableType::Shell].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##Shell", &Configuration::ESP.Forageables[EForageableType::Shell].Color);
                    ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
                    ImGui::TableNextColumn();
                    ImGui::Text("Flower");
                    ImGui::TableNextColumn();
                    ImGui::Text("Normal");
                    ImGui::TableNextColumn();
                    ImGui::Text("Star");
                    ImGui::TableNextColumn();
                    ImGui::Text("Color");
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Briar Daisy");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##PoisonFlower", &Configuration::ESP.Forageables[EForageableType::PoisonFlower].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##PoisonFlowerP", &Configuration::ESP.Forageables[EForageableType::PoisonFlower].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##PoisonFlower", &Configuration::ESP.Forageables[EForageableType::PoisonFlower].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Crystal Lake Lotus");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##WaterFlower", &Configuration::ESP.Forageables[EForageableType::WaterFlower].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##WaterFlowerP", &Configuration::ESP.Forageables[EForageableType::WaterFlower].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##WaterFlower", &Configuration::ESP.Forageables[EForageableType::WaterFlower].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Heartdrop Lily");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##Heartdrop", &Configuration::ESP.Forageables[EForageableType::Heartdrop].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##HeartdropP", &Configuration::ESP.Forageables[EForageableType::Heartdrop].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##Heartdrop", &Configuration::ESP.Forageables[EForageableType::Heartdrop].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Sundrop Lily");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##Sundrop", &Configuration::ESP.Forageables[EForageableType::Sundrop].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##SundropP", &Configuration::ESP.Forageables[EForageableType::Sundrop].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##Sundrop", &Configuration::ESP.Forageables[EForageableType::Sundrop].Color);
                    ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
                    ImGui::TableNextColumn();
                    ImGui::Text("Moss");
                    ImGui::TableNextColumn();
                    ImGui::Text("Normal");
                    ImGui::TableNextColumn();
                    ImGui::Text("Star");
                    ImGui::TableNextColumn();
                    ImGui::Text("Color");
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Dragon's Beard Peat");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##DragonsBeard", &Configuration::ESP.Forageables[EForageableType::DragonsBeard].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##DragonsBeardP", &Configuration::ESP.Forageables[EForageableType::DragonsBeard].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##DragonsBeard", &Configuration::ESP.Forageables[EForageableType::DragonsBeard].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Emerald Carpet Moss");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##EmeraldCarpet", &Configuration::ESP.Forageables[EForageableType::EmeraldCarpet].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##EmeraldCarpetP", &Configuration::ESP.Forageables[EForageableType::EmeraldCarpet].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##EmeraldCarpet", &Configuration::ESP.Forageables[EForageableType::EmeraldCarpet].Color);
                    ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
                    ImGui::TableNextColumn();
                    ImGui::Text("Mushroom");
                    ImGui::TableNextColumn();
                    ImGui::Text("Normal");
                    ImGui::TableNextColumn();
                    ImGui::Text("Star");
                    ImGui::TableNextColumn();
                    ImGui::Text("Color");
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Brightshroom");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##MushroomBlue", &Configuration::ESP.Forageables[EForageableType::MushroomBlue].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##MushroomBlueP", &Configuration::ESP.Forageables[EForageableType::MushroomBlue].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##MushroomBlue", &Configuration::ESP.Forageables[EForageableType::MushroomBlue].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Mountain Morel");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##MushroomRed", &Configuration::ESP.Forageables[EForageableType::MushroomRed].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##MushroomRedP", &Configuration::ESP.Forageables[EForageableType::MushroomRed].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##MushroomRed", &Configuration::ESP.Forageables[EForageableType::MushroomRed].Color);
                    ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
                    ImGui::TableNextColumn();
                    ImGui::Text("Spice");
                    ImGui::TableNextColumn();
                    ImGui::Text("Normal");
                    ImGui::TableNextColumn();
                    ImGui::Text("Star");
                    ImGui::TableNextColumn();
                    ImGui::Text("Color");
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Dari Cloves");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##DariCloves", &Configuration::ESP.Forageables[EForageableType::DariCloves].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##DariClovesP", &Configuration::ESP.Forageables[EForageableType::DariCloves].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##DariCloves", &Configuration::ESP.Forageables[EForageableType::DariCloves].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Heat Root");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##HeatRoot", &Configuration::ESP.Forageables[EForageableType::HeatRoot].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##HeatRootP", &Configuration::ESP.Forageables[EForageableType::HeatRoot].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##HeatRoot", &Configuration::ESP.Forageables[EForageableType::HeatRoot].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Spice Sprouts");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##SpicedSprouts", &Configuration::ESP.Forageables[EForageableType::SpicedSprouts].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##SpicedSproutsP", &Configuration::ESP.Forageables[EForageableType::SpicedSprouts].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##SpicedSprouts", &Configuration::ESP.Forageables[EForageableType::SpicedSprouts].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Sweet Leaf");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##SweetLeaves", &Configuration::ESP.Forageables[EForageableType::SweetLeaves].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##SweetLeavesP", &Configuration::ESP.Forageables[EForageableType::SweetLeaves].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##SweetLeaves", &Configuration::ESP.Forageables[EForageableType::SweetLeaves].Color);

                    ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
                    ImGui::TableNextColumn();
                    ImGui::Text("Vegetable");
                    ImGui::TableNextColumn();
                    ImGui::Text("Normal");
                    ImGui::TableNextColumn();
                    ImGui::Text("Star");
                    ImGui::TableNextColumn();
                    ImGui::Text("Color");
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Wild Garlic");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##Garlic", &Configuration::ESP.Forageables[EForageableType::Garlic].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##GarlicP", &Configuration::ESP.Forageables[EForageableType::Garlic].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##Garlic", &Configuration::ESP.Forageables[EForageableType::Garlic].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Wild Ginger");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##Ginger", &Configuration::ESP.Forageables[EForageableType::Ginger].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##GingerP", &Configuration::ESP.Forageables[EForageableType::Ginger].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##Ginger", &Configuration::ESP.Forageables[EForageableType::Ginger].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Wild Green Onion");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##GreenOnion", &Configuration::ESP.Forageables[EForageableType::GreenOnion].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##GreenOnionP", &Configuration::ESP.Forageables[EForageableType::GreenOnion].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##GreenOnion", &Configuration::ESP.Forageables[EForageableType::GreenOnion].Color);
                }
                ImGui::EndTable();
            }

            ImGui::NextColumn();

            if (ImGui::CollapsingHeader("Bugs##BugsSettingsHeader")) {

                if (ImGui::Button("Common##Bugs")) {
                    for (EBugKind kind = begin(EBugKind()); kind != end(EBugKind()); ++kind) {
                        bool newState = !Configuration::ESP.Bugs[kind][EBugQuality::Common].Normal;
                        Configuration::ESP.Bugs[kind][EBugQuality::Common].Normal = newState;
                        Configuration::ESP.Bugs[kind][EBugQuality::Common].Star = newState;
                    }
                    Configuration::Save();
                }
                ImGui::SameLine();
                if (ImGui::Button("Uncommon##Bugs")) {
                    for (EBugKind kind = begin(EBugKind()); kind != end(EBugKind()); ++kind) {
                        bool newState = !Configuration::ESP.Bugs[kind][EBugQuality::Uncommon].Normal;
                        Configuration::ESP.Bugs[kind][EBugQuality::Uncommon].Normal = newState;
                        Configuration::ESP.Bugs[kind][EBugQuality::Uncommon].Star = newState;
                    }
                    Configuration::Save();
                }
                ImGui::SameLine();
                if (ImGui::Button("Rare##Bugs")) {
                    for (EBugKind kind = begin(EBugKind()); kind != end(EBugKind()); ++kind) {
                        bool newStateRare = !Configuration::ESP.Bugs[kind][EBugQuality::Rare].Normal;
                        Configuration::ESP.Bugs[kind][EBugQuality::Rare].Normal = newStateRare;
                        Configuration::ESP.Bugs[kind][EBugQuality::Rare].Star = newStateRare;

                        bool newStateRare2 = !Configuration::ESP.Bugs[kind][EBugQuality::Rare2].Normal;
                        Configuration::ESP.Bugs[kind][EBugQuality::Rare2].Normal = newStateRare2;
                        Configuration::ESP.Bugs[kind][EBugQuality::Rare2].Star = newStateRare2;
                    }
                    Configuration::Save();
                }
                ImGui::SameLine();
                if (ImGui::Button("Epic##Bugs")) {
                    for (EBugKind kind = begin(EBugKind()); kind != end(EBugKind()); ++kind) {
                        bool newState = !Configuration::ESP.Bugs[kind][EBugQuality::Epic].Normal;
                        Configuration::ESP.Bugs[kind][EBugQuality::Epic].Normal = newState;
                        Configuration::ESP.Bugs[kind][EBugQuality::Epic].Star = newState;
                    }
                    Configuration::Save();
                }
                ImGui::SameLine();
                if (ImGui::Button("Star##Bugs")) {
                    for (EBugKind kind = begin(EBugKind()); kind != end(EBugKind()); ++kind) {
                        for (EBugQuality quality = begin(EBugQuality()); quality != end(EBugQuality()); ++quality) {
                            Configuration::ESP.Bugs[kind][quality].Star = !Configuration::ESP.Bugs[kind][quality].Star;
                        }
                    }
                    Configuration::Save();
                }

                ImGui::BeginTable("Bugs", 4);
                {
                    ImGui::TableSetupColumn("Name");
                    ImGui::TableSetupColumn("Normal", ImGuiTableColumnFlags_WidthFixed, 40);
                    ImGui::TableSetupColumn("Star", ImGuiTableColumnFlags_WidthFixed, 40);
                    ImGui::TableSetupColumn("Color", ImGuiTableColumnFlags_WidthFixed, 40);
                    ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
                    ImGui::TableNextColumn();
                    ImGui::Text("Bee");
                    ImGui::TableNextColumn();
                    ImGui::Text("Normal");
                    ImGui::TableNextColumn();
                    ImGui::Text("Star");
                    ImGui::TableNextColumn();
                    ImGui::Text("Color");
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Bahari Bee");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##BeeU", &Configuration::ESP.Bugs[EBugKind::Bee][EBugQuality::Uncommon].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##BeeUP", &Configuration::ESP.Bugs[EBugKind::Bee][EBugQuality::Uncommon].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##BeeU", &Configuration::ESP.Bugs[EBugKind::Bee][EBugQuality::Uncommon].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Golden Glory Bee");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##BeeR", &Configuration::ESP.Bugs[EBugKind::Bee][EBugQuality::Rare].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##BeeRP", &Configuration::ESP.Bugs[EBugKind::Bee][EBugQuality::Rare].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##Bee", &Configuration::ESP.Bugs[EBugKind::Bee][EBugQuality::Rare].Color);
                    ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
                    ImGui::TableNextColumn();
                    ImGui::Text("Beetle");
                    ImGui::TableNextColumn();
                    ImGui::Text("Normal");
                    ImGui::TableNextColumn();
                    ImGui::Text("Star");
                    ImGui::TableNextColumn();
                    ImGui::Text("Color");
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Spotted Stink Bug");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##BeetleC", &Configuration::ESP.Bugs[EBugKind::Beetle][EBugQuality::Common].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##BeetleCP", &Configuration::ESP.Bugs[EBugKind::Beetle][EBugQuality::Common].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##BeetleC", &Configuration::ESP.Bugs[EBugKind::Beetle][EBugQuality::Common].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Proudhorned Stag Beetle");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##BeetleU", &Configuration::ESP.Bugs[EBugKind::Beetle][EBugQuality::Uncommon].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##BeetleUP", &Configuration::ESP.Bugs[EBugKind::Beetle][EBugQuality::Uncommon].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##BeetleU", &Configuration::ESP.Bugs[EBugKind::Beetle][EBugQuality::Uncommon].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Raspberry Beetle");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##BeetleR", &Configuration::ESP.Bugs[EBugKind::Beetle][EBugQuality::Rare].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##BeetleRP", &Configuration::ESP.Bugs[EBugKind::Beetle][EBugQuality::Rare].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##BeetleR", &Configuration::ESP.Bugs[EBugKind::Beetle][EBugQuality::Rare].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Ancient Amber Beetle");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##BeetleE", &Configuration::ESP.Bugs[EBugKind::Beetle][EBugQuality::Epic].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##BeetleEP", &Configuration::ESP.Bugs[EBugKind::Beetle][EBugQuality::Epic].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##BeetleE", &Configuration::ESP.Bugs[EBugKind::Beetle][EBugQuality::Epic].Color);
                    ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
                    ImGui::TableNextColumn();
                    ImGui::Text("Butterfly");
                    ImGui::TableNextColumn();
                    ImGui::Text("Normal");
                    ImGui::TableNextColumn();
                    ImGui::Text("Star");
                    ImGui::TableNextColumn();
                    ImGui::Text("Color");
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Common Blue Butterfly");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##ButterflyC", &Configuration::ESP.Bugs[EBugKind::Butterfly][EBugQuality::Common].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##ButterflyCP", &Configuration::ESP.Bugs[EBugKind::Butterfly][EBugQuality::Common].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##ButterflyC", &Configuration::ESP.Bugs[EBugKind::Butterfly][EBugQuality::Common].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Duskwing Butterfly");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##ButterflyU", &Configuration::ESP.Bugs[EBugKind::Butterfly][EBugQuality::Uncommon].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##ButterflyUP", &Configuration::ESP.Bugs[EBugKind::Butterfly][EBugQuality::Uncommon].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##ButterflyU", &Configuration::ESP.Bugs[EBugKind::Butterfly][EBugQuality::Uncommon].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Brighteye Butterfly");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##ButterflyR", &Configuration::ESP.Bugs[EBugKind::Butterfly][EBugQuality::Rare].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##ButterflyRP", &Configuration::ESP.Bugs[EBugKind::Butterfly][EBugQuality::Rare].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##ButterflyR", &Configuration::ESP.Bugs[EBugKind::Butterfly][EBugQuality::Rare].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Rainbow-Tipped Butterfly");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##ButterflyE", &Configuration::ESP.Bugs[EBugKind::Butterfly][EBugQuality::Epic].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##ButterflyEP", &Configuration::ESP.Bugs[EBugKind::Butterfly][EBugQuality::Epic].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##ButterflyE", &Configuration::ESP.Bugs[EBugKind::Butterfly][EBugQuality::Epic].Color);
                    ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
                    ImGui::TableNextColumn();
                    ImGui::Text("Cicada");
                    ImGui::TableNextColumn();
                    ImGui::Text("Normal");
                    ImGui::TableNextColumn();
                    ImGui::Text("Star");
                    ImGui::TableNextColumn();
                    ImGui::Text("Color");
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Common Bark Cicada");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##CicadaC", &Configuration::ESP.Bugs[EBugKind::Cicada][EBugQuality::Common].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##CicadaCP", &Configuration::ESP.Bugs[EBugKind::Cicada][EBugQuality::Common].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##CicadaC", &Configuration::ESP.Bugs[EBugKind::Cicada][EBugQuality::Common].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Cerulean Cicada");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##CicadaU", &Configuration::ESP.Bugs[EBugKind::Cicada][EBugQuality::Uncommon].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##CicadaUP", &Configuration::ESP.Bugs[EBugKind::Cicada][EBugQuality::Uncommon].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##CicadaU", &Configuration::ESP.Bugs[EBugKind::Cicada][EBugQuality::Uncommon].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Spitfire Cicada");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##CicadaR", &Configuration::ESP.Bugs[EBugKind::Cicada][EBugQuality::Rare].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##CicadaRP", &Configuration::ESP.Bugs[EBugKind::Cicada][EBugQuality::Rare].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##CicadaR", &Configuration::ESP.Bugs[EBugKind::Cicada][EBugQuality::Rare].Color);
                    ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
                    ImGui::TableNextColumn();
                    ImGui::Text("Crab");
                    ImGui::TableNextColumn();
                    ImGui::Text("Normal");
                    ImGui::TableNextColumn();
                    ImGui::Text("Star");
                    ImGui::TableNextColumn();
                    ImGui::Text("Color");
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Bahari Crab");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##CrabC", &Configuration::ESP.Bugs[EBugKind::Crab][EBugQuality::Common].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##CrabCP", &Configuration::ESP.Bugs[EBugKind::Crab][EBugQuality::Common].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##CrabC", &Configuration::ESP.Bugs[EBugKind::Crab][EBugQuality::Common].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Spineshell Crab");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##CrabU", &Configuration::ESP.Bugs[EBugKind::Crab][EBugQuality::Uncommon].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##CrabUP", &Configuration::ESP.Bugs[EBugKind::Crab][EBugQuality::Uncommon].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##CrabU", &Configuration::ESP.Bugs[EBugKind::Crab][EBugQuality::Uncommon].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Vampire Crab");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##CrabR", &Configuration::ESP.Bugs[EBugKind::Crab][EBugQuality::Rare].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##CrabRP", &Configuration::ESP.Bugs[EBugKind::Crab][EBugQuality::Rare].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##CrabR", &Configuration::ESP.Bugs[EBugKind::Crab][EBugQuality::Rare].Color);
                    ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
                    ImGui::TableNextColumn();
                    ImGui::Text("Cricket");
                    ImGui::TableNextColumn();
                    ImGui::Text("Normal");
                    ImGui::TableNextColumn();
                    ImGui::Text("Star");
                    ImGui::TableNextColumn();
                    ImGui::Text("Color");
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Common Field Cricket");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##CricketC", &Configuration::ESP.Bugs[EBugKind::Cricket][EBugQuality::Common].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##CricketCP", &Configuration::ESP.Bugs[EBugKind::Cricket][EBugQuality::Common].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##CricketC", &Configuration::ESP.Bugs[EBugKind::Cricket][EBugQuality::Common].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Garden Leafhopper");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##CricketU", &Configuration::ESP.Bugs[EBugKind::Cricket][EBugQuality::Uncommon].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##CricketUP", &Configuration::ESP.Bugs[EBugKind::Cricket][EBugQuality::Uncommon].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##CricketU", &Configuration::ESP.Bugs[EBugKind::Cricket][EBugQuality::Uncommon].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Azure Stonehopper");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##CricketR", &Configuration::ESP.Bugs[EBugKind::Cricket][EBugQuality::Rare].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##CricketRP", &Configuration::ESP.Bugs[EBugKind::Cricket][EBugQuality::Rare].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##CricketR", &Configuration::ESP.Bugs[EBugKind::Cricket][EBugQuality::Rare].Color);
                    ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
                    ImGui::TableNextColumn();
                    ImGui::Text("Dragonfly");
                    ImGui::TableNextColumn();
                    ImGui::Text("Normal");
                    ImGui::TableNextColumn();
                    ImGui::Text("Star");
                    ImGui::TableNextColumn();
                    ImGui::Text("Color");
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Brushtail Dragonfly");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##DragonflyC", &Configuration::ESP.Bugs[EBugKind::Dragonfly][EBugQuality::Common].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##DragonflyCP", &Configuration::ESP.Bugs[EBugKind::Dragonfly][EBugQuality::Common].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##DragonflyC", &Configuration::ESP.Bugs[EBugKind::Dragonfly][EBugQuality::Common].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Inky Dragonfly");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##DragonflyU", &Configuration::ESP.Bugs[EBugKind::Dragonfly][EBugQuality::Uncommon].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##DragonflyUP", &Configuration::ESP.Bugs[EBugKind::Dragonfly][EBugQuality::Uncommon].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##DragonflyU", &Configuration::ESP.Bugs[EBugKind::Dragonfly][EBugQuality::Uncommon].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Firebreathing Dragonfly");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##DragonflyR", &Configuration::ESP.Bugs[EBugKind::Dragonfly][EBugQuality::Rare].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##DragonflyRP", &Configuration::ESP.Bugs[EBugKind::Dragonfly][EBugQuality::Rare].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##DragonflyR", &Configuration::ESP.Bugs[EBugKind::Dragonfly][EBugQuality::Rare].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Jewelwing Dragonfly");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##DragonflyE", &Configuration::ESP.Bugs[EBugKind::Dragonfly][EBugQuality::Epic].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##DragonflyEP", &Configuration::ESP.Bugs[EBugKind::Dragonfly][EBugQuality::Epic].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##DragonflyE", &Configuration::ESP.Bugs[EBugKind::Dragonfly][EBugQuality::Epic].Color);
                    ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
                    ImGui::TableNextColumn();
                    ImGui::Text("Glowbug");
                    ImGui::TableNextColumn();
                    ImGui::Text("Normal");
                    ImGui::TableNextColumn();
                    ImGui::Text("Star");
                    ImGui::TableNextColumn();
                    ImGui::Text("Color");
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Paper Lantern Bug");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##GlowbugC", &Configuration::ESP.Bugs[EBugKind::Glowbug][EBugQuality::Common].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##GlowbugCP", &Configuration::ESP.Bugs[EBugKind::Glowbug][EBugQuality::Common].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##GlowbugC", &Configuration::ESP.Bugs[EBugKind::Glowbug][EBugQuality::Common].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Bahari Glowbug");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##GlowbugU", &Configuration::ESP.Bugs[EBugKind::Glowbug][EBugQuality::Uncommon].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##GlowbugUP", &Configuration::ESP.Bugs[EBugKind::Glowbug][EBugQuality::Uncommon].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##GlowbugU", &Configuration::ESP.Bugs[EBugKind::Glowbug][EBugQuality::Uncommon].Color);
                    ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
                    ImGui::TableNextColumn();
                    ImGui::Text("Ladybug");
                    ImGui::TableNextColumn();
                    ImGui::Text("Normal");
                    ImGui::TableNextColumn();
                    ImGui::Text("Star");
                    ImGui::TableNextColumn();
                    ImGui::Text("Color");
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Garden Ladybug");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##LadybugC", &Configuration::ESP.Bugs[EBugKind::Ladybug][EBugQuality::Common].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##LadybugCP", &Configuration::ESP.Bugs[EBugKind::Ladybug][EBugQuality::Common].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##LadybugC", &Configuration::ESP.Bugs[EBugKind::Ladybug][EBugQuality::Common].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Princess Ladybug");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##LadybugU", &Configuration::ESP.Bugs[EBugKind::Ladybug][EBugQuality::Uncommon].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##LadybugUP", &Configuration::ESP.Bugs[EBugKind::Ladybug][EBugQuality::Uncommon].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##LadybugU", &Configuration::ESP.Bugs[EBugKind::Ladybug][EBugQuality::Uncommon].Color);
                    ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
                    ImGui::TableNextColumn();
                    ImGui::Text("Mantis");
                    ImGui::TableNextColumn();
                    ImGui::Text("Normal");
                    ImGui::TableNextColumn();
                    ImGui::Text("Star");
                    ImGui::TableNextColumn();
                    ImGui::Text("Color");
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Garden Mantis");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##MantisU", &Configuration::ESP.Bugs[EBugKind::Mantis][EBugQuality::Uncommon].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##MantisUP", &Configuration::ESP.Bugs[EBugKind::Mantis][EBugQuality::Uncommon].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##MantisU", &Configuration::ESP.Bugs[EBugKind::Mantis][EBugQuality::Uncommon].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Spotted Mantis");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##MantisR", &Configuration::ESP.Bugs[EBugKind::Mantis][EBugQuality::Rare].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##MantisRP", &Configuration::ESP.Bugs[EBugKind::Mantis][EBugQuality::Rare].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##MantisR", &Configuration::ESP.Bugs[EBugKind::Mantis][EBugQuality::Rare].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Leafstalker Mantis");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##MantisR2", &Configuration::ESP.Bugs[EBugKind::Mantis][EBugQuality::Rare2].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##MantisR2P", &Configuration::ESP.Bugs[EBugKind::Mantis][EBugQuality::Rare2].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##MantisR2", &Configuration::ESP.Bugs[EBugKind::Mantis][EBugQuality::Rare2].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Fairy Mantis");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##MantisE", &Configuration::ESP.Bugs[EBugKind::Mantis][EBugQuality::Epic].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##MantisEP", &Configuration::ESP.Bugs[EBugKind::Mantis][EBugQuality::Epic].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##MantisE", &Configuration::ESP.Bugs[EBugKind::Mantis][EBugQuality::Epic].Color);
                    ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
                    ImGui::TableNextColumn();
                    ImGui::Text("Moth");
                    ImGui::TableNextColumn();
                    ImGui::Text("Normal");
                    ImGui::TableNextColumn();
                    ImGui::Text("Star");
                    ImGui::TableNextColumn();
                    ImGui::Text("Color");
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Kilima Night Moth");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##MothC", &Configuration::ESP.Bugs[EBugKind::Moth][EBugQuality::Common].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##MothCP", &Configuration::ESP.Bugs[EBugKind::Moth][EBugQuality::Common].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##MothC", &Configuration::ESP.Bugs[EBugKind::Moth][EBugQuality::Common].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Lunar Fairy Moth");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##MothU", &Configuration::ESP.Bugs[EBugKind::Moth][EBugQuality::Uncommon].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##MothUP", &Configuration::ESP.Bugs[EBugKind::Moth][EBugQuality::Uncommon].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##MothU", &Configuration::ESP.Bugs[EBugKind::Moth][EBugQuality::Uncommon].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Gossamer Veil Moth");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##MothR", &Configuration::ESP.Bugs[EBugKind::Moth][EBugQuality::Rare].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##MothRP", &Configuration::ESP.Bugs[EBugKind::Moth][EBugQuality::Rare].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##MothR", &Configuration::ESP.Bugs[EBugKind::Moth][EBugQuality::Rare].Color);
                    ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
                    ImGui::TableNextColumn();
                    ImGui::Text("Pede");
                    ImGui::TableNextColumn();
                    ImGui::Text("Normal");
                    ImGui::TableNextColumn();
                    ImGui::Text("Star");
                    ImGui::TableNextColumn();
                    ImGui::Text("Color");
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Garden Millipede");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##PedeU", &Configuration::ESP.Bugs[EBugKind::Pede][EBugQuality::Uncommon].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##PedeUP", &Configuration::ESP.Bugs[EBugKind::Pede][EBugQuality::Uncommon].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##PedeU", &Configuration::ESP.Bugs[EBugKind::Pede][EBugQuality::Uncommon].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Hairy Millipede");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##PedeR", &Configuration::ESP.Bugs[EBugKind::Pede][EBugQuality::Rare].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##PedeRP", &Configuration::ESP.Bugs[EBugKind::Pede][EBugQuality::Rare].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##PedeR", &Configuration::ESP.Bugs[EBugKind::Pede][EBugQuality::Rare].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Scintillating Centipede");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##PedeR2", &Configuration::ESP.Bugs[EBugKind::Pede][EBugQuality::Rare2].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##PedeR2P", &Configuration::ESP.Bugs[EBugKind::Pede][EBugQuality::Rare2].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##PedeR2", &Configuration::ESP.Bugs[EBugKind::Pede][EBugQuality::Rare2].Color);
                    ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
                    ImGui::TableNextColumn();
                    ImGui::Text("Snail");
                    ImGui::TableNextColumn();
                    ImGui::Text("Normal");
                    ImGui::TableNextColumn();
                    ImGui::Text("Star");
                    ImGui::TableNextColumn();
                    ImGui::Text("Color");
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Garden Snail");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##SnailU", &Configuration::ESP.Bugs[EBugKind::Snail][EBugQuality::Uncommon].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##SnailUP", &Configuration::ESP.Bugs[EBugKind::Snail][EBugQuality::Uncommon].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##SnailU", &Configuration::ESP.Bugs[EBugKind::Snail][EBugQuality::Uncommon].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Stripeshell Snail");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##SnailR", &Configuration::ESP.Bugs[EBugKind::Snail][EBugQuality::Rare].Normal)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##SnailRP", &Configuration::ESP.Bugs[EBugKind::Snail][EBugQuality::Rare].Star)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##SnailR", &Configuration::ESP.Bugs[EBugKind::Snail][EBugQuality::Rare].Color);
                }
                ImGui::EndTable();
            }
            if (ImGui::CollapsingHeader("Trees##TreesSettingHeader")) {
                if (ImGui::Button("Bush##BushBtn")) {
                    bool newState = !Configuration::ESP.Trees[ETreeType::Bush].Small;
                    Configuration::ESP.Trees[ETreeType::Bush].Small = newState;
                    Configuration::Save();
                }
                ImGui::SameLine();
                if (ImGui::Button("Sapwood##SapwoodBtn")) {
                    bool newState = !Configuration::ESP.Trees[ETreeType::Sapwood].Large;
                    Configuration::ESP.Trees[ETreeType::Sapwood].Small = newState;
                    Configuration::ESP.Trees[ETreeType::Sapwood].Medium = newState;
                    Configuration::ESP.Trees[ETreeType::Sapwood].Large = newState;
                    Configuration::Save();
                }
                ImGui::SameLine();
                if (ImGui::Button("Heartwood##HeartwoodBtn")) {
                    bool newState = !Configuration::ESP.Trees[ETreeType::Heartwood].Large;
                    Configuration::ESP.Trees[ETreeType::Heartwood].Small = newState;
                    Configuration::ESP.Trees[ETreeType::Heartwood].Medium = newState;
                    Configuration::ESP.Trees[ETreeType::Heartwood].Large = newState;
                    Configuration::Save();
                }
                ImGui::SameLine();
                if (ImGui::Button("Flow##FlowBtn")) {
                    bool newState = !Configuration::ESP.Trees[ETreeType::Flow].Large;
                    Configuration::ESP.Trees[ETreeType::Flow].Small = newState;
                    Configuration::ESP.Trees[ETreeType::Flow].Medium = newState;
                    Configuration::ESP.Trees[ETreeType::Flow].Large = newState;
                    Configuration::Save();
                }

                ImGui::BeginTable("Trees", 5);
                {
                    ImGui::TableSetupColumn("Name");
                    ImGui::TableSetupColumn("Sm", ImGuiTableColumnFlags_WidthFixed, 40);
                    ImGui::TableSetupColumn("Med", ImGuiTableColumnFlags_WidthFixed, 40);
                    ImGui::TableSetupColumn("Lg", ImGuiTableColumnFlags_WidthFixed, 40);
                    ImGui::TableSetupColumn("Color", ImGuiTableColumnFlags_WidthFixed, 40);
                    ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
                    ImGui::TableNextColumn();
                    ImGui::TableNextColumn();
                    ImGui::Text("Sm");
                    ImGui::TableNextColumn();
                    ImGui::Text("Med");
                    ImGui::TableNextColumn();
                    ImGui::Text("Lg");
                    ImGui::TableNextColumn();
                    ImGui::Text("Color");
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Bush");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##BushSm", &Configuration::ESP.Trees[ETreeType::Bush].Small)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::TableNextColumn();
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##Bush", &Configuration::ESP.Trees[ETreeType::Bush].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Sapwood");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##SapwoodSm", &Configuration::ESP.Trees[ETreeType::Sapwood].Small)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##SapwoodMed", &Configuration::ESP.Trees[ETreeType::Sapwood].Medium)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##SapwoodLg", &Configuration::ESP.Trees[ETreeType::Sapwood].Large)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##Sapwood", &Configuration::ESP.Trees[ETreeType::Sapwood].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Heartwood");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##HeartwoodSm", &Configuration::ESP.Trees[ETreeType::Heartwood].Small)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##HeartwoodMed", &Configuration::ESP.Trees[ETreeType::Heartwood].Medium)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##HeartwoodLg", &Configuration::ESP.Trees[ETreeType::Heartwood].Large)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##Heartwood", &Configuration::ESP.Trees[ETreeType::Heartwood].Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Flow-Infused");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##FlowSm", &Configuration::ESP.Trees[ETreeType::Flow].Small)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##FlowMed", &Configuration::ESP.Trees[ETreeType::Flow].Medium)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##FlowLg", &Configuration::ESP.Trees[ETreeType::Flow].Large)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##Flow", &Configuration::ESP.Trees[ETreeType::Flow].Color);
                }
                ImGui::EndTable();
            }
            if (ImGui::CollapsingHeader("Player & Entities##PlayerEntitiesSettingHeader")) {
                if (ImGui::Button("Toggle All##MiscBtn")) {
                    bool newState = !Configuration::ESP.PlayerEntities.Player.Enabled;

                    Configuration::ESP.PlayerEntities.RummagePiles.Enabled = newState;
                    Configuration::ESP.PlayerEntities.FishHook.Enabled = newState;
                    Configuration::ESP.PlayerEntities.FishPool.Enabled = newState;
                    Configuration::ESP.PlayerEntities.Stables.Enabled = newState;
                    Configuration::ESP.PlayerEntities.Player.Enabled = newState;
                    Configuration::ESP.PlayerEntities.Others.Enabled = newState;
                    Configuration::ESP.PlayerEntities.Quest.Enabled = newState;
                    Configuration::ESP.PlayerEntities.Loot.Enabled = newState;
                    Configuration::ESP.PlayerEntities.NPC.Enabled = newState;

                    Configuration::Save();
                }

                ImGui::BeginTable("Odds", 3);
                {
                    ImGui::TableSetupColumn("Name");
                    ImGui::TableSetupColumn("Show", ImGuiTableColumnFlags_WidthFixed, 40);
                    ImGui::TableSetupColumn("Color", ImGuiTableColumnFlags_WidthFixed, 40);
                    ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
                    ImGui::TableNextColumn();
                    ImGui::TableNextColumn();
                    ImGui::Text("Show");
                    ImGui::TableNextColumn();
                    ImGui::Text("Color");
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Players");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##Players", &Configuration::ESP.PlayerEntities.Player.Enabled)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##Players", &Configuration::ESP.PlayerEntities.Player.Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("NPCs");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##NPC", &Configuration::ESP.PlayerEntities.NPC.Enabled)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##NPC", &Configuration::ESP.PlayerEntities.NPC.Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Fish");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##Fish", &Configuration::ESP.PlayerEntities.FishHook.Enabled)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##Fish", &Configuration::ESP.PlayerEntities.FishHook.Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Fish Pools");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##Pools", &Configuration::ESP.PlayerEntities.FishPool.Enabled)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##Pools", &Configuration::ESP.PlayerEntities.FishPool.Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Loot");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##Loot", &Configuration::ESP.PlayerEntities.Loot.Enabled)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##Loot", &Configuration::ESP.PlayerEntities.Loot.Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Quests");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##Quest", &Configuration::ESP.PlayerEntities.Quest.Enabled)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##Quest", &Configuration::ESP.PlayerEntities.Quest.Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Rummage Piles");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##RummagePiles", &Configuration::ESP.PlayerEntities.RummagePiles.Enabled)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##RummagePiles", &Configuration::ESP.PlayerEntities.RummagePiles.Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Stables");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##Stables", &Configuration::ESP.PlayerEntities.Stables.Enabled)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##Stables", &Configuration::ESP.PlayerEntities.Stables.Color);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Others");
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##Others", &Configuration::ESP.PlayerEntities.Others.Enabled)) {
                        Configuration::Save();
                    }
                    ImGui::TableNextColumn();
                    ImGui::ColorPicker("##Others", &Configuration::ESP.PlayerEntities.Others.Color);
                }
                ImGui::EndTable();
            }
        }
        // ==================================== 1 Aimbots & Fun TAB
        else if (OpenTab == 1) {
            ImGui::Columns(2, nullptr, false);

            AValeriaCharacter* ValeriaCharacter = GetValeriaData();

            // InteliTarget Controls
            if (ImGui::CollapsingHeader("InteliTarget Settings - General##InteliTargetSettingsHeader", ImGuiTreeNodeFlags_DefaultOpen)) {
                if (ValeriaCharacter) {
                    if (ImGui::Checkbox("Enable Silent Aimbot", &Configuration::Aimbot.SilentAimbot)) {
                        Configuration::Save();
                    }
                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("Teleport Bug Bombs & Arrows To Your Target");

                    if (ImGui::Checkbox("Enable Legacy Aimbot", &Configuration::Aimbot.LegacyAimbot)) {
                        Configuration::Save();
                    }
                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("Typical Aimbot system for targets");

                    if (Configuration::Aimbot.LegacyAimbot) {
                        ImGui::Text("Aim Smoothing:");
                        ImGui::SliderFloat("Smoothing Factor", &SmoothingFactor, 1.0f, 100.0f, "%1.0f"); //TODO: Add Config?
                        ImGui::Text("Aim Offset Adjustment (Drag Point):");
                        const auto canvas_size = ImVec2(200, 200); // Canvas size
                        static auto cursor_pos = ImVec2(0, 0); // Start at the center (0, 0 relative to center)
                        constexpr float scaling_factor = 0.5f; // Reduced scaling factor for finer control

                        ImU32 gridColor = IM_COL32(50, 45, 139, 255); // Grid lines color
                        ImU32 gridBackgroundColor = IM_COL32(26, 28, 33, 255); // Background color
                        ImU32 cursorColor = IM_COL32(69, 39, 160, 255); // Cursor color

                        if (ImGui::BeginChild("GridArea", ImVec2(200, 200), false, ImGuiWindowFlags_NoScrollbar)) {
                            ImDrawList* draw_list = ImGui::GetWindowDrawList();
                            ImVec2 canvas_p0 = ImGui::GetCursorScreenPos(); // Top-left corner of the canvas
                            auto grid_center = ImVec2(canvas_p0.x + canvas_size.x * 0.5f, canvas_p0.y + canvas_size.y * 0.5f);

                            draw_list->AddRectFilled(canvas_p0, ImVec2(canvas_p0.x + canvas_size.x, canvas_p0.y + canvas_size.y), gridBackgroundColor);
                            draw_list->AddLine(ImVec2(grid_center.x, canvas_p0.y), ImVec2(grid_center.x, canvas_p0.y + canvas_size.y), gridColor);
                            draw_list->AddLine(ImVec2(canvas_p0.x, grid_center.y), ImVec2(canvas_p0.x + canvas_size.x, grid_center.y), gridColor);

                            ImGui::SetCursorScreenPos(ImVec2(grid_center.x + cursor_pos.x - 5, grid_center.y + cursor_pos.y - 5));
                            ImGui::InvisibleButton("cursor", ImVec2(10, 10));
                            if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
                                cursor_pos.x = ImClamp(cursor_pos.x - ImGui::GetIO().MouseDelta.x * scaling_factor, -canvas_size.x * 0.5f, canvas_size.x * 0.5f);
                                cursor_pos.y = ImClamp(cursor_pos.y - ImGui::GetIO().MouseDelta.y * scaling_factor, -canvas_size.y * 0.5f, canvas_size.y * 0.5f);
                            }

                            draw_list->AddCircleFilled(ImVec2(grid_center.x + cursor_pos.x, grid_center.y + cursor_pos.y), 5, cursorColor, 12);

                            // Sliders for fine-tuned control
                            ImGui::SetCursorPosY(canvas_p0.y + canvas_size.y + 5);
                            ImGui::SliderFloat2("Horizontal & Vertical", reinterpret_cast<float*>(&cursor_pos), -canvas_size.x * 0.5f, canvas_size.x * 0.5f, "H: %.1f, V: %.1f");
                        }
                        ImGui::EndChild();

                        // Convert cursor_pos to AimOffset affecting Pitch and Yaw
                        AimOffset = { cursor_pos.x * scaling_factor, cursor_pos.y * scaling_factor, 0.0f };
                        ImGui::Text("Current Offset: Pitch: %.2f, Yaw: %.2f", AimOffset.X, AimOffset.Y);
                    }
                    if (ImGui::Checkbox("Teleport to Targeted", &Configuration::Teleport.TeleportToTargeted)) {
                        Configuration::Save();
                    }
                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("Teleport directly to your targeted entity");

                    ImGui::SameLine();
                    ImGui::Text("[ hotkey: X1 Mouse Button (Back) ]");

                    if (ImGui::Checkbox("Avoid Teleporting To Targeted Players", &Configuration::Teleport.AvoidTeleportingToPlayers)) {
                        Configuration::Save();
                    }
                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("Don't teleport to targeted players.");

                    ImGui::Checkbox("Avoid Teleporting To Targeted When Players Are Near", &Configuration::Teleport.RadiusPlayersAvoidance);
                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("Don't teleport if a player is detected near your destination.");

                    if (Configuration::Teleport.RadiusPlayersAvoidance) {
                        ImGui::SetNextItemWidth(200.0f);
                        ImGui::InputInt("Avoidance Radius (meters)", &Configuration::Teleport.AvoidanceRadius);
                        Configuration::Teleport.AvoidanceRadius = std::clamp(Configuration::Teleport.AvoidanceRadius, 1, 100);
                        Configuration::Save();
                    }
                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("Distance to avoid players when target teleporting");
                }
                else {
                    ImGui::Text("Waiting for character initialization...");
                }
            }

            ImGui::NextColumn();

            // Fun Mods - Entities column
            if (ImGui::CollapsingHeader("Fun Mods - General##FunModsHeader", ImGuiTreeNodeFlags_DefaultOpen)) {
                if (ValeriaCharacter) {
                    static bool teleportLootDisabled = true;
                    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, teleportLootDisabled);
                    if (ImGui::Checkbox("[Disabled] Teleport Dropped Loot to Player", &Configuration::Teleport.LootbagTeleportation)) {
                        Configuration::Save();
                    }
                    ImGui::PopItemFlag();
                    //if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("Automatically teleport dropped loot to your current location.");

                    if (ImGui::Checkbox("Anti AFK", &Configuration::Misc.AntiAfk)) {
                        Configuration::Save();
                    }
                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("Stop inactivity disconnects and play forever");

                    if (ImGui::Checkbox("Skip Minigames", &Configuration::Misc.MinigameSkip)) {
                        Configuration::Save();
                    }
                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("Skips the cooking minigame process completely");

                    if (ImGui::Checkbox("Teleport To Map Waypoint", &Configuration::Teleport.WaypointTeleport)) {
                        Configuration::Save();
                    }
                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("Automatically teleports you at your world map's waypoint.");
                }
                else {
                    ImGui::Text("Waiting for character initialization...");
                }
            }
        }
        // ==================================== 2 Movement & Teleport TAB
        else if (OpenTab == 2) {
            // Setting the columns layout
            ImGui::Columns(2, nullptr, false);

            //ULocalPlayer* LocalPlayer = nullptr;
            //APlayerController* PlayerController = nullptr;
            AValeriaCharacter* ValeriaCharacter = nullptr;

            UWorld* World = GetWorld();
            if (World) {
                if (UGameInstance* GameInstance = World->OwningGameInstance; GameInstance && GameInstance->LocalPlayers.Num() > 0) {
                    if (ULocalPlayer* LocalPlayer = GameInstance->LocalPlayers[0]) {
                        if (APlayerController* PlayerController = LocalPlayer->PlayerController) {
                            if (PlayerController && PlayerController->Pawn) {
                                ValeriaCharacter = static_cast<AValeriaPlayerController*>(PlayerController)->GetValeriaCharacter();
                            }
                        }
                    }
                }
            }

            UValeriaCharacterMoveComponent* MovementComponent = nullptr;
            if (ValeriaCharacter) {
                MovementComponent = ValeriaCharacter->GetValeriaCharacterMovementComponent();
            }

            // Movement settings column
            if (ImGui::CollapsingHeader("Movement Settings - General##MovementGeneralSettingsHeader", ImGuiTreeNodeFlags_DefaultOpen)) {
                if (MovementComponent) {
                    ImGui::Text("Map: %s", CurrentMap.c_str());
                    ImGui::Spacing();
                    static const char* movementModes[] = { "Walking", "Flying", "Fly No Collision" };
                    // Dropdown menu options

                    ImGui::Checkbox("Enable Noclip", &bEnableNoclip);

                    // Create a combo box for selecting the movement mode
                    ImGui::Text("Movement Mode");
                    ImGui::SetNextItemWidth(200.0f); // Adjust the width as needed
                    if (ImGui::BeginCombo("##MovementMode", movementModes[currentMovementModeIndex])) {
                        for (int n = 0; n < IM_ARRAYSIZE(movementModes); n++) {
                            const bool isSelected = (currentMovementModeIndex == n);
                            if (ImGui::Selectable(movementModes[n], isSelected)) {
                                currentMovementModeIndex = n;
                            }
                            // Set the initial focus when opening the combo
                            if (isSelected) {
                                ImGui::SetItemDefaultFocus();
                            }
                        }
                        ImGui::EndCombo();
                    }
                    ImGui::SameLine();
                    // Button to apply the selected movement mode
                    if (ImGui::Button("Set")) {
                        switch (currentMovementModeIndex) {
                        case 0: // Walking
                            MovementComponent->SetMovementMode(EMovementMode::MOVE_Walking, 1);
                            ValeriaCharacter->CapsuleComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
                            break;
                        case 1: // Swimming
                            MovementComponent->SetMovementMode(EMovementMode::MOVE_Flying, 4);
                            ValeriaCharacter->CapsuleComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
                            break;
                        case 2: // Noclip
                            MovementComponent->SetMovementMode(EMovementMode::MOVE_Flying, 5);
                            ValeriaCharacter->CapsuleComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
                            ValeriaCharacter->CapsuleComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Ignore);
                            ValeriaCharacter->CapsuleComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Ignore);
                            ValeriaCharacter->CapsuleComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
                            ValeriaCharacter->CapsuleComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_PhysicsBody, ECollisionResponse::ECR_Ignore);
                            ValeriaCharacter->CapsuleComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Destructible, ECollisionResponse::ECR_Ignore);
                            break;
                        default:
                            break;
                        }
                    }

                    constexpr float f1000 = 1000.0f, f5 = 5.f, f1 = 1.f;

                    // Global Game Speed with slider
                    ImGui::Text("Global Game Speed: ");
                    if (ImGui::InputScalar("##GlobalGameSpeed", ImGuiDataType_Float, &CustomGameSpeed, &f1, &f1000, "%.2f", ImGuiInputTextFlags_None)) {
                        static_cast<UGameplayStatics*>(UGameplayStatics::StaticClass()->DefaultObject)->SetGlobalTimeDilation(World, CustomGameSpeed);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("R##GlobalGameSpeed")) {
                        CustomGameSpeed = GameSpeed;
                        static_cast<UGameplayStatics*>(UGameplayStatics::StaticClass()->DefaultObject)->SetGlobalTimeDilation(World, GameSpeed);
                    }

                    // Walk Speed
                    ImGui::Text("Walk Speed: ");
                    if (ImGui::InputScalar("##WalkSpeed", ImGuiDataType_Float, &Configuration::GameModifiers.CustomWalkSpeed, &f5)) {
                        MovementComponent->MaxWalkSpeed = Configuration::GameModifiers.CustomWalkSpeed;
                        Configuration::Save();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("R##WalkSpeed")) {
                        Configuration::GameModifiers.CustomWalkSpeed = WalkSpeed;
                        MovementComponent->MaxWalkSpeed = WalkSpeed;
                        Configuration::Save();
                    }

                    // Sprint Speed
                    ImGui::Text("Sprint Speed: ");
                    if (ImGui::InputScalar("##SprintSpeedMultiplier", ImGuiDataType_Float, &Configuration::GameModifiers.CustomSprintSpeedMultiplier, &f5)) {
                        MovementComponent->SprintSpeedMultiplier = Configuration::GameModifiers.CustomSprintSpeedMultiplier;
                        Configuration::Save();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("R##SprintSpeedMultiplier")) {
                        Configuration::GameModifiers.CustomSprintSpeedMultiplier = SprintSpeedMultiplier;
                        MovementComponent->SprintSpeedMultiplier = SprintSpeedMultiplier;
                        Configuration::Save();
                    }

                    // Climbing Speed
                    ImGui::Text("Climbing Speed: ");
                    if (ImGui::InputScalar("##ClimbingSpeed", ImGuiDataType_Float, &Configuration::GameModifiers.CustomClimbingSpeed, &f5)) {
                        MovementComponent->ClimbingSpeed = Configuration::GameModifiers.CustomClimbingSpeed;
                        Configuration::Save();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("R##ClimbingSpeed")) {
                        Configuration::GameModifiers.CustomClimbingSpeed = ClimbingSpeed;
                        MovementComponent->ClimbingSpeed = ClimbingSpeed;
                        Configuration::Save();
                    }

                    // Gliding Speed
                    ImGui::Text("Gliding Speed: ");
                    if (ImGui::InputScalar("##GlidingSpeed", ImGuiDataType_Float, &Configuration::GameModifiers.CustomGlidingSpeed, &f5)) {
                        MovementComponent->GlidingMaxSpeed = Configuration::GameModifiers.CustomGlidingSpeed;
                        Configuration::Save();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("R##GlidingSpeed")) {
                        Configuration::GameModifiers.CustomGlidingSpeed = GlidingSpeed;
                        MovementComponent->GlidingMaxSpeed = GlidingSpeed;
                        Configuration::Save();
                    }

                    // Gliding Fall Speed
                    ImGui::Text("Gliding Fall Speed: ");
                    if (ImGui::InputScalar("##GlidingFallSpeed", ImGuiDataType_Float, &Configuration::GameModifiers.CustomGlidingFallSpeed, &f5)) {
                        MovementComponent->GlidingFallSpeed = Configuration::GameModifiers.CustomGlidingFallSpeed;
                        Configuration::Save();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("R##GlidingFallSpeed")) {
                        Configuration::GameModifiers.CustomGlidingFallSpeed = GlidingFallSpeed;
                        MovementComponent->GlidingFallSpeed = GlidingFallSpeed;
                        Configuration::Save();
                    }

                    // Jump Velocity
                    ImGui::Text("Jump Velocity: ");
                    if (ImGui::InputScalar("##JumpVelocity", ImGuiDataType_Float, &Configuration::GameModifiers.CustomJumpVelocity, &f5)) {
                        MovementComponent->JumpZVelocity = Configuration::GameModifiers.CustomJumpVelocity;
                        Configuration::Save();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("R##JumpVelocity")) {
                        Configuration::GameModifiers.CustomJumpVelocity = JumpVelocity;
                        MovementComponent->JumpZVelocity = JumpVelocity;
                        Configuration::Save();
                    }

                    // Step Height
                    ImGui::Text("Step Height: ");
                    if (ImGui::InputScalar("##MaxStepHeight", ImGuiDataType_Float, &Configuration::GameModifiers.CustomMaxStepHeight, &f5)) {
                        MovementComponent->MaxStepHeight = Configuration::GameModifiers.CustomMaxStepHeight;
                        Configuration::Save();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("R##MaxStepHeight")) {
                        Configuration::GameModifiers.CustomMaxStepHeight = MaxStepHeight;
                        MovementComponent->MaxStepHeight = MaxStepHeight;
                        Configuration::Save();
                    }
                }
                else {
                    if (!ValeriaCharacter) {
                        ImGui::Text("Waiting for character initialization...");
                    }
                    else {
                        ImGui::Text("Movement component not available.");
                    }
                }
            }

            ImGui::NextColumn();

            if (ImGui::CollapsingHeader("Locations & Coordinates - General##LocationSettingsHeader", ImGuiTreeNodeFlags_DefaultOpen)) {
                if (ValeriaCharacter) {
                    // Locations and exploits column
                    ImGui::Text("Teleport List");
                    ImGui::Text("Double-click a location listing to teleport");
                    ImGui::ListBoxHeader("##TeleportList", ImVec2(-1, 150));
                    for (auto& [MapName, Type, Name, Location, Rotate] : TeleportLocations) {
                        if (CurrentMap == MapName || MapName == "UserDefined") {
                            if (ImGui::Selectable(Name.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick)) {
                                if (ImGui::IsMouseDoubleClicked(0)) {
                                    if (Type == ELocation::Global_Home) {
                                        ValeriaCharacter->GetTeleportComponent()->RpcServerTeleport_Home();
                                    }
                                    else {
                                        FHitResult HitResult;
                                        ValeriaCharacter->K2_SetActorLocation(Location, false, &HitResult, true);
                                        // NOTE: Disabled for now. (testing)
                                        //PaliaContext.PlayerController->ClientForceGarbageCollection();
                                        //PaliaContext.PlayerController->ClientFlushLevelStreaming();
                                    }
                                }
                            }
                        }
                    }
                    ImGui::ListBoxFooter();

                    auto [PlayerX, PlayerY, PlayerZ] = ValeriaCharacter->K2_GetActorLocation();
                    auto PlayerYaw = ValeriaCharacter->K2_GetActorRotation().Yaw;
                    ImGui::Text("Current Coords: %.3f, %.3f, %.3f, %.3f", PlayerX, PlayerY, PlayerZ, PlayerYaw);
                    ImGui::Spacing();

                    // Set the width for the labels and inputs
                    constexpr float labelWidth = 50.0f;
                    constexpr float inputWidth = 200.0f;

                    // 
                    static FVector TeleportLocation;
                    static FRotator TeleportRotate;

                    constexpr double d5 = 5., d1 = 1.;

                    // X Coordinate
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("X: ");
                    ImGui::SameLine(labelWidth);
                    ImGui::SetNextItemWidth(inputWidth);
                    ImGui::InputScalar("##TeleportLocationX", ImGuiDataType_Double, &TeleportLocation.X, &d5);

                    // Y Coordinate
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Y: ");
                    ImGui::SameLine(labelWidth);
                    ImGui::SetNextItemWidth(inputWidth);
                    ImGui::InputScalar("##TeleportLocationY", ImGuiDataType_Double, &TeleportLocation.Y, &d5);

                    // Z Coordinate
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Z: ");
                    ImGui::SameLine(labelWidth);
                    ImGui::SetNextItemWidth(inputWidth);
                    ImGui::InputScalar("##TeleportLocationZ", ImGuiDataType_Double, &TeleportLocation.Z, &d5);

                    // Yaw
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("YAW: ");
                    ImGui::SameLine(labelWidth);
                    ImGui::SetNextItemWidth(inputWidth);
                    ImGui::InputScalar("##TeleportRotateYaw", ImGuiDataType_Double, &TeleportRotate.Yaw, &d1);

                    ImGui::Spacing();

                    if (ImGui::Button("Get Current Coordinates")) {
                        TeleportLocation = ValeriaCharacter->K2_GetActorLocation();
                        TeleportRotate = ValeriaCharacter->K2_GetActorRotation();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Teleport To Coordinates")) {
                        FHitResult HitResult;
                        ValeriaCharacter->K2_SetActorLocation(TeleportLocation, false, &HitResult, true);
                        // NOTE: Disabled, testing for now.
                        // PaliaContext.PlayerController->ClientForceGarbageCollection();
                        // PaliaContext.PlayerController->ClientFlushLevelStreaming();
                    }
                }
                else {
                    ImGui::Text("Waiting for character initialization...");
                }
            }

            if (ImGui::CollapsingHeader("Gatherable Items - General##GatherableSettingsHeader")) {
                if (ValeriaCharacter) {
                    ImGui::Text("Pickable List. Double-click a pickable to teleport to it.");
                    ImGui::Text("Populates from enabled Forageable ESP options.");

                    // Automatically sort by name before showing the list
                    std::ranges::sort(CachedActors, [](const FEntry& a, const FEntry& b) {
                        return a.DisplayName < b.DisplayName;
                    });

                    if (ImGui::ListBoxHeader("##PickableTeleportList", ImVec2(-1, 150))) {
                        for (auto& [Actor, WorldPosition, DisplayName, ActorType, Type, Quality, Variant, shouldAdd] : CachedActors) {
                            if (shouldAdd && (ActorType == EType::Forage || ActorType == EType::Loot)) {
                                // Enabled ESP options only
                                auto forageConfig = Configuration::GetConfig<ESPStarItem>(Type);
                                if (ActorType == EType::Forage && !forageConfig.Enabled(Quality))
                                    continue;

                                if (IsActorValid(Actor)) {
                                    FVector PickableLocation = Actor->K2_GetActorLocation();

                                    if (ImGui::Selectable(DisplayName.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick)) {
                                        if (ImGui::IsMouseDoubleClicked(0)) {
                                            PickableLocation.Z += 150;

                                            FHitResult PickableHitResult;
                                            ValeriaCharacter->K2_SetActorLocation(PickableLocation, false, &PickableHitResult, true);
                                        }
                                    }
                                }
                            }
                        }
                        ImGui::ListBoxFooter();
                    }
                }
                else {
                    ImGui::Text("Waiting for character initialization...");
                }
            }
        }
        // ==================================== 3 Selling & Items TAB
        else if (OpenTab == 3) {
            ImGui::Columns(2, nullptr, false);

            AValeriaCharacter* ValeriaCharacter = GetValeriaData();

            if (ImGui::CollapsingHeader("Selling Settings - Bag 1##SellingSettingsHeader", ImGuiTreeNodeFlags_DefaultOpen)) {
                if (ValeriaCharacter) {
                    UVillagerStoreComponent* StoreComponent = ValeriaCharacter->StoreComponent;

                    ImGui::Text("Quickly Sell Items - Bag 1");
                    ImGui::Spacing();
                    ImGui::Text("Select the bag, slot, and quantity to sell.");
                    ImGui::Spacing();
                    static int selectedSlot = 0;
                    static int selectedQuantity = 1;
                    static const char* quantities[] = { "1", "10", "50", "999", "Custom" };
                    static char customQuantity[64] = "100";

                    // Slot selection dropdown
                    if (ImGui::BeginCombo("Slot", std::to_string(selectedSlot + 1).c_str())) {
                        for (int i = 0; i < 8; i++) {
                            const bool isSelected = (selectedSlot == i);
                            if (ImGui::Selectable(std::to_string(i + 1).c_str(), isSelected)) {
                                selectedSlot = i;
                            }
                            if (isSelected) {
                                ImGui::SetItemDefaultFocus();
                            }
                        }
                        ImGui::EndCombo();
                    }

                    // Quantity selection dropdown
                    if (ImGui::BeginCombo("Quantity", quantities[selectedQuantity])) {
                        for (int i = 0; i < IM_ARRAYSIZE(quantities); i++) {
                            const bool isSelected = (selectedQuantity == i);
                            if (ImGui::Selectable(quantities[i], isSelected)) {
                                selectedQuantity = i;
                            }
                            if (isSelected) {
                                ImGui::SetItemDefaultFocus();
                            }
                        }
                        ImGui::EndCombo();
                    }

                    if (selectedQuantity == 4) {
                        ImGui::InputText("##CustomQuantity", customQuantity, IM_ARRAYSIZE(customQuantity));
                    }

                    if (ImGui::Button("Sell Items")) {
                        FBagSlotLocation bag = {};
                        bag.BagIndex = 0;
                        bag.SlotIndex = selectedSlot;

                        if (!StoreComponent->StoreCanBuyItem(bag)) {
                            StoreComponent->Client_SetVillagerStore(2);
                            StoreComponent->Client_OpenStore();
                        }

                        const int quantityToSell = selectedQuantity < 4
                            ? atoi(quantities[selectedQuantity])
                            : atoi(customQuantity);

                        ValeriaCharacter->StoreComponent->RpcServer_SellItem(bag, quantityToSell);
                    }
                }
                else {
                    ImGui::Text("Waiting for character initialization...");
                }
            }

            ImGui::NextColumn();

            if (ImGui::CollapsingHeader("Player Features - General##PlayerSettingsHeader", ImGuiTreeNodeFlags_DefaultOpen)) {
                if (ValeriaCharacter) {
                    if (ImGui::Button("Toggle Challenge Easy Mode")) {
                        ValeriaCharacter->RpcServer_ToggleDevChallengeEasyMode();
                        Configuration::Misc.ChallengeEasyMode = !Configuration::Misc.ChallengeEasyMode;
                    }
                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("Experimental - May skip questing requirements");

                    if (Configuration::Misc.ChallengeEasyMode) {
                        ImGui::Text("CHALLENGE EASY MODE ON");
                    }
                    else {
                        ImGui::Text("CHALLENGE EASY MODE OFF");
                    }
                }
                else {
                    ImGui::Text("Waiting for character initialization...");
                }
            }
        }
        // ==================================== 4 Skills & Tools TAB
        else if (OpenTab == 4) {
            ImGui::Columns(2, nullptr, false);

            AValeriaCharacter* ValeriaCharacter = GetValeriaData();
            UFishingComponent* FishingComponent = nullptr;
            auto EquippedTool = ETools::None;

            if (ValeriaCharacter) {
                FishingComponent = ValeriaCharacter->GetFishing();
            }

            if (ImGui::CollapsingHeader("Skill Settings - General##SkillsHeader", ImGuiTreeNodeFlags_DefaultOpen)) {
                if (ValeriaCharacter) {
                    std::string EquippedName;
                    EquippedName = ValeriaCharacter->GetEquippedItem().ItemType->Name.ToString();
                    //FishingComponent = ValeriaCharacter->GetFishing();

                    if (EquippedName.find("Tool_Axe_") != std::string::npos) {
                        EquippedTool = ETools::Axe;
                    }
                    else if (EquippedName.find("Tool_InsectBallLauncher_") != std::string::npos) {
                        EquippedTool = ETools::Belt;
                    }
                    else if (EquippedName.find("Tool_Bow_") != std::string::npos) {
                        EquippedTool = ETools::Bow;
                    }
                    else if (EquippedName.find("Tool_Rod_") != std::string::npos) {
                        EquippedTool = ETools::FishingRod;
                    }
                    else if (EquippedName.find("Tool_Hoe_") != std::string::npos) {
                        EquippedTool = ETools::Hoe;
                    }
                    else if (EquippedName.find("Tool_Pick") != std::string::npos) {
                        EquippedTool = ETools::Pick;
                    }
                    else if (EquippedName.find("Tool_WateringCan_") != std::string::npos) {
                        EquippedTool = ETools::WateringCan;
                    }
                    else {
                        EquippedTool = ETools::None;
                    }

                    ImGui::Text("Equipped Tool : %s", STools[static_cast<int>(EquippedTool)]);

                }
                else {
                    ImGui::Text("Waiting for character initialization...");
                }
            }

            ImGui::NextColumn();

            if (ImGui::CollapsingHeader("Fishing Settings - General##FishingHeader", ImGuiTreeNodeFlags_DefaultOpen)) {
                if (FishingComponent) {
                    if (ImGui::Checkbox("Disable Durability Loss", &Configuration::Fishing.NoDurability)) {
                        Configuration::Save();
                    }
                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("Never break your fishing rod");

                    if (ImGui::Checkbox("Enable Multiplayer Help", &Configuration::Fishing.MultiplayerHelp)) {
                        Configuration::Save();
                    }
                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("Force fishing with other players for extra quest completion");

                    if (ImGui::Checkbox("Always Perfect Catch", &Configuration::Fishing.PerfectCatch)) {
                        Configuration::Save();
                    }
                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("Fishing will always result in a perfect catch");

                    if (ImGui::Checkbox("Instant Catch", &Configuration::Fishing.InstantCatch)) {
                        Configuration::Save();
                    }
                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("Catch fish as soon as your bobber hits the water");

                    if (ImGui::Checkbox("Sell All Fish", &Configuration::Fishing.SellFish)) {
                        Configuration::Save();
                    }
                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("When fishing, automatically sell all fish from your inventory");

                    if (ImGui::Checkbox("Discard All Junk", &Configuration::Fishing.DiscardTrash)) {
                        Configuration::Save();
                    }
                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("When fishing, automatically remove junk items from your inventory");

                    if (ImGui::Checkbox("Open and Store Makeshift Decor", &Configuration::Fishing.OpenStoreWaterlogged)) {
                        Configuration::Save();
                    }
                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("When fishing, automatically move valuables to your home base storage");

                    if (EquippedTool == ETools::FishingRod) {
                        if (ImGui::Checkbox("Auto Fast Fishing", &bEnableAutoFishing)) {
                            Configuration::Save();
                        }
                        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("Fish extremely fast. Pairs nicely with other fishing features");

                        if (ImGui::Checkbox("Require Holding Left-Click To Auto Fish", &Configuration::Fishing.RequireClickFishing)) {
                            Configuration::Save();
                        }
                        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("Require holding the left mouse button to toggle the fast fishing");
                    }
                    else {
                        ImGui::Spacing();
                        ImGui::Text("[ EQUIP FISHING ROD TO VIEW FAST FISHING OPTIONS ]");
                        ImGui::Spacing();
                        Configuration::Fishing.RequireClickFishing = true;
                    }

                    ImGui::Checkbox("Force Fishing Pool", &bOverrideFishingSpot);
                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("Force all catches to result from the selected pool");

                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(200.0f);
                    if (ImGui::Combo("##FishingSpotsCombo", &bSelectedFishingSpot, bFishingSpots, IM_ARRAYSIZE(bFishingSpots))) {
                        if (bSelectedFishingSpot > 0) {
                            sOverrideFishingSpot = SDK::UKismetStringLibrary::Conv_StringToName(bFishingSpotsFString[bSelectedFishingSpot - 1]);
                        }
                        else {
                            bSelectedFishingSpot = 0;
                            sOverrideFishingSpot = FName(0);
                            bOverrideFishingSpot = false;
                        }
                    }
                }
                else {
                    if (!ValeriaCharacter) {
                        ImGui::Text("Waiting for character initialization...");
                    }
                    else {
                        ImGui::Text("Fishing component not available.");
                    }
                }
            }
        }
        // ==================================== 5 Housing & Decorating TAB
        else if (OpenTab == 5) {
            ImGui::Columns(1, nullptr, false);
            AValeriaCharacter* ValeriaCharacter = GetValeriaCharacter();

            if (ImGui::CollapsingHeader("Housing Base Settings##HousingBaseSettingsHeader", ImGuiTreeNodeFlags_DefaultOpen)) {
                if (ValeriaCharacter && ValeriaCharacter->GetPlacement()) {
                    UPlacementComponent* PlacementComponent = ValeriaCharacter->GetPlacement();
                    if (ImGui::Checkbox("Place Items Anywhere", &Configuration::Housing.PlaceAnywhere)) {
                        Configuration::Save();
                    }
                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("Allow for placement of housing items anywhere");

                    ImGui::SetNextItemWidth(200.0f);
                    if (ImGui::SliderFloat("Max Placement Height", &Configuration::Housing.MaxUpAngle, 1.0f, 3600.0f, "%10.0f degrees")) {
                        Configuration::Save();
                    }
                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("Modify the max height you're allowed to place items");
                }
                else {
                    ImGui::Text("No Placement Component available.");
                }
            }
            else {
                ImGui::Text("Waiting for character initialization...");
            }
        }
    }

    ImGui::End();

    if (!show)
        ShowOverlay(false);
}

void PaliaOverlay::ProcessActors(int step) {
    std::erase_if(CachedActors, [this, step](const FEntry& Entry) {
        return static_cast<int>(Entry.ActorType) == step;
    });

    auto World = GetWorld();
    if (!World)
        return;

    const auto ActorType = static_cast<EType>(step);
    std::vector<AActor*> Actors;
    std::vector<UClass*> SearchClasses;

    // What are gates anyways?
    STATIC_CLASS("BP_Stables_FrontGate_01_C", SearchClasses)
    STATIC_CLASS("BP_Stables_FrontGate_02_C", SearchClasses)

    switch (ActorType) {
    case EType::Tree:
        if (AnyTrue(Configuration::ESP.Trees)) {
            STATIC_CLASS("BP_ValeriaGatherableLoot_Lumber_C", SearchClasses)
        }
        break;
    case EType::Ore:
        if (AnyTrue(Configuration::ESP.Ores)) {
            STATIC_CLASS("BP_ValeriaGatherableLoot_Mining_Base_C", SearchClasses)
        }
        break;
    case EType::Bug:
        if (AnyTrue2D(Configuration::ESP.Bugs)) {
            STATIC_CLASS("BP_ValeriaBugCatchingCreature_C", SearchClasses)
        }
        break;
    case EType::Animal:
        if (AnyTrue2D(Configuration::ESP.Animals)) {
            STATIC_CLASS("BP_ValeriaHuntingCreature_C", SearchClasses)
        }
        break;
    case EType::Forage:
        if (AnyTrue(Configuration::ESP.Forageables)) {
            STATIC_CLASS("BP_Valeria_Gatherable_Placed_C", SearchClasses)
        }
        break;
    case EType::Loot:
        if (Configuration::ESP.PlayerEntities.Loot.Enabled || Configuration::Teleport.LootbagTeleportation) {
            STATIC_CLASS("BP_Loot_C", SearchClasses)
        }
        break;
    case EType::Players:
        if (Configuration::ESP.PlayerEntities.Player.Enabled) {
            SearchClasses.push_back(AValeriaCharacter::StaticClass());
        }
        break;
    case EType::NPCs:
        if (Configuration::ESP.PlayerEntities.NPC.Enabled) {
            SearchClasses.push_back(AValeriaVillagerCharacter::StaticClass());
        }
        break;
    case EType::Quest:
        if (Configuration::ESP.PlayerEntities.Quest.Enabled) {
            STATIC_CLASS("BP_SimpleInspect_Base_C", SearchClasses)
                STATIC_CLASS("BP_QuestInspect_Base_C", SearchClasses)
                STATIC_CLASS("BP_QuestItem_BASE_C", SearchClasses)
        }
        break;
    case EType::RummagePiles:
        if (Configuration::ESP.PlayerEntities.RummagePiles.Enabled) {
            STATIC_CLASS("BP_BeachPile_C", SearchClasses)
                STATIC_CLASS("BP_ChapaaPile_C", SearchClasses)
        }
        break;
    case EType::Stables:
        if (Configuration::ESP.PlayerEntities.Stables.Enabled) {
            STATIC_CLASS("BP_Stables_Sign_C", SearchClasses)
        }
        break;
    case EType::Fish:
        if (Configuration::ESP.PlayerEntities.FishHook.Enabled || Configuration::ESP.PlayerEntities.FishPool.Enabled) {
            STATIC_CLASS("BP_WaterPlane_Fishing_Base_SQ_C", SearchClasses)
                STATIC_CLASS("BP_Minigame_Fish_C", SearchClasses)
        }
        break;
    default:
        break;
    }

    if (!SearchClasses.empty()) {
        if (ActorType == EType::RummagePiles || ActorType == EType::Stables) {
            Actors = FindAllActorsOfTypes(World, SearchClasses);
        }
        else {
            Actors = FindActorsOfTypes(World, SearchClasses);
        }
    }

    for (AActor* Actor : Actors) {
        if (!IsActorValid(Actor))
            continue;

        auto ClassName = Actor->Class->GetName();

        // [HACK] Gates-Begone
        if (ClassName.find("_FrontGate_") != std::string::npos) {
            // Destroy and move on, no caching.
            Actor->K2_DestroyActor();
            continue;
        }

        const FVector ActorPosition = Actor->K2_GetActorLocation();
        if (ActorPosition.IsZero() || ActorPosition == FVector{ 2, 0, -9900 })
            continue;

        TypeEnum Type;
        VariantEnum Variant;
        int Quality = 0;

        bool shouldAdd = false;

        switch (ActorType) {
        case EType::Tree: {
            if (auto Tree = GetFlagSingle(ClassName, TREE_TYPE_MAPPINGS); Tree != ETreeType::Unknown) {
                if (auto Size = GetFlagSingle(ClassName, GATHERABLE_SIZE_MAPPINGS); Size != EGatherableSize::Unknown) {
                    shouldAdd = true;
                    Type = Tree;
                    Variant = Size;
                }
            }
            break;
        }
        case EType::Ore: {
            if (auto OreType = GetFlagSingle(ClassName, MINING_TYPE_MAPPINGS); OreType != EOreType::Unknown) {
                auto Size = GetFlagSingle(ClassName, GATHERABLE_SIZE_MAPPINGS);
                if (OreType == EOreType::Clay)
                    Size = EGatherableSize::Large;
                if (Size != EGatherableSize::Unknown) {
                    shouldAdd = true;
                    Type = OreType;
                    Variant = Size;
                }
            }
            break;
        }
        case EType::Bug: {
            if (auto BugType = GetFlagSingle(ClassName, CREATURE_BUGKIND_MAPPINGS); BugType != EBugKind::Unknown) {
                if (auto BVar = GetFlagSingleEnd(ClassName, CREATURE_BUGQUALITY_MAPPINGS); BVar != EBugQuality::Unknown) {
                    shouldAdd = true;
                    Type = BugType;
                    Variant = BVar;
                    if (ClassName.ends_with("+_C")) {
                        Quality = 1;
                    }
                }
            }
            break;
        }
        case EType::Animal: {
            if (auto CKType = GetFlagSingle(ClassName, CREATURE_KIND_MAPPINGS); CKType != ECreatureKind::Unknown) {
                if (auto CQType = GetFlagSingleEnd(ClassName, CREATURE_KINDQUALITY_MAPPINGS); CQType != ECreatureQuality::Unknown) {
                    shouldAdd = true;
                    Type = CKType;
                    Variant = CQType;
                }
            }
            break;
        }
        case EType::Forage: {
            if (!Actor->bActorEnableCollision)
                continue;

            if (auto ForageType = GetFlagSingle(ClassName, FORAGEABLE_TYPE_MAPPINGS); ForageType != EForageableType::Unknown) {
                shouldAdd = true;
                Type = ForageType;
                if (ClassName.ends_with("+_C")) {
                    Quality = 1;
                }
            }
            break;
        }
        case EType::Loot: {
            shouldAdd = true;
            //Type = 1; // doesn't matter, but isn't "unknown"
            break;
        }
        case EType::Players: {
            shouldAdd = true;
            //Type = 1; // doesn't matter, but isn't "unknown"
            const auto VActor = static_cast<AValeriaCharacter*>(Actor);
            ClassName = VActor->CharacterName.ToString();
            break;
        }
        case EType::NPCs: {
            shouldAdd = true;
            //Type = 1; // doesn't matter, but isn't "unknown"
            break;
        }
        case EType::Quest: {
            if (!Actor->bActorEnableCollision)
                continue;

            shouldAdd = true;
            //Type = 1;
            break;
        }
        case EType::RummagePiles: {
            shouldAdd = true;
            //Type = 1;
            break;
        }
        case EType::Stables: {
            shouldAdd = true;
            //Type = 1;
            break;
        }
        case EType::Fish: {
            if (auto FishType = GetFlagSingle(ClassName, FISH_TYPE_MAPPINGS); FishType != EFishType::Unknown) {
                shouldAdd = true;
                Type = FishType;
            }
            break;
        }
        default:
            break;
        }

        if (!shouldAdd && !Configuration::ESP.PlayerEntities.Others.Enabled)
            continue;

        const std::string Name = CLASS_NAME_ALIAS.contains(ClassName) ? CLASS_NAME_ALIAS[ClassName] : ClassName;
        CachedActors.push_back({ Actor, ActorPosition, Name, ActorType, Type, Quality, Variant, shouldAdd });
    }
}

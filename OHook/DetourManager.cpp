﻿#include "DetourManager.h"
#include "PaliaOverlay.h"
#include <SDK/Palia_parameters.hpp>

#include "SDKExt.h"
#include "Utils.h"
#include "format"
#include "functional"
#include "Configuration.h"

using namespace SDK;

// Cache

void ClearActorCache(PaliaOverlay* Overlay) {
    const UWorld* World = GetWorld();

    const auto GameplayStatics = static_cast<UGameplayStatics*>(UGameplayStatics::StaticClass()->DefaultObject);
    if (!GameplayStatics)
        return;

    // Clear cache on level change
    if (Overlay->CurrentLevel != World->PersistentLevel) {
        Overlay->CachedActors.clear();
        Overlay->CurrentLevel = World->PersistentLevel;
        Overlay->CurrentMap = GameplayStatics->GetCurrentLevelName(World, false).ToString();
    }
}

void ManageActorCache(PaliaOverlay* Overlay) {
    const UWorld* World = GetWorld();

    const auto GameplayStatics = static_cast<UGameplayStatics*>(UGameplayStatics::StaticClass()->DefaultObject);
    if (!GameplayStatics)
        return;

    if (const double WorldTime = GameplayStatics->GetTimeSeconds(World); abs(WorldTime - Overlay->LastCachedTime) > 0.1) {
        Overlay->LastCachedTime = WorldTime;
        Overlay->ProcessActors(Overlay->ActorStep);

        Overlay->ActorStep++;
        if (Overlay->ActorStep >= static_cast<int>(EType::MAX)) {
            Overlay->ActorStep = 0;
        }
    }
}

// [Fun]

inline void Func_DoTeleportToTargeted(PaliaOverlay* Overlay, const double BestScore) {
    if (Configuration::Teleport.TeleportToTargeted) {
        const auto now = std::chrono::steady_clock::now();
        if (IsKeyHeld(VK_XBUTTON2) && std::abs(BestScore - FLT_MAX) > 0.0001f) {
            if (duration_cast<std::chrono::seconds>(now - Overlay->LastTeleportToTargetTime).count() >= 1) {
                const auto ValeriaCharacter = GetValeriaCharacter();

                if (!ValeriaCharacter)
                    return;

                bool shouldTeleport = true;

                // Avoid teleporting to players
                if (Configuration::Teleport.AvoidTeleportingToPlayers && Overlay->BestTargetActorType == EType::Players) {
                    shouldTeleport = false;
                }

                // Avoid teleporting to targeted if there are nearby players
                if (Configuration::Teleport.RadiusPlayersAvoidance && shouldTeleport) {
                    for (auto& [Actor, WorldPosition, DisplayName, ActorType, Type, Quality, Variant, shouldAdd] : Overlay->CachedActors) {
                        if (ActorType == EType::Players) {
                            if (!IsActorValid(Actor) || !IsActorValid(Overlay->BestTargetActor) || WorldPosition.IsZero())
                                continue;

                            // Don't count itself or us
                            if (Actor == Overlay->BestTargetActor || Actor == ValeriaCharacter)
                                continue;

                            // Check for actors within X meters of this actor
                            if (WorldPosition.GetDistanceToInMeters(Overlay->BestTargetActor->K2_GetActorLocation()) < Configuration::Teleport.AvoidanceRadius) {
                                shouldTeleport = false;
                                break;
                            }
                        }
                    }
                }

                // Teleportation logic
                if (shouldTeleport) {
                    FVector TargetLocation = Overlay->BestTargetLocation;
                    TargetLocation.Z += 150.0f;

                    // Apply horizontal offset for animal targets
                    if (Overlay->BestTargetActorType == EType::Animal) {
                        FVector RightVector = ValeriaCharacter->GetActorRightVector();
                        TargetLocation += RightVector * 160.0f;
                    }

                    FHitResult HitResult;
                    ValeriaCharacter->K2_SetActorLocation(TargetLocation, false, &HitResult, true);
                    Overlay->LastTeleportToTargetTime = now;
                }
            }
        }
    }
}

inline void Func_DoTeleportToWaypoint(const PaliaOverlay* Overlay, const Params::TrackingComponent_RpcClient_SetUserMarkerViaWorldMap* SetUserMarkerViaWorldMap) {
    if (Configuration::Teleport.WaypointTeleport) {
        const auto ValeriaCharacter = GetValeriaCharacter();
        if (ValeriaCharacter) {
            FVector TargetLocation = SetUserMarkerViaWorldMap->MarkerLocation;
            if (!TargetLocation.IsZero()) {
                FHitResult WaypointHitResult;
                TargetLocation.Z += 300.0f;
                ValeriaCharacter->K2_SetActorLocation(TargetLocation, false, &WaypointHitResult, true);
            }
        }
    }
}

std::chrono::steady_clock::time_point lastExecutionTime = std::chrono::steady_clock::now();
inline void Func_DoAntiAfk(const PaliaOverlay* Overlay) {
    if (Configuration::Misc.AntiAfk) {
        auto currentTime = std::chrono::steady_clock::now();
        auto elapsedTime = std::chrono::duration_cast<std::chrono::minutes>(currentTime - lastExecutionTime);

        if (elapsedTime.count() >= 1) {
            const auto ValeriaController = GetValeriaController();
            if (ValeriaController) {
                ValeriaController->Client_InactivityHeartbeat();
                ValeriaController->RpcServer_NotifyInactivityInterrupted();

                // Update the last execution time
                lastExecutionTime = currentTime;
            }
        }
    }
}

// [HUD]

inline void DrawCircle(UCanvas* Canvas, const float Radius, const int32 NumSegments, const FLinearColor Color, const float Thickness = 1.0f) {
    // Calculate screen center more accurately
    const FVector2D ScreenCenter = { static_cast<double>(Canvas->ClipX) / 2.0, static_cast<double>(Canvas->ClipY) / 2.0 };

    const double Increment = 360.0 / static_cast<double>(NumSegments);
    FVector2D LastPos = { ScreenCenter.X + Radius, ScreenCenter.Y };

    for (int i = 1; i <= NumSegments; i++) {
        const float Rad = CustomMath::DegreesToRadians(static_cast<float>(Increment * i));
        FVector2D NewPos = { ScreenCenter.X + Radius * cos(Rad), ScreenCenter.Y + Radius * sin(Rad) };
        Canvas->K2_DrawLine(LastPos, NewPos, Thickness, Color);
        LastPos = NewPos;
    }
}

inline void Func_DoInteliAim(PaliaOverlay* Overlay) {
    if (!Configuration::Aimbot.LegacyAimbot && !Configuration::ESP.DrawFOVCircle)
        return;

    UWorld* World = GetWorld();
    const auto PlayerController = GetPlayerController();
    const auto ValeriaCharacter = GetValeriaCharacter();

    if (!PlayerController || !ValeriaCharacter)
        return;

    FVector CharacterLocation = ValeriaCharacter->K2_GetActorLocation();
    FRotator CharacterRotation = PlayerController->GetControlRotation();
    FVector ForwardVector = UKismetMathLibrary::GetForwardVector(CharacterRotation);
    double BestScore = FLT_MAX; // Using a scoring system based on various factors such as distance, area fov, prediction

    for (auto& [Actor, WorldPosition, DisplayName, ActorType, Type, Quality, Variant, shouldAdd] : Overlay->CachedActors) {
        if (!IsActorValid(Actor) || WorldPosition.IsZero())
            continue;

        bool bShouldConsider = false;

        switch (ActorType) {
        case EType::Animal:
        {
            auto animalConfig = Configuration::GetConfig<ESPSingleItem>(Type, Variant);
            bShouldConsider = animalConfig.Enabled; // Toggle for different types of animals
        }
        break;
        case EType::Ore:
        {
            auto oreConfig = Configuration::GetConfig<ESPSizeItem>(Type);
            auto oreVariant = std::get<EGatherableSize>(Variant);
            bShouldConsider = oreConfig.Enabled(oreVariant); // Toggle for different types of ores
        }
        break;
        case EType::Bug:
        {
            auto bugConfig = Configuration::GetConfig<ESPStarItem>(Type, Variant);
            bShouldConsider = bugConfig.Enabled(Quality); // Toggle for different types of bugs
        }
        break;
        case EType::Forage:
        {
            auto forageConfig = Configuration::GetConfig<ESPStarItem>(Type);
            bShouldConsider = forageConfig.Enabled(Quality); // Toggle for forageable items
        }
        break;
        case EType::Players:
            bShouldConsider = Configuration::ESP.PlayerEntities.Player.Enabled; // Toggle for player visibility
            break;
        case EType::NPCs:
            bShouldConsider = Configuration::ESP.PlayerEntities.NPC.Enabled; // Toggle for NPCs
            break;
        case EType::Quest:
            bShouldConsider = Configuration::ESP.PlayerEntities.Quest.Enabled; // Toggle for quest items
            break;
        case EType::Loot:
            bShouldConsider = Configuration::ESP.PlayerEntities.Loot.Enabled; // Toggle for loot
            break;
        case EType::RummagePiles:
            if (Configuration::ESP.PlayerEntities.RummagePiles.Enabled) {
                if (Configuration::ESP.PlayerEntities.Others.Enabled) {
                    bShouldConsider = true;
                    break;
                }

                if (auto Pile = static_cast<ATimedLootPile*>(Actor)) {
                    if (ValeriaCharacter && Pile->CanGather(ValeriaCharacter) && Pile->bActivated) {
                        bShouldConsider = true;
                        break;
                    }
                }
            }
            break;
        case EType::Stables:
            bShouldConsider = Configuration::ESP.PlayerEntities.Stables.Enabled; // Toggle for Stables
            break;
        case EType::Tree:
        {
            auto treeConfig = Configuration::GetConfig<ESPSizeItem>(Type);
            auto treeVariant = std::get<EGatherableSize>(Variant);
            bShouldConsider = treeConfig.Enabled(treeVariant); // Toggle for trees
        }
        break;
        case EType::Fish:
        {
            auto fishConfig = Configuration::GetConfig<ESPSingleItem>(Type);
            bShouldConsider = fishConfig.Enabled; // Toggle for fish types
        }
        break;
        default:
            break;
        }

        if (!bShouldConsider)
            continue;

        FVector ActorLocation = Actor->K2_GetActorLocation();
        FVector DirectionToActor = (ActorLocation - CharacterLocation).GetNormalized();
        FVector TargetVelocity = Actor->GetVelocity();

        FVector RelativeVelocity = TargetVelocity - ValeriaCharacter->GetVelocity();
        FVector RelativeDirection = RelativeVelocity.GetNormalized();

        double Distance = CharacterLocation.GetDistanceToInMeters(ActorLocation);
        float Angle = CustomMath::RadiansToDegrees(acosf(static_cast<float>(ForwardVector.Dot(DirectionToActor))));

        if (ActorLocation.IsZero())
            continue;

        if (Distance < 2.0)
            continue;
        if (Configuration::ESP.Culling && Distance > Configuration::ESP.CullDistance)
            continue;

        // Weighting factors for different factors
        double AngleWeight, DistanceWeight, MovementWeight;

        // Adjust weighting factors based on EType
        switch (ActorType) {
        case EType::Animal:
            AngleWeight = 0.10;
            DistanceWeight = 0.0;
            MovementWeight = 0.0;
            break;
        case EType::Ore:
            AngleWeight = 0.10;
            DistanceWeight = 0.0;
            MovementWeight = 0.0;
            break;
        case EType::Bug:
            AngleWeight = 0.10;
            DistanceWeight = 0.0;
            MovementWeight = 0.0;
            break;
        default:
            AngleWeight = 0.10;
            DistanceWeight = 0.0;
            MovementWeight = 0.0;
            break;
        }

        // Calculate score based on weighted sum of factors
        if (double Score = AngleWeight * Angle + DistanceWeight * Distance + MovementWeight * RelativeDirection.Magnitude(); Angle <= Configuration::ESP.FOVRadius / 2.0 && Score < Overlay->SelectionThreshold) {
            if (Score < BestScore) {
                BestScore = Score;
                Overlay->BestTargetActor = Actor;
                Overlay->BestTargetActorType = ActorType;
                Overlay->BestTargetLocation = ActorLocation;
                Overlay->BestTargetRotation = UKismetMathLibrary::FindLookAtRotation(CharacterLocation, ActorLocation);
            }
        }
    }

    Func_DoTeleportToTargeted(Overlay, BestScore);

    // Don't aimbot while the overlay is showing
    if (Configuration::Aimbot.LegacyAimbot && !Overlay->ShowOverlay()) {
        if (IsKeyHeld(VK_LBUTTON) && std::abs(BestScore - FLT_MAX) > 0.0001f) {
            // Only aimbot when a bow is equipped
            if (ValeriaCharacter->GetEquippedItem().ItemType->Name.ToString().find("Tool_Bow_") != std::string::npos) {
                bool IsAnimal = false;
                for (auto& [Actor, WorldPosition, DisplayName, ActorType, Type, Quality, Variant, shouldAdd] : Overlay->CachedActors) {
                    if (shouldAdd && ActorType == EType::Animal && IsActorValid(Actor)) {
                        if (FVector ActorLocation = Actor->K2_GetActorLocation(); ActorLocation == Overlay->BestTargetLocation) {
                            IsAnimal = true;
                            break;
                        }
                    }
                }
                // Adjust the aim rotation only if the selected best target is an animal
                if (IsAnimal) {
                    // Apply offset to pitch and yaw directly
                    FRotator TargetRotation = UKismetMathLibrary::FindLookAtRotation(CharacterLocation, Overlay->BestTargetLocation);
                    TargetRotation.Pitch += Overlay->AimOffset.X;
                    TargetRotation.Yaw += Overlay->AimOffset.Y;

                    // Smooth rotation adjustment
                    FRotator NewRotation = CustomMath::RInterpTo(CharacterRotation, TargetRotation, UGameplayStatics::GetTimeSeconds(World), Overlay->SmoothingFactor);
                    PlayerController->SetControlRotation(NewRotation);
                }
            }
        }
    }
}

inline void Func_DoESP(PaliaOverlay* Overlay, const AHUD* HUD) {
    if (!Configuration::ESP.Enabled) {
        Overlay->CachedActors.clear();
        return;
    }

    // Manage Cache Logic
    ClearActorCache(Overlay);
    ManageActorCache(Overlay);

    AValeriaCharacter* VC = GetValeriaCharacter();
    if (!VC)
        return;

    APlayerController* PlayerController = GetPlayerController();
    if (!PlayerController)
        return;

    FVector PawnLocation = PlayerController->K2_GetPawn()->K2_GetActorLocation();

    // Draw ESP Names Entities
    for (auto& [Actor, WorldPosition, DisplayName, ActorType, Type, Quality, Variant, shouldAdd] : Overlay->CachedActors) {
        FVector ActorLocation = WorldPosition;
        if (ActorType == EType::Animal || ActorType == EType::Bug || ActorType == EType::Players || ActorType == EType::Loot) {
            if (!IsActorValid(Actor))
                continue;
            if (ActorLocation = Actor->K2_GetActorLocation(); ActorLocation.IsZero())
                continue;
            if (Actor == VC)
                continue;
        }

        // Adjust Z coordinate for head-level display
        float HeightAdjustment = 100.0f; // Adjust this value based on typical actor height
        ActorLocation.Z += HeightAdjustment;

        double Distance = PawnLocation.GetDistanceToInMeters(ActorLocation);

        if (Distance < 2.0)
            continue;
        if (Configuration::ESP.Culling && Distance > Configuration::ESP.CullDistance)
            continue;

        FVector2D ScreenLocation;
        if (PlayerController->ProjectWorldLocationToScreen(ActorLocation, &ScreenLocation, true)) {
            ImU32 Color = IM_COL32(0xFF, 0xFF, 0xFF, 0xFF);
            bool bShouldDraw = false;

            switch (ActorType) {
            case EType::Forage:
            {
                auto forageConfig = Configuration::GetConfig<ESPStarItem>(Type);
                if (forageConfig.Enabled(Quality)) {
                    bShouldDraw = true;
                    Color = forageConfig.Color;
                }
            }
            break;

            case EType::Ore:
            {
                auto oreConfig = Configuration::GetConfig<ESPSizeItem>(Type);
                auto oreVariant = std::get<EGatherableSize>(Variant);
                if (oreConfig.Enabled(oreVariant)) {
                    if (auto Ore = static_cast<ABP_ValeriaGatherableLoot_C*>(Actor)) {
                        if (Ore->IAmAlive) { //Show Ore only if it has not been gathered by localPlayer
                            bShouldDraw = true;
                            Color = oreConfig.Color;
                        }
                    }
                }
            }
            break;

            case EType::Players:
            {
                if (Configuration::ESP.PlayerEntities.Player.Enabled) {
                    bShouldDraw = true;
                    Color = Configuration::ESP.PlayerEntities.Player.Color;
                }
            }
            break;

            case EType::Animal:
            {
                auto animalConfig = Configuration::GetConfig<ESPSingleItem>(Type, Variant);
                if (animalConfig.Enabled) {
                    bShouldDraw = true;
                    Color = animalConfig.Color;
                }
            }
            break;

            case EType::Tree:
            {
                auto treeConfig = Configuration::GetConfig<ESPSizeItem>(Type);
                auto treeVariant = std::get<EGatherableSize>(Variant);
                if (treeConfig.Enabled(treeVariant)) {
                    bShouldDraw = true;
                    Color = treeConfig.Color;
                }
            }
            break;

            case EType::Bug:
            {
                auto bugConfig = Configuration::GetConfig<ESPStarItem>(Type, Variant);
                if (bugConfig.Enabled(Quality)) {
                    bShouldDraw = true;
                    Color = bugConfig.Color;
                }

            }
            break;

            case EType::NPCs:
            {
                if (Configuration::ESP.PlayerEntities.NPC.Enabled) {
                    bShouldDraw = true;
                    Color = Configuration::ESP.PlayerEntities.NPC.Color;
                }
            }
            break;

            case EType::Loot:
            {
                if (Configuration::ESP.PlayerEntities.Loot.Enabled) {
                    bShouldDraw = true;
                    Color = Configuration::ESP.PlayerEntities.Loot.Color;
                }
            }
            break;

            case EType::Quest:
            {
                if (Configuration::ESP.PlayerEntities.Quest.Enabled) {
                    bShouldDraw = true;
                    Color = Configuration::ESP.PlayerEntities.Quest.Color;
                }
            }
            break;

            case EType::RummagePiles:
            {
                if (Configuration::ESP.PlayerEntities.RummagePiles.Enabled) {
                    if (auto Pile = static_cast<ATimedLootPile*>(Actor)) {
                        const auto ValeriaCharacter = GetValeriaCharacter();
                        if (ValeriaCharacter && Pile->CanGather(ValeriaCharacter) && Pile->bActivated) {
                            bShouldDraw = true;
                            Color = Configuration::ESP.PlayerEntities.RummagePiles.Color;
                        }
                        else if (Configuration::ESP.PlayerEntities.Others.Enabled) {
                            bShouldDraw = true;
                            Color = Pile->bActivated ? IM_COL32(0xFF, 0xFF, 0xFF, 0xFF) : IM_COL32(0xFF, 0x00, 0x00, 0xFF);
                        }
                    }
                }
            }
            break;

            case EType::Stables:
            {
                if (Configuration::ESP.PlayerEntities.Stables.Enabled) {
                    bShouldDraw = true;
                    Color = Configuration::ESP.PlayerEntities.Stables.Color;
                }
            }
            break;

            case EType::Fish:
            {
                auto fishConfig = Configuration::GetConfig<ESPSingleItem>(Type, Variant);
                if (fishConfig.Enabled) {
                    bShouldDraw = true;
                    Color = fishConfig.Color;
                }
            }
            break;

            default:
                break;
            }

            if (Configuration::ESP.PlayerEntities.Others.Enabled && Configuration::IsUnknown(Type))
                bShouldDraw = true;

            if (!bShouldDraw)
                continue;

            if (!Roboto) {
                Roboto = reinterpret_cast<UFont*>(UObject::FindObject("Font Roboto.Roboto", EClassCastFlags::None));

                if (!Roboto)
                    continue;
            }

            // Construct text string
            std::string qualityName = (Quality > 0) ? PaliaOverlay::GetQualityName(Quality, ActorType) : "";

            // Prepare text with optional parts depending on the index values
            std::string text = DisplayName;
            if (!qualityName.empty()) {
                text += " [" + qualityName + "]";
            }
            text += std::format(" [{:.2f}m]", Distance);

			std::string despawnText = "";
			if (ActorType == EType::Ore && shouldAdd) {
				if (auto GatherableLoot = static_cast<ABP_ValeriaGatherableLoot_Mining_MultiHarvest_C*>(Actor); IsActorValid(GatherableLoot)) {
                    if (!GatherableLoot || !GatherableLoot->IsValidLowLevel()) continue;

					double seconds = 0;

					GatherableLoot->GetSecondsUntilDespawn(&seconds);

					if (seconds > 0)
						despawnText += "Despawning in " + std::to_string(static_cast<int>(seconds)) + "s";
                }
			}

			else if (ActorType == EType::Forage && shouldAdd) {
				if (auto ForageableLoot = static_cast<ABP_ValeriaGatherable_C*>(Actor); IsActorValid(ForageableLoot)) {
                    if (!ForageableLoot || !ForageableLoot->IsValidLowLevel()) continue;
					float seconds = ForageableLoot->Gatherable->GetSecondsUntilDespawn();

					if (seconds > 0)
						despawnText += "Despawning in " + std::to_string(static_cast<int>(seconds)) + "s";
				}
			}


            std::wstring wideText(text.begin(), text.end());
            std::wstring wideDespawnText(despawnText.begin(), despawnText.end());

            double BaseScale = 1.0; // Default scale at a reference distance
            double ReferenceDistance = 100.0; // Distance at which no scaling is applied
            double ScalingFactor = 0; // Determines how much the scale changes with distance

            double DistanceScale;
            DistanceScale = BaseScale - ScalingFactor * (Distance - ReferenceDistance);
            DistanceScale = CustomMath::Clamp(DistanceScale, 0.5, BaseScale); // Clamp the scale to a reasonable range

            const FVector2D TextScale = { DistanceScale * Configuration::ESP.TextScale, DistanceScale * Configuration::ESP.TextScale };
            ImColor IMC(Color);
            FLinearColor TextColor = { IMC.Value.x, IMC.Value.y, IMC.Value.z, IMC.Value.w };

            // Setup shadow properties
            ImColor IMCS(Color);
            FLinearColor ShadowColor = { IMCS.Value.x, IMCS.Value.y, IMCS.Value.z, IMCS.Value.w };

            // Calculate positions
            FVector2D TextPosition = ScreenLocation;
            FVector2D ShadowPosition = { TextPosition.X + 1.0, TextPosition.Y + 1.0 };

            // Despawn text positions
            FVector2D DespawnTextPosition = { TextPosition.X, TextPosition.Y + 13 };
            FVector2D DespawnShadowPosition = { DespawnTextPosition.X + 1.0, DespawnTextPosition.Y + 1.0 };

            // Draw shadow text
            HUD->Canvas->K2_DrawText(Roboto, FString(wideText.data()), ShadowPosition, TextScale, TextColor, 0, { 0, 0, 0, 1 }, { 1.0f, 1.0f }, true, true, true, { 0, 0, 0, 1 });
            // Draw main text
            HUD->Canvas->K2_DrawText(Roboto, FString(wideText.data()), TextPosition, TextScale, ShadowColor, 0, { 0, 0, 0, 1 }, { 1.0f, 1.0f }, true, true, true, { 0, 0, 0, 1 });

            if (Configuration::ESP.DespawnTimer) {
                // Draw despawn shadow text
                HUD->Canvas->K2_DrawText(Roboto, FString(wideDespawnText.data()), DespawnShadowPosition, TextScale, TextColor, 0, { 0, 0, 0, 1 }, { 1.0f, 1.0f }, true, true, true, { 0, 0, 0, 1 });
                // Draw despawn main text
                HUD->Canvas->K2_DrawText(Roboto, FString(wideDespawnText.data()), DespawnTextPosition, TextScale, ShadowColor, 0, { 0, 0, 0, 1 }, { 1.0f, 1.0f }, true, true, true, { 0, 0, 0, 1 });
            }
        }
    }

    // Logic for FOV and Targeting Drawing
    if (Configuration::ESP.DrawFOVCircle) {
        FVector2D PlayerScreenPosition;
        FVector2D TargetScreenPosition;

        if (PlayerController->ProjectWorldLocationToScreen(PawnLocation, &PlayerScreenPosition, true)) {
            // Calculate the center of the FOV circle based on the player's screen position
            FVector2D FOVCenter = { HUD->Canvas->ClipX * 0.5f, HUD->Canvas->ClipY * 0.5f };
            DrawCircle(HUD->Canvas, Configuration::ESP.FOVRadius, 1200, { 0.485f, 0.485f, 0.485f, 0.485f }, 1.0f);

            if (Overlay->BestTargetLocation.IsZero())
                return;
            if (!PlayerController->ProjectWorldLocationToScreen(Overlay->BestTargetLocation, &TargetScreenPosition, true))
                return;
            if (!(CustomMath::DistanceBetweenPoints(TargetScreenPosition, FOVCenter) <= Configuration::ESP.FOVRadius))
                return;

            HUD->Canvas->K2_DrawLine(FOVCenter, TargetScreenPosition, 0.5f, { 0.485f, 0.485f, 0.485f, 0.485f });
        }
    }
}

// [Movement]

inline void Func_DoNoClip(PaliaOverlay* Overlay) {
    if (!Overlay->bEnableNoclip && Overlay->bEnableNoclip == Overlay->bPreviousNoclipState)
        return;

    const auto ValeriaCharacter = GetValeriaCharacter();
    if (!ValeriaCharacter)
        return;

    UValeriaCharacterMoveComponent* ValeriaMovementComponent = ValeriaCharacter->GetValeriaCharacterMovementComponent();
    if (!ValeriaMovementComponent || ValeriaMovementComponent->IsDefaultObject() || !ValeriaMovementComponent->IsValidLowLevel())
        return;

    if (Overlay->bEnableNoclip != Overlay->bPreviousNoclipState) {
        if (Overlay->bEnableNoclip) {
            ValeriaMovementComponent->SetMovementMode(EMovementMode::MOVE_Flying, 5);
            ValeriaCharacter->CapsuleComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
        }
        else {
            ValeriaMovementComponent->SetMovementMode(EMovementMode::MOVE_Walking, 1);
            ValeriaCharacter->CapsuleComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
            ValeriaCharacter->CapsuleComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Block);
        }

        Overlay->bPreviousNoclipState = Overlay->bEnableNoclip;
    }

    // Logic for Noclip Camera
    if (Overlay->bEnableNoclip) {
        if (Overlay->ShowOverlay() || !IsGameWindowActive())
            return;

        const auto PlayerController = GetPlayerController();
        if (!PlayerController)
            return;

        // Calculate forward and right vectors based on the camera's yaw
        const FRotator& CameraRot = PlayerController->PlayerCameraManager->GetCameraRotation();

        FVector CameraForward = UKismetMathLibrary::GetForwardVector(CameraRot);
        FVector CameraRight = UKismetMathLibrary::GetRightVector(CameraRot);
        constexpr FVector CameraUp = { 0.f, 0.f, 1.f };

        CameraForward.Normalize();
        CameraRight.Normalize();

        FVector MovementDirection = { 0.f, 0.f, 0.f };
        float FlySpeed = 800.0f;

        if (IsKeyHeld('W')) {
            MovementDirection += CameraForward * FlySpeed;
        }
        if (IsKeyHeld('S')) {
            MovementDirection -= CameraForward * FlySpeed;
        }
        if (IsKeyHeld('D')) {
            MovementDirection += CameraRight * FlySpeed;
        }
        if (IsKeyHeld('A')) {
            MovementDirection -= CameraRight * FlySpeed;
        }
        if (IsKeyHeld(VK_SPACE)) {
            MovementDirection += CameraUp * FlySpeed;
        }
        if (IsKeyHeld(VK_CONTROL)) {
            MovementDirection -= CameraUp * FlySpeed;
        }
        if (IsKeyHeld(VK_SHIFT)) {
            FlySpeed *= 2.0f;
        }

        // Normalize the total movement direction
        MovementDirection.Normalize();
        MovementDirection *= FlySpeed;

        // Time delta
        constexpr float DeltaTime = 1.0f / 60.0f; // Assuming 60 FPS

        const FVector MovementDelta = MovementDirection * DeltaTime;

        // Update character position
        FHitResult HitResult;
        ValeriaCharacter->K2_SetActorLocation(ValeriaCharacter->K2_GetActorLocation() + MovementDelta, false, &HitResult, false);
    }
}

inline void Func_DoPersistentMovement(const PaliaOverlay* Overlay) {
    const auto ValeriaCharacter = GetValeriaCharacter();
    if (!ValeriaCharacter)
        return;

    UValeriaCharacterMoveComponent* ValeriaMovementComponent = ValeriaCharacter->GetValeriaCharacterMovementComponent();
    if (!ValeriaMovementComponent)
        return;

    ValeriaMovementComponent->MaxWalkSpeed = Configuration::GameModifiers.CustomWalkSpeed;
    ValeriaMovementComponent->SprintSpeedMultiplier = Configuration::GameModifiers.CustomSprintSpeedMultiplier;
    ValeriaMovementComponent->ClimbingSpeed = Configuration::GameModifiers.CustomClimbingSpeed;
    ValeriaMovementComponent->GlidingMaxSpeed = Configuration::GameModifiers.CustomGlidingSpeed;
    ValeriaMovementComponent->GlidingFallSpeed = Configuration::GameModifiers.CustomGlidingFallSpeed;
    ValeriaMovementComponent->JumpZVelocity = Configuration::GameModifiers.CustomJumpVelocity;
    ValeriaMovementComponent->MaxStepHeight = Configuration::GameModifiers.CustomMaxStepHeight;
}

// [Placement]

inline void Func_DoPlaceAnywhere(const PaliaOverlay* Overlay) {
    if (!Configuration::Housing.PlaceAnywhere)
        return;

    const auto ValeriaCharacter = GetValeriaCharacter();
    if (!ValeriaCharacter)
        return;

    UPlacementComponent* PlacementComponent = ValeriaCharacter->GetPlacement();
    if (PlacementComponent) {
        PlacementComponent->CanPlaceHere = true;
        PlacementComponent->MaxPlacementUpAngle = Configuration::Housing.MaxUpAngle;
    }
}

// [Fishing]

inline void ToggleFishingDelays(const bool RemoveDelays) {
    const auto ValeriaController = GetValeriaController();
    if (!ValeriaController) {
        return;
    }
    UValeriaGameInstance* ValeriaGameInstance = ValeriaController->GameInst;
    if (!ValeriaGameInstance || ValeriaGameInstance->IsDefaultObject() || !ValeriaGameInstance->IsValidLowLevel() ) {
        return;
    }

    auto& CastSettings = ValeriaGameInstance->Configs.Globals.Fishing->CastSettings;

    // Avoid continuously setting values if already set properly
    float newCastDelay = RemoveDelays ? 0.0f : 0.150f;
    if (std::abs(CastSettings.CastDelay - newCastDelay) < 0.0001f) {
        return;
    }

    auto& FishingSettings = ValeriaGameInstance->Configs.Globals.Fishing;
    auto& EndSettings = ValeriaGameInstance->Configs.Globals.Fishing->EndSettings;

    CastSettings.CastDelay = newCastDelay;
    CastSettings.MaxDistanceToCast = 1500.0f;
    CastSettings.MinDistanceToCast = RemoveDelays ? 1500.0f : 500.0f;
    CastSettings.LaunchOffset = RemoveDelays ? FVector{ 1500, 0, -300 } : FVector{};
    CastSettings.WindupSpeed = RemoveDelays ? FLT_MAX : 0.350f;

    FishingSettings->AfterFinishDestroyBobberWhenAtDistanceToRod = RemoveDelays ? FLT_MAX : 50.0;
    FishingSettings->FishingFinishReelInSpeed = RemoveDelays ? FLT_MAX : 1600.0f;
    FishingSettings->TotalCelebrationDuration = RemoveDelays ? 0.0f : 4.0f;
    FishingSettings->OnBeginReelingInitialCooldown = RemoveDelays ? 0.0f : 1.0f;

    EndSettings.MaxTimeOfEndFishingCelebrate = RemoveDelays ? 0.0f : 4.5f;
    EndSettings.MaxTimeOfEndFishingDefault = RemoveDelays ? 0.0f : 2.0f;
    EndSettings.MaxTimeOfEndFishingEmptyHanded = RemoveDelays ? 0.0f : 2.2f;
    EndSettings.MaxTimeOfEndFishingFailure = RemoveDelays ? 0.0f : 1.75f;
}

inline void Func_DoFastAutoFishing(const PaliaOverlay* Overlay) {
    // Toggle values (Safe to leave looped)
    ToggleFishingDelays(Overlay->bEnableAutoFishing);

    if (!Overlay->bEnableAutoFishing) {
        return;
    }

    const auto ValeriaCharacter = GetValeriaCharacter();
    if (!ValeriaCharacter || !ValeriaCharacter->GetEquippedItem().ItemType->IsFishingRod()) {
        return;
    }

    if (Configuration::Fishing.RequireClickFishing ? (!Overlay->ShowOverlay() && IsGameWindowActive() && IsKeyHeld(VK_LBUTTON)) : true) {
        // Instant Catch
        auto FishingComponent = ValeriaCharacter->GetFishing();
        if (FishingComponent) {
            if (static_cast<EFishingState_NEW>(FishingComponent->GetFishingState()) == EFishingState_NEW::Bite) {
                FFishingEndContext Context;
                FishingComponent->RpcServer_EndFishing(Context);
                FishingComponent->SetFishingState(EFishingState_OLD::None);
            }
        }

        // Cast the rod
        ValeriaCharacter->ToolPrimaryActionPressed();
        ValeriaCharacter->ToolPrimaryActionReleased();
    }
}

inline void Func_DoInstantCatch(const PaliaOverlay* Overlay) {
    if (!Configuration::Fishing.InstantCatch)
        return;

    const auto ValeriaCharacter = GetValeriaCharacter();
    if (!ValeriaCharacter)
        return;

    UFishingComponent* FishingComponent = ValeriaCharacter->GetFishing();
    if (!FishingComponent)
        return;

    if (static_cast<EFishingState_NEW>(FishingComponent->GetFishingState()) == EFishingState_NEW::Bite) {
        FFishingEndContext Context;
        FishingComponent->RpcServer_EndFishing(Context);
        FishingComponent->SetFishingState(EFishingState_OLD::None);
    }
}
int fishingFlushCounter = 0;
inline void Func_DoFishingCleanup(const PaliaOverlay* Overlay) {
    const auto ValeriaController = GetValeriaController();
    const auto ValeriaCharacter = GetValeriaCharacter();
    if (!ValeriaController || !ValeriaCharacter) {
        return;
    }

    // Avoid doing extra work
    if (!Configuration::Fishing.SellFish && !Configuration::Fishing.DiscardTrash && !Configuration::Fishing.OpenStoreWaterlogged) {
        return;
    }

    UVillagerStoreComponent* StoreComponent = ValeriaCharacter->StoreComponent;
    const UInventoryComponent* InventoryComponent = ValeriaCharacter->GetInventory();
    if (!InventoryComponent) {
        return;
    }

    // Sell / Discard / Storage
    for (int BagIndex = 0; BagIndex < InventoryComponent->Bags.Num(); BagIndex++) {
        for (int SlotIndex = 0; SlotIndex < 8; SlotIndex++) {
            FBagSlotLocation Slot{ BagIndex, SlotIndex };
            FValeriaItem Item = InventoryComponent->GetItemAt(Slot);

            if (Configuration::Fishing.SellFish && Item.ItemType->Category == EItemCategory::Fish && StoreComponent) {
                if (!StoreComponent->StoreCanBuyItem(Slot)) {
                    StoreComponent->Client_SetVillagerStore(2);
                    StoreComponent->Client_OpenStore();
                }

                StoreComponent->RpcServer_SellItem(Slot, 10);
            }
            else if (Configuration::Fishing.DiscardTrash && Item.ItemType->Category == EItemCategory::Junk) {
                // Don't ever discard more than the amount of the stack
                ValeriaController->DiscardItem(Slot, Item.Amount);
            }
            else if (Item.ItemType->PersistId == 2810) { // Waterlogged Chest
                if (!Configuration::Fishing.OpenStoreWaterlogged) {
                    // Don't ever discard more than the amount of the stack
                    ValeriaController->DiscardItem(Slot, Item.Amount);
                }
                else {
                    ValeriaController->ConsumeItem(Slot);
                }
            }
            else if (Configuration::Fishing.OpenStoreWaterlogged && Item.ItemType->Name.ToString().find("DA_ItemType_Decor_Makeshift_") != std::string::npos) {
                ValeriaController->MoveItemSlotToStorage(Slot, 1, EStoragePoolType::Primary);
            }
        }
    }
    fishingFlushCounter++;
    if (fishingFlushCounter >= 30) {
        if (APlayerController* PlayerController = GetPlayerController()) {
            PlayerController->ClientFlushLevelStreaming();
            PlayerController->ClientForceGarbageCollection();

            fishingFlushCounter = 0;
        }
    }
}

inline void Func_DoFishingCaptureOverride(PaliaOverlay* Overlay, Params::FishingComponent_RpcServer_SelectLoot* SelectLoot) {
    if (Overlay->bCaptureFishingSpot) {
        memcpy(&Overlay->sOverrideFishingSpot, &SelectLoot->RPCLootParams.WaterType_Deprecated, sizeof(FName));
        Overlay->bCaptureFishingSpot = false;
    }
    if (Overlay->bOverrideFishingSpot) {
        memcpy(&SelectLoot->RPCLootParams.WaterType_Deprecated, &Overlay->sOverrideFishingSpot, sizeof(FName));
    }
}

Params::FishingComponent_RpcServer_EndFishing* EndFishingDetoured(const PaliaOverlay* Overlay, Params::FishingComponent_RpcServer_EndFishing* EndFishing) {
    if (Configuration::Fishing.InstantCatch || Overlay->bEnableAutoFishing) {
        EndFishing->Context.Result = EFishingMiniGameResult::Success;
    }

    if (Configuration::Fishing.NoDurability) {
        EndFishing->Context.DurabilityReduction = 0;
    }

    EndFishing->Context.Perfect = Configuration::Fishing.PerfectCatch ? true : Configuration::Fishing.InstantCatch ? false : EndFishing->Context.Perfect;
    EndFishing->Context.SourceWaterBody = nullptr;
    EndFishing->Context.bUsedMultiplayerHelp = Configuration::Fishing.MultiplayerHelp;
    EndFishing->Context.StartRodHealth = 100.0f;
    EndFishing->Context.EndRodHealth = 100.0f;
    EndFishing->Context.StartFishHealth = 100.0f;
    EndFishing->Context.EndFishHealth = 100.0f;
    return EndFishing;
}

// [Firing]

inline void Func_DoSilentAim(const PaliaOverlay* Overlay, void* Params) {
    auto FireProjectile = static_cast<Params::ProjectileFiringComponent_RpcServer_FireProjectile*>(Params);
    const auto ValeriaCharacter = GetValeriaCharacter();

    if (!ValeriaCharacter)
        return;

    auto FiringComponent = ValeriaCharacter->GetFiringComponent();
    if (!FiringComponent)
        return;

    if (Configuration::Aimbot.SilentAimbot) {
        // Initial Target Check
        if (!Overlay->BestTargetActor || !IsActorValid(Overlay->BestTargetActor))
            return;

        FVector TargetLocation = Overlay->BestTargetActor->K2_GetActorLocation();
        FVector HitLocation = TargetLocation;

        for (auto& [ProjectileId, Pad_22C8, ProjectileActor, HasHit, Pad_22C9] : FiringComponent->FiredProjectiles) {
            if (ProjectileId == FireProjectile->ProjectileId) {

                // Projectile Check
                if (!ProjectileActor || !IsActorValid(ProjectileActor))
                    continue;

                FVector ProjectileLocation = ProjectileActor->K2_GetActorLocation();

                // Checks Before FiringTargetLocation
                if (!Overlay->BestTargetActor || !IsActorValid(Overlay->BestTargetActor))
                    continue;

                FVector FiringTargetLocation = Overlay->BestTargetActor->K2_GetActorLocation();

                FVector DirectionToTarget = (FiringTargetLocation - ProjectileLocation).GetNormalized();
                float DistanceBeforeTarget = 50.0f;
                FVector NewProjectileLocation = FiringTargetLocation - (DirectionToTarget * DistanceBeforeTarget);

                HasHit = true;
                FHitResult HitResult;
                ProjectileActor->K2_SetActorLocation(NewProjectileLocation, false, &HitResult, false);
                HitResult.Location = { NewProjectileLocation };

                FiringComponent->RpcServer_NotifyProjectileHit(FireProjectile->ProjectileId, Overlay->BestTargetActor, HitLocation);
            }
        }
    }
}

inline void Func_DoCompleteMinigame(const PaliaOverlay* Overlay) {
    if (!Configuration::Misc.MinigameSkip) return;

    const auto ValeriaCharacter = GetValeriaCharacter();
    if (!ValeriaCharacter) return;

    const auto MinigameComponent = ValeriaCharacter->MinigameQTE;
    if (!MinigameComponent) return;

    if (MinigameComponent->IsPlaying()) {
        MinigameComponent->RpcServer_ChangeState(EMinigameState::Success);
    }
}

// Detouring

void DetourManager::ProcessEventDetour(const UObject* Class, const UFunction* Function, void* Params) {
    const auto Overlay = dynamic_cast<PaliaOverlay*>(OverlayBase::Instance);
    const auto fn = Function->GetFullName();
    invocations.insert(fn);

    // PlayerTick
    if (fn == "Function Engine.Actor.ReceiveTick") {
        Func_DoFastAutoFishing(Overlay);
        Func_DoPersistentMovement(Overlay);
        Func_DoNoClip(Overlay);
        Func_DoAntiAfk(Overlay);
    }
    // HUD
    else if (fn == "Function Engine.HUD.ReceiveDrawHUD") {
        Func_DoESP(Overlay, reinterpret_cast<const AHUD*>(Class));
        Func_DoInteliAim(Overlay);
        Func_DoPlaceAnywhere(Overlay);
        Func_DoCompleteMinigame(Overlay);
    }
    // Fishing Capture/Override
    else if (fn == "Function Palia.FishingComponent.RpcServer_SelectLoot") {
        Func_DoFishingCaptureOverride(Overlay, static_cast<Params::FishingComponent_RpcServer_SelectLoot*>(Params));
    }
    // Fishing Instant Catch
    else if (fn == "Function Palia.FishingComponent.RpcClient_StartFishingAt_Deprecated") {
        Func_DoInstantCatch(Overlay);
    }
    // Fishing End (Perfect/Durability/PlayerHelp)
    else if (fn == "Function Palia.FishingComponent.RpcServer_EndFishing") {
        EndFishingDetoured(Overlay, static_cast<Params::FishingComponent_RpcServer_EndFishing*>(Params));
    }
    // Fishing Cleanup (Sell/Discard/Move)
    else if (fn == "Function Palia.FishingComponent.RpcClient_FishCaught") {
        Func_DoFishingCleanup(Overlay);
    }
    // Teleport To Waypoint
    else if (fn == "Function Palia.TrackingComponent.RpcClient_SetUserMarkerViaWorldMap") {
        Func_DoTeleportToWaypoint(Overlay, static_cast<Params::TrackingComponent_RpcClient_SetUserMarkerViaWorldMap*>(Params));
    }
    // Silent Aim
    else if (fn == "Function Palia.ProjectileFiringComponent.RpcServer_FireProjectile") {
        Func_DoSilentAim(Overlay, Params);
    }
    // ??
    else if (fn == "Function Palia.ValeriaClientPriMovementComponent.RpcServer_SendMovement") {
        static_cast<Params::ValeriaClientPriMovementComponent_RpcServer_SendMovement*>(Params)->MoveInfo.TargetVelocity = {0, 0, 0};
    }
    else if (fn == "Function Engine.WorldPartitionBlueprintLibrary.UnloadActors") {
        ClearActorCache(Overlay);
    }
    else if (fn == "Function Engine.WorldPartitionBlueprintLibrary.LoadActors") {
        ManageActorCache(Overlay);
    }

    if (OriginalProcessEvent) {
        OriginalProcessEvent(Class, Function, Params);
    }
}

// Essentials

void DetourManager::SetupDetour(void* Instance, void (*DetourFunc)(const UObject*, const UFunction*, void*)) {
    const void** Vtable = *static_cast<const void***>(Instance);

    DWORD OldProtection;
    VirtualProtect(Vtable, sizeof(DWORD) * 1024, PAGE_EXECUTE_READWRITE, &OldProtection);

    const int32_t Idx = Offsets::ProcessEventIdx;
    OriginalProcessEvent = reinterpret_cast<void(*)(const UObject*, const UFunction*, void*)>(reinterpret_cast<uintptr_t>(GetModuleHandle(nullptr)) + Offsets::ProcessEvent);
    Vtable[Idx] = DetourFunc;

    HookedClient = Instance;
    VirtualProtect(Vtable, sizeof(DWORD) * 1024, OldProtection, &OldProtection);
}

void DetourManager::SetupDetour(void* Instance) {
    SetupDetour(Instance, &DetourManager::ProcessEventDetour);
}

void DetourManager::ProcessEventDetourCallback(const UObject* Class, const UFunction* Function, void* Params, const DetourManager* manager) {
    manager->ProcessEventDetour(Class, Function, Params);
}

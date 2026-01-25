#include "StashGameMode.h"
#include "StashPlayerController.h"
#include "StashHUD.h"

AStashGameMode::AStashGameMode()
{
    // Use our custom PlayerController that handles Stash Pay callbacks
    PlayerControllerClass = AStashPlayerController::StaticClass();
    
    // Use our custom HUD that displays the checkout button
    HUDClass = AStashHUD::StaticClass();
}

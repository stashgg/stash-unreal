#include "StashHUD.h"
#include "Blueprint/UserWidget.h"

AStashHUD::AStashHUD()
{
    // Default checkout URL for testing
    CheckoutURL = TEXT("https://checkout.stash.gg/demo");
    StatusText = TEXT("Ready");
}

void AStashHUD::BeginPlay()
{
    Super::BeginPlay();

    // Create the widget if a class is specified
    if (CheckoutWidgetClass)
    {
        APlayerController* PC = GetOwningPlayerController();
        if (PC)
        {
            CheckoutWidget = CreateWidget<UUserWidget>(PC, CheckoutWidgetClass);
            if (CheckoutWidget)
            {
                CheckoutWidget->AddToViewport();
                UE_LOG(LogTemp, Log, TEXT("[StashPay] Checkout widget created and added to viewport"));
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[StashPay] No CheckoutWidgetClass specified. Create a Widget Blueprint and assign it in the HUD Blueprint."));
    }
}

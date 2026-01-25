#include "StashPlayerController.h"
#include "MobileNativeCodeBlueprint.h"

AStashPlayerController::AStashPlayerController()
{
}

void AStashPlayerController::BeginPlay()
{
    Super::BeginPlay();

    // Bind to Stash Pay payment callbacks
    UMobileNativeCodeBlueprint::OnPaymentSuccess.AddDynamic(this, &AStashPlayerController::HandlePaymentSuccess);
    UMobileNativeCodeBlueprint::OnPaymentFailure.AddDynamic(this, &AStashPlayerController::HandlePaymentFailure);
    UMobileNativeCodeBlueprint::OnDialogDismissed.AddDynamic(this, &AStashPlayerController::HandleDialogDismissed);
    UMobileNativeCodeBlueprint::OnPageLoaded.AddDynamic(this, &AStashPlayerController::HandlePageLoaded);

    UE_LOG(LogTemp, Log, TEXT("[StashPay] StashPlayerController initialized and callbacks bound"));
}

void AStashPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Unbind from callbacks to avoid issues
    UMobileNativeCodeBlueprint::OnPaymentSuccess.RemoveDynamic(this, &AStashPlayerController::HandlePaymentSuccess);
    UMobileNativeCodeBlueprint::OnPaymentFailure.RemoveDynamic(this, &AStashPlayerController::HandlePaymentFailure);
    UMobileNativeCodeBlueprint::OnDialogDismissed.RemoveDynamic(this, &AStashPlayerController::HandleDialogDismissed);
    UMobileNativeCodeBlueprint::OnPageLoaded.RemoveDynamic(this, &AStashPlayerController::HandlePageLoaded);

    Super::EndPlay(EndPlayReason);
}

void AStashPlayerController::OpenCheckout(const FString& CheckoutURL)
{
    UE_LOG(LogTemp, Log, TEXT("[StashPay] Opening checkout: %s"), *CheckoutURL);

#if PLATFORM_IOS
    UMobileNativeCodeBlueprint::OpenStashPayCheckoutIOS(CheckoutURL);
#elif PLATFORM_ANDROID
    UMobileNativeCodeBlueprint::OpenStashPayCheckout(CheckoutURL);
#else
    UE_LOG(LogTemp, Warning, TEXT("[StashPay] Checkout not supported on this platform. iOS/Android only."));
#endif
}

bool AStashPlayerController::IsCheckoutOpen()
{
#if PLATFORM_IOS
    return UMobileNativeCodeBlueprint::IsStashPayCheckoutOpenIOS();
#elif PLATFORM_ANDROID
    return UMobileNativeCodeBlueprint::IsStashPayCheckoutOpen();
#else
    return false;
#endif
}

void AStashPlayerController::DismissCheckout()
{
    UE_LOG(LogTemp, Log, TEXT("[StashPay] Dismissing checkout"));

#if PLATFORM_IOS
    UMobileNativeCodeBlueprint::DismissStashPayCheckoutIOS();
#elif PLATFORM_ANDROID
    UMobileNativeCodeBlueprint::DismissStashPayCheckout();
#endif
}

void AStashPlayerController::HandlePaymentSuccess()
{
    UE_LOG(LogTemp, Log, TEXT("[StashPay] Payment succeeded!"));
    OnPaymentSucceeded();
}

void AStashPlayerController::HandlePaymentFailure()
{
    UE_LOG(LogTemp, Log, TEXT("[StashPay] Payment failed!"));
    OnPaymentFailed();
}

void AStashPlayerController::HandleDialogDismissed()
{
    UE_LOG(LogTemp, Log, TEXT("[StashPay] Checkout dismissed"));
    OnCheckoutDismissed();
}

void AStashPlayerController::HandlePageLoaded(float LoadTimeMs)
{
    UE_LOG(LogTemp, Log, TEXT("[StashPay] Page loaded in %.2f ms"), LoadTimeMs);
    OnCheckoutPageLoaded(LoadTimeMs);
}

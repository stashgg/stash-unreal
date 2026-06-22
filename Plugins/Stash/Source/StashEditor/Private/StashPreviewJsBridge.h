// Copyright Stash. All Rights Reserved.
// JavaScript injected into the CEF preview webview to emulate window.stash_sdk.

#pragma once

#include "CoreMinimal.h"

namespace StashPreviewJsBridge
{
	inline constexpr TCHAR SchemePrefix[] = TEXT("stash-unreal-preview://");

	inline FString GetInjectionScript()
	{
		return TEXT(R"JS(
(function() {
  if (window.__stashUnrealPreviewInjected) { return; }
  window.__stashUnrealPreviewInjected = true;
  window.stash_sdk = window.stash_sdk || {};
  function notify(path, query) {
    try {
      var q = query ? ('?' + query) : '';
      window.location.href = 'stash-unreal-preview://' + path + q;
    } catch (e) {}
  }
  window.stash_sdk.onPaymentSuccess = function(data) {
    notify('paymentSuccess');
  };
  window.stash_sdk.onPaymentFailure = function(data) {
    notify('paymentFailure');
  };
  window.stash_sdk.onPurchaseProcessing = function(data) {
    notify('purchaseProcessing');
  };
  window.stash_sdk.setPaymentChannel = function(optinType) {
    notify('optin', 'type=' + encodeURIComponent(optinType || ''));
  };
  window.stash_sdk.openExternalBrowser = function(url) {
    notify('externalBrowser', 'url=' + encodeURIComponent(url || ''));
  };
  var originalClose = window.close;
  window.close = function() {
    notify('dismiss');
    if (typeof originalClose === 'function') {
      try { originalClose.apply(window, arguments); } catch (e) {}
    }
  };
})();
)JS");
	}
}

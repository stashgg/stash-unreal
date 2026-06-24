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
  function applyMobilePreviewStyles() {
    try {
      var meta = document.querySelector('meta[name="viewport"]');
      if (!meta) {
        meta = document.createElement('meta');
        meta.name = 'viewport';
        meta.content = 'width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no';
        (document.head || document.documentElement).appendChild(meta);
      }
      if (!document.getElementById('stash-unreal-preview-mobile')) {
        var style = document.createElement('style');
        style.id = 'stash-unreal-preview-mobile';
        (document.head || document.documentElement).appendChild(style);
      }
      var mobileStyle = document.getElementById('stash-unreal-preview-mobile');
      if (mobileStyle) {
        mobileStyle.textContent = 'html, body { overflow-x: hidden !important; overflow-y: auto !important; overscroll-behavior: none; -webkit-overflow-scrolling: touch; -ms-overflow-style: none; scrollbar-width: none; } html::-webkit-scrollbar, body::-webkit-scrollbar { display: none; width: 0; height: 0; } html.stash-unreal-preview-dragging, html.stash-unreal-preview-dragging * { -webkit-user-select: none !important; user-select: none !important; -webkit-touch-callout: none !important; }';
      }
    } catch (e) {}
  }

  function applyDragToScroll() {
    if (window.__stashUnrealPreviewDragScroll) { return; }
    window.__stashUnrealPreviewDragScroll = true;

    var dragThreshold = 6;
    var active = null;

    function findScrollable(node) {
      var el = node;
      while (el && el !== document.documentElement) {
        try {
          var style = window.getComputedStyle(el);
          var overflowY = style.overflowY;
          if ((overflowY === 'auto' || overflowY === 'scroll' || overflowY === 'overlay') && el.scrollHeight > el.clientHeight + 1) {
            return el;
          }
        } catch (e) {}
        el = el.parentElement;
      }
      return document.scrollingElement || document.documentElement || document.body;
    }

    function pointerY(e) {
      if (e.touches && e.touches.length) { return e.touches[0].clientY; }
      if (e.changedTouches && e.changedTouches.length) { return e.changedTouches[0].clientY; }
      return e.clientY;
    }

    function clearTextSelection() {
      try {
        var sel = window.getSelection ? window.getSelection() : null;
        if (sel && sel.removeAllRanges) { sel.removeAllRanges(); }
        if (document.selection && document.selection.empty) { document.selection.empty(); }
      } catch (e) {}
    }

    function setDragScrolling(on) {
      try {
        if (on) {
          document.documentElement.classList.add('stash-unreal-preview-dragging');
          clearTextSelection();
        } else {
          document.documentElement.classList.remove('stash-unreal-preview-dragging');
        }
      } catch (e) {}
    }

    function onPointerDown(e) {
      if (e.type === 'mousedown' && e.button !== 0) { return; }
      active = {
        scrollEl: findScrollable(e.target),
        startY: pointerY(e),
        startScrollTop: 0,
        dragging: false
      };
      active.startScrollTop = active.scrollEl.scrollTop;
    }

    function onPointerMove(e) {
      if (!active) { return; }
      var y = pointerY(e);
      if (y === undefined) { return; }
      var deltaY = y - active.startY;
      if (!active.dragging) {
        if (Math.abs(deltaY) < dragThreshold) { return; }
        active.dragging = true;
        setDragScrolling(true);
      }
      try { e.preventDefault(); } catch (err) {}
      clearTextSelection();
      active.scrollEl.scrollTop = active.startScrollTop - deltaY;
    }

    function onPointerUp() {
      setDragScrolling(false);
      active = null;
    }

    function onSelectStart(e) {
      if (active && active.dragging) {
        try { e.preventDefault(); } catch (err) {}
      }
    }

    function onDragStart(e) {
      if (active && active.dragging) {
        try { e.preventDefault(); } catch (err) {}
      }
    }

    var opts = { passive: false };
    document.addEventListener('mousedown', onPointerDown, opts);
    document.addEventListener('mousemove', onPointerMove, opts);
    document.addEventListener('mouseup', onPointerUp, opts);
    document.addEventListener('mouseleave', onPointerUp, opts);
    document.addEventListener('touchstart', onPointerDown, opts);
    document.addEventListener('touchmove', onPointerMove, opts);
    document.addEventListener('touchend', onPointerUp, opts);
    document.addEventListener('touchcancel', onPointerUp, opts);
    document.addEventListener('selectstart', onSelectStart, opts);
    document.addEventListener('dragstart', onDragStart, opts);
  }

  applyMobilePreviewStyles();
  applyDragToScroll();

  function ensureStashSdkBridge() {
    window.stash_sdk = window.stash_sdk || {};
    function notify(path, query) {
      try {
        var q = query ? ('?' + query) : '';
        console.log('__STASH_PREVIEW__:' + path + q);
        window.location.href = 'stash-unreal-preview:///' + path + q;
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
    window.stash_sdk.onProcessingCompleted = function(data) {
      notify('processingCompleted');
    };
    window.stash_sdk.setPaymentChannel = function(optinType) {
      notify('optin', 'type=' + encodeURIComponent(optinType || ''));
    };
    window.stash_sdk.openExternalBrowser = function(url) {
      notify('externalBrowser', 'url=' + encodeURIComponent(url || ''));
    };
    window.stash_sdk.expand = function() {
      notify('expand');
    };
    window.stash_sdk.collapse = function() {
      notify('collapse');
    };
    if (!window.__stashUnrealPreviewCloseWrapped) {
      window.__stashUnrealPreviewCloseWrapped = true;
      var originalClose = window.close;
      window.close = function() {
        notify('dismiss');
        if (typeof originalClose === 'function') {
          try { originalClose.apply(window, arguments); } catch (e) {}
        }
      };
    }
  }
  ensureStashSdkBridge();

  if (window.__stashUnrealPreviewInjected) { return; }
  window.__stashUnrealPreviewInjected = true;
})();
)JS");
	}
}

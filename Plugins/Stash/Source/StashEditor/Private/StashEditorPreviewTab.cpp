// Copyright Stash. All Rights Reserved.

#include "StashEditorPreviewTab.h"
#include "SStashPreviewPanel.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"

namespace StashEditorPreviewTabInternal
{
	static const FName TabId(TEXT("StashPreview"));
}

void FStashEditorPreviewTab::RegisterTabSpawner()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		StashEditorPreviewTabInternal::TabId,
		FOnSpawnTab::CreateStatic(&FStashEditorPreviewTab::SpawnPreviewTab))
		.SetDisplayName(NSLOCTEXT("StashEditor", "StashPreviewTabTitle", "Stash Preview"))
		.SetTooltipText(NSLOCTEXT("StashEditor", "StashPreviewTabTooltip", "Preview Stash card, modal, and browser flows in the editor."))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory());
}

void FStashEditorPreviewTab::UnregisterTabSpawner()
{
	if (FSlateApplication::IsInitialized())
	{
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(StashEditorPreviewTabInternal::TabId);
	}
}

void FStashEditorPreviewTab::OpenPreviewTab()
{
	FGlobalTabmanager::Get()->TryInvokeTab(StashEditorPreviewTabInternal::TabId);
}

TSharedRef<SDockTab> FStashEditorPreviewTab::SpawnPreviewTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		.Label(NSLOCTEXT("StashEditor", "StashPreviewTabLabel", "Stash Preview"))
		[
			SNew(SStashPreviewPanel)
		];
}

FName FStashEditorPreviewTab::GetMenuOwnerName()
{
	return FName(TEXT("StashEditorPreviewMenu"));
}

void FStashEditorPreviewTab::RegisterMenuEntry()
{
	UToolMenus* ToolMenus = UToolMenus::Get();
	if (!ToolMenus)
	{
		return;
	}
	// Scope the entry to a named owner so ShutdownModule can cleanly unregister it (hot-reload / live coding).
	FToolMenuOwnerScoped OwnerScoped(GetMenuOwnerName());
	UToolMenu* Menu = ToolMenus->ExtendMenu("LevelEditor.MainMenu.Window");
	if (!Menu)
	{
		return;
	}
	FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
	Section.AddMenuEntry(
		"OpenStashPreview",
		NSLOCTEXT("StashEditor", "OpenStashPreviewMenu", "Stash Preview"),
		NSLOCTEXT("StashEditor", "OpenStashPreviewMenuTooltip", "Open the Stash checkout preview panel."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateStatic(&FStashEditorPreviewTab::OpenPreviewTab)));
}

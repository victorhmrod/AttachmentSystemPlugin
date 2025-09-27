/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Howaajin. All rights reserved.
 *  Licensed under the MIT License. See License in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include "CoreMinimal.h"
#include "EdGraph/EdGraph.h"
#include "Editor.h"
#include "Formatter.h"
#include "FormatterCommands.h"
#include "FormatterSettings.h"
#include "FormatterStyle.h"
#include "ISettingsModule.h"
#include "ISettingsSection.h"
#include "Modules/ModuleManager.h"
#include "Framework/MultiBox/MultiBoxDefs.h"
#include "GraphEditorSettings.h"
#include "ScopedTransaction.h"
#include "FormatterLog.h"
#include "SGraphPanel.h"
#include "Widgets/Input/SSpinBox.h"

#define LOCTEXT_NAMESPACE "GraphFormatter"

class FFormatterModule : public IModuleInterface
{
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
    
    virtual void FormatGraphAutomated(TObjectPtr<UObject> Object);

    void HandleAssetEditorOpened(UObject* Object, IAssetEditorInstance* Instance);
    void ToggleStraightenConnections();
    bool IsStraightenConnectionsEnabled() const;
    SGraphEditor* FindEditorForObject(UObject* Object) const;
    FDelegateHandle GraphEditorDelegateHandle;
    TArray<TSharedPtr<EGraphFormatterPositioningAlgorithm>> AlgorithmOptions;
    TSharedPtr<FExtender> ToolbarExtender;
public:
    void FillToolbar(FToolBarBuilder& ToolbarBuilder);
    void ExtendToolBar(IAssetEditorInstance* Instance);
    void MapCommands(UObject* Object, IAssetEditorInstance* Instance);
    void OnGraphEditorDetected(UObject* Object, IAssetEditorInstance* Instance);
};

class FAssetEditorInstance
{
public:
    static inline TMap<UObject*, FAssetEditorInstance*> Instances;
    FFormatterModule* Module;
    IAssetEditorInstance* Instance;
    UObject* Object;
    void HandleEditorWidgetCreated(UObject* Object);
};

IMPLEMENT_MODULE(FFormatterModule, GraphFormatter)

void FFormatterModule::StartupModule()
{
    FFormatterStyle::Initialize();

    ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
    if (SettingsModule != nullptr)
    {
        ISettingsSectionPtr SettingsSection = SettingsModule->RegisterSettings(
            "Editor",
            "Plugins",
            "GraphFormatter",
            LOCTEXT("GraphFormatterSettingsName", "Graph Formatter"),
            LOCTEXT("GraphFormatterSettingsDescription", "Configure the Graph Formatter plug-in."),
            GetMutableDefault<UFormatterSettings>()
        );
    }

    FFormatterCommands::Register();
    if(GEditor)
    {
        GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OnAssetOpenedInEditor().AddRaw(this, &FFormatterModule::HandleAssetEditorOpened);
    }
    AlgorithmOptions.Add(MakeShareable(new EGraphFormatterPositioningAlgorithm(EGraphFormatterPositioningAlgorithm::EEvenlyInLayer)));
    AlgorithmOptions.Add(MakeShareable(new EGraphFormatterPositioningAlgorithm(EGraphFormatterPositioningAlgorithm::EFastAndSimpleMethodTop)));
    AlgorithmOptions.Add(MakeShareable(new EGraphFormatterPositioningAlgorithm(EGraphFormatterPositioningAlgorithm::EFastAndSimpleMethodMedian)));
    AlgorithmOptions.Add(MakeShareable(new EGraphFormatterPositioningAlgorithm(EGraphFormatterPositioningAlgorithm::ELayerSweep)));
}

void FFormatterModule::ExtendToolBar(IAssetEditorInstance* Instance)
{
    const UFormatterSettings* Settings = GetDefault<UFormatterSettings>();

    if (!Settings->DisableToolbar)
    {
        FAssetEditorToolkit* AssetEditorToolkit = StaticCast<FAssetEditorToolkit*>(Instance);
        TSharedRef<FUICommandList> ToolkitCommands = AssetEditorToolkit->GetToolkitCommands();
        if (!ToolbarExtender.IsValid())
        {
            ToolbarExtender = MakeShareable(new FExtender);
            ToolbarExtender->AddToolBarExtension(
                "Asset",
                EExtensionHook::After,
                ToolkitCommands,
                FToolBarExtensionDelegate::CreateRaw(this, &FFormatterModule::FillToolbar)
            );
        }
        AssetEditorToolkit->AddToolbarExtender(ToolbarExtender);
    }
}

void FFormatterModule::MapCommands(UObject* Object, IAssetEditorInstance* Instance)
{
    FAssetEditorToolkit* AssetEditorToolkit = StaticCast<FAssetEditorToolkit*>(Instance);

    const FFormatterCommands& Commands = FFormatterCommands::Get();
    TSharedRef<FUICommandList> ToolkitCommands = AssetEditorToolkit->GetToolkitCommands();
    if (ToolkitCommands->IsActionMapped(Commands.FormatGraph))
    {
        return;
    }
    ToolkitCommands->MapAction(
        Commands.FormatGraph,
        FExecuteAction::CreateLambda([this, Object, Instance]
        {
            if (SGraphEditor* Editor = FindEditorForObject(Object))
            {
                const UFormatterSettings* Settings = GetDefault<UFormatterSettings>();
                if (Settings->AutoDetectGraphEditor)
                {
                    OnGraphEditorDetected(Object, Instance);
                }
                FFormatter::Instance().SetCurrentEditor(Editor, Object);
                FFormatter::Instance().Format();
            }
        }),
        FCanExecuteAction()
    );
    ToolkitCommands->MapAction(
        Commands.PlaceBlock,
        FExecuteAction::CreateLambda([this, Object, Instance]
        {
            if (SGraphEditor* Editor = FindEditorForObject(Object))
            {
                const UFormatterSettings* Settings = GetDefault<UFormatterSettings>();
                if (Settings->AutoDetectGraphEditor)
                {
                    OnGraphEditorDetected(Object, Instance);
                }
                FFormatter::Instance().SetCurrentEditor(Editor, Object);
                FFormatter::Instance().PlaceBlock();
            }
        }),
        FCanExecuteAction()
    );
    ToolkitCommands->MapAction(
        Commands.StraightenConnections,
        FExecuteAction::CreateRaw(this, &FFormatterModule::ToggleStraightenConnections),
        FCanExecuteAction(),
        FIsActionChecked::CreateRaw(this, &FFormatterModule::IsStraightenConnectionsEnabled)
    );
}

void FFormatterModule::HandleAssetEditorOpened(UObject* Object, IAssetEditorInstance* Instance)
{
    UE_LOG(LogGraphFormatter, Log, TEXT("AssetEditorOpened for: %s"), *Object->GetClass()->GetName());
    if (!GEditor)
    {
        return;
    }
    const UFormatterSettings* Settings = GetDefault<UFormatterSettings>();
    if (FFormatter::Instance().IsAssetSupported(Object))
    {
        MapCommands(Object, Instance);
        ExtendToolBar(Instance);
    }
    else
    {
        if (Settings->AutoDetectGraphEditor)
        {
            MapCommands(Object, Instance);
            if (!FAssetEditorInstance::Instances.Contains(Object))
            {
                auto EditorInstance = new FAssetEditorInstance{this, Instance, Object};
                GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OnAssetEditorOpened().AddRaw(EditorInstance, &FAssetEditorInstance::HandleEditorWidgetCreated);
                FAssetEditorInstance::Instances.Add(Object, EditorInstance);
            }
            else
            {
                auto EditorInstance = FAssetEditorInstance::Instances[Object];
                EditorInstance->Instance = Instance;
            }
        }
    }
}

static FText GetEnumAsString(EGraphFormatterPositioningAlgorithm EnumValue)
{
    switch (EnumValue)
    {
    case EGraphFormatterPositioningAlgorithm::EEvenlyInLayer:
        return FText::FromString("Place node evenly in layer");
    case EGraphFormatterPositioningAlgorithm::EFastAndSimpleMethodTop:
        return FText::FromString("FAS Top");
    case EGraphFormatterPositioningAlgorithm::EFastAndSimpleMethodMedian:
        return FText::FromString("FAS Median");
    case EGraphFormatterPositioningAlgorithm::ELayerSweep:
        return FText::FromString("Layer sweep");
    default:
        return FText::FromString("Invalid");
    }
}

void FFormatterModule::FillToolbar(FToolBarBuilder& ToolbarBuilder)
{
    const UFormatterSettings* Settings = GetDefault<UFormatterSettings>();
    UFormatterSettings* MutableSettings = GetMutableDefault<UFormatterSettings>();
    const FFormatterCommands& Commands = FFormatterCommands::Get();
    // clang-format off
    ToolbarBuilder.BeginSection("GraphFormatter");
    {
        TSharedPtr<SComboBox<TSharedPtr<EGraphFormatterPositioningAlgorithm>>> AlgorithmComboBox;
        int32 SelectedAlgorithmIndex = AlgorithmOptions.IndexOfByPredicate([Settings](const TSharedPtr<EGraphFormatterPositioningAlgorithm>& Option)
        {
            return *Option == Settings->PositioningAlgorithm;
        });
        ToolbarBuilder.AddToolBarButton(
            Commands.FormatGraph,
            NAME_None,
            TAttribute<FText>(),
            TAttribute<FText>(),
            TAttribute<FSlateIcon>(FSlateIcon(FFormatterStyle::Get()->GetStyleSetName(), "GraphFormatter.ApplyIcon")),
            FName(TEXT("GraphFormatter"))
        );
        auto HorizontalEditArea = SNew(SHorizontalBox) + SHorizontalBox::Slot().Padding(2.0f)
        [
            SNew(SSpinBox<int32>)
            .MinValue(0)
            .MaxValue(1000)
            .MinSliderValue(0)
            .MaxSliderValue(1000)
            .ToolTipText(LOCTEXT("GraphFormatterHorizontalSpacingiToolTips", "Spacing between two layers."))
            .Value(Settings->HorizontalSpacing)
            .MinDesiredWidth(80)
            .OnValueCommitted_Lambda([MutableSettings](int32 Number, ETextCommit::Type CommitInfo)
            {
                MutableSettings->HorizontalSpacing = Number; MutableSettings->PostEditChange(); MutableSettings->SaveConfig();
            })
        ]
        +SHorizontalBox::Slot()
        .Padding(2.0f)
        [
            SAssignNew(AlgorithmComboBox, SComboBox<TSharedPtr<EGraphFormatterPositioningAlgorithm>>)
            .ContentPadding(FMargin(6.0f, 2.0f))
            .OptionsSource(&AlgorithmOptions)
            .ToolTipText_Lambda([]() { return FText::FromString("Positioning Algorithm"); })
            .InitiallySelectedItem(AlgorithmOptions[SelectedAlgorithmIndex])
            .OnSelectionChanged_Lambda([=](TSharedPtr<EGraphFormatterPositioningAlgorithm> NewOption, ESelectInfo::Type SelectInfo)
            {
                if (SelectInfo != ESelectInfo::Direct)
                {
                    MutableSettings->PositioningAlgorithm = *NewOption;
                    MutableSettings->PostEditChange();
                    MutableSettings->SaveConfig();
                }
            })
            .OnGenerateWidget_Lambda([](TSharedPtr<EGraphFormatterPositioningAlgorithm> AlgorithmEnum)
            {
                return SNew(STextBlock).Text(GetEnumAsString(*AlgorithmEnum.Get()));
            })
            [
                SNew(STextBlock)
                .Text_Lambda([Settings]() { return GetEnumAsString(Settings->PositioningAlgorithm); })
            ]
        ];

        auto VerticalEditArea = SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .Padding(2.0f)
        [
            SNew(SSpinBox<int32>)
            .MinValue(0)
            .MaxValue(1000)
            .MinSliderValue(0)
            .MaxSliderValue(1000)
            .ToolTipText(LOCTEXT("GraphFormatterHorizontalSpacingiToolTips", "Spacing between two layers."))
            .Value(Settings->HorizontalSpacing)
            .MinDesiredWidth(80)
            .OnValueCommitted_Lambda([MutableSettings](int32 Number, ETextCommit::Type CommitInfo)
            {
                MutableSettings->HorizontalSpacing = Number;
                MutableSettings->PostEditChange();
                MutableSettings->SaveConfig();
            })
        ]
        +SVerticalBox::Slot()
        .Padding(2.0f)
        [
            SAssignNew(AlgorithmComboBox, SComboBox<TSharedPtr<EGraphFormatterPositioningAlgorithm>>)
            .ContentPadding(FMargin(6.0f, 2.0f))
            .OptionsSource(&AlgorithmOptions)
            .ToolTipText_Lambda([]()
            {
                return FText::FromString("Positioning Algorithm");
            })
            .InitiallySelectedItem(AlgorithmOptions[SelectedAlgorithmIndex])
            .OnSelectionChanged_Lambda([=](TSharedPtr<EGraphFormatterPositioningAlgorithm> NewOption, ESelectInfo::Type SelectInfo)
            {
                if (SelectInfo != ESelectInfo::Direct)
                {
                    MutableSettings->PositioningAlgorithm = *NewOption;
                    MutableSettings->PostEditChange();
                    MutableSettings->SaveConfig();
                }
            })
            .OnGenerateWidget_Lambda([](TSharedPtr<EGraphFormatterPositioningAlgorithm> AlgorithmEnum)
            {
                return SNew(STextBlock).Text(GetEnumAsString(*AlgorithmEnum.Get()));
            })
            [
                SNew(STextBlock)
                .Text_Lambda([Settings]() { return GetEnumAsString(Settings->PositioningAlgorithm); })
            ]
        ];
        ToolbarBuilder.AddWidget(HorizontalEditArea);
        //ToolbarBuilder.AddWidget(VerticalEditArea);
        ToolbarBuilder.AddToolBarButton(
            Commands.StraightenConnections,
            NAME_None,
            TAttribute<FText>(),
            TAttribute<FText>(),
            TAttribute<FSlateIcon>(FSlateIcon(FFormatterStyle::Get()->GetStyleSetName(), "GraphFormatter.StraightenIcon")),
            FName(TEXT("GraphFormatter"))
        );
    }
    // clang-format on
    ToolbarBuilder.EndSection();
}

void FFormatterModule::ToggleStraightenConnections()
{
    auto GraphEditorSettings = GetMutableDefault<UGraphEditorSettings>();

    if (IsStraightenConnectionsEnabled())
    {
        const UFormatterSettings* Settings = GetDefault<UFormatterSettings>();
        GraphEditorSettings->ForwardSplineTangentFromHorizontalDelta = Settings->ForwardSplineTangentFromHorizontalDelta;
        GraphEditorSettings->ForwardSplineTangentFromVerticalDelta = Settings->ForwardSplineTangentFromVerticalDelta;
        GraphEditorSettings->BackwardSplineTangentFromHorizontalDelta = Settings->BackwardSplineTangentFromHorizontalDelta;
        GraphEditorSettings->BackwardSplineTangentFromVerticalDelta = Settings->BackwardSplineTangentFromVerticalDelta;
        GraphEditorSettings->PostEditChange();
        GraphEditorSettings->SaveConfig();
    }
    else
    {
        UFormatterSettings* Settings = GetMutableDefault<UFormatterSettings>();
        Settings->ForwardSplineTangentFromHorizontalDelta = GraphEditorSettings->ForwardSplineTangentFromHorizontalDelta;
        Settings->ForwardSplineTangentFromVerticalDelta = GraphEditorSettings->ForwardSplineTangentFromVerticalDelta;
        Settings->BackwardSplineTangentFromHorizontalDelta = GraphEditorSettings->BackwardSplineTangentFromHorizontalDelta;
        Settings->BackwardSplineTangentFromVerticalDelta = GraphEditorSettings->BackwardSplineTangentFromVerticalDelta;
        Settings->PostEditChange();
        Settings->SaveConfig();
        GraphEditorSettings->ForwardSplineTangentFromHorizontalDelta = FVector2D(0, 0);
        GraphEditorSettings->ForwardSplineTangentFromVerticalDelta = FVector2D(0, 0);
        GraphEditorSettings->BackwardSplineTangentFromHorizontalDelta = FVector2D(0, 0);
        GraphEditorSettings->BackwardSplineTangentFromVerticalDelta = FVector2D(0, 0);
        GraphEditorSettings->PostEditChange();
        GraphEditorSettings->SaveConfig();
    }
}

bool FFormatterModule::IsStraightenConnectionsEnabled() const
{
    auto GraphEditorSettings = GetDefault<UGraphEditorSettings>();
    if (GraphEditorSettings->ForwardSplineTangentFromHorizontalDelta == FVector2D(0, 0) &&
        GraphEditorSettings->ForwardSplineTangentFromVerticalDelta == FVector2D(0, 0) &&
        GraphEditorSettings->BackwardSplineTangentFromHorizontalDelta == FVector2D(0, 0) &&
        GraphEditorSettings->BackwardSplineTangentFromVerticalDelta == FVector2D(0, 0))
    {
        return true;
    }
    return false;
}

SGraphEditor* FFormatterModule::FindEditorForObject(UObject* Object) const
{
    SGraphEditor* Editor = FFormatter::Instance().FindGraphEditorByCursor();
    if (!Editor)
    {
        return FFormatter::Instance().FindGraphEditorForTopLevelWindow();
    }
    return Editor;
}

void FFormatterModule::OnGraphEditorDetected(UObject* Object, IAssetEditorInstance* Instance)
{
    UFormatterSettings* MutableSettings = GetMutableDefault<UFormatterSettings>();
    if(!MutableSettings->SupportedAssetTypes.Contains(Object->GetClass()->GetName()))
    {
        MutableSettings->SupportedAssetTypes.Add(Object->GetClass()->GetName(), true);
        MutableSettings->PostEditChange();
        MutableSettings->SaveConfig();
        FAssetEditorToolkit* AssetEditorToolkit = StaticCast<FAssetEditorToolkit*>(Instance);
        ExtendToolBar(Instance);
        AssetEditorToolkit->RegenerateMenusAndToolbars();
    }
}

void FAssetEditorInstance::HandleEditorWidgetCreated(UObject* InObject)
{
    const UFormatterSettings* Settings = GetDefault<UFormatterSettings>();
    if (Settings->AutoDetectGraphEditor)
    {
        if (auto Editor = FFormatter::Instance().FindGraphEditorForTopLevelWindow())
        {
            Module->MapCommands(Object, Instance);
            Module->OnGraphEditorDetected(Object, Instance);
        }
    }
    if (GEditor)
    {
        GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OnAssetEditorOpened().RemoveAll(this);
    }
    Instances.Remove(Object);
    delete this;
}

void FFormatterModule::ShutdownModule()
{
    ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
    if (SettingsModule != nullptr)
    {
        SettingsModule->UnregisterSettings("Editor", "Plugins", "GraphFormatter");
    }
    if (GEditor)
    {
        GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OnAssetOpenedInEditor().RemoveAll(this);
    }
    FFormatterStyle::Shutdown();
}

void FFormatterModule::FormatGraphAutomated(const TObjectPtr<UObject> Object)
{
	const auto AssetEditor = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	
	AssetEditor->OpenEditorForAsset(Object);

	const auto EditorInstance = AssetEditor->FindEditorForAsset(Object.Get(), false);
	
	if(!EditorInstance)
	{
		return;
	}

	if (SGraphEditor* Editor = FindEditorForObject(Object.Get()))
	{
		Editor->GetGraphPanel()->Update();

		FFormatter::Instance().SetCurrentEditor(Editor, Object.Get());
		FFormatter::Instance().Format();
	}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3 
	EditorInstance->CloseWindow(EAssetEditorCloseReason::AssetEditorHostClosed);
#else
    EditorInstance->CloseWindow();
#endif
    
}

#undef LOCTEXT_NAMESPACE

/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Howaajin. All rights reserved.
 *  Licensed under the MIT License. See License in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include "Formatter.h"
#include "FormatterCommands.h"
#include "FormatterGraph.h"
#include "FormatterSettings.h"
#include "UEGraphAdapter.h"
#include "FormatterLog.h"

#include "BehaviorTree/BehaviorTree.h"
#include "EdGraphNode_Comment.h"
#include "Math/Ray.h"
#include "SGraphNodeComment.h"
#include "SGraphPanel.h"

#include "graph_layout/graph_layout.h"
using namespace graph_layout;

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >=  6
typedef FVector2f FCompatibleVector2;
typedef FDeprecateSlateVector2D FCompatibleDeprecateSlateVector2D;
#else
typedef FVector2D FCompatibleVector2;
typedef FVector2D FCompatibleDeprecateSlateVector2D;
#endif
static inline FCompatibleVector2 CompatibleVector2(FVector2D const& V)
{
	return FCompatibleVector2((float)V.X, (float)V.Y);
}
#define GRAPH_FORMATTER_HACK

/** Hack start. Access to private member legally. */
#ifdef GRAPH_FORMATTER_HACK
#include "PrivateAccessor.h"

DECLARE_PRIVATE_MEMBER_ACCESSOR(FAccess_SGraphPanel_ZoomLevels, SNodePanel, TUniquePtr<FZoomLevelsContainer>, ZoomLevels)
DECLARE_PRIVATE_MEMBER_ACCESSOR(FAccess_SGraphNodeResizable_UserSize, SGraphNodeResizable, FCompatibleDeprecateSlateVector2D, UserSize);
DECLARE_PRIVATE_MEMBER_ACCESSOR(FAccess_SNodePanel_CurrentLOD, SNodePanel, EGraphRenderingLOD::Type, CurrentLOD);

static TUniquePtr<FZoomLevelsContainer> TopZoomLevels;
static TUniquePtr<FZoomLevelsContainer> TempZoomLevels;
static EGraphRenderingLOD::Type OldLOD;

class FTopZoomLevelContainer : public FZoomLevelsContainer
{
public:
    virtual float GetZoomAmount(int32 InZoomLevel) const override { return 1.0f; }
    virtual int32 GetNearestZoomLevel(float InZoomAmount) const override { return 0; }
    virtual FText GetZoomText(int32 InZoomLevel) const override { return FText::FromString(TEXT("1:1")); }
    virtual int32 GetNumZoomLevels() const override { return 1; }
    virtual int32 GetDefaultZoomLevel() const override { return 0; }
    virtual EGraphRenderingLOD::Type GetLOD(int32 InZoomLevel) const override { return EGraphRenderingLOD::DefaultDetail; }
};

// Tick all nodes manually
static void TickWidgetRecursively(SWidget* Widget)
{
    Widget->GetChildren();
    if (auto Children = Widget->GetChildren())
    {
        for (int32 ChildIndex = 0; ChildIndex < Children->Num(); ++ChildIndex)
        {
            auto ChildWidget = &Children->GetChildAt(ChildIndex).Get();
            TickWidgetRecursively(ChildWidget);
        }
    }
    Widget->Tick(Widget->GetCachedGeometry(), FSlateApplication::Get().GetCurrentTime(), 0);
}

// Set the GraphPanel ZoomLevel to 1:1 before formatting the graph.
// From 1:1 scale to -5 scale, the result will be consistent but
// can see slight variation due to the round-off error of floating point number
void FFormatter::SetZoomLevelTo11Scale() const
{
    if (!TopZoomLevels.IsValid())
    {
        TopZoomLevels = MakeUnique<FTopZoomLevelContainer>();
    }
    auto& ZoomLevels = CurrentPanel->*FPrivateAccessor<FAccess_SGraphPanel_ZoomLevels>::Member;
    TempZoomLevels = MoveTemp(ZoomLevels);
    ZoomLevels = MoveTemp(TopZoomLevels);
    OldLOD = CurrentPanel->GetCurrentLOD();
    CurrentPanel->*FPrivateAccessor<FAccess_SNodePanel_CurrentLOD>::Member = EGraphRenderingLOD::DefaultDetail;
    auto Nodes = GetAllNodes();
    for (auto Node : Nodes)
    {
        auto GraphNode = GetWidget(Node);
        if (GraphNode != nullptr)
        {
            TickWidgetRecursively(GraphNode);
        }
    }
    CurrentPanel->SlatePrepass();
}

void FFormatter::RestoreZoomLevel() const
{
    if (!TopZoomLevels.IsValid())
    {
        TopZoomLevels = MakeUnique<FTopZoomLevelContainer>();
    }
    auto& ZoomLevels = CurrentPanel->*FPrivateAccessor<FAccess_SGraphPanel_ZoomLevels>::Member;
    ZoomLevels = MoveTemp(TempZoomLevels);
    CurrentPanel->*FPrivateAccessor<FAccess_SNodePanel_CurrentLOD>::Member = OldLOD;
}

#endif
/** Hack end  */

void FFormatter::SetCurrentEditor(SGraphEditor* Editor, UObject* Object)
{
    CurrentEditor = Editor;
    IsVerticalLayout = false;
    IsBehaviorTree = false;
    IsBlueprint = false;
    if (Cast<UBehaviorTree>(Object))
    {
        IsVerticalLayout = true;
        IsBehaviorTree = true;
    }
    if (Cast<UBlueprint>(Object))
    {
        IsBlueprint = true;
    }
    if (Object->GetClass()->GetName() == "NiagaraSystem")
    {
        IsVerticalLayout = true;
    }
}

bool FFormatter::IsAssetSupported(const UObject* Object)
{
    const UFormatterSettings* Settings = GetDefault<UFormatterSettings>();
    if (const bool* Enabled = Settings->SupportedAssetTypes.Find(Object->GetClass()->GetName()))
    {
        return Enabled != nullptr && *Enabled;
    }
    return false;
}

/** Matches widgets by type */
struct FWidgetTypeMatcher
{
    FWidgetTypeMatcher(const FName& InType)
        : TypeName(InType)
    {
    }

    bool IsMatch(const TSharedRef<const SWidget>& InWidget) const
    {
        return TypeName == InWidget->GetType();
    }

    const FName& TypeName;
};

SGraphEditor* FFormatter::FindGraphEditorForTopLevelWindow() const
{
    FSlateApplication& Application = FSlateApplication::Get();
    auto ActiveWindow = Application.GetActiveTopLevelWindow();
    if (!ActiveWindow.IsValid())
    {
        return nullptr;
    }
    FGeometry InnerWindowGeometry = ActiveWindow->GetWindowGeometryInWindow();
    FArrangedChildren JustWindow(EVisibility::Visible);
    JustWindow.AddWidget(FArrangedWidget(ActiveWindow.ToSharedRef(), InnerWindowGeometry));

    FWidgetPath WidgetPath(ActiveWindow.ToSharedRef(), JustWindow);
    if (WidgetPath.ExtendPathTo(FWidgetTypeMatcher("SGraphEditor"), EVisibility::Visible))
    {
        return StaticCast<SGraphEditor*>(&WidgetPath.GetLastWidget().Get());
    }
    return nullptr;
}

SGraphEditor* FFormatter::FindGraphEditorByCursor() const
{
    FSlateApplication& Application = FSlateApplication::Get();
    FWidgetPath WidgetPath = Application.LocateWindowUnderMouse(Application.GetCursorPos(), Application.GetInteractiveTopLevelWindows());
    for (int i = WidgetPath.Widgets.Num() - 1; i >= 0; i--)
    {
        if (WidgetPath.Widgets[i].Widget->GetTypeAsString() == "SGraphEditor")
        {
            return StaticCast<SGraphEditor*>(&WidgetPath.Widgets[i].Widget.Get());
        }
    }
    return nullptr;
}

SGraphPanel* FFormatter::GetCurrentPanel() const
{
    return CurrentEditor->GetGraphPanel();
}

SGraphNode* FFormatter::GetWidget(const UEdGraphNode* Node) const
{
    SGraphPanel* GraphPanel = GetCurrentPanel();
    if (GraphPanel != nullptr)
    {
        TSharedPtr<SGraphNode> NodeWidget = GraphPanel->GetNodeWidgetFromGuid(Node->NodeGuid);
        return NodeWidget.Get();
    }
    return nullptr;
}

TSet<UEdGraphNode*> FFormatter::GetAllNodes() const
{
    TSet<UEdGraphNode*> Nodes;
    if (CurrentEditor)
    {
        for (UEdGraphNode* Node : CurrentEditor->GetCurrentGraph()->Nodes)
        {
            Nodes.Add(Node);
        }
    }
    return Nodes;
}

float FFormatter::GetCommentNodeTitleHeight(const UEdGraphNode* Node) const
{
    /** Titlebar Offset - taken from SGraphNodeComment.cpp */
    static const FSlateRect TitleBarOffset(13, 8, -3, 0);

    SGraphNode* CommentNode = GetWidget(Node);
    if (CommentNode)
    {
        SGraphNodeComment* NodeWidget = StaticCast<SGraphNodeComment*>(CommentNode);
        FSlateRect Rect = NodeWidget->GetTitleRect();
        return Rect.GetSize().Y + TitleBarOffset.Top;
    }
    return 0;
}

FVector2D FFormatter::GetNodeSize(const UEdGraphNode* Node) const
{
    auto GraphNode = GetWidget(Node);
    if (GraphNode != nullptr)
    {
        return GraphNode->GetDesiredSize();
    }
    return FVector2D(Node->NodeWidth, Node->NodeHeight);
}

FVector2D FFormatter::GetNodePosition(const UEdGraphNode* Node) const
{
    auto GraphNode = GetWidget(Node);
    if (GraphNode != nullptr)
    {
        return GraphNode->GetPosition();
    }
    return FVector2D();
}

FVector2D FFormatter::GetPinOffset(const UEdGraphPin* Pin) const
{
    auto GraphNode = GetWidget(Pin->GetOwningNodeUnchecked());
    if (GraphNode != nullptr)
    {
        auto PinWidget = GraphNode->FindWidgetForPin(const_cast<UEdGraphPin*>(Pin));
        if (PinWidget.IsValid())
        {
            auto Offset = PinWidget->GetNodeOffset();
            return Offset;
        }
    }
    return FVector2D::ZeroVector;
}

FSlateRect FFormatter::GetNodesBound(const TSet<UEdGraphNode*> Nodes) const
{
    FSlateRect Bound;
    for (auto Node : Nodes)
    {
        FVector2D Pos = GetNodePosition(Node);
        FVector2D Size = GetNodeSize(Node);
        FSlateRect NodeBound = FSlateRect::FromPointAndExtent(Pos, Size);
        Bound = Bound.IsValid() ? Bound.Expand(NodeBound) : NodeBound;
    }
    return Bound;
}

bool FFormatter::PreCommand()
{
    if (!CurrentEditor)
    {
        return false;
    }
    CurrentGraph = CurrentEditor->GetCurrentGraph();
    if (!CurrentGraph)
    {
        return false;
    }
    CurrentPanel = GetCurrentPanel();
    if (!CurrentPanel)
    {
        return false;
    }

#ifdef GRAPH_FORMATTER_HACK
    SetZoomLevelTo11Scale();
#endif

    const UFormatterSettings* Settings = GetDefault<UFormatterSettings>();
    FFormatterGraph::HorizontalSpacing = Settings->HorizontalSpacing;
    FFormatterGraph::VerticalSpacing = Settings->VerticalSpacing;
    FFormatterGraph::SpacingFactorOfGroup = Settings->SpacingFactorOfParameterGroup;
    FFormatterGraph::MaxLayerNodes = Settings->MaxLayerNodes;
    FFormatterGraph::MaxOrderingIterations = Settings->MaxOrderingIterations;
    FFormatterGraph::PositioningAlgorithm = Settings->PositioningAlgorithm;
    return true;
}

void FFormatter::PostCommand()
{
#ifdef GRAPH_FORMATTER_HACK
    RestoreZoomLevel();
#endif
}

void FFormatter::Translate(TSet<UEdGraphNode*> Nodes, FVector2D Offset) const
{
    UEdGraph* Graph = CurrentEditor->GetCurrentGraph();
    if (!Graph || !CurrentEditor)
    {
        return;
    }
    if (Offset.X == 0 && Offset.Y == 0)
    {
        return;
    }
    for (auto Node : Nodes)
    {
        auto WidgetNode = GetWidget(Node);
        SGraphPanel::SNode::FNodeSet Filter;
        WidgetNode->MoveTo(CompatibleVector2(WidgetNode->GetPosition() + Offset) , Filter, true);
    }
}

static TSet<UEdGraphNode*> GetSelectedNodes(const SGraphEditor* GraphEditor)
{
    TSet<UEdGraphNode*> SelectedGraphNodes;
    TSet<UObject*> SelectedNodes = GraphEditor->GetSelectedNodes();
    for (UObject* Node : SelectedNodes)
    {
        UEdGraphNode* GraphNode = Cast<UEdGraphNode>(Node);
        if (GraphNode)
        {
            SelectedGraphNodes.Add(GraphNode);
        }
    }
    return SelectedGraphNodes;
}

static bool IsNodeUnderRect(const TSharedRef<SGraphNode> InNodeWidget, const FSlateRect& Rect)
{
    const FVector2D NodePosition = Rect.GetTopLeft();
    const FVector2D NodeSize = Rect.GetSize();
    const FSlateRect CommentRect(NodePosition.X, NodePosition.Y, NodePosition.X + NodeSize.X, NodePosition.Y + NodeSize.Y);

    const FVector2D InNodePosition = InNodeWidget->GetPosition();
    const FVector2D InNodeSize = InNodeWidget->GetDesiredSize();

    const FSlateRect NodeGeometryGraphSpace(InNodePosition.X, InNodePosition.Y, InNodePosition.X + InNodeSize.X, InNodePosition.Y + InNodeSize.Y);
    const FVector2D CommentRectSize = CommentRect.GetSize();
    const FVector2D Center = NodeGeometryGraphSpace.GetCenter();
    return CommentRect.ContainsPoint(Center) && CommentRectSize.X > InNodeSize.X && CommentRectSize.Y > InNodeSize.Y;
}

TSet<UEdGraphNode*> FFormatter::GetNodesUnderComment(const UEdGraphNode_Comment* CommentNode) const
{
    TSet<UEdGraphNode*> Result;
    if (IsAutoSizeComment)
    {
        auto NodesUnderComment = CommentNode->GetNodesUnderComment();
        for (auto Object : NodesUnderComment)
        {
            UEdGraphNode* Node = StaticCast<UEdGraphNode*>(Object);
            Result.Add(Node);
        }
        return Result;
    }
    SGraphNode* CommentNodeWidget = GetWidget(CommentNode);
    auto CommentSize = CommentNodeWidget->GetDesiredSize();
    if (CommentSize.IsZero())
    {
        return TSet<UEdGraphNode*>();
    }
    SGraphPanel* Panel = GetCurrentPanel();
    FChildren* PanelChildren = Panel->GetAllChildren();
    int32 NumChildren = PanelChildren->Num();
    FVector2D CommentNodePosition = CommentNodeWidget->GetPosition();
    FSlateRect CommentRect = FSlateRect(CommentNodePosition, CommentNodePosition + CommentSize);
    for (int32 NodeIndex = 0; NodeIndex < NumChildren; ++NodeIndex)
    {
        const TSharedRef<SGraphNode> SomeNodeWidget = StaticCastSharedRef<SGraphNode>(PanelChildren->GetChildAt(NodeIndex));
        UObject* GraphObject = SomeNodeWidget->GetObjectBeingDisplayed();
        if (GraphObject != CommentNode)
        {
            if (IsNodeUnderRect(SomeNodeWidget, CommentRect))
            {
                Result.Add(Cast<UEdGraphNode>(GraphObject));
            }
        }
    }
    return Result;
}

static TMap<UEdGraphNode*, UEdGraphNode_Comment*> NodeUnderCommentHash()
{
    TMap<UEdGraphNode*, UEdGraphNode_Comment*> Result;
    TSet<UEdGraphNode*> Nodes = FFormatter::Instance().GetAllNodes();
    while (true)
    {
        TArray<UEdGraphNode_Comment*> SortedCommentNodes = UEGraphAdapter::GetSortedCommentNodes(Nodes);
        if (SortedCommentNodes.Num() != 0)
        {
            // Topmost comment node has smallest negative depth value
            const int32 Depth = SortedCommentNodes[0]->CommentDepth;

            // Collapse all topmost comment nodes into virtual nodes.
            for (auto CommentNode : SortedCommentNodes)
            {
                if (CommentNode->CommentDepth == Depth)
                {
                    auto NodesUnderComment = FFormatter::Instance().GetNodesUnderComment(Cast<UEdGraphNode_Comment>(CommentNode));
                    NodesUnderComment = Nodes.Intersect(NodesUnderComment);
                    Nodes = Nodes.Difference(NodesUnderComment);
                    Nodes.Remove(CommentNode);
                    for (auto Node : NodesUnderComment)
                    {
                        Result.Add(Node, CommentNode);
                    }
                }
                else
                {
                    break;
                }
            }
        }
        else
        {
            break;
        }
    }
    return Result;
}

// Get all nodes connected to InNode, and if a node is under a Comment node, consider it connected to all nodes under that Comment node.
static TSet<UEdGraphNode*> GetLinkedNodes(UEdGraphNode* InNode)
{
    auto NodeToCommentMap = NodeUnderCommentHash();

    TSet<UEdGraphNode*> VisitedNodes;
    TQueue<UEdGraphNode*> PendingNodes;
    PendingNodes.Enqueue(InNode);
    while (!PendingNodes.IsEmpty())
    {
        UEdGraphNode* Node;
        PendingNodes.Dequeue(Node);
        VisitedNodes.Add(Node);
        for (const UEdGraphPin* Pin : Node->Pins)
        {
            for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
            {
                UEdGraphNode* LinkedNode = LinkedPin->GetOwningNodeUnchecked();
                if (!VisitedNodes.Contains(LinkedNode))
                {
                    if (NodeToCommentMap.Contains(LinkedNode))
                    {
                        UEdGraphNode_Comment* Comment = NodeToCommentMap[LinkedNode];
                        if (!VisitedNodes.Contains(Comment))
                        {
                            VisitedNodes.Add(Comment);
                            auto NodesUnderComment = FFormatter::Instance().GetNodesUnderComment(Comment);
                            for (auto NodeUnderComment : NodesUnderComment)
                            {
                                if (!VisitedNodes.Contains(NodeUnderComment))
                                {
                                    PendingNodes.Enqueue(NodeUnderComment);
                                }
                            }
                        }
                    }
                    else
                    {
                        PendingNodes.Enqueue(LinkedNode);
                    }
                }
            }
        }
    }
    return VisitedNodes;
}

static TSet<UEdGraphNode*> SingleNodeSelectionStrategy(UEdGraphNode* InNode)
{
    return GetLinkedNodes(InNode);
}

static TSet<UEdGraphNode*> DoSelectionStrategy(UEdGraph* Graph, TSet<UEdGraphNode*> Selected)
{
    if (Selected.Num() != 0)
    {
        if (Selected.Num() == 1)
        {
            for (UEdGraphNode* GraphNode : Selected)
            {
                if (!GraphNode->IsA(UEdGraphNode_Comment::StaticClass()))
                {
                    return SingleNodeSelectionStrategy(GraphNode);
                }
            }
        }
        TSet<UEdGraphNode*> SelectedGraphNodes;
        for (UEdGraphNode* GraphNode : Selected)
        {
            SelectedGraphNodes.Add(GraphNode);
            if (GraphNode->IsA(UEdGraphNode_Comment::StaticClass()))
            {
                UEdGraphNode_Comment* CommentNode = Cast<UEdGraphNode_Comment>(GraphNode);
                auto NodesInComment = FFormatter::Instance().GetNodesUnderComment(CommentNode);
                for (UObject* ObjectInComment : NodesInComment)
                {
                    UEdGraphNode* NodeInComment = Cast<UEdGraphNode>(ObjectInComment);
                    SelectedGraphNodes.Add(NodeInComment);
                }
            }
        }
        return SelectedGraphNodes;
    }
    TSet<UEdGraphNode*> SelectedGraphNodes;
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        SelectedGraphNodes.Add(Node);
    }
    return SelectedGraphNodes;
}

void FFormatter::Format()
{
    if (!PreCommand())
    {
        return;
    }
    auto SelectedNodes = GetSelectedNodes(CurrentEditor);
    SelectedNodes = DoSelectionStrategy(CurrentGraph, SelectedNodes);
    for (const auto SelectedNode : SelectedNodes)
    {
        CurrentEditor->SetNodeSelection(SelectedNode, true);
    }
    auto Graph = UEGraphAdapter::Build(SelectedNodes, IsVerticalLayout);
    if (IsBehaviorTree)
    {
        Graph->NodeComparer = [](const FFormatterNode& A, const FFormatterNode& B)
        {
            return A.GetPosition().X < B.GetPosition().X;
        };
    }
    Graph->Format();
    auto BoundMap = Graph->GetBoundMap();
    delete Graph;
    const FScopedTransaction Transaction(FFormatterCommands::Get().FormatGraph->GetLabel());
    for (auto [Node, Rect] : BoundMap)
    {
        UEdGraphNode* UEdNode = static_cast<UEdGraphNode*>(Node);
        auto WidgetNode = GetWidget(UEdNode);
        SGraphPanel::SNode::FNodeSet Filter;
        WidgetNode->MoveTo(CompatibleVector2(Rect.Min), Filter, true);
        if (UEdNode->IsA(UEdGraphNode_Comment::StaticClass()))
        {
            auto CommentNode = Cast<UEdGraphNode_Comment>(UEdNode);
            CommentNode->SetBounds(FSlateRect(Rect.Min, Rect.Max));

#ifdef GRAPH_FORMATTER_HACK
            if (auto NodeResizable = StaticCast<SGraphNodeResizable*>(WidgetNode))
            {
                NodeResizable->*FPrivateAccessor<FAccess_SGraphNodeResizable_UserSize>::Member = Rect.GetSize();
            }
#endif
        }
    }
    PostCommand();
}

void FFormatter::PlaceBlock()
{
    if (!PreCommand())
    {
        return;
    }
    auto SelectedNodes = GetSelectedNodes(CurrentEditor);
    auto ConnectedNodesLeft = UEGraphAdapter::GetNodesConnected(SelectedNodes, EFormatterPinDirection::In);
    FVector2D ConnectCenter;
    const UFormatterSettings& Settings = *GetDefault<UFormatterSettings>();
    const FScopedTransaction Transaction(FFormatterCommands::Get().PlaceBlock->GetLabel());
    if (UEGraphAdapter::GetNodesConnectCenter(SelectedNodes, ConnectCenter, EFormatterPinDirection::In))
    {
        auto Center = FVector(ConnectCenter.X, ConnectCenter.Y, 0);
        auto Direction = IsVerticalLayout ? FVector(0, 1, 0) : FVector(1, 0, 0);
        auto RightRay = FRay(Center, Direction, true);
        FSlateRect Bound = GetNodesBound(ConnectedNodesLeft);
        auto RightBound = IsVerticalLayout ? FVector(0, Bound.Bottom, 0) : FVector(Bound.Right, 0, 0);
        auto LinkedCenter3D = RightRay.PointAt(RightRay.GetParameter(RightBound));
        auto LinkedCenterTo = FVector2D(LinkedCenter3D) + (IsVerticalLayout ? FVector2D(0, Settings.HorizontalSpacing) : FVector2D(Settings.HorizontalSpacing, 0));
        UEGraphAdapter::GetNodesConnectCenter(SelectedNodes, ConnectCenter, EFormatterPinDirection::In, true);
        Center = FVector(ConnectCenter.X, ConnectCenter.Y, 0);
        Direction = IsVerticalLayout ? FVector(0, -1, 0) : FVector(-1, 0, 0);
        auto LeftRay = FRay(Center, Direction, true);
        Bound = GetNodesBound(SelectedNodes);
        auto LeftBound = IsVerticalLayout ? FVector(0, Bound.Top, 0) : FVector(Bound.Left, 0, 0);
        LinkedCenter3D = LeftRay.PointAt(LeftRay.GetParameter(LeftBound));
        auto LinkedCenterFrom = FVector2D(LinkedCenter3D);
        FVector2D Offset = LinkedCenterTo - LinkedCenterFrom;
        Translate(SelectedNodes, Offset);
    }
    auto ConnectedNodesRight = UEGraphAdapter::GetNodesConnected(SelectedNodes, EFormatterPinDirection::Out);
    if (UEGraphAdapter::GetNodesConnectCenter(SelectedNodes, ConnectCenter, EFormatterPinDirection::Out))
    {
        auto Center = FVector(ConnectCenter.X, ConnectCenter.Y, 0);
        auto Direction = IsVerticalLayout ? FVector(0, -1, 0) : FVector(-1, 0, 0);
        auto LeftRay = FRay(Center, Direction, true);
        FSlateRect Bound = GetNodesBound(ConnectedNodesRight);
        auto LeftBound = IsVerticalLayout ? FVector(0, Bound.Top, 0) : FVector(Bound.Left, 0, 0);
        auto LinkedCenter3D = LeftRay.PointAt(LeftRay.GetParameter(LeftBound));
        auto LinkedCenterTo = FVector2D(LinkedCenter3D) - (IsVerticalLayout ? FVector2D(0, Settings.HorizontalSpacing) : FVector2D(Settings.HorizontalSpacing, 0));
        UEGraphAdapter::GetNodesConnectCenter(SelectedNodes, ConnectCenter, EFormatterPinDirection::Out, true);
        Center = FVector(ConnectCenter.X, ConnectCenter.Y, 0);
        Direction = IsVerticalLayout ? FVector(0, 1, 0) : FVector(1, 0, 0);
        auto RightRay = FRay(Center, Direction, true);
        Bound = GetNodesBound(SelectedNodes);
        auto RightBound = IsVerticalLayout ? FVector(0, Bound.Bottom, 0) : FVector(Bound.Right, 0, 0);
        LinkedCenter3D = RightRay.PointAt(RightRay.GetParameter(RightBound));
        auto LinkedCenterFrom = FVector2D(LinkedCenter3D);
        FVector2D Offset = LinkedCenterFrom - LinkedCenterTo;
        Translate(ConnectedNodesRight, Offset);
    }
    PostCommand();
}

FFormatter& FFormatter::Instance()
{
    static FFormatter Context;
    return Context;
}

FFormatter::FFormatter()
{
    if (FModuleManager::Get().GetModule(FName("AutoSizeComments")))
    {
        IsAutoSizeComment = true;
    }
}

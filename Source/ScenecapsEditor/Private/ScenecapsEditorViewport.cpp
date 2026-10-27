#include "ScenecapsEditorViewport.h"

#include "ScenecapsEditorToolkit.h"
#include "ScenecapsEditorViewportClient.h"
#include "PreviewScene.h"

void SScenecapsEditorViewport::Construct(const FArguments& InArgs)
{
	ToolkitPtr = InArgs._Toolkit;
	SEditorViewport::Construct(SEditorViewport::FArguments());
}

TSharedRef<FEditorViewportClient> SScenecapsEditorViewport::MakeEditorViewportClient()
{
	TSharedPtr<FScenecapsEditorToolkit> Toolkit = ToolkitPtr.Pin();
	check(Toolkit.IsValid());

	FPreviewScene* Scene = Toolkit->GetPreviewScene();
	check(Scene);

	ViewportClient = MakeShareable(new FScenecapsEditorViewportClient(
		Scene,
		SharedThis(this),
		Toolkit));

	return ViewportClient.ToSharedRef();
}
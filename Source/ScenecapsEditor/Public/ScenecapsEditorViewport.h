#pragma once

#include "CoreMinimal.h"
#include "SEditorViewport.h"

class FScenecapsEditorToolkit;
class FScenecapsEditorViewportClient;

class SScenecapsEditorViewport : public SEditorViewport
{
public:
	SLATE_BEGIN_ARGS(SScenecapsEditorViewport) {}
		SLATE_ARGUMENT(TSharedPtr<FScenecapsEditorToolkit>, Toolkit)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;

	TSharedPtr<FScenecapsEditorViewportClient> GetViewportClient() const { return ViewportClient; }

private:
	TWeakPtr<FScenecapsEditorToolkit> ToolkitPtr;
	TSharedPtr<FScenecapsEditorViewportClient> ViewportClient;
};
/*
  ==============================================================================

	Object.h
	Created: 26 Sep 2020 10:02:32am
	Author:  bkupe

  ==============================================================================
*/

#pragma once


class CompositionLayer :
	public BaseItem,
	public MediaTarget,
	public MeshGrid::Listener
{
public:
	CompositionLayer(const String& name = "Layer", var params = var());
	virtual ~CompositionLayer();

	Media* media;

	Point2DParameter* position;
	Point2DParameter* size;
	FloatParameter* alpha;
	FloatParameter* rotation;

	enum blendPreset {
		STANDARD, ADDITION, MULTIPLICATION, SCREEN, DARKEN, PREMULTALPHA, LIGHTEN, INVERT, COLORADD, COLORSCREEN, BLUR, INVERTCOLOR, SUBSTRACT, COLORDIFF, INVERTMULT, CUSTOM
	};
	enum blendOption { ZERO, ONE, SRC_ALPHA, ONE_MINUS_SRC_ALPHA, DST_ALPHA, ONE_MINUS_DST_ALPHA, SRC_COLOR, ONE_MINUS_SRC_COLOR, DST_COLOR, ONE_MINUS_DST_COLOR };

	EnumParameter* blendFunction;
	EnumParameter* blendFunctionSourceFactor;
	EnumParameter* blendFunctionDestinationFactor;

	// Mesh Warping
	ControllableContainer meshCC;
	BoolParameter* meshEnabled;
	BoolParameter* meshVisible;
	BoolParameter* meshLocked;
	IntParameter* meshSubdivisions;
	EnumParameter* meshWarpMode;
	Trigger* meshReset;
	Trigger* meshAddPoint;
	Trigger* meshAddSubdivision;
	Trigger* meshRemoveSubdivision;
	
	std::unique_ptr<MeshGrid> meshGrid;
	std::unique_ptr<MeshWarper> meshWarper;
	bool meshNeedsUpdate;

	virtual void onContainerParameterChangedInternal(Parameter* p);
	virtual void onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c) override;
	virtual void onContainerTriggerTriggered(Trigger* t) override;

	virtual void setMedia(Media* m);

	bool isUsingMedia(Media* m) override;

	// Mesh Grid Listener
	void meshGridChanged(MeshGrid* grid) override;
	void meshPointMoved(MeshGrid* grid, MeshControlPoint* point) override;
	void meshSelectionChanged(MeshGrid* grid) override;

	// Serialization
	var getJSONData() override;
	void loadJSONDataItemInternal(var data) override;

	// Mesh access
	MeshGrid* getMeshGrid() { return meshGrid.get(); }
	MeshWarper* getMeshWarper() { return meshWarper.get(); }
	bool isMeshEnabled() const { return meshEnabled->boolValue(); }
	bool isMeshVisible() const { return meshVisible->boolValue(); }
};

class ReferenceCompositionLayer : public CompositionLayer
{
public:
	ReferenceCompositionLayer(var params = var());
	virtual ~ReferenceCompositionLayer();

	TargetParameter* targetMedia;

	void onContainerParameterChangedInternal(Parameter* p) override;

	DECLARE_TYPE("Reference Layer");
};

class OwnedCompositionLayer : public CompositionLayer
{
public:
	OwnedCompositionLayer(var params = var());
	virtual ~OwnedCompositionLayer();

	void setMedia(Media* m) override;

	std::unique_ptr<Media> ownedMedia;

	String getTypeString() const override { return ownedMedia != nullptr ? ownedMedia->getTypeString() : ""; }
	static OwnedCompositionLayer* create(var params) { return new OwnedCompositionLayer(params); }
};

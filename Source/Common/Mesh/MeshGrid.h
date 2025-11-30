/*
  ==============================================================================

    MeshGrid.h
    Created: 2024
    Author:  MapGyver

    Mesh warping grid system for layer deformation.
    Provides control point management and interpolation for mesh-based warping.

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"

/**
 * Represents a single control point in the mesh grid.
 * Each point has a position (where it appears on screen) and
 * a UV coordinate (where it samples from the source texture).
 */
class MeshControlPoint
{
public:
    MeshControlPoint(Point<float> pos = Point<float>(), Point<float> uv = Point<float>());
    ~MeshControlPoint();

    // Screen position (normalized 0-1)
    Point<float> position;
    
    // UV texture coordinate (normalized 0-1)
    Point<float> uv;
    
    // Bezier handle positions (for bezier mode)
    Point<float> bezierHandle1;
    Point<float> bezierHandle2;
    
    // Whether this point uses bezier interpolation
    bool useBezier;
    
    // Whether this point is selected
    bool selected;
    
    // Original grid index (for tracking purposes)
    int gridRow;
    int gridCol;
    
    // Whether this is an additional manually-added point
    bool isManuallyAdded;

    var toJSON() const;
    void fromJSON(const var& data);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MeshControlPoint)
};

/**
 * Warping interpolation mode enumeration.
 */
enum class MeshWarpMode
{
    Bilinear,       // Standard bilinear texture mapping
    Perspective,    // Perspective-correct texture mapping
    Bezier          // Bezier curve-based warping
};

/**
 * MeshGrid class manages a grid of control points for mesh warping.
 * Supports dynamic subdivision, point manipulation, and multiple
 * interpolation modes.
 */
class MeshGrid
{
public:
    MeshGrid();
    ~MeshGrid();

    //==============================================================================
    // Grid Management
    
    /**
     * Initialize the mesh with default 2x2 subdivisions (3x3 grid).
     * @param bounds The normalized bounds (typically 0-1)
     */
    void initializeGrid(Rectangle<float> bounds = Rectangle<float>(0, 0, 1, 1));
    
    /**
     * Reset mesh to default 2x2 configuration.
     */
    void resetToDefault();
    
    /**
     * Add a subdivision, increasing grid resolution.
     */
    void addSubdivision();
    
    /**
     * Remove a subdivision, decreasing grid resolution.
     * Minimum is 1x1 subdivision (2x2 grid).
     */
    void removeSubdivision();
    
    /**
     * Get current number of subdivisions in each direction.
     */
    int getSubdivisions() const { return subdivisions; }
    
    /**
     * Get the number of control points in one dimension.
     */
    int getGridSize() const { return subdivisions + 1; }
    
    /**
     * Get total number of control points.
     */
    int getTotalPoints() const;
    
    //==============================================================================
    // Control Point Access
    
    /**
     * Get a control point by grid coordinates.
     */
    MeshControlPoint* getPoint(int row, int col);
    const MeshControlPoint* getPoint(int row, int col) const;
    
    /**
     * Get a control point by index.
     */
    MeshControlPoint* getPointByIndex(int index);
    const MeshControlPoint* getPointByIndex(int index) const;
    
    /**
     * Get all control points.
     */
    const OwnedArray<MeshControlPoint>& getPoints() const { return points; }
    OwnedArray<MeshControlPoint>& getPoints() { return points; }
    
    /**
     * Get all manually added points.
     */
    Array<MeshControlPoint*> getManualPoints();
    
    //==============================================================================
    // Point Manipulation
    
    /**
     * Add a manual control point at a specific position.
     * @param position The position in normalized coordinates
     * @return Pointer to the newly created point
     */
    MeshControlPoint* addManualPoint(Point<float> position);
    
    /**
     * Remove a manually added point.
     */
    bool removeManualPoint(MeshControlPoint* point);
    
    /**
     * Find the closest control point to a given position.
     * @param position Query position
     * @param maxDistance Maximum distance to consider (squared)
     * @return Pointer to closest point, or nullptr if none within range
     */
    MeshControlPoint* findClosestPoint(Point<float> position, float maxDistance = 0.05f);
    
    /**
     * Get all selected points.
     */
    Array<MeshControlPoint*> getSelectedPoints();
    
    /**
     * Clear all selections.
     */
    void clearSelection();
    
    /**
     * Select all points within a rectangle.
     */
    void selectPointsInRect(Rectangle<float> rect);
    
    //==============================================================================
    // Warping Configuration
    
    /**
     * Set the warping interpolation mode.
     */
    void setWarpMode(MeshWarpMode mode) { warpMode = mode; }
    MeshWarpMode getWarpMode() const { return warpMode; }
    
    /**
     * Enable/disable mesh editing.
     */
    void setEditMode(bool editing) { editModeEnabled = editing; }
    bool isEditModeEnabled() const { return editModeEnabled; }
    
    /**
     * Enable/disable grid visibility.
     */
    void setGridVisible(bool visible) { gridVisible = visible; }
    bool isGridVisible() const { return gridVisible; }
    
    /**
     * Lock/unlock mesh for editing.
     */
    void setLocked(bool locked) { isLocked = locked; }
    bool getLocked() const { return isLocked; }
    
    //==============================================================================
    // Interpolation
    
    /**
     * Interpolate a UV coordinate for a given screen position.
     * Uses the current warp mode for interpolation.
     */
    Point<float> interpolateUV(Point<float> screenPos) const;
    
    /**
     * Get interpolated grid positions for rendering.
     * Returns a grid of points suitable for drawing the warped mesh.
     */
    Array<Point<float>> getInterpolatedGrid(int resolution = 10) const;
    
    //==============================================================================
    // Serialization
    
    var toJSON() const;
    void fromJSON(const var& data);
    
    //==============================================================================
    // Listeners
    
    class Listener
    {
    public:
        virtual ~Listener() {}
        virtual void meshGridChanged(MeshGrid* grid) {}
        virtual void meshPointMoved(MeshGrid* grid, MeshControlPoint* point) {}
        virtual void meshSelectionChanged(MeshGrid* grid) {}
    };
    
    void addListener(Listener* listener) { listeners.add(listener); }
    void removeListener(Listener* listener) { listeners.remove(listener); }
    
protected:
    void notifyGridChanged();
    void notifyPointMoved(MeshControlPoint* point);
    void notifySelectionChanged();
    
private:
    // Grid properties
    int subdivisions;
    Rectangle<float> bounds;
    
    // Control points
    OwnedArray<MeshControlPoint> points;
    OwnedArray<MeshControlPoint> manualPoints;
    
    // Configuration
    MeshWarpMode warpMode;
    bool editModeEnabled;
    bool gridVisible;
    bool isLocked;
    
    // Listeners
    ListenerList<Listener> listeners;
    
    // Helper methods
    void rebuildGrid();
    int getPointIndex(int row, int col) const;
    
    // Interpolation helpers
    Point<float> bilinearInterpolate(Point<float> pos) const;
    Point<float> perspectiveInterpolate(Point<float> pos) const;
    Point<float> bezierInterpolate(Point<float> pos) const;
    
    // Bezier curve calculation
    static Point<float> cubicBezier(Point<float> p0, Point<float> p1, 
                                      Point<float> p2, Point<float> p3, float t);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MeshGrid)
};

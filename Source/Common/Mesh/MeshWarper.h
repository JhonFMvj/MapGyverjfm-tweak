/*
  ==============================================================================

    MeshWarper.h
    Created: 2024
    Author:  MapGyver

    GPU-based mesh warping renderer for applying mesh deformation to textures.

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"
#include "MeshGrid.h"

/**
 * MeshWarper handles the GPU rendering of mesh-warped textures.
 * It generates vertex/index buffers from a MeshGrid and renders
 * the warped texture using OpenGL.
 */
class MeshWarper : public MeshGrid::Listener
{
public:
    MeshWarper();
    ~MeshWarper();

    /**
     * Set the mesh grid to use for warping.
     */
    void setMeshGrid(MeshGrid* grid);
    MeshGrid* getMeshGrid() const { return meshGrid; }

    /**
     * Update the mesh vertices for rendering.
     * Call this when the mesh has changed.
     */
    void updateVertices();

    /**
     * Render the warped texture to the current framebuffer.
     * @param textureID The source texture to warp
     * @param width Viewport width
     * @param height Viewport height
     */
    void render(GLuint textureID, int width, int height);

    /**
     * Draw the mesh grid overlay for editing.
     * @param width Viewport width
     * @param height Viewport height
     * @param showPoints Whether to draw control points
     */
    void drawGridOverlay(int width, int height, bool showPoints = true);

    /**
     * Set colors for rendering.
     */
    void setGridLineColor(Colour color) { gridLineColor = color; }
    void setUnselectedPointColor(Colour color) { unselectedPointColor = color; }
    void setSelectedPointColor(Colour color) { selectedPointColor = color; }
    void setBezierPointColor(Colour color) { bezierPointColor = color; }

    /**
     * Set point rendering size.
     */
    void setPointSize(float size) { pointSize = size; }

    // MeshGrid::Listener
    void meshGridChanged(MeshGrid* grid) override;
    void meshPointMoved(MeshGrid* grid, MeshControlPoint* point) override;
    void meshSelectionChanged(MeshGrid* grid) override;

private:
    MeshGrid* meshGrid;

    // Vertex data
    Array<GLfloat> vertices;
    Array<GLuint> indices;
    bool verticesNeedUpdate;

    // OpenGL resources
    GLuint vbo;
    GLuint ebo;
    bool glInitialized;

    // Rendering settings
    int gridResolution;
    Colour gridLineColor;
    Colour unselectedPointColor;
    Colour selectedPointColor;
    Colour bezierPointColor;
    float pointSize;

    // Internal methods
    void initGL();
    void cleanupGL();
    void buildVertexData();
    void buildVertexDataForCell(int row, int col, int resolution);

    // Add vertices for a quad
    void addQuadVertices(Point<float> p1, Point<float> uv1,
                         Point<float> p2, Point<float> uv2,
                         Point<float> p3, Point<float> uv3,
                         Point<float> p4, Point<float> uv4);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MeshWarper)
};

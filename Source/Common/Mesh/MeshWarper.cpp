/*
  ==============================================================================

    MeshWarper.cpp
    Created: 2024
    Author:  MapGyver

    Implementation of GPU-based mesh warping renderer.

  ==============================================================================
*/

#include "MeshWarper.h"

using namespace juce::gl;

MeshWarper::MeshWarper()
    : meshGrid(nullptr)
    , verticesNeedUpdate(true)
    , vbo(0)
    , ebo(0)
    , vao(0)
    , glInitialized(false)
    , gridResolution(10)
    , gridLineColor(Colour::fromFloatRGBA(0.5f, 0.5f, 0.5f, 0.8f))
    , unselectedPointColor(Colour::fromFloatRGBA(0.6f, 0.6f, 0.6f, 1.0f))
    , selectedPointColor(Colour::fromFloatRGBA(1.0f, 0.6f, 0.0f, 1.0f))
    , bezierPointColor(Colour::fromFloatRGBA(0.0f, 0.8f, 1.0f, 1.0f))
    , pointSize(8.0f)
{
}

MeshWarper::~MeshWarper()
{
    if (meshGrid != nullptr)
        meshGrid->removeListener(this);
    
    cleanupGL();
}

void MeshWarper::setMeshGrid(MeshGrid* grid)
{
    if (meshGrid != nullptr)
        meshGrid->removeListener(this);
    
    meshGrid = grid;
    
    if (meshGrid != nullptr)
        meshGrid->addListener(this);
    
    verticesNeedUpdate = true;
}

void MeshWarper::updateVertices()
{
    if (meshGrid == nullptr) return;
    
    buildVertexData();
    verticesNeedUpdate = false;
}

void MeshWarper::render(GLuint textureID, int width, int height)
{
    if (meshGrid == nullptr) return;
    
    if (verticesNeedUpdate)
        updateVertices();
    
    if (vertices.isEmpty()) return;
    
    // Initialize GL resources if needed
    if (!glInitialized)
        initGL();
    
    // Set up viewport and projection
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 1, 1, 0, -1, 1);  // Normalized coordinates
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // Enable texturing
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Bind texture
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    // Draw the mesh
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    
    // Upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * vertices.size(), 
                 vertices.getRawDataPointer(), GL_DYNAMIC_DRAW);
    
    // Set up vertex attributes (position: 2 floats, texcoord: 2 floats)
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    
    glVertexPointer(2, GL_FLOAT, 4 * sizeof(GLfloat), 0);
    glTexCoordPointer(2, GL_FLOAT, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
    
    // Upload and draw indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indices.size(),
                 indices.getRawDataPointer(), GL_DYNAMIC_DRAW);
    
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    
    // Cleanup
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void MeshWarper::drawGridOverlay(int width, int height, bool showPoints)
{
    if (meshGrid == nullptr) return;
    if (!meshGrid->isGridVisible()) return;
    
    // Set up viewport and projection
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 1, 1, 0, -1, 1);  // Normalized coordinates (Y flipped)
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    
    // Draw grid lines
    glLineWidth(1.5f);
    glColor4f(gridLineColor.getFloatRed(), gridLineColor.getFloatGreen(),
              gridLineColor.getFloatBlue(), gridLineColor.getFloatAlpha());
    
    int gridSize = meshGrid->getGridSize();
    
    // Draw horizontal lines
    glBegin(GL_LINES);
    for (int row = 0; row < gridSize; row++)
    {
        for (int col = 0; col < gridSize - 1; col++)
        {
            const MeshControlPoint* p1 = meshGrid->getPoint(row, col);
            const MeshControlPoint* p2 = meshGrid->getPoint(row, col + 1);
            if (p1 && p2)
            {
                glVertex2f(p1->position.x, p1->position.y);
                glVertex2f(p2->position.x, p2->position.y);
            }
        }
    }
    
    // Draw vertical lines
    for (int col = 0; col < gridSize; col++)
    {
        for (int row = 0; row < gridSize - 1; row++)
        {
            const MeshControlPoint* p1 = meshGrid->getPoint(row, col);
            const MeshControlPoint* p2 = meshGrid->getPoint(row + 1, col);
            if (p1 && p2)
            {
                glVertex2f(p1->position.x, p1->position.y);
                glVertex2f(p2->position.x, p2->position.y);
            }
        }
    }
    glEnd();
    
    // Draw control points
    if (showPoints)
    {
        glPointSize(pointSize);
        glEnable(GL_POINT_SMOOTH);
        
        // Draw all grid points
        for (int i = 0; i < meshGrid->getTotalPoints(); i++)
        {
            const MeshControlPoint* point = meshGrid->getPointByIndex(i);
            if (!point) continue;
            
            Colour pointColor;
            if (point->selected)
            {
                pointColor = point->useBezier ? bezierPointColor : selectedPointColor;
            }
            else
            {
                pointColor = unselectedPointColor;
            }
            
            // Draw point background (slightly larger, darker)
            glPointSize(pointSize + 2);
            glBegin(GL_POINTS);
            glColor4f(0.1f, 0.1f, 0.1f, 0.8f);
            glVertex2f(point->position.x, point->position.y);
            glEnd();
            
            // Draw point
            glPointSize(pointSize);
            glBegin(GL_POINTS);
            glColor4f(pointColor.getFloatRed(), pointColor.getFloatGreen(),
                      pointColor.getFloatBlue(), pointColor.getFloatAlpha());
            glVertex2f(point->position.x, point->position.y);
            glEnd();
            
            // Draw bezier handles if the point uses bezier and is selected
            if (point->useBezier && point->selected)
            {
                glLineWidth(1.0f);
                glColor4f(bezierPointColor.getFloatRed(), bezierPointColor.getFloatGreen(),
                          bezierPointColor.getFloatBlue(), 0.5f);
                
                glBegin(GL_LINES);
                glVertex2f(point->position.x, point->position.y);
                glVertex2f(point->bezierHandle1.x, point->bezierHandle1.y);
                glVertex2f(point->position.x, point->position.y);
                glVertex2f(point->bezierHandle2.x, point->bezierHandle2.y);
                glEnd();
                
                // Draw handle points
                glPointSize(pointSize * 0.6f);
                glBegin(GL_POINTS);
                glColor4f(bezierPointColor.getFloatRed(), bezierPointColor.getFloatGreen(),
                          bezierPointColor.getFloatBlue(), 1.0f);
                glVertex2f(point->bezierHandle1.x, point->bezierHandle1.y);
                glVertex2f(point->bezierHandle2.x, point->bezierHandle2.y);
                glEnd();
            }
        }
        
        glDisable(GL_POINT_SMOOTH);
    }
    
    glDisable(GL_LINE_SMOOTH);
}

void MeshWarper::meshGridChanged(MeshGrid* grid)
{
    verticesNeedUpdate = true;
}

void MeshWarper::meshPointMoved(MeshGrid* grid, MeshControlPoint* point)
{
    verticesNeedUpdate = true;
}

void MeshWarper::meshSelectionChanged(MeshGrid* grid)
{
    // Selection change doesn't require vertex update
}

void MeshWarper::initGL()
{
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glInitialized = true;
}

void MeshWarper::cleanupGL()
{
    if (glInitialized)
    {
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &ebo);
        glInitialized = false;
    }
}

void MeshWarper::buildVertexData()
{
    if (meshGrid == nullptr) return;
    
    vertices.clear();
    indices.clear();
    
    int subdivisions = meshGrid->getSubdivisions();
    
    // Build mesh for each cell
    for (int row = 0; row < subdivisions; row++)
    {
        for (int col = 0; col < subdivisions; col++)
        {
            buildVertexDataForCell(row, col, gridResolution);
        }
    }
}

void MeshWarper::buildVertexDataForCell(int row, int col, int resolution)
{
    const MeshControlPoint* tl = meshGrid->getPoint(row, col);
    const MeshControlPoint* tr = meshGrid->getPoint(row, col + 1);
    const MeshControlPoint* bl = meshGrid->getPoint(row + 1, col);
    const MeshControlPoint* br = meshGrid->getPoint(row + 1, col + 1);
    
    if (!tl || !tr || !bl || !br) return;
    
    float step = 1.0f / (float)resolution;
    
    // Generate subdivided grid within this cell
    for (int y = 0; y < resolution; y++)
    {
        for (int x = 0; x < resolution; x++)
        {
            float u0 = x * step;
            float v0 = y * step;
            float u1 = (x + 1) * step;
            float v1 = (y + 1) * step;
            
            // Bilinear interpolation for positions
            auto lerp = [](Point<float> a, Point<float> b, float t) {
                return a + (b - a) * t;
            };
            
            auto bilinear = [&](float u, float v) {
                Point<float> top = lerp(tl->position, tr->position, u);
                Point<float> bottom = lerp(bl->position, br->position, u);
                return lerp(top, bottom, v);
            };
            
            auto bilinearUV = [&](float u, float v) {
                Point<float> top = lerp(tl->uv, tr->uv, u);
                Point<float> bottom = lerp(bl->uv, br->uv, u);
                return lerp(top, bottom, v);
            };
            
            // Get the four corners of this sub-quad
            Point<float> p00 = bilinear(u0, v0);
            Point<float> p10 = bilinear(u1, v0);
            Point<float> p01 = bilinear(u0, v1);
            Point<float> p11 = bilinear(u1, v1);
            
            Point<float> uv00 = bilinearUV(u0, v0);
            Point<float> uv10 = bilinearUV(u1, v0);
            Point<float> uv01 = bilinearUV(u0, v1);
            Point<float> uv11 = bilinearUV(u1, v1);
            
            addQuadVertices(p00, uv00, p10, uv10, p01, uv01, p11, uv11);
        }
    }
}

void MeshWarper::addQuadVertices(Point<float> p1, Point<float> uv1,
                                  Point<float> p2, Point<float> uv2,
                                  Point<float> p3, Point<float> uv3,
                                  Point<float> p4, Point<float> uv4)
{
    // Start index for this quad
    GLuint baseIndex = vertices.size() / 4;
    
    // Add vertices (position.x, position.y, uv.x, uv.y)
    vertices.add(p1.x); vertices.add(p1.y); vertices.add(uv1.x); vertices.add(uv1.y);
    vertices.add(p2.x); vertices.add(p2.y); vertices.add(uv2.x); vertices.add(uv2.y);
    vertices.add(p3.x); vertices.add(p3.y); vertices.add(uv3.x); vertices.add(uv3.y);
    vertices.add(p4.x); vertices.add(p4.y); vertices.add(uv4.x); vertices.add(uv4.y);
    
    // Add indices for two triangles
    // Triangle 1: p1, p2, p3
    indices.add(baseIndex);
    indices.add(baseIndex + 1);
    indices.add(baseIndex + 2);
    
    // Triangle 2: p2, p4, p3
    indices.add(baseIndex + 1);
    indices.add(baseIndex + 3);
    indices.add(baseIndex + 2);
}

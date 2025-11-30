/*
  ==============================================================================

    MeshGrid.cpp
    Created: 2024
    Author:  MapGyver

    Implementation of mesh warping grid system.

  ==============================================================================
*/

#include "MeshGrid.h"

//==============================================================================
// MeshControlPoint Implementation
//==============================================================================

MeshControlPoint::MeshControlPoint(Point<float> pos, Point<float> uvCoord)
    : position(pos)
    , uv(uvCoord)
    , bezierHandle1(pos)
    , bezierHandle2(pos)
    , useBezier(false)
    , selected(false)
    , gridRow(-1)
    , gridCol(-1)
    , isManuallyAdded(false)
{
}

MeshControlPoint::~MeshControlPoint()
{
}

var MeshControlPoint::toJSON() const
{
    DynamicObject* obj = new DynamicObject();
    
    Array<var> posArray;
    posArray.add(position.x);
    posArray.add(position.y);
    obj->setProperty("position", posArray);
    
    Array<var> uvArray;
    uvArray.add(uv.x);
    uvArray.add(uv.y);
    obj->setProperty("uv", uvArray);
    
    if (useBezier)
    {
        Array<var> h1Array;
        h1Array.add(bezierHandle1.x);
        h1Array.add(bezierHandle1.y);
        obj->setProperty("bezierHandle1", h1Array);
        
        Array<var> h2Array;
        h2Array.add(bezierHandle2.x);
        h2Array.add(bezierHandle2.y);
        obj->setProperty("bezierHandle2", h2Array);
    }
    
    obj->setProperty("useBezier", useBezier);
    obj->setProperty("gridRow", gridRow);
    obj->setProperty("gridCol", gridCol);
    obj->setProperty("isManuallyAdded", isManuallyAdded);
    
    return var(obj);
}

void MeshControlPoint::fromJSON(const var& data)
{
    if (data.hasProperty("position"))
    {
        var pos = data["position"];
        if (pos.isArray() && pos.size() >= 2)
        {
            position.x = (float)pos[0];
            position.y = (float)pos[1];
        }
    }
    
    if (data.hasProperty("uv"))
    {
        var uvData = data["uv"];
        if (uvData.isArray() && uvData.size() >= 2)
        {
            uv.x = (float)uvData[0];
            uv.y = (float)uvData[1];
        }
    }
    
    useBezier = data.getProperty("useBezier", false);
    
    if (useBezier)
    {
        if (data.hasProperty("bezierHandle1"))
        {
            var h1 = data["bezierHandle1"];
            if (h1.isArray() && h1.size() >= 2)
            {
                bezierHandle1.x = (float)h1[0];
                bezierHandle1.y = (float)h1[1];
            }
        }
        
        if (data.hasProperty("bezierHandle2"))
        {
            var h2 = data["bezierHandle2"];
            if (h2.isArray() && h2.size() >= 2)
            {
                bezierHandle2.x = (float)h2[0];
                bezierHandle2.y = (float)h2[1];
            }
        }
    }
    
    gridRow = data.getProperty("gridRow", -1);
    gridCol = data.getProperty("gridCol", -1);
    isManuallyAdded = data.getProperty("isManuallyAdded", false);
}

//==============================================================================
// MeshGrid Implementation
//==============================================================================

// Epsilon value for floating-point comparisons
static constexpr float MESH_EPSILON = 0.0001f;

// Maximum grid dimension supported for hash key generation
// Grid positions are encoded as: row * MAX_GRID_DIM + col
static constexpr int MAX_GRID_DIM = 1000;

MeshGrid::MeshGrid()
    : subdivisions(2)
    , bounds(0, 0, 1, 1)
    , warpMode(MeshWarpMode::Bilinear)
    , editModeEnabled(false)
    , gridVisible(true)
    , isLocked(false)
{
    initializeGrid();
}

MeshGrid::~MeshGrid()
{
}

void MeshGrid::initializeGrid(Rectangle<float> newBounds)
{
    bounds = newBounds;
    rebuildGrid();
}

void MeshGrid::resetToDefault()
{
    subdivisions = 2;
    manualPoints.clear();
    warpMode = MeshWarpMode::Bilinear;
    isLocked = false;
    rebuildGrid();
    notifyGridChanged();
}

void MeshGrid::addSubdivision()
{
    if (isLocked) return;
    
    subdivisions++;
    rebuildGrid();
    notifyGridChanged();
}

void MeshGrid::removeSubdivision()
{
    if (isLocked) return;
    if (subdivisions <= 1) return;  // Minimum 1x1 subdivision (2x2 grid)
    
    subdivisions--;
    rebuildGrid();
    notifyGridChanged();
}

int MeshGrid::getTotalPoints() const
{
    int gridPoints = (subdivisions + 1) * (subdivisions + 1);
    return gridPoints + manualPoints.size();
}

MeshControlPoint* MeshGrid::getPoint(int row, int col)
{
    int index = getPointIndex(row, col);
    if (index >= 0 && index < points.size())
        return points[index];
    return nullptr;
}

const MeshControlPoint* MeshGrid::getPoint(int row, int col) const
{
    int index = getPointIndex(row, col);
    if (index >= 0 && index < points.size())
        return points[index];
    return nullptr;
}

MeshControlPoint* MeshGrid::getPointByIndex(int index)
{
    if (index >= 0 && index < points.size())
        return points[index];
    
    // Check manual points
    int manualIndex = index - points.size();
    if (manualIndex >= 0 && manualIndex < manualPoints.size())
        return manualPoints[manualIndex];
    
    return nullptr;
}

const MeshControlPoint* MeshGrid::getPointByIndex(int index) const
{
    if (index >= 0 && index < points.size())
        return points[index];
    
    int manualIndex = index - points.size();
    if (manualIndex >= 0 && manualIndex < manualPoints.size())
        return manualPoints[manualIndex];
    
    return nullptr;
}

Array<MeshControlPoint*> MeshGrid::getManualPoints()
{
    Array<MeshControlPoint*> result;
    for (auto* p : manualPoints)
        result.add(p);
    return result;
}

MeshControlPoint* MeshGrid::addManualPoint(Point<float> position)
{
    if (isLocked) return nullptr;
    
    auto* point = new MeshControlPoint(position, position);  // UV same as position initially
    point->isManuallyAdded = true;
    manualPoints.add(point);
    
    notifyGridChanged();
    return point;
}

bool MeshGrid::removeManualPoint(MeshControlPoint* point)
{
    if (isLocked) return false;
    if (point == nullptr || !point->isManuallyAdded) return false;
    
    int index = manualPoints.indexOf(point);
    if (index >= 0)
    {
        manualPoints.remove(index);
        notifyGridChanged();
        return true;
    }
    return false;
}

MeshControlPoint* MeshGrid::findClosestPoint(Point<float> position, float maxDistance)
{
    MeshControlPoint* closest = nullptr;
    float closestDistSq = maxDistance * maxDistance;
    
    // Check grid points
    for (auto* p : points)
    {
        float distSq = position.getDistanceSquaredFrom(p->position);
        if (distSq < closestDistSq)
        {
            closestDistSq = distSq;
            closest = p;
        }
    }
    
    // Check manual points
    for (auto* p : manualPoints)
    {
        float distSq = position.getDistanceSquaredFrom(p->position);
        if (distSq < closestDistSq)
        {
            closestDistSq = distSq;
            closest = p;
        }
    }
    
    return closest;
}

Array<MeshControlPoint*> MeshGrid::getSelectedPoints()
{
    Array<MeshControlPoint*> selected;
    
    for (auto* p : points)
        if (p->selected) selected.add(p);
    
    for (auto* p : manualPoints)
        if (p->selected) selected.add(p);
    
    return selected;
}

void MeshGrid::clearSelection()
{
    for (auto* p : points)
        p->selected = false;
    
    for (auto* p : manualPoints)
        p->selected = false;
    
    notifySelectionChanged();
}

void MeshGrid::selectPointsInRect(Rectangle<float> rect)
{
    for (auto* p : points)
        p->selected = rect.contains(p->position);
    
    for (auto* p : manualPoints)
        p->selected = rect.contains(p->position);
    
    notifySelectionChanged();
}

Point<float> MeshGrid::interpolateUV(Point<float> screenPos) const
{
    switch (warpMode)
    {
        case MeshWarpMode::Bilinear:
            return bilinearInterpolate(screenPos);
        case MeshWarpMode::Perspective:
            return perspectiveInterpolate(screenPos);
        case MeshWarpMode::Bezier:
            return bezierInterpolate(screenPos);
        default:
            return bilinearInterpolate(screenPos);
    }
}

Array<Point<float>> MeshGrid::getInterpolatedGrid(int resolution) const
{
    Array<Point<float>> result;
    
    int gridSize = getGridSize();
    float step = 1.0f / (float)((gridSize - 1) * resolution);
    
    for (float y = 0; y <= 1.0f; y += step)
    {
        for (float x = 0; x <= 1.0f; x += step)
        {
            result.add(interpolateUV(Point<float>(x, y)));
        }
    }
    
    return result;
}

var MeshGrid::toJSON() const
{
    DynamicObject* obj = new DynamicObject();
    
    obj->setProperty("subdivisions", subdivisions);
    obj->setProperty("warpMode", (int)warpMode);
    obj->setProperty("editModeEnabled", editModeEnabled);
    obj->setProperty("gridVisible", gridVisible);
    obj->setProperty("isLocked", isLocked);
    
    Array<var> boundsArray;
    boundsArray.add(bounds.getX());
    boundsArray.add(bounds.getY());
    boundsArray.add(bounds.getWidth());
    boundsArray.add(bounds.getHeight());
    obj->setProperty("bounds", boundsArray);
    
    // Save grid points
    Array<var> pointsArray;
    for (auto* p : points)
        pointsArray.add(p->toJSON());
    obj->setProperty("points", pointsArray);
    
    // Save manual points
    Array<var> manualArray;
    for (auto* p : manualPoints)
        manualArray.add(p->toJSON());
    obj->setProperty("manualPoints", manualArray);
    
    return var(obj);
}

void MeshGrid::fromJSON(const var& data)
{
    subdivisions = data.getProperty("subdivisions", 2);
    warpMode = (MeshWarpMode)(int)data.getProperty("warpMode", 0);
    editModeEnabled = data.getProperty("editModeEnabled", false);
    gridVisible = data.getProperty("gridVisible", true);
    isLocked = data.getProperty("isLocked", false);
    
    if (data.hasProperty("bounds"))
    {
        var b = data["bounds"];
        if (b.isArray() && b.size() >= 4)
        {
            bounds = Rectangle<float>((float)b[0], (float)b[1], (float)b[2], (float)b[3]);
        }
    }
    
    // Rebuild grid first
    rebuildGrid();
    
    // Load grid point data
    if (data.hasProperty("points"))
    {
        var pointsData = data["points"];
        if (pointsData.isArray())
        {
            for (int i = 0; i < jmin(pointsData.size(), points.size()); i++)
            {
                points[i]->fromJSON(pointsData[i]);
            }
        }
    }
    
    // Load manual points
    manualPoints.clear();
    if (data.hasProperty("manualPoints"))
    {
        var manualData = data["manualPoints"];
        if (manualData.isArray())
        {
            for (int i = 0; i < manualData.size(); i++)
            {
                auto* point = new MeshControlPoint();
                point->fromJSON(manualData[i]);
                manualPoints.add(point);
            }
        }
    }
    
    notifyGridChanged();
}

void MeshGrid::notifyGridChanged()
{
    listeners.call([this](Listener& l) { l.meshGridChanged(this); });
}

void MeshGrid::notifyPointMoved(MeshControlPoint* point)
{
    listeners.call([this, point](Listener& l) { l.meshPointMoved(this, point); });
}

void MeshGrid::notifySelectionChanged()
{
    listeners.call([this](Listener& l) { l.meshSelectionChanged(this); });
}

void MeshGrid::rebuildGrid()
{
    // Store old point positions if any exist
    HashMap<int, Point<float>> oldPositions;
    for (auto* p : points)
    {
        if (p->gridRow >= 0 && p->gridCol >= 0)
        {
            int key = p->gridRow * MAX_GRID_DIM + p->gridCol;
            oldPositions.set(key, p->position);
        }
    }
    
    points.clear();
    
    int gridSize = subdivisions + 1;
    float stepX = bounds.getWidth() / (float)subdivisions;
    float stepY = bounds.getHeight() / (float)subdivisions;
    
    for (int row = 0; row < gridSize; row++)
    {
        for (int col = 0; col < gridSize; col++)
        {
            float x = bounds.getX() + col * stepX;
            float y = bounds.getY() + row * stepY;
            
            Point<float> pos(x, y);
            Point<float> uv(x, y);  // UV matches position in default grid
            
            auto* point = new MeshControlPoint(pos, uv);
            point->gridRow = row;
            point->gridCol = col;
            
            // Restore old position if it was the same grid position
            int key = row * MAX_GRID_DIM + col;
            if (oldPositions.contains(key))
            {
                point->position = oldPositions[key];
            }
            
            points.add(point);
        }
    }
}

int MeshGrid::getPointIndex(int row, int col) const
{
    int gridSize = subdivisions + 1;
    if (row < 0 || row >= gridSize || col < 0 || col >= gridSize)
        return -1;
    return row * gridSize + col;
}

Point<float> MeshGrid::bilinearInterpolate(Point<float> pos) const
{
    if (points.isEmpty()) return pos;
    
    // Find which cell the position is in
    int gridSize = subdivisions + 1;
    float cellSize = 1.0f / (float)subdivisions;
    
    int cellX = jmin((int)(pos.x / cellSize), subdivisions - 1);
    int cellY = jmin((int)(pos.y / cellSize), subdivisions - 1);
    
    cellX = jmax(0, cellX);
    cellY = jmax(0, cellY);
    
    // Get the four corners of the cell
    const MeshControlPoint* tl = getPoint(cellY, cellX);
    const MeshControlPoint* tr = getPoint(cellY, cellX + 1);
    const MeshControlPoint* bl = getPoint(cellY + 1, cellX);
    const MeshControlPoint* br = getPoint(cellY + 1, cellX + 1);
    
    if (!tl || !tr || !bl || !br) return pos;
    
    // Calculate local position within cell
    float localX = (pos.x - cellX * cellSize) / cellSize;
    float localY = (pos.y - cellY * cellSize) / cellSize;
    
    // Bilinear interpolation
    Point<float> top = tl->uv + (tr->uv - tl->uv) * localX;
    Point<float> bottom = bl->uv + (br->uv - bl->uv) * localX;
    
    return top + (bottom - top) * localY;
}

Point<float> MeshGrid::perspectiveInterpolate(Point<float> pos) const
{
    if (points.isEmpty()) return pos;
    
    // Find which cell the position is in
    int gridSize = subdivisions + 1;
    float cellSize = 1.0f / (float)subdivisions;
    
    int cellX = jmin((int)(pos.x / cellSize), subdivisions - 1);
    int cellY = jmin((int)(pos.y / cellSize), subdivisions - 1);
    
    cellX = jmax(0, cellX);
    cellY = jmax(0, cellY);
    
    // Get the four corners of the cell
    const MeshControlPoint* tl = getPoint(cellY, cellX);
    const MeshControlPoint* tr = getPoint(cellY, cellX + 1);
    const MeshControlPoint* bl = getPoint(cellY + 1, cellX);
    const MeshControlPoint* br = getPoint(cellY + 1, cellX + 1);
    
    if (!tl || !tr || !bl || !br) return pos;
    
    // Calculate perspective-correct interpolation using homogeneous coordinates
    // This provides better quality for perspective distortions
    
    float localX = (pos.x - cellX * cellSize) / cellSize;
    float localY = (pos.y - cellY * cellSize) / cellSize;
    
    // Calculate weights using perspective correction
    Point<float> p1 = tl->position;
    Point<float> p2 = tr->position;
    Point<float> p3 = br->position;
    Point<float> p4 = bl->position;
    
    // Find center intersection for perspective weights
    Point<float> center;
    float d = (p1.x - p3.x) * (p2.y - p4.y) - (p1.y - p3.y) * (p2.x - p4.x);
    
    if (std::abs(d) < MESH_EPSILON)
    {
        // No perspective correction needed, fall back to bilinear
        return bilinearInterpolate(pos);
    }
    
    float t1 = ((p1.x - p4.x) * (p2.y - p4.y) - (p1.y - p4.y) * (p2.x - p4.x)) / d;
    center = p1 + (p3 - p1) * t1;
    
    // Calculate distance weights
    float dtl = center.getDistanceFrom(p1);
    float dtr = center.getDistanceFrom(p2);
    float dbr = center.getDistanceFrom(p3);
    float dbl = center.getDistanceFrom(p4);
    
    // Perspective correction factors
    float ztl = (dtl + dbr) / dbr;
    float ztr = (dtr + dbl) / dbl;
    float zbr = (dbr + dtl) / dtl;
    float zbl = (dbl + dtr) / dtr;
    
    // Interpolate with perspective correction
    float w1 = (1 - localX) * (1 - localY) * ztl;
    float w2 = localX * (1 - localY) * ztr;
    float w3 = localX * localY * zbr;
    float w4 = (1 - localX) * localY * zbl;
    
    float wSum = w1 + w2 + w3 + w4;
    
    if (wSum < MESH_EPSILON) return bilinearInterpolate(pos);
    
    return (tl->uv * w1 + tr->uv * w2 + br->uv * w3 + bl->uv * w4) / wSum;
}

Point<float> MeshGrid::bezierInterpolate(Point<float> pos) const
{
    if (points.isEmpty()) return pos;
    
    // Find which cell the position is in
    int gridSize = subdivisions + 1;
    float cellSize = 1.0f / (float)subdivisions;
    
    int cellX = jmin((int)(pos.x / cellSize), subdivisions - 1);
    int cellY = jmin((int)(pos.y / cellSize), subdivisions - 1);
    
    cellX = jmax(0, cellX);
    cellY = jmax(0, cellY);
    
    // Get the four corners of the cell
    const MeshControlPoint* tl = getPoint(cellY, cellX);
    const MeshControlPoint* tr = getPoint(cellY, cellX + 1);
    const MeshControlPoint* bl = getPoint(cellY + 1, cellX);
    const MeshControlPoint* br = getPoint(cellY + 1, cellX + 1);
    
    if (!tl || !tr || !bl || !br) return pos;
    
    float localX = (pos.x - cellX * cellSize) / cellSize;
    float localY = (pos.y - cellY * cellSize) / cellSize;
    
    // Check if any points use bezier
    bool anyBezier = tl->useBezier || tr->useBezier || bl->useBezier || br->useBezier;
    
    if (!anyBezier)
    {
        // Fall back to bilinear if no bezier handles
        return bilinearInterpolate(pos);
    }
    
    // Cubic bezier interpolation for edges with bezier handles
    Point<float> topEdge = cubicBezier(tl->uv, tl->bezierHandle2, tr->bezierHandle1, tr->uv, localX);
    Point<float> bottomEdge = cubicBezier(bl->uv, bl->bezierHandle2, br->bezierHandle1, br->uv, localX);
    
    // Then interpolate vertically
    return cubicBezier(topEdge, topEdge + Point<float>(0, 0.33f), 
                       bottomEdge - Point<float>(0, 0.33f), bottomEdge, localY);
}

Point<float> MeshGrid::cubicBezier(Point<float> p0, Point<float> p1, 
                                    Point<float> p2, Point<float> p3, float t)
{
    float t2 = t * t;
    float t3 = t2 * t;
    float mt = 1 - t;
    float mt2 = mt * mt;
    float mt3 = mt2 * mt;
    
    return p0 * mt3 + p1 * (3 * mt2 * t) + p2 * (3 * mt * t2) + p3 * t3;
}

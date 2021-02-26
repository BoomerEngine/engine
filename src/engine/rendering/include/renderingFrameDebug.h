/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\debug #]
***/

#pragma once

#include "renderingFrameDebugGeometry.h"

BEGIN_BOOMER_NAMESPACE_EX(rendering)

//--

struct DebugVertex;

//--

// a line rendering context, all gathered lines will be written out as one batch (unless the flush() was used)
class ENGINE_RENDERING_API DebugDrawerBase : public NoCopy
{
public:
	DebugDrawerBase(DebugGeometry& dg, const Matrix& localToWorld = Matrix::IDENTITY());
    ~DebugDrawerBase();

    //--

    // set transform for current geometry generation
    void localToWorld(const Matrix& localToWorld);

	// change target selectable
	void selectable(Selectable selectable);

	// change image
	void image(gpu::ImageSampledView* view);

    // change color
    INLINE void color(Color color) { m_color = color; }

    //--

    // append vertices, vertices are transformed by current matrix (with exception to 2D vertices that are not)
    uint32_t appendVertices(const DebugVertex* vertex, uint32_t numVertices, const float* sizeOverride = nullptr, const uint32_t* subSelectionIDOverride = nullptr);
    uint32_t appendVertices(const Vector2* points, uint32_t numVertices, const float* sizeOverride = nullptr, const uint32_t* subSelectionIDOverride = nullptr, const Color* colorOverride = nullptr);
    uint32_t appendVertices(const Vector3* points, uint32_t numVertices, const float* sizeOverride = nullptr, const uint32_t* subSelectionIDOverride = nullptr, const Color* colorOverride = nullptr);

    // append indices to the current element
    uint32_t appendIndices(const uint32_t* indices, uint32_t numIndices, uint32_t firstVertex);
	uint32_t appendIndices(const uint16_t* indices, uint32_t numIndices, uint32_t firstVertex);

    // append auto generated indices to current element
    uint32_t appendAutoPointIndices(uint32_t firstVertex, uint32_t numVertices);
    uint32_t appendAutoLineLoopIndices(uint32_t firstVertex, uint32_t numVertices);
    uint32_t appendAutoLineListIndices(uint32_t firstVertex, uint32_t numVertices);
    uint32_t appendAutoTriangleFanIndices(uint32_t firstVertex, uint32_t numVertices, bool swap = false);
    uint32_t appendAutoTriangleListIndices(uint32_t firstVertex, uint32_t numVertices, bool swap = false);

    //--

    // flush current state
    void flush();

    //--

protected:
	static const uint32_t STACK_VERTICES = 4096;
	static const uint32_t STACK_INDICES = 6146;

    DebugGeometry& m_geometry;

	Matrix m_localToWorld;
	bool m_localToWorldSet = false;

	Color m_color = Color::WHITE;

	struct
	{
		Selectable selectable;
		DebugGeometryType geometryType;
		gpu::ImageSampledViewPtr image;

	} m_batchState;

	InplaceArray<DebugVertex, STACK_VERTICES> m_verticesData;
    InplaceArray<uint32_t, STACK_INDICES> m_indicesData;
	uint32_t m_baseVertexIndex = 0;

	void changeGeometryType(DebugGeometryType type);
};

//--

// measure size of debug text
extern ENGINE_RENDERING_API Point MeasureDebugText(StringView txt, DebugFont font = DebugFont::Normal);

//--

// debug text params
struct DebugTextParams
{
	DebugFont _font = DebugFont::Normal;
	Color _color = Color::WHITE;
	Color _boxBackground = Color(0, 0, 0, 0);
	Color _boxFrame = Color(0, 0, 0, 0);
	float _boxFrameWidth = 1.0f;
	uint8_t _boxMargin = 4;
	char _alignX = -1;
	char _alignY = -1;
	int _offsetX = 0;
	int _offsetY = 0;

	DebugTextParams& font(DebugFont font) { _font = font; return *this; }
	/*DebugTextParams& small() { _font = DebugFont::Small; return *this; }
	DebugTextParams& big() { _font = DebugFont::Big; return *this; }
	DebugTextParams& italic() { _font = DebugFont::Italic; return *this; }
	DebugTextParams& bold() { _font = DebugFont::Bold; return *this; }*/

	DebugTextParams& color(Color color) { _color = color; return *this; }
	DebugTextParams& background(Color color) { _boxBackground = color; return *this; }
	DebugTextParams& backgroundMargin(int margin) { _boxMargin = margin; }
	DebugTextParams& frame(Color color, float width = 1.0f) { _boxFrame = color; _boxFrameWidth = width; return *this; }

	DebugTextParams& alignX(int align) { _alignX = align; return *this; }
	DebugTextParams& alignY(int align) { _alignY = align; return *this; }
	DebugTextParams& left() { _alignX = -1; return *this; }
	DebugTextParams& center() { _alignX = 0; return *this; }
	DebugTextParams& right() { _alignX = 1; return *this; }
	DebugTextParams& top() { _alignY = -1; return *this; }
	DebugTextParams& middle() { _alignY = 0; return *this; }
	DebugTextParams& bottom() { _alignY = 1; return *this; }

	DebugTextParams& offset(int x, int y) { _offsetX = x; _offsetY = y; return *this; }
};

//--

// a line rendering context, all gathered lines will be written out as one batch (unless the flush() was used)
class ENGINE_RENDERING_API DebugDrawer : public DebugDrawerBase
{
public:
	DebugDrawer(DebugGeometry& dg, const Matrix& localToWorld = Matrix::IDENTITY());

    //--

    // draw a single line form point A to point B
    void line(const Vector3& a, const Vector3& b);

    // draw list of 2D lines, active color and sub selection ID is used
    void lines(const Vector2* points, uint32_t numPoints);

    // draw list of 3D lines, active color and sub selection ID is used
    void lines(const Vector3* points, uint32_t numPoints);

    // draw list of 3D lines with custom colors
    void lines(const DebugVertex* points, uint32_t numPoints);

    // draw a line loop from 2D vertices, active color and sub selection ID is used
    void loop(const Vector2* points, uint32_t numPoints);

    // draw a line loop from 3D vertices, active color and sub selection ID is used
    void loop(const Vector3* points, uint32_t numPoints);

    // draw line loop from 3D vertices with custom colors
    void loop(const DebugVertex* points, uint32_t numPoints);

    // draw an arrow with a head from "start" point to the "end" point
    void arrow(const Vector3& start, const Vector3& end);

    // draw a 3-axis XYZ coordinate system, the axes are colored Red, Green, Blue
    void axes(const Matrix& additionalTransform, float length = 0.5f);

    // draw a 3-axis XYZ coordinate system, the axes are colored Red, Green, Blue
    void axes(const Vector3& origin, const Vector3& x, const Vector3& y, const Vector3& z, float length = 0.5f);

    // add box bracket (corners highlight)
    void brackets(const Vector3* corners, float length = 0.1f);

    // add box bracket (corners highlight)
    void brackets(const Box& box, float length = 0.1f);
			
	//--

	// draw a polygon from 2D vertices
	void polygon(const Vector2* points, uint32_t numPoints, bool swap = false);

	// draw a polygon from 3D vertices
	void polygon(const Vector3* points, uint32_t numPoints, bool swap = false);

	// draw a polygon from 3D vertices with custom colors
	void polygon(const DebugVertex* points, uint32_t numPoints, bool swap = false);

	// draw indexed triangles made from 2D vertices
	void indexedTris(const Vector2* points, uint32_t numPoints, const uint32_t* indices, uint32_t numIndices);

	// draw indexed triangles made from 3D vertices
	void indexedTris(const Vector3* points, uint32_t numPoints, const uint32_t* indices, uint32_t numIndices);

	// draw indexed triangles made from 3D vertices with custom colors
	void indexedTris(const DebugVertex* points, uint32_t numPoints, const uint32_t* indices, uint32_t numIndices);

    //--

    // box shape - from 8 corners, can be any points actually (good for frustum)
    void wireBox(const Vector3* corners);
	void solidBox(const Vector3* corners);

    // box shape from extens
    void wireBox(const Vector3& boxMin, const Vector3& boxMax);
	void solidBox(const Vector3& boxMin, const Vector3& boxMax);

    // box shape
    void wireBox(const Box& box);
	void solidBox(const Box& box);

    // a sphere
    void wireSphere(const Vector3& center, float radius);
	void solidSphere(const Vector3& center, float radius);

    // capsule, oriented on Z axis
    void wireCapsule(const Vector3& center, float radius, float halfHeight);
	void solidCapsule(const Vector3& center, float radius, float halfHeight);

    // a cylinder, can be capped
    void wireCylinder(const Vector3& center1, const Vector3& center2, float radius1, float radius2);
	void solidCylinder(const Vector3& center1, const Vector3& center2, float radius1, float radius2);

    // cone from origin and direction
    void wireCone(const Vector3& top, const Vector3& dir, float radius, float angleDeg);
	void solidCone(const Vector3& top, const Vector3& dir, float radius, float angleDeg);

    // plane (grid of lines)
    void wirePlane(const Vector3& pos, const Vector3& normal, float size = 10.0f, int gridSize = 10);
	void solidPlane(const Vector3& pos, const Vector3& normal, float size = 10.0f, int gridSize = 10);

    // camera projection frustum
    void wireFrustum(const Matrix& frustumMatrix, float farPlaneScale = 1.0f);
	void solidFrustum(const Matrix& frustumMatrix, float farPlaneScale = 1.0f);

    // a simple 2D circle
    void wireCircle(const Vector3& center, const Vector3& normal, float radius, float startAngle = 0.0f, float endAngle = 360.0);
	void solidCircle(const Vector3& center, const Vector3& normal, float radius, float startAngle = 0.0f, float endAngle = 360.0);

    //--

    // 2D on-screen rectangle
    void wireRect(float x0, float y0, float w, float h);
    void wireRecti(int x0, int  y0, int  w, int  h);
	void solidRect(float x0, float y0, float w, float h);
	void solidRecti(int x0, int  y0, int  w, int  h);

    void wireBounds(float x0, float y0, float x1, float y1);
    void wireBoundsi(int  x0, int  y0, int  x1, int  y1);
	void solidBounds(float x0, float y0, float x1, float y1);
	void solidBoundsi(int x0, int  y0, int  x1, int  y1);

	//--

	// draw simple debug text 2d screen position
	Point text(int x, int y, StringView txt, const DebugTextParams& params = DebugTextParams());

	//--

	// draw screen aligned sprite
	//void sprite(float x, float y, float size);

	//--

};

//--

END_BOOMER_NAMESPACE_EX(rendering)

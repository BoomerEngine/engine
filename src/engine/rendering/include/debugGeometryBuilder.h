/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\debug #]
***/

#pragma once

#include "debugGeometry.h"

BEGIN_BOOMER_NAMESPACE()

//--

struct DebugVertex;

//--

// base debug geometry builder
class ENGINE_RENDERING_API DebugGeometryBuilderBase : public NoCopy
{
public:
	DebugGeometryBuilderBase(DebugGeometryLayer layer);

    //--

    INLINE DebugGeometryLayer layer() const { return m_layer; }

    INLINE void selectable(Selectable selectable) { m_selectable = selectable; }
    INLINE void size(float size = 1.0f) { m_size = size; }
    INLINE void color(Color color) { m_color = color; }

    INLINE void shading(bool flag) { m_solidShading = flag; updateFlags(); }
    INLINE void edges(bool flag) { m_solidEdges = flag; updateFlags(); }

    INLINE const DebugVertex* vertices() const { return m_verticesData.typedData(); }
    INLINE const uint32_t* indices() const { return m_indicesData.typedData(); }

    INLINE uint32_t vertexCount() const { return m_verticesData.size(); }
    INLINE uint32_t indexCount() const { return m_indicesData.size(); }

    //--

    // clear geometry
    void clear();

    //--

    // build a geometry chunk
    DebugGeometryChunkPtr buildChunk(gpu::ImageSampledView* customImageView = nullptr, bool autoReset = true);

    //--

protected:
    static const uint32_t STACK_VERTICES = 4096;
    static const uint32_t STACK_INDICES = 6146;

    void updateFlags();

    DebugGeometryLayer m_layer;

    Color m_color = Color::WHITE;
    float m_size = 1.0f;
    uint32_t m_flags = 0;
    Selectable m_selectable;

    bool m_solidShading = false;
    bool m_solidEdges = false;

    InplaceArray<DebugVertex, STACK_VERTICES> m_verticesData;
    InplaceArray<uint32_t, STACK_INDICES> m_indicesData;
};

//--

// a line rendering context, all gathered lines will be written out as one batch (unless the flush() was used)
class ENGINE_RENDERING_API DebugGeometryBuilder : public DebugGeometryBuilderBase
{
public:
	DebugGeometryBuilder(DebugGeometryLayer layer = DebugGeometryLayer::SceneSolid, const Matrix& localToWorld = Matrix::IDENTITY());
    ~DebugGeometryBuilder();

    //--

    // set transform for current geometry generation
    void localToWorld(const Matrix& localToWorld);

    //--

	// append a single vertex
	uint32_t appendVertex(const Vector2& pos, float z=0.0f, float u=0.0f, float v=0.0f);
	uint32_t appendVertex(const Vector3& pos, float u = 0.0f, float v = 0.0f);
	uint32_t appendVertex(float x, float y, float z, float u = 0.0f, float v = 0.0f);

	//--

	// append single line segment
	void appendWire(uint32_t a, uint32_t b);

	// append a wireframe triangle (a-b, b-c, c-a)
    void appendWireTriangle(uint32_t a, uint32_t b, uint32_t c);

    // append a wireframe quad
    void appendWireQuad(uint32_t a, uint32_t b, uint32_t c, uint32_t d);

	// append a general polygon (line loop)
	void appendWirePoly(uint32_t firstPoint, const uint32_t* points, uint32_t numPoints);

    // append a general polygon (line loop)
    void appendWirePoly(uint32_t firstPoint, uint32_t numPoints);

    // append a general polygon (line loop) with a center point
    void appendWirePolyWithCenter(uint32_t firstPoint, uint32_t numPoints, uint32_t center);

	//--

	// append solid triangle
	void appendSolidTriangle(uint32_t a, uint32_t b, uint32_t c);

    // append solid quad
    void appendSolidQuad(uint32_t a, uint32_t b, uint32_t c, uint32_t d);

	// append a general polygon (triangle fan)
    void appendSolidPoly(uint32_t firstPoint, const uint32_t* points, uint32_t numPoints, bool flip = false);

    // append a general polygon (triangle fan)
    void appendSolidPoly(uint32_t firstPoint, uint32_t numPoints, bool flip = false);

    // append a general polygon (triangle fan) but centered at given point
    void appendSolidPolyWithCenter(uint32_t firstPoint, uint32_t numPoints, uint32_t center, bool flip=false);

	//--

    // direct line between points
    void wire(const Vector3& start, const Vector3& end);

    // line segment
    void wire(const Vector3* pos, uint32_t count, bool closed=false);

    // wire frame arrow from "start" to "end", sizes are as precentage relative to length
    void wireArrow(const Vector3& start, const Vector3& end, float headPos = 0.8f, float headSize = 0.05f);

    // 3-axis XYZ coordinate system, the axes are colored Red, Green, Blue
    void wireAxes(const Vector3& origin, const Vector3& x, const Vector3& y, const Vector3& z, float length = 0.5f);
    void wireAxes(const Matrix& space, float length = 0.5f);

	// wireframe box brackets (corners highlight)
    void wireBrackets(const Box& box, float length = 0.1f);
    void wireBrackets(const Vector3* corners, float length = 0.1f);

	// wireframe box
    void wireBox(const Box& box);
    void wireBox(const Vector3& boxMin, const Vector3& boxMax);
	void wireBox(const Vector3* corners);

    // wireframe sphere
    void wireSphere(const Vector3& center, float radius);

    // wireframe hemisphere
    void wireHemiSphere(const Vector3& center, const Vector3& normal, float radius);
    
    // capsule between two points
    void wireCapsule(const Vector3& a, const Vector3& b, float radius);

	// cylinder/cone between two points, NOTE: radius can be zero
	void wireCylinder(const Vector3& a, const Vector3& b, float radiusA, float radiusB, bool capA=true, bool capB = true);

	// simple wire grid at origin, snapping U,V vectors
	void wireGrid(const Vector3& o, const Vector3& u, const Vector3& v, uint32_t count);

    // simple wire grid at point, facing a given normal
    void wireGrid(const Vector3& o, const Vector3& n, float size, uint32_t count);

    // circle/disk
    void wireCircle(const Vector3& center, const Vector3& normal, float radius, bool outlineOnly);
	void wireCircle(const Vector3& center, const Vector3& normal, float radius, float startAngle, float endAngle, bool outlineOnly);

    // circle/disk
    void wireCircleCut(const Vector3& center, const Vector3& normal, float innerRadius, float outerRadius, bool outlineOnly);
    void wireCircleCut(const Vector3& center, const Vector3& normal, float innerRadius, float outerRadius, float startAngle, float endAngle, bool outlineOnly);

    // arbitrary ellipse
    void wireEllipse(const Vector3& o, const Vector3& u, const Vector3& v, float innerRadius, float outerRadius, bool outlineOnly); // ellipse
    void wireEllipse(const Vector3& o, const Vector3& u, const Vector3& v, float innerRadius, float outerRadius, float startAngle, float endAngle, bool outlineOnly);

	//--

	// solid box
    void solidBox(const Box& box);
    void solidBox(const Vector3& boxMin, const Vector3& boxMax);
    void solidBox(const Vector3* corners);

    // solid sphere
    void solidSphere(const Vector3& center, float radius);

    // solid hemisphere
    void solidHemiSphere(const Vector3& center, const Vector3& normal, float radius);

    // solid capsule between two points
    void solidCapsule(const Vector3& a, const Vector3& b, float radius);

    // cylinder/cone between two points, NOTE: radius can be zero
    void solidCylinder(const Vector3& a, const Vector3& b, float radiusA, float radiusB, bool capA = true, bool capB = true);

    // solid arrow from "start" to "end", sizes are as precentage relative to length
    void solidArrow(const Vector3& start, const Vector3& end, float radius = 0.02f, float headPos = 0.8f, float headSizeMult = 2.0f);

    // solid circle/disk
    void solidCircle(const Vector3& center, const Vector3& normal, float radius);
    void solidCircle(const Vector3& center, const Vector3& normal, float radius, float startAngle, float endAngle);

    // circle/disk
    void solidCircleCut(const Vector3& center, const Vector3& normal, float innerRadius, float outerRadius);
    void solidCircleCut(const Vector3& center, const Vector3& normal, float innerRadius, float outerRadius, float startAngle, float endAngle);

    // solid arbitrary ellipse
    void solidEllipse(const Vector3& o, const Vector3& u, const Vector3& v, float innerRadius, float outerRadius);// ellipse
    void solidEllipse(const Vector3& o, const Vector3& u, const Vector3& v, float innerRadius, float outerRadius, float startAngle, float endAngle);

    //--

    // add a single sprite
    void sprite(const Vector3& o);

    // add list of sprites
    void sprites(const Vector3* pos, uint32_t count);

    //--

protected:
	Matrix m_localToWorld;
	BaseTransformation m_localToWorldTrans;
	bool m_localToWorldSet = false;

	bool pushCapsule(const Vector3& a, const Vector3& b, float radius, uint32_t& outVTop, uint32_t& outVBottom, bool& outHasCenter);
	void pushCylinder(const Vector3& a, const Vector3& b, float radiusA, float radiusB, uint32_t& outVTop, uint32_t& outVBottom, bool& outTPoint, bool& outBPoint, uint32_t& outCount);
	void pushCircle(const Vector3& o, const Vector3& u, const Vector3& v, uint32_t& outFirst, uint32_t& outCount);
    bool pushCircle(const Vector3& o, const Vector3& u, const Vector3& v, float startAngle, float endAngle, uint32_t& outFirst, uint32_t& outCount);

	void pushWireCylinder(uint32_t tfirst, const uint16_t* tindices, uint32_t bfirst, const uint16_t* bindices, uint32_t count);
	void pushWireCylinder(uint32_t tfirst, uint32_t bfirst, uint32_t count);
	void pushWireCone(uint32_t top, uint32_t bfirst, uint32_t count);
    void pushWireIndices(uint32_t first, const uint16_t* indices, const uint32_t count);

    void pushSolidCylinder(uint32_t tfirst, const uint16_t* tindices, uint32_t bfirst, const uint16_t* bindices, uint32_t count, bool flip = false);
    void pushSolidCylinder(uint32_t tfirst, uint32_t bfirst, uint32_t count, uint32_t topCenter, uint32_t bottomCenter, bool flip=false);
    void pushSolidCone(uint32_t top, uint32_t bfirst, uint32_t count, uint32_t centerVertex, bool flip = false);
    void pushSolidIndices(uint32_t first, const uint16_t* indices, const uint32_t count, bool flip=false);

    INLINE void writeVertex(DebugVertex* vt, const Vector3& pos, float u=0.0f, float v=0.0f)
    {
        vt->p = pos;
        vt->c = m_color;
        vt->f = m_flags;
        vt->w = m_size;
        vt->t.x = u;
        vt->t.y = v;
        vt->s = m_selectable;
    }
};

//--

class ENGINE_RENDERING_API DebugGeometryBuilderScreen : public DebugGeometryBuilderBase
{
public:
    DebugGeometryBuilderScreen();

    //--

    INLINE void textAlignV(int align) { m_textAlignV = align; }
    INLINE void textAlignH(int align) { m_textAlignH = align; }

    INLINE void fontSize(int size) { m_fontSize = size; }
    INLINE void fontBold(bool flag) { m_fontBold = flag; }
    INLINE void fontItalic(bool flag) { m_fontItalic = flag; }

    //--

    // rectangle
    void rect(const Rect& r, float u0 = 0.0f, float v0 = 0.0f, float u1 = 1.0f, float v1 = 1.0f);
    void rect(const Point& min, const Point& max, float u0 = 0.0f, float v0 = 0.0f, float u1 = 1.0f, float v1 = 1.0f);
    void rect(int x, int y, int w, int h, float u0 = 0.0f, float v0 = 0.0f, float u1 = 1.0f, float v1 = 1.0f);
    void rect2(int x0, int y0, int x1, int y1, float u0 = 0.0f, float v0 = 0.0f, float u1 = 1.0f, float v1 = 1.0f);

    // frame - built from one pixel rectangle to avoid switching geometry type
    void frame(const Rect& r);
    void frame(const Point& min, const Point& max);
    void frame(int x, int y, int w, int h);
    void frame2(int x0, int y0, int x1, int y1);

    // a line
    void line(const Vector2& a, const Vector2& b);
    void line(float x0, float y0, float x1, float y1);
    
    // measure text
    Point measureText(StringView txt) const;

    // print text, leaves the text advanced
    void text(StringView txt);

    //--

    // calculate the min/max of all vertices
    void calcBounds(Rect& outBounds) const;

    //--

private:
    int m_fontSize = 16;
    bool m_fontBold = false;
    bool m_fontItalic = false;
    int m_textAlignV = -1;
    int m_textAlignH = -1;
};

//--

END_BOOMER_NAMESPACE()

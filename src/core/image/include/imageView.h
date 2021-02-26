/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: image #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE_EX(image)

enum ENativeLayout { NATIVE_LAYOUT };

/// view on an image content
class CORE_IMAGE_API ImageView
{
public:
    INLINE ImageView() = default; // empty
    INLINE ImageView(PixelFormat format, uint8_t channels, const void* data, uint32_t width, uint32_t pixelPitch); // 1D slice
    INLINE ImageView(PixelFormat format, uint8_t channels, const void* data, uint32_t width, uint32_t height, uint32_t pixelPitch, uint32_t rowPitch); // 2D slice
    INLINE ImageView(PixelFormat format, uint8_t channels, const void* data, uint32_t width, uint32_t height, uint32_t depth, uint32_t pixelPitch, uint32_t rowPitch, uint32_t slicePitch); // 3D slice

    // build a "packed" view layout
    INLINE ImageView(ENativeLayout, PixelFormat format, uint8_t channels, const void* data, uint32_t width); // 1D slice
    INLINE ImageView(ENativeLayout, PixelFormat format, uint8_t channels, const void* data, uint32_t width, uint32_t height); // 2D slice
    INLINE ImageView(ENativeLayout, PixelFormat format, uint8_t channels, const void* data, uint32_t width, uint32_t height, uint32_t depth); // 3D slice
            

    INLINE ImageView(const ImageView& other) = default;
    INLINE ImageView(ImageView&& other) = default;
    INLINE ImageView& operator=(const ImageView& other) = default;
    INLINE ImageView& operator=(ImageView&& other) = default;

    //--

    INLINE PixelFormat format() const { return m_format; }

    INLINE uint8_t channels() const { return m_channels; }

    INLINE uint32_t width() const { return m_width; }
    INLINE uint32_t height() const { return m_height; }
    INLINE uint32_t depth() const { return m_depth; }

    INLINE uint32_t pixelPitch() const { return m_pixelPitch; }
    INLINE uint32_t rowPitch() const { return m_rowPitch; }
    INLINE uint32_t slicePitch() const { return m_slicePitch; }

    INLINE void* data() { return m_data; }
    INLINE const void* data() const { return m_data; }

    INLINE bool empty() const { return m_data == nullptr || !m_width || !m_height || !m_depth; }

    //--

    // compute data size needed to store pixels in this view
    INLINE uint64_t dataSize() const;

    // extract data as buffer
    Buffer toBuffer(PoolTag id = POOL_IMAGE) const;

    // copy out the buffer into a packed memory 
    // NOTE: memory must have at least dataSize() bytes
    void copy(void* memPtr) const;

    // get pointer to data at given pixel coordinates
    INLINE void* pixelPtr(uint32_t x); // 1D
    INLINE void* pixelPtr(uint32_t x, uint32_t y); // 2D
    INLINE void* pixelPtr(uint32_t x, uint32_t y, uint32_t z); // 3D

    // get pointer to data at given pixel coordinates
    INLINE const void* pixelPtr(uint32_t x) const;
    INLINE const void* pixelPtr(uint32_t x, uint32_t y) const;
    INLINE const void* pixelPtr(uint32_t x, uint32_t y, uint32_t z) const;

    // get a sub view
    ImageView subView(uint32_t x, uint32_t width) const;
    ImageView subView(uint32_t x, uint32_t y, uint32_t width, uint32_t height) const;
    ImageView subView(uint32_t x, uint32_t y, uint32_t z, uint32_t width, uint32_t height, uint32_t depth) const;

    //--

private:
    void* m_data = nullptr;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    uint32_t m_depth = 0;
    uint32_t m_pixelPitch = 0;
    uint32_t m_rowPitch = 0;
    uint32_t m_slicePitch = 0;
    uint8_t m_channels = 0;
    PixelFormat m_format = PixelFormat::Uint8_Norm;
};

//--

static uint32_t PixelPitchForFormat(PixelFormat pf, uint8_t channels)
{
    switch (pf)
    {
        case PixelFormat::Uint8_Norm: return channels; break;
        case PixelFormat::Uint16_Norm: return 2 * channels; break;
        case PixelFormat::Float16_Raw: return 2 * channels; break;
        case PixelFormat::Float32_Raw: return 4 * channels; break;
    }

    return 0;
}

INLINE ImageView::ImageView(PixelFormat format, uint8_t channels, const void* data, uint32_t width, uint32_t pixelPitch)
    : m_format(format)
    , m_channels(channels)
    , m_data((void*)data)
    , m_width(width)
    , m_height(1)
    , m_depth(1)
    , m_pixelPitch(pixelPitch)
{
    m_rowPitch = m_width * m_pixelPitch;
    m_slicePitch = m_height * m_rowPitch;
}

INLINE ImageView::ImageView(PixelFormat format, uint8_t channels, const void* data, uint32_t width, uint32_t height, uint32_t pixelPitch, uint32_t rowPitch)
    : m_format(format)
    , m_channels(channels)
    , m_data((void*)data)
    , m_width(width)
    , m_height(height)
    , m_depth(1)
    , m_pixelPitch(pixelPitch)
    , m_rowPitch(rowPitch)
{
    m_slicePitch = m_height * m_rowPitch;
}

INLINE ImageView::ImageView(PixelFormat format, uint8_t channels, const void* data, uint32_t width, uint32_t height, uint32_t depth, uint32_t pixelPitch, uint32_t rowPitch, uint32_t slicePitch)
    : m_format(format)
    , m_channels(channels)
    , m_data((void*)data)
    , m_width(width)
    , m_height(height)
    , m_depth(depth)
    , m_pixelPitch(pixelPitch)
    , m_rowPitch(rowPitch)
    , m_slicePitch(slicePitch)
{}

INLINE ImageView::ImageView(ENativeLayout, PixelFormat format, uint8_t channels, const void* data, uint32_t width)
    : m_format(format)
    , m_channels(channels)
    , m_data((void*)data)
    , m_width(width)
    , m_height(1)
    , m_depth(1)
{            
    m_pixelPitch = PixelPitchForFormat(format, channels);
    m_rowPitch = m_pixelPitch * m_width;
    m_slicePitch = m_rowPitch * m_height;
}

INLINE ImageView::ImageView(ENativeLayout, PixelFormat format, uint8_t channels, const void* data, uint32_t width, uint32_t height)
    : m_format(format)
    , m_channels(channels)
    , m_data((void*)data)
    , m_width(width)
    , m_height(height)
    , m_depth(1)
{
    m_pixelPitch = PixelPitchForFormat(format, channels);
    m_rowPitch = m_pixelPitch * m_width;
    m_slicePitch = m_rowPitch * m_height;
}

INLINE ImageView::ImageView(ENativeLayout, PixelFormat format, uint8_t channels, const void* data, uint32_t width, uint32_t height, uint32_t depth)
    : m_format(format)
    , m_channels(channels)
    , m_data((void*)data)
    , m_width(width)
    , m_height(height)
    , m_depth(depth)
{
    m_pixelPitch = PixelPitchForFormat(format, channels);
    m_rowPitch = m_pixelPitch * m_width;
    m_slicePitch = m_rowPitch * m_height;
}

INLINE uint64_t ImageView::dataSize() const
{
    return m_pixelPitch * m_width * m_height * m_depth;
}

INLINE void* ImageView::pixelPtr(uint32_t x)
{
    return OffsetPtr(m_data, x * m_pixelPitch);
}

INLINE void* ImageView::pixelPtr(uint32_t x, uint32_t y)
{
    return OffsetPtr(m_data, x * m_pixelPitch + y * m_rowPitch);
}

INLINE void* ImageView::pixelPtr(uint32_t x, uint32_t y, uint32_t z)
{
    return OffsetPtr(m_data, x * m_pixelPitch + y * m_rowPitch + z * m_slicePitch);
}

INLINE const void* ImageView::pixelPtr(uint32_t x) const
{
    return OffsetPtr(m_data, x * m_pixelPitch);
}

INLINE const void* ImageView::pixelPtr(uint32_t x, uint32_t y) const
{
    return OffsetPtr(m_data, x * m_pixelPitch + y * m_rowPitch);
}

INLINE const void* ImageView::pixelPtr(uint32_t x, uint32_t y, uint32_t z) const
{
    return OffsetPtr(m_data, x * m_pixelPitch + y * m_rowPitch + z * m_slicePitch);
}


//--

struct ImageViewPixelIterator;
struct ImageViewRowIterator;

//--

// slice iterator
struct ImageViewSliceIterator : public NoCopy
{
public:
    INLINE ImageViewSliceIterator() = default;

    INLINE ImageViewSliceIterator(const ImageView& view)
    {
        m_slicePitch = view.slicePitch();
        m_pixelPitch = view.pixelPitch();
        m_rowPitch = view.rowPitch();
        m_width = view.width();
        m_height = view.height();

        m_slicePtr = (uint8_t*)view.data();
        m_endSlicePtr = m_slicePtr + view.depth() * m_slicePitch;
    }

    INLINE ImageViewSliceIterator(const ImageView& view, uint32_t first, uint32_t last)
    {
        first = std::min(first, view.depth());
        last = std::max(first, std::min(last, view.depth()));

        m_pixelPitch = view.pixelPitch();
        m_rowPitch = view.rowPitch();
        m_slicePitch = view.slicePitch();
        m_width = view.width();
        m_height = view.height();

        m_slicePtr = (uint8_t*)view.data() + first * m_slicePitch;
        m_endSlicePtr = (uint8_t*)view.data() + last * m_slicePitch;
        m_z = first;
    }

    INLINE operator bool() const
    {
        return m_slicePtr < m_endSlicePtr;
    }

    INLINE uint32_t pos() const
    {
        return m_z;
    }

    INLINE uint8_t* data() const
    {
        return m_slicePtr;
    }

    INLINE void operator++()
    {
        m_slicePtr += m_slicePitch;
        m_z += 1;
    }

    INLINE void operator++(int)
    {
        m_slicePtr += m_slicePitch;
        m_z += 1;
    }

    INLINE uint32_t width() const
    {
        return m_width;
    }

    INLINE uint32_t height() const
    {
        return m_height;
    }

    INLINE uint32_t rowPitch() const
    {
        return m_rowPitch;
    }

    INLINE uint32_t pixelPitch() const
    {
        return m_pixelPitch;
    }

private:
    uint8_t* m_slicePtr = nullptr;
    uint8_t* m_endSlicePtr = nullptr;

    uint32_t m_slicePitch = 0;
    uint32_t m_rowPitch = 0;
    uint32_t m_z = 0;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    uint8_t m_pixelPitch = 0;
};
        
//--

// row iterator
struct ImageViewRowIterator : public NoCopy
{
public:
    INLINE ImageViewRowIterator() = default;

    INLINE ImageViewRowIterator(const ImageView& view)
    {
        m_pixelPitch = view.pixelPitch();
        m_rowPitch = view.rowPitch();
        m_width = view.width();

        m_rowPtr = (uint8_t*)view.data();
        m_endRowPtr = m_rowPtr + view.height() * m_rowPitch;
    }

    INLINE ImageViewRowIterator(const ImageViewSliceIterator& slice)
    {
        m_pixelPitch = slice.pixelPitch();
        m_rowPitch = slice.rowPitch();
        m_width = slice.width();

        m_rowPtr = (uint8_t*)slice.data();
        m_endRowPtr = m_rowPtr + slice.height() * m_rowPitch;
    }

    INLINE ImageViewRowIterator(const ImageView& view, uint32_t first, uint32_t last)
    {
        first = std::min(first, view.height());
        last = std::max(first, std::min(last, view.height()));

        m_pixelPitch = view.pixelPitch();
        m_rowPitch = view.rowPitch();
        m_width = view.width();

        m_rowPtr = (uint8_t*)view.data() + first * m_rowPitch;
        m_endRowPtr = (uint8_t*)view.data() + last * m_rowPitch;
        m_y = first;
    }

    INLINE ImageViewRowIterator(const ImageViewSliceIterator& slice, uint32_t first, uint32_t last)
    {
        first = std::min(first, slice.height());
        last = std::max(first, std::min(last, slice.height()));

        m_pixelPitch = slice.pixelPitch();
        m_rowPitch = slice.rowPitch();
        m_width = slice.width();

        m_rowPtr = (uint8_t*)slice.data() + first * m_rowPitch;
        m_endRowPtr = (uint8_t*)slice.data() + last * m_rowPitch;
        m_y = first;
    }

    INLINE operator bool() const
    {
        return m_rowPtr < m_endRowPtr;
    }

    INLINE uint32_t pos() const
    {
        return m_y;
    }

    INLINE uint8_t* data() const
    {
        return m_rowPtr;
    }

    INLINE void operator++()
    {
        m_rowPtr += m_rowPitch;
        m_y += 1;
    }

    INLINE void operator++(int)
    {
        m_rowPtr += m_rowPitch;
        m_y += 1;
    }

    INLINE uint32_t width() const
    {
        return m_width;
    }

    INLINE uint32_t pixelPitch() const
    {
        return m_pixelPitch;
    }

private:
    uint8_t* m_rowPtr = nullptr;
    uint8_t* m_endRowPtr = nullptr;
    uint32_t m_rowPitch = 0;
    uint32_t m_y = 0;
    uint32_t m_width = 0;
    uint8_t m_pixelPitch = 0;
};

//--

// pixel iterator
struct ImageViewPixelIterator : public NoCopy
{
public:
    INLINE ImageViewPixelIterator() = default;

    INLINE ImageViewPixelIterator(const ImageView& view)
    {
        m_pixelPtr = (uint8_t*)view.data();
        m_pixelPitch = view.pixelPitch();
        m_endPixelPtr = m_pixelPtr + view.width() * m_pixelPitch;
        m_x = 0;
    }

    INLINE ImageViewPixelIterator(const ImageViewRowIterator& row)
    {
        m_pixelPtr = (uint8_t*)row.data();
        m_pixelPitch = row.pixelPitch();
        m_endPixelPtr = m_pixelPtr + row.width() * m_pixelPitch;
        m_x = 0;
    }

    INLINE ImageViewPixelIterator(const ImageView& view, uint32_t first, uint32_t last)
    {
        first = std::min(first, view.width());
        last = std::max(first, std::min(last, view.width()));

        m_pixelPitch = view.pixelPitch();
        m_pixelPtr = (uint8_t*)view.data() + first * m_pixelPitch;
        m_endPixelPtr = (uint8_t*)view.data() + last * m_pixelPitch;
        m_x = first;
    }

    INLINE ImageViewPixelIterator(const ImageViewRowIterator& row, uint32_t first, uint32_t last)
    {
        first = std::min(first, row.width());
        last = std::max(first, std::min(last, row.width()));

        m_pixelPitch = row.pixelPitch();
        m_pixelPtr = row.data() + first * m_pixelPitch;
        m_endPixelPtr = row.data() + last * m_pixelPitch;
        m_x = first;
    }

    INLINE operator bool() const
    {
        return m_pixelPtr < m_endPixelPtr;
    }

    INLINE uint32_t pos() const
    {
        return m_x;
    }

    INLINE uint8_t* data() const
    {
        return m_pixelPtr;
    }

    INLINE void operator++()
    {
        m_pixelPtr += m_pixelPitch;
        m_x += 1;
    }

    INLINE void operator++(int)
    {
        m_pixelPtr += m_pixelPitch;
        m_x += 1;
    }

private:
    uint8_t* m_pixelPtr = nullptr;
    uint8_t* m_endPixelPtr = nullptr;
    uint8_t m_pixelPitch = 0;
    uint32_t m_x = 0;
};

//--

// typed pixel iterator
template< typename T >
struct ImageViewPixelIteratorT : public NoCopy
{
public:
    INLINE ImageViewPixelIteratorT() = default;

    INLINE ImageViewPixelIteratorT(const ImageView& view)
    {
        m_pixelPtr = (uint8_t*)view.data();
        m_endPixelPtr = m_pixelPtr + view.width() * m_pixelPitch;
        m_x = 0;
    }

    INLINE ImageViewPixelIteratorT(const ImageViewRowIterator& row)
    {
        m_pixelPtr = (uint8_t*)row.data();
        m_endPixelPtr = m_pixelPtr + row.width() * sizeof(T);
        m_x = 0;
    }

    INLINE ImageViewPixelIteratorT(const ImageView& view, uint32_t first, uint32_t last)
    {
        first = std::min(first, view.width());
        last = std::max(first, std::min(last, view.width()));

        m_pixelPtr = (uint8_t*)view.data() + first * sizeof(T);
        m_endPixelPtr = (uint8_t*)view.data() + last * sizeof(T);
        m_x = first;
    }

    INLINE ImageViewPixelIteratorT(const ImageViewRowIterator& row, uint32_t first, uint32_t last)
    {
        first = std::min(first, row.width());
        last = std::max(first, std::min(last, row.width()));

        m_pixelPtr = row.data() + first * sizeof(T);
        m_endPixelPtr = row.data() + last * sizeof(T);
        m_x = first;
    }

    INLINE operator bool() const
    {
        return m_pixelPtr < m_endPixelPtr;
    }

    INLINE uint32_t pos() const
    {
        return m_x;
    }

    INLINE T& data() const
    {
        return *(T*)m_pixelPtr;
    }

    INLINE void operator++()
    {
        m_pixelPtr += sizeof(T);
        m_x += 1;
    }

    INLINE void operator++(int)
    {
        m_pixelPtr += sizeof(T);
        m_x += 1;
    }

private:
    uint8_t* m_pixelPtr = nullptr;
    uint8_t* m_endPixelPtr = nullptr;
    uint32_t m_x = 0;
};

//--

END_BOOMER_NAMESPACE_EX(image)

///--------------------------------------------------------------------------

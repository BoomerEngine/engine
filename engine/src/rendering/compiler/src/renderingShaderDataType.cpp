/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler\data #]
***/

#include "build.h"
#include "renderingShaderDataType.h"
#include "renderingShaderTypeLibrary.h"
#include "renderingShaderFunction.h"
#include "renderingShaderProgram.h"

#include "base/containers/include/stringBuilder.h"

namespace rendering
{
    namespace compiler
    {

        //--

        ArrayCounts::ArrayCounts()
        {
            for (uint32_t i = 0; i < MAX_ARRAY_DIMS; ++i)
                m_sizes[i] = 0;
        }

        ArrayCounts::ArrayCounts(uint32_t dim)
        {
            for (uint32_t i = 0; i < MAX_ARRAY_DIMS; ++i)
                m_sizes[i] = 0;
            m_sizes[0] = range_cast<uint16_t>(dim);
        }

        ArrayCounts::ArrayCounts(const ArrayCounts& baseDim)
        {
            for (uint32_t i = 0; i < MAX_ARRAY_DIMS; ++i)
                m_sizes[i] = baseDim.m_sizes[i];
        }

        ArrayCounts ArrayCounts::innerCounts() const
        {
            ASSERT(!empty());

            ArrayCounts ret;
            for (uint32_t i = 1; i < MAX_ARRAY_DIMS; ++i)
                ret.m_sizes[i - 1] = m_sizes[i];
            return ret;
        }

        uint32_t ArrayCounts::outermostCount() const
        {
            return (uint32_t)std::max<int>(m_sizes[0], 1);
        }

        int ArrayCounts::arraySize(const uint32_t dimmension /*= 0*/) const
        {
            ASSERT(dimmension < MAX_ARRAY_DIMS);
            ASSERT(m_sizes[dimmension] != 0);
            return m_sizes[dimmension];
        }

        ArrayCounts ArrayCounts::prependUndefinedArray() const
        {
            ASSERT(!full());

            ArrayCounts ret;
            ret.m_sizes[0] = COUNT_NOT_DEFINED;

            for (uint32_t i = 1; i < MAX_ARRAY_DIMS; ++i)
                ret.m_sizes[i] = m_sizes[i - 1];

            return ret;
        }

        ArrayCounts ArrayCounts::appendUndefinedArray() const
        {
            ASSERT(!full());

            ArrayCounts ret(*this);
            ret.m_sizes[ret.dimensionCount()] = COUNT_NOT_DEFINED;
            return ret;
        }

        ArrayCounts ArrayCounts::prependArray(uint32_t count) const
        {
            ASSERT(!full());
            ASSERT(count != 0);

            ArrayCounts ret;
            ret.m_sizes[0] = range_cast<short>(count);

            for (uint32_t i = 1; i < MAX_ARRAY_DIMS; ++i)
                ret.m_sizes[i] = m_sizes[i - 1];

            return ret;
        }

        ArrayCounts ArrayCounts::appendArray(uint32_t count) const
        {
            ASSERT(!full());
            ASSERT(count != 0);

            ArrayCounts ret(*this);
            ret.m_sizes[ret.dimensionCount()] = range_cast<short>(count);;
            return ret;
        }

        bool ArrayCounts::isSizeDefined() const
        {
            for (uint32_t i = 0; i < MAX_ARRAY_DIMS; ++i)
                if (m_sizes[i] == COUNT_NOT_DEFINED)
                    return false;

            return true;
        }
        
        uint8_t ArrayCounts::dimensionCount() const
        {
            uint8_t numDims = 0;
            while (numDims < MAX_ARRAY_DIMS)
            {
                if (0 == m_sizes[numDims])
                    break;
                ++numDims;
            }

            return numDims;
        }

        uint32_t ArrayCounts::elementCount() const
        {
            uint32_t numElems = 1;
            for (uint32_t i = 0; i < MAX_ARRAY_DIMS; ++i)
            {
                if (m_sizes[i] != 0)
                {
                    auto indexSize = m_sizes[i];
                    if (indexSize > 1)
                        numElems *= indexSize;
                }
                else
                {
                    break;
                }
            }

            return numElems;
        }

        void ArrayCounts::print(base::IFormatStream& f) const
        {
            for (uint32_t i = 0; i < MAX_ARRAY_DIMS; ++i)
            {
                if (m_sizes[i])
                    f.appendf("[{}]", m_sizes[i]);
                else
                    break;
            }
        }

		bool ArrayCounts::operator==(const ArrayCounts& other) const
		{
			for (uint32_t i = 0; i < MAX_ARRAY_DIMS; ++i)
				if (m_sizes[i] != other.m_sizes[i])
					return false;

			return true;
		}

		bool ArrayCounts::operator!=(const ArrayCounts& other) const
		{
			return !operator==(other);
		}

        uint64_t ArrayCounts::typeHash() const
        {
            base::CRC64 crc;

            for (uint32_t i = 0; i < MAX_ARRAY_DIMS; ++i)
                crc << m_sizes[i];

            return crc.crc();
        }

        bool ArrayCounts::StrictMatch(const ArrayCounts& a, const ArrayCounts& b)
        {
            for (uint32_t i = 0; i < MAX_ARRAY_DIMS; ++i)
                if (a.m_sizes[i] != b.m_sizes[i])
                    return false;

            return true;
        }

        bool ArrayCounts::GeneralMatch(const ArrayCounts& a, const ArrayCounts& b)
        {
            for (uint32_t i = 0; i < MAX_ARRAY_DIMS; ++i)
                if ((a.m_sizes[i] != 0) != (b.m_sizes[i] != 0))
                    return false;

            return true;
        }

        //---

        bool ResourceType::calcTypeHash(base::CRC64& crc) const
        {
            crc << (char)type;
            attributes.calcTypeHash(crc);
            return true;
        }

        uint8_t ResourceType::dimensions() const
        {
            if (type == DeviceObjectViewType::Image || type == DeviceObjectViewType::ImageWritable || type == DeviceObjectViewType::SampledImage)
            {
                switch (resolvedViewType)
                {
                case ImageViewType::View1D:
                case ImageViewType::View1DArray:
                    return 1;

                case ImageViewType::View2D:
                case ImageViewType::View2DArray:
                    return 2;

                case ImageViewType::ViewCube:
                case ImageViewType::ViewCubeArray:
                case ImageViewType::View3D:
                    return 3;
                }
            }
            else if (type == DeviceObjectViewType::Buffer || type == DeviceObjectViewType::BufferStructured 
				|| type == DeviceObjectViewType::BufferStructuredWritable || type == DeviceObjectViewType::BufferWritable)
            {
                return 1;
            }

            return 0;
        }

        uint8_t ResourceType::addressComponentCount() const
        {
            switch (resolvedViewType)
            {
            case ImageViewType::View1D:
                return 1;

            case ImageViewType::View2D:
            case ImageViewType::View1DArray:
                return 2;

            case ImageViewType::View3D:
            case ImageViewType::ViewCube:
            case ImageViewType::View2DArray:
                return 3;

            case ImageViewType::ViewCubeArray:
                return 4;
            }

            return 0;
        }

        uint8_t ResourceType::sizeComponentCount() const
        {
            switch (resolvedViewType)
            {
            case ImageViewType::View1D:
                return 1;

            case ImageViewType::ViewCube:
            case ImageViewType::ViewCubeArray:
            case ImageViewType::View2D:
            case ImageViewType::View1DArray:
                return 2;

            case ImageViewType::View3D:
            case ImageViewType::View2DArray:
                return 3;
            }

            return 0;
        }

        uint8_t ResourceType::offsetComponentCount() const
        {
            switch (resolvedViewType)
            {
            case ImageViewType::View1DArray:
            case ImageViewType::View1D:
                return 1;

            case ImageViewType::View2DArray:
            case ImageViewType::View2D:
                return 2;

            case ImageViewType::View3D:
                return 3;

            case ImageViewType::ViewCube:
            case ImageViewType::ViewCubeArray:
                return 0;
            }

            return 0;
        }

        void ResourceType::print(base::IFormatStream& f) const
        {
            f << type;

            if (!attributes.attributes.empty())
            {
                f << " ";
                attributes.print(f);
            }
        }

        //---

        DataType::DataType()
            : m_baseType(BaseType::Invalid)
        {}

        DataType::DataType(BaseType baseType)
            : m_baseType(baseType)            
        {
            // only simple types can be created directly
            ASSERT(baseType == BaseType::Void || baseType == BaseType::Float || baseType == BaseType::Int || baseType == BaseType::Uint || baseType == BaseType::Boolean);
        }

        DataType::DataType(const CompositeType* composite)
            : m_baseType(BaseType::Struct)
            , m_composite(composite)
        {
            ASSERT(composite != nullptr);
        }

        DataType::DataType(const Function* func)
            : m_baseType(BaseType::Function)
            , m_function(func)
        {
            ASSERT(m_function != nullptr);
        }

        DataType::DataType(const Program* program)
            : m_baseType(BaseType::Program)
            , m_program(program)
        {
            ASSERT(m_program != nullptr);
        }

        DataType::DataType(const ResourceType* res)
            : m_baseType(BaseType::Resource)
            , m_resource(res)
        {}

        void DataType::print(base::IFormatStream& ret) const
        {
            if (m_flags.test(TypeFlags::Pointer))
                ret.append("&");

            switch (m_baseType)
            {
                case BaseType::Invalid: ret.append("invalid"); break;
                case BaseType::Void: ret.append("void"); break;
                case BaseType::Float: ret.append("float"); break;
                case BaseType::Int: ret.append("int"); break;
                case BaseType::Uint: ret.append("uint"); break;
                case BaseType::Boolean: ret.append("bool"); break;
                case BaseType::Name: ret.append("name"); break;
                case BaseType::Function: ret.append("function "); break;
                case BaseType::Program: ret.append("program "); break;
                case BaseType::Struct: ret.append(m_composite->name().c_str()); break;
                case BaseType::Resource: ret.append("resource "); break;
            }

            if (m_function != nullptr)
            {
                ret << m_function->returnType() << "(";

                uint32_t numParams = 0;
                for (auto param  : m_function->inputParameters())
                {
                    if (numParams++>0)
                        ret.append(",");
                    ret << param->dataType;
                }
                ret << ")";
            }
            else if (m_program != nullptr)
            {
                ret.append(m_program->name().c_str());
            }
            else if (m_baseType == BaseType::Resource)
            {
            }

            if (!m_arrayCounts.empty())
                m_arrayCounts.print(ret);
        }

		bool DataType::operator==(const DataType& other) const
		{
			if (m_baseType != other.m_baseType)
				return false;
			if (m_arrayCounts != other.m_arrayCounts)
				return false;

			switch (m_baseType)
			{
				case BaseType::Function:
					return m_function == other.m_function;
				case BaseType::Struct:
					return m_composite == other.m_composite;
				case BaseType::Program:
					return m_program == other.m_program;
				case BaseType::Resource:
					return m_resource == other.m_resource;
			}

			return true;
		}

		bool DataType::operator!=(const DataType& other) const
		{
			return !operator==(other);
		}

		uint32_t DataType::CalcHash(const DataType& type)
		{
			base::CRC64 crc;
			type.calcTypeHash(crc);
			return crc.crc();
		}

        bool DataType::calcTypeHash(base::CRC64& crc) const
        {
            crc << (uint8_t)m_baseType;
            crc << m_flags.rawValue();
            crc << m_arrayCounts.typeHash();
            if (m_composite)
                crc << m_composite->typeHash();
            if (m_function)
                crc << m_function->name();
            if (nullptr != m_resource)
                m_resource->calcTypeHash(crc);
            return true;
        }

        bool DataType::isVector() const
        {
            return isComposite() && composite().hint() == CompositeTypeHint::VectorType;
        }

        bool DataType::isNumericalVectorLikeOperand(bool allowBool/* = false*/) const
        {
            if (isNumericalScalar())
                return true;

            if (isComposite() && composite().hint() == CompositeTypeHint::VectorType)
            {
                auto baseType = composite().members()[0].type.baseType();
                return (baseType == BaseType::Float) || (baseType == BaseType::Int) || (baseType == BaseType::Uint) || (allowBool && baseType == BaseType::Boolean);
            }

            return false;
        }

        bool DataType::isNumericalMatrixLikeOperand() const
        {
            if (isComposite() && composite().hint() == CompositeTypeHint::MatrixType)
            {
                auto innerType = composite().members()[0].type;
                if (innerType.baseType() == BaseType::Struct)
                {
                    auto baseType = innerType.composite().members()[0].type.baseType();
                    return (baseType == BaseType::Float) || (baseType == BaseType::Int) || (baseType == BaseType::Uint);
                }
            }

            return false;
        }

        bool DataType::isMatrix() const
        {
            return isComposite() && composite().hint() == CompositeTypeHint::MatrixType;
        }

        bool DataType::MatchNoFlags(const DataType& a, const DataType& b)
        {
            if (!ArrayCounts::StrictMatch(a.m_arrayCounts, b.m_arrayCounts))
                return false;

            if (a.m_baseType != b.m_baseType)
                return false;

            switch (a.m_baseType)
            {
                case BaseType::Struct:
                    return (a.m_composite == b.m_composite);

                case BaseType::Resource:
                    return (a.m_resource == b.m_resource);

                case BaseType::Program:
                    return (a.m_program == b.m_program);

                case BaseType::Function:
                {
                    if (a.m_function == b.m_function)
                        return true;

                    if (!MatchNoFlags(a.m_function->returnType(), b.m_function->returnType()))
                        return false;

                    auto& aArgs = a.m_function->inputParameters();
                    auto& bArgs = b.m_function->inputParameters();
                    if (aArgs.size() != bArgs.size())
                        return false;

                    for (uint32_t i=0; i<aArgs.size(); ++i)
                    {
                        if (!MatchNoFlags(aArgs[i]->dataType, bArgs[i]->dataType))
                            return false;
                    }
                    return true;
                }
            }

            return true;
        }

        DataType DataType::makePointer() const
        {
            DataType ret(*this);
            ret.m_flags.set(TypeFlags::Pointer);
            return ret;
        }

        DataType DataType::unmakePointer() const
        {
            DataType ret(*this);
            ret.m_flags.clear(TypeFlags::Pointer);
            return ret;
        }

        DataType DataType::makeAtomic() const
        {
            DataType ret(*this);
            ret.m_flags.set(TypeFlags::Atomic);
            return ret;
        }

        DataType DataType::unmakeAtomic() const
        {
            DataType ret(*this);
            ret.m_flags.clear(TypeFlags::Atomic);
            return ret;
        }

        DataType DataType::applyFlags(const base::DirectFlags<TypeFlags>& flags) const
        {
            DataType ret(*this);
            ret.m_flags = flags;
            return ret;
        }

        DataType DataType::applyArrayCounts(const ArrayCounts& arrayCounts) const
        {
            DataType ret(*this);
            ret.m_arrayCounts = arrayCounts;
            return ret;
        }

        DataType DataType::removeArrayCounts() const
        {
            DataType ret(*this);
            ret.m_arrayCounts = ArrayCounts();
            return ret;
        }
		
        DataType DataType::BoolScalarType()
        {
            DataType ret;
            ret.m_baseType = BaseType::Boolean;
            return ret;
        }

        DataType DataType::NameScalarType()
        {
            DataType ret;
            ret.m_baseType = BaseType::Name;
            return ret;
        }

        DataType DataType::IntScalarType()
        {
            DataType ret;
            ret.m_baseType = BaseType::Int;
            return ret;
        }

        DataType DataType::UnsignedScalarType()
        {
            DataType ret;
            ret.m_baseType = BaseType::Uint;
            return ret;
        }

        DataType DataType::FloatScalarType()
        {
            DataType ret;
            ret.m_baseType = BaseType::Float;
            return ret;
        }

        DataType DataType::ProgramType()
        {
            DataType ret;
            ret.m_baseType = BaseType::Program;
            return ret;
        }
        
        uint32_t DataType::computeScalarComponentCount() const
        {
            uint32_t baseCount = 1;
            if (m_baseType == BaseType::Struct)
                baseCount = m_composite->scalarComponentCount();

            return baseCount * m_arrayCounts.elementCount();
        }

    } // shader
} // rendering


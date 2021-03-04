/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: material #]
***/

#include "build.h"
#include "parameters.h"

#include "engine/texture/include/texture.h"

BEGIN_BOOMER_NAMESPACE()

//---

RTTI_BEGIN_TYPE_CLASS(MaterialTemplateParameter_StaticBool);
RTTI_CATEGORY("Value");
RTTI_PROPERTY(m_defaultValue).editable("Default value of the static boolean");
RTTI_END_TYPE();

MaterialTemplateParameter_StaticBool::MaterialTemplateParameter_StaticBool()
{}

MaterialDataLayoutParameterType MaterialTemplateParameter_StaticBool::queryType() const
{
    return MaterialDataLayoutParameterType::StaticBool;
}

Type MaterialTemplateParameter_StaticBool::queryDataType() const
{
    return GetTypeObject<bool>();
}

const void* MaterialTemplateParameter_StaticBool::queryDefaultValue() const
{
    return &m_defaultValue;
}

//---

RTTI_BEGIN_TYPE_CLASS(MaterialTemplateParameter_Scalar);
RTTI_CATEGORY("Value");
RTTI_PROPERTY(m_defaultValue).editable("Default value of the scalar parameter");
RTTI_CATEGORY("Limit");
RTTI_PROPERTY(m_limited).editable("Don't allow user to setup any value");
RTTI_PROPERTY(m_limitMin).editable("Minimum value");
RTTI_PROPERTY(m_limitMax).editable("Maximum value");
RTTI_END_TYPE();

MaterialTemplateParameter_Scalar::MaterialTemplateParameter_Scalar()
{}

MaterialDataLayoutParameterType MaterialTemplateParameter_Scalar::queryType() const
{
    return MaterialDataLayoutParameterType::Float;
}

Type MaterialTemplateParameter_Scalar::queryDataType() const
{
    return GetTypeObject<float>();
}

const void* MaterialTemplateParameter_Scalar::queryDefaultValue() const
{
    return &m_defaultValue;
}

//---

RTTI_BEGIN_TYPE_CLASS(MaterialTemplateParameter_Vector2);
RTTI_CATEGORY("Value");
RTTI_PROPERTY(m_defaultValue).editable("Default value of the parameter");
RTTI_CATEGORY("Limit");
RTTI_PROPERTY(m_limited).editable("Don't allow user to setup any value");
RTTI_PROPERTY(m_limitMin).editable("Minimum value");
RTTI_PROPERTY(m_limitMax).editable("Maximum value");
RTTI_END_TYPE();

MaterialTemplateParameter_Vector2::MaterialTemplateParameter_Vector2() 
{}

MaterialDataLayoutParameterType MaterialTemplateParameter_Vector2::queryType() const
{
    return MaterialDataLayoutParameterType::Vector2;
}

Type MaterialTemplateParameter_Vector2::queryDataType() const
{
    return GetTypeObject<Vector2>();
}

const void* MaterialTemplateParameter_Vector2::queryDefaultValue() const
{
    return &m_defaultValue;
}

//---

RTTI_BEGIN_TYPE_CLASS(MaterialTemplateParameter_Vector3);
RTTI_CATEGORY("Value");
RTTI_PROPERTY(m_defaultValue).editable("Default value of the parameter");
RTTI_CATEGORY("Limit");
RTTI_PROPERTY(m_limited).editable("Don't allow user to setup any value");
RTTI_PROPERTY(m_limitMin).editable("Minimum value");
RTTI_PROPERTY(m_limitMax).editable("Maximum value");
RTTI_END_TYPE();

MaterialTemplateParameter_Vector3::MaterialTemplateParameter_Vector3()
{}

MaterialDataLayoutParameterType MaterialTemplateParameter_Vector3::queryType() const
{
    return MaterialDataLayoutParameterType::Vector3;
}

Type MaterialTemplateParameter_Vector3::queryDataType() const
{
    return GetTypeObject<Vector3>();
}

const void* MaterialTemplateParameter_Vector3::queryDefaultValue() const
{
    return &m_defaultValue;
}

//---

RTTI_BEGIN_TYPE_CLASS(MaterialTemplateParameter_Vector4);
RTTI_CATEGORY("Value");
RTTI_PROPERTY(m_defaultValue).editable("Default value of the parameter");
RTTI_CATEGORY("Limit");
RTTI_PROPERTY(m_limited).editable("Don't allow user to setup any value");
RTTI_PROPERTY(m_limitMin).editable("Minimum value");
RTTI_PROPERTY(m_limitMax).editable("Maximum value");
RTTI_END_TYPE();

MaterialTemplateParameter_Vector4::MaterialTemplateParameter_Vector4()
{}

MaterialDataLayoutParameterType MaterialTemplateParameter_Vector4::queryType() const
{
    return MaterialDataLayoutParameterType::Vector4;
}

Type MaterialTemplateParameter_Vector4::queryDataType() const
{
    return GetTypeObject<Vector4>();
}

const void* MaterialTemplateParameter_Vector4::queryDefaultValue() const
{
    return &m_defaultValue;
}

//---

RTTI_BEGIN_TYPE_CLASS(MaterialTemplateParameter_Color);
    RTTI_CATEGORY("Value");
    RTTI_PROPERTY(m_defaultValue).editable("Default value of the parameter");
    RTTI_CATEGORY("Advanced");
    RTTI_PROPERTY(m_assumeAlreadyLinear).editable("Do not convert color to linear space");
RTTI_END_TYPE();

MaterialTemplateParameter_Color::MaterialTemplateParameter_Color()
{}

MaterialDataLayoutParameterType MaterialTemplateParameter_Color::queryType() const
{
    return MaterialDataLayoutParameterType::Color;
}

Type MaterialTemplateParameter_Color::queryDataType() const
{
    return GetTypeObject<Color>();
}

const void* MaterialTemplateParameter_Color::queryDefaultValue() const
{
    return &m_defaultValue;
}

//---

RTTI_BEGIN_TYPE_CLASS(MaterialTemplateParameter_Texture2D);
RTTI_CATEGORY("Value");
RTTI_PROPERTY(m_defaultValue).editable("Default value of the parameter");
RTTI_CATEGORY("Advanced");
RTTI_PROPERTY(m_fallbackColor).editable("Color to use if texture is not bound");
RTTI_END_TYPE();

MaterialTemplateParameter_Texture2D::MaterialTemplateParameter_Texture2D()
{}

MaterialDataLayoutParameterType MaterialTemplateParameter_Texture2D::queryType() const
{
    return MaterialDataLayoutParameterType::Texture2D;
}

Type MaterialTemplateParameter_Texture2D::queryDataType() const
{
    return GetTypeObject<TextureRef>();
}

const void* MaterialTemplateParameter_Texture2D::queryDefaultValue() const
{
    return &m_defaultValue;
}

//---

END_BOOMER_NAMESPACE()

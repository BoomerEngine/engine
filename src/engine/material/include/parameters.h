/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: material #]
***/

#pragma once

#include "materialTemplate.h"

BEGIN_BOOMER_NAMESPACE()

///---

/// static boolean parameters - static permutation selector
class MaterialTemplateParameter_StaticBool : public IMaterialTemplateParam
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialTemplateParameter_StaticBool, IMaterialTemplateParam);

public:
    MaterialTemplateParameter_StaticBool();

    // IMaterialTemplateParam
    virtual MaterialDataLayoutParameterType queryType() const override;
    virtual Type queryDataType() const override;
    virtual const void* queryDefaultValue() const override;

protected:
    bool m_defaultValue = false;
};

///---

/// scalar parameters
class MaterialTemplateParameter_Scalar : public IMaterialTemplateParam
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialTemplateParameter_Scalar, IMaterialTemplateParam);

public:
    MaterialTemplateParameter_Scalar();

    // IMaterialTemplateParam
    virtual MaterialDataLayoutParameterType queryType() const override;
    virtual Type queryDataType() const override;
    virtual const void* queryDefaultValue() const override;

protected:
    float m_defaultValue = 0.0f;

    float m_limitMin = 0.0f;
    float m_limitMax = 1.0f;
    bool m_limited = false;
};

///---

/// vector parameter
class MaterialTemplateParameter_Vector2 : public IMaterialTemplateParam
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialTemplateParameter_Vector2, IMaterialTemplateParam);

public:
    MaterialTemplateParameter_Vector2();

    // IMaterialTemplateParam
    virtual MaterialDataLayoutParameterType queryType() const override;
    virtual Type queryDataType() const override;
    virtual const void* queryDefaultValue() const override;

protected:
    Vector2 m_defaultValue;

    float m_limitMin = 0.0f;
    float m_limitMax = 1.0f;
    bool m_limited = false;
};

///---

/// 3D vector parameter
class MaterialTemplateParameter_Vector3 : public IMaterialTemplateParam
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialTemplateParameter_Vector3, IMaterialTemplateParam);

public:
    MaterialTemplateParameter_Vector3();

    // IMaterialTemplateParam
    virtual MaterialDataLayoutParameterType queryType() const override;
    virtual Type queryDataType() const override;
    virtual const void* queryDefaultValue() const override;

protected:
    Vector3 m_defaultValue;

    float m_limitMin = 0.0f;
    float m_limitMax = 1.0f;
    bool m_limited = false;
};

///---

/// 4D vector parameter
class MaterialTemplateParameter_Vector4 : public IMaterialTemplateParam
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialTemplateParameter_Vector4, IMaterialTemplateParam);

public:
    MaterialTemplateParameter_Vector4();

    // IMaterialTemplateParam
    virtual MaterialDataLayoutParameterType queryType() const override;
    virtual Type queryDataType() const override;
    virtual const void* queryDefaultValue() const override;

protected:
    Vector4 m_defaultValue;

    float m_limitMin = 0.0f;
    float m_limitMax = 1.0f;
    bool m_limited = false;
};

///---

/// Color parameter
class MaterialTemplateParameter_Color : public IMaterialTemplateParam
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialTemplateParameter_Color, IMaterialTemplateParam);

public:
    MaterialTemplateParameter_Color();

    // IMaterialTemplateParam
    virtual MaterialDataLayoutParameterType queryType() const override;
    virtual Type queryDataType() const override;
    virtual const void* queryDefaultValue() const override;

protected:
    Color m_defaultValue;
    bool m_assumeAlreadyLinear = false;
};

///---

/// 2D Texture parameter
class MaterialTemplateParameter_Texture2D : public IMaterialTemplateParam
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialTemplateParameter_Texture2D, IMaterialTemplateParam);

public:
    MaterialTemplateParameter_Texture2D();

    // IMaterialTemplateParam
    virtual MaterialDataLayoutParameterType queryType() const override;
    virtual Type queryDataType() const override;
    virtual const void* queryDefaultValue() const override;

protected:
    TextureRef m_defaultValue;
    Color m_fallbackColor;
};

///---

END_BOOMER_NAMESPACE()

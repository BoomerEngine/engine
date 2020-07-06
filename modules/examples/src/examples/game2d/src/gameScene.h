/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: example #]
***/

#pragma once

namespace example
{

    //--

    // game "asset"
    class GameSpriteAsset : public base::IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(GameSpriteAsset, base::IObject);

    public:
        GameSpriteAsset(const ImageRef& ptr, float scale = 1.0f, bool circle = false);

        INLINE const Vector2& size() const { return m_size; }
        INLINE const Geometry& geometry(bool flip=false) const { return *m_geometry[flip ? 1 : 0]; }
        INLINE const StringBuf& name() const { return m_name; }

        void bounds(Vector2& outMin, Vector2& outMax) const;
        void setupSnapBottom();

    private:
        ImageRef m_image;
        Vector2 m_size;

        bool snapBottom = false;
        float m_scale;
        bool m_circle;
        GeometryPtr m_geometry[2];
        StringBuf m_name;

        virtual void onPropertyChanged(StringView<char> path) override;

        void buildGeometry();
        void buildGeometry(GeometryBuilder& outBuilder, bool flip) const;
    };

    typedef RefPtr<GameSpriteAsset> GameSpriteAssetPtr;

    //--

    // sequence of animation frames
    class GameSpriteSequenceAsset : public base::IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(GameSpriteSequenceAsset, base::IObject);

    public:
        GameSpriteSequenceAsset(StringView<char> baseDepotPath, uint32_t numFrames, float fps, int downsample=0);

        INLINE const Vector2& size() const { return m_size; }

        const Geometry& geometry(float time, bool flip) const;

    private:
        base::Array<GameSpriteAssetPtr> m_frameAssets;
        Vector2 m_size;
        float m_fps = 1.0f;
    };

    typedef RefPtr<GameSpriteSequenceAsset> GameSpriteSequenceAssetPtr;

    //--

    class GameSceneLayer;

    // game "object"
    class GameObject : public base::IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(GameObject, base::IObject);

    public:
        Vector2 pos = Vector2(0,0);
        float rot = 0.0f;

        GameObject();

        virtual void added(GameSceneLayer* layer);
        virtual void removed(GameSceneLayer* layer);
        virtual void render(Canvas& canvas) = 0;
        virtual void tick(float dt);
        virtual void debug() = 0;
    };

    typedef RefPtr<GameObject> GameObjectPtr;

    //--

    // game "sprite" object
    class GameSprite : public GameObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(GameSprite, GameObject);

    public:
        GameSprite(Vector2 pos, GameSpriteAsset* asset);

        virtual void render(Canvas& canvas) override;
        virtual void debug() override;

        void bounds(Vector2& outMin, Vector2& outMax) const;

    private:
        GameSpriteAssetPtr m_asset;
        bool m_renderBBox = false;
    };

    //--

    class GameTerrianAsset : public base::IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(GameTerrianAsset, base::IObject);

    public:
        GameTerrianAsset(float tileSize, StringView<char> baseDepotPath, uint32_t numTiles);

        INLINE float tileSize() const { return m_tileSize; }

        void placeTile(GeometryBuilder& builder, int x, int y, char c);

    private:
        Array<ImageRef> m_tileImages;
        float m_tileSize;
    };

    typedef RefPtr<GameTerrianAsset> GameTerrianAssetPtr;

    //--

    struct TileCode
    {
        uint16_t tl : 1;
        uint16_t t : 1;
        uint16_t tr : 1;
        uint16_t l : 1;
        uint16_t c : 1;
        uint16_t r : 1;
        uint16_t bl : 1;
        uint16_t b : 1;
        uint16_t br : 1;
    };

    // game "terrain" - direct draw example
    class GameTerrain : public GameObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(GameTerrain, IObject);

    public:
        GameTerrain(uint32_t width, uint32_t height, GameTerrianAsset* asset);

        char tile(int x, int y) const; // 0 if out of range
        void tile(int x, int y, char tile);

        TileCode tileCode(int x, int y) const;

        Vector2 tileCenter(int x, int y) const;
        Vector2 tileSnap(int x, int y) const; // center X, bottom Y

        // TODO: collision

        float groundHeight(float x, float y) const;

        virtual void render(Canvas& canvas) override;
        virtual void added(GameSceneLayer* layer) override;
        virtual void removed(GameSceneLayer* layer) override;
        virtual void debug() override;

    private:
        GeometryPtr m_geometry;

        uint32_t m_width = 1;
        uint32_t m_height = 1;

        Array<uint8_t> m_tiles;
        GameSceneLayer* m_layer = nullptr;

        GameTerrianAssetPtr m_asset;

        void invalidateGeometry();
        void buildGeometry();
    };

    typedef RefPtr< GameTerrain> GameTerrainPtr;

    //--

    // game scene "layer"
    class GameSceneLayer : public NoCopy
    {
    public:
        GameSceneLayer();
        ~GameSceneLayer();

        //--

        void tick(float dt);

        void render(Canvas& canvas) const;
        void render(CommandWriter& cmd, Vector2 center, uint32_t width, uint32_t height, uint32_t outputWidth, uint32_t outputHeight) const;

        void debug(int index);

        //--

        void addObject(GameObject* object);
        void removeObject(GameObject* object);

    private:
        Array<GameObjectPtr> m_objects;
    };

    //--

    // 2D game scene
    class GameScene : public IReferencable
    {
    public:
        GameScene();
        ~GameScene();

        //--

        static const auto MAX_LAYERS = 3;

        base::Vector2 m_centerPos = base::Vector2(0,0); // at layer 0
        GameSceneLayer m_layer[MAX_LAYERS];

        //--

        void bounds(uint32_t width, uint32_t height, Vector2& outMin, Vector2& outMax) const;

        void tick(float dt);
        void render(CommandWriter& cmd, uint32_t width, uint32_t height, uint32_t outputWidth, uint32_t outputHeight) const;

    };

    typedef RefPtr<GameScene> GameScenePtr;

    //--

} // example


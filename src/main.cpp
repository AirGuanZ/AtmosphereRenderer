#include <agz-utils/mesh.h>

#include "./aerial_lut.h"
#include "./camera.h"
#include "./final.h"
#include "./mesh.h"
#include "./multiscatter.h"
#include "./sky_lut.h"
#include "./shadow.h"
#include "./transmittance.h"

class AtmosphereRendererDemo : public Demo
{
public:

    using Demo::Demo;

private:

    struct Mesh
    {
        VertexBuffer<Vertex> vertexBuffer;
        Mat4                 world;
    };

    float worldScale_        = 200;
    float maxAerialDistance_ = 1000;

    Camera camera_;

    float sunAngle_           = 18;
    float sunIntensity_       = 10;

    bool  enableShadow_       = true;
    bool  enableMultiScatter_ = true;

    Int3 aerialRes_ = { 128, 128, 64 };

    AtmosphereProperties atmos_;

    AerialPerspectiveLUT_ aerialLUT_;
    FinalRenderer         final_;
    MeshRenderer          meshRenderer_;
    MultiScatteringLUT    msLUT_;
    ShadowMap             shadowMap_;
    SkyLUT                skyLUT_;
    TransmittanceLUT      transLUT_;

    std::vector<Mesh> meshes_;

    void initialize() override
    {
        aerialLUT_.initialize(aerialRes_);

        final_.initialize();

        meshRenderer_.initialize();

        shadowMap_.initialize({ 2048, 2048 });

        skyLUT_.initialize({ 128, 128 });
        skyLUT_.setParams(128, 128, 20);
        skyLUT_.enableMultiScattering(enableMultiScatter_);

        transLUT_.generate({ 256, 256 }, atmos_);

        msLUT_.generate(
            { 256, 256 }, transLUT_.getSRV(), Float3(0.3f), atmos_);

        loadMesh(
            "./asset/terrain.obj", Float3(0.3f), Trans4::translate(0, 1, 0));

        camera_.setPosition({ -2.82f, 2.515f, -2.6f });
        camera_.setDirection(PI, 0);
        camera_.setPerspective(60.0f, 0.1f, 100.0f);

        mouse_->setCursorLock(
            true, window_->getClientWidth() / 2, window_->getClientHeight() / 2);
        mouse_->showCursor(false);
        window_->doEvents();
    }
    
    void frame() override
    {
        if(keyboard_->isDown(KEY_ESCAPE))
            window_->setCloseFlag(true);

        if(keyboard_->isDown(KEY_LCTRL))
        {
            mouse_->showCursor(!mouse_->isVisible());
            mouse_->setCursorLock(
                !mouse_->isLocked(), mouse_->getLockX(), mouse_->getLockY());
        }

        if(ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::SliderFloat("Sun Angle", &sunAngle_, 0, 90);
            ImGui::InputFloat("Sun Intensity", &sunIntensity_);

            if(ImGui::Checkbox("Enable Multi Scattering", &enableMultiScatter_))
                skyLUT_.enableMultiScattering(enableMultiScatter_);
            ImGui::Checkbox("Enable Shadow", &enableShadow_);

            ImGui::InputFloat("World Scale", &worldScale_);
            ImGui::InputFloat("Aerial Distance", &maxAerialDistance_);

            if(ImGui::InputInt3("Aerial Resolution", &aerialRes_.x))
            {
                aerialRes_ = aerialRes_.clamp_low(1);
                aerialLUT_.resize(aerialRes_);
            }

            const Float3 eye = camera_.getPosition() * worldScale_;
            ImGui::Text("Eye: (%f, %f, %f)", eye.x, eye.y, eye.z);
        }
        ImGui::End();

        camera_.setWOverH(window_->getClientWOverH());
        if(!mouse_->isVisible())
        {
            camera_.update({
                .front      = keyboard_->isPressed('W'),
                .left       = keyboard_->isPressed('A'),
                .right      = keyboard_->isPressed('D'),
                .back       = keyboard_->isPressed('S'),
                .up         = keyboard_->isPressed(KEY_SPACE),
                .down       = keyboard_->isPressed(KEY_LSHIFT),
                .cursorRelX = static_cast<float>(mouse_->getRelativePositionX()),
                .cursorRelY = static_cast<float>(mouse_->getRelativePositionY())
                });
        }
        camera_.recalculateMatrics();

        const float sunRad = -agz::math::deg2rad(sunAngle_);
        const Float3 sunDirection = Float3(cos(sunRad), sin(sunRad), 0).normalize();
        const Float3 sunRadiance = Float3(sunIntensity_);

        const Mat4 sunViewProj =
            Trans4::look_at(-sunDirection * 20, { 0, 0, 0 }, { 0, 1, 0 }) *
            Trans4::orthographic(-10, 10, 10, -10, 0.1f, 80);

        shadowMap_.setLight(sunViewProj);
        shadowMap_.begin();
        for(auto &m : meshes_)
            shadowMap_.render(m.vertexBuffer, m.world);
        shadowMap_.end();

        aerialLUT_.setParams(
            worldScale_, enableMultiScatter_, enableShadow_, maxAerialDistance_, 16);
        aerialLUT_.setCamera(
            camera_.getPosition(),
            worldScale_ * camera_.getPosition().y,
            camera_.getFrustumDirections());
        aerialLUT_.setSun(sunDirection);
        aerialLUT_.render(
            sunViewProj, shadowMap_.getShadowMap(),
            atmos_, msLUT_.getSRV(), transLUT_.getSRV());
        
        skyLUT_.setCamera(worldScale_ * camera_.getPosition());
        skyLUT_.generate(
            atmos_, sunDirection, sunRadiance,
            transLUT_.getSRV(), msLUT_.getSRV());

        window_->useDefaultRTVAndDSV();
        window_->useDefaultViewport();
        window_->clearDefaultDepth(1);
        window_->clearDefaultRenderTarget({ 0, 1, 1, 0 });

        meshRenderer_.setCamera(camera_.getPosition(), camera_.getViewProj());
        meshRenderer_.setAtmosphere(
            atmos_, transLUT_.getSRV(), aerialLUT_.getOutput(), maxAerialDistance_);
        meshRenderer_.setLight(sunDirection, sunRadiance);
        meshRenderer_.setWorldScale(worldScale_);
        meshRenderer_.begin();
        for(auto &m : meshes_)
            meshRenderer_.render(m.vertexBuffer, m.world);
        meshRenderer_.end();

        final_.setCamera(camera_.getFrustumDirections());
        final_.render(skyLUT_.getLUT());
    }

    void loadMesh(
        const std::string &objFilename,
        const Float3      &albedo,
        const Mat4        &world)
    {
        auto tris = agz::mesh::load_from_file(objFilename);
        std::vector<Vertex> vertices;
        for(auto &t : tris)
        {
            for(auto &v : t.vertices)
                vertices.push_back({ v.position, v.normal, albedo / PI });
        }

        Mesh mesh;
        mesh.vertexBuffer.initialize(vertices.size(), vertices.data());
        mesh.world  = world;
        meshes_.push_back(std::move(mesh));
    }
};

int main()
{
    AtmosphereRendererDemo(WindowDesc
    {
        .clientSize = { 640, 480 },
        .title      = L"AtmosphereRenderer"
    }).run();
}

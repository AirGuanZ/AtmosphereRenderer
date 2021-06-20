#include <agz-utils/mesh.h>

#include "./aerial_lut.h"
#include "./camera.h"
#include "./mesh.h"
#include "./multiscatter.h"
#include "./sky.h"
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

    float worldScale_               = 200;
    float maxAerialDistance_        = 1000;
    int   aerialPerSliceMarchCount_ = 2;
    float apJitterRadius_           = 1;

    Camera camera_;
    
    float sunAngleX_          = 0;
    float sunAngleY_          = 18;
    float sunIntensity_       = 10;

    bool enableTerrain_      = true;
    bool enableShadow_       = true;
    bool enableMultiScatter_ = true;

    int skyMarchStepCount_ = 40;

    Int2 transLUTRes_  = { 256, 256 };
    Int2 msLUTRes_     = { 256, 256 };
    Int2 skyLUTRes_    = { 64, 64};
    Int3 aerialLUTRes_ = { 200, 150, 32 };

    AtmosphereProperties atmos_;
    AtmosphereProperties stdUnitAtmos_;

    MultiScatteringLUT msLUT_;
    TransmittanceLUT   transLUT_;

    SkyLUT               skyLUT_;
    AerialPerspectiveLUT aerialLUT_;

    ShadowMap    shadowMap_;
    SkyRenderer  skyRenderer_;
    MeshRenderer meshRenderer_;

    std::vector<Mesh> meshes_;

    void initialize() override
    {
        stdUnitAtmos_ = atmos_.toStdUnit();

        transLUT_.generate(transLUTRes_, stdUnitAtmos_);
        msLUT_.generate(
            msLUTRes_, transLUT_.getSRV(), Float3(0.3f), stdUnitAtmos_);

        shadowMap_.initialize({ 2048, 2048 });

        aerialLUT_.initialize(aerialLUTRes_);
        skyLUT_.initialize(skyLUTRes_);
        
        skyRenderer_.initialize();
        meshRenderer_.initialize();

        loadMesh("./asset/terrain.obj", Float3(0.1f), Trans4::translate(0, 1, 0));

        camera_.setPosition({ -2.8f, 2.5f, -2.6f });
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

        showGUI();

        updateCamera();

        const float sunRadX = agz::math::deg2rad(sunAngleX_);
        const float sunRadY = agz::math::deg2rad(-sunAngleY_);
        const Float3 sunDirection = Float3(
            cos(sunRadX) * cos(sunRadY),
            sin(sunRadY),
            sin(sunRadX) * cos(sunRadY)).normalize();

        const Float3 sunRadiance = Float3(sunIntensity_);

        const Mat4 sunViewProj =
            Trans4::look_at(-sunDirection * 20, { 0, 0, 0 }, { 0, 1, 0 }) *
            Trans4::orthographic(-10, 10, 10, -10, 0.1f, 80);

        buildShadowMap(sunViewProj);

        buildSkyLUT(sunDirection, sunRadiance);

        buildAerialLUT(sunDirection, sunViewProj);

        window_->useDefaultRTVAndDSV();
        window_->useDefaultViewport();
        window_->clearDefaultDepth(1);
        window_->clearDefaultRenderTarget({ 0, 1, 1, 0 });

        if(enableTerrain_)
            renderMeshes(sunDirection, sunRadiance, sunViewProj);

        renderSky();
    }

    void showGUI()
    {
        if(!ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            return;
        AGZ_SCOPE_GUARD({ ImGui::End(); });

        ImGui::Checkbox("Enable Terrain", &enableTerrain_);
        ImGui::Checkbox("Enable Multi Scattering", &enableMultiScatter_);
        ImGui::Checkbox("Enable Shadow", &enableShadow_);
        ImGui::InputFloat("World Scale", &worldScale_);

        ImGui::SetNextTreeNodeOpen(false, ImGuiCond_Once);
        if(ImGui::TreeNode("Atmosphere"))
        {
            bool changed = false;
            changed |= ImGui::InputFloat ("Planet Radius         (km)   ", &atmos_.planetRadius);
            changed |= ImGui::InputFloat ("Atmosphere Radius     (km)   ", &atmos_.atmosphereRadius);
            changed |= ImGui::InputFloat3("Rayleight Scattering  (um^-1)", &atmos_.scatterRayleigh.x);
            changed |= ImGui::InputFloat ("Rayleight Density H   (km)   ", &atmos_.hDensityRayleigh);
            changed |= ImGui::InputFloat ("Mie Scatter           (um^-1)", &atmos_.scatterMie);
            changed |= ImGui::InputFloat ("Mie Absorb            (um^-1)", &atmos_.absorbMie);
            changed |= ImGui::InputFloat ("Mie Density H         (km)   ", &atmos_.hDensityMie);
            changed |= ImGui::InputFloat ("Mie Scatter Asymmetry        ", &atmos_.asymmetryMie);
            changed |= ImGui::InputFloat3("Ozone Absorb          (um^-1)", &atmos_.absorbOzone.x);
            changed |= ImGui::InputFloat ("Ozone Center Height   (km)   ", &atmos_.ozoneCenterHeight);
            changed |= ImGui::InputFloat ("Ozone Thickness       (km)   ", &atmos_.ozoneThickness);

            if(changed)
            {
                stdUnitAtmos_ = atmos_.toStdUnit();

                transLUT_.generate(transLUTRes_, stdUnitAtmos_);
                msLUT_.generate(
                    msLUTRes_, transLUT_.getSRV(), Float3(0.3f), stdUnitAtmos_);
            }

            ImGui::TreePop();
        }

        ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
        if(ImGui::TreeNode("Sun"))
        {
            ImGui::SliderFloat("Sun Angle Y", &sunAngleY_, 0, 90);
            ImGui::SliderFloat("Sun Angle X", &sunAngleX_, 0, 360);
            ImGui::InputFloat("Sun Intensity", &sunIntensity_);
            ImGui::TreePop();
        }

        ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
        if(ImGui::TreeNode("Sky LUT"))
        {
            if(ImGui::InputInt2("Sky LUT Resolution", &skyLUTRes_.x))
            {
                skyLUTRes_ = skyLUTRes_.clamp_low(1);
                skyLUT_.resize(skyLUTRes_);
            }
            ImGui::InputInt("Ray March Steps", &skyMarchStepCount_);
            ImGui::TreePop();
        }

        ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
        if(ImGui::TreeNode("Aerial LUT"))
        {
            ImGui::InputFloat("Aerial Distance", &maxAerialDistance_);
            if(ImGui::InputInt3("Aerial Resolution", &aerialLUTRes_.x))
            {
                aerialLUTRes_ = aerialLUTRes_.clamp_low(1);
                aerialLUT_.resize(aerialLUTRes_);
            }
            ImGui::InputInt("Aerial March Steps", &aerialPerSliceMarchCount_);
            ImGui::InputFloat("Aerial Jitter Radius", &apJitterRadius_);
            ImGui::TreePop();
        }
    }

    void updateCamera()
    {
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
    }

    void buildShadowMap(const Mat4 &sunViewProj)
    {
        shadowMap_.setSun(sunViewProj);

        shadowMap_.begin();
        for(auto &m : meshes_)
            shadowMap_.render(m.vertexBuffer, m.world);
        shadowMap_.end();
    }

    void buildSkyLUT(
        const Float3 &sunDirection,
        const Float3 &sunRadiance)
    {
        skyLUT_.enableMultiScattering(enableMultiScatter_);
        skyLUT_.setRayMarching(skyMarchStepCount_);
        skyLUT_.setCamera(worldScale_ * camera_.getPosition());

        skyLUT_.generate(
            stdUnitAtmos_, sunDirection, sunRadiance,
            transLUT_.getSRV(), msLUT_.getSRV());
    }

    void buildAerialLUT(
        const Float3 &sunDirection,
        const Mat4   &sunViewProj)
    {
        const float atmosEyeHeight = worldScale_ * camera_.getPosition().y;
        aerialLUT_.setCamera(
            camera_.getPosition(), atmosEyeHeight,
            camera_.getFrustumDirections());
        aerialLUT_.setWorldScale(worldScale_);
        aerialLUT_.setAtmosphere(stdUnitAtmos_);

        aerialLUT_.setSun(sunDirection);
        aerialLUT_.setShadow(
            enableShadow_, sunViewProj, shadowMap_.getShadowMap());

        aerialLUT_.setMarchingParams(
            maxAerialDistance_, aerialPerSliceMarchCount_);

        aerialLUT_.setMultiScatterLUT(msLUT_.getSRV());
        aerialLUT_.setTransmittanceLUT(transLUT_.getSRV());

        aerialLUT_.render();
    }

    void renderMeshes(
        const Float3 &sunDirection,
        const Float3 &sunRadiance,
        const Mat4   &sunViewProj)
    {
        meshRenderer_.setAtmosphere(
            stdUnitAtmos_,
            transLUT_.getSRV(),
            aerialLUT_.getOutput(),
            apJitterRadius_, maxAerialDistance_);

        meshRenderer_.setRenderTarget(window_->getClientSize());
        meshRenderer_.setCamera(camera_.getPosition(), camera_.getViewProj());
        meshRenderer_.setWorldScale(worldScale_);

        meshRenderer_.setShadowMap(shadowMap_.getShadowMap(), sunViewProj);
        meshRenderer_.setSun(sunDirection, sunRadiance);

        meshRenderer_.begin();
        for(auto &m : meshes_)
            meshRenderer_.render(m.vertexBuffer, m.world);
        meshRenderer_.end();
    }

    void renderSky()
    {
        skyRenderer_.setCamera(camera_.getFrustumDirections());
        skyRenderer_.render(skyLUT_.getLUT());
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
        mesh.world = world;
        meshes_.push_back(std::move(mesh));
    }
};

int main()
{
    AtmosphereRendererDemo(WindowDesc{
        .clientSize  = { 640, 480 },
        .title       = L"AtmosphereRenderer",
        .sampleCount = 4
    }).run();
}

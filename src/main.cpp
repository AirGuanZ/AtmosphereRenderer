#include <agz-utils/mesh.h>

#include "./final.h"
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
        Float3               albedo;
        Mat4                 world;
    };

    float sunAngle_           = 45;
    float sunIntensity_       = 10;
    bool  enableMultiScatter_ = true;

    AtmosphereProperties atmos_;

    FinalRenderer        final_;
    MultiScatteringTable mulscatter_;
    ShadowMap            shadowMap_;
    SkyLUT               skyLUT_;
    TransmittanceTable   transmittance_;

    std::vector<Mesh> meshes_;

    void initialize() override
    {
        final_.initialize();
        shadowMap_.initialize({ 2048, 2048 });
        skyLUT_.initialize({ 128, 128 });
        skyLUT_.setParams(256, 256, 10);
        skyLUT_.enableMultiScattering(enableMultiScatter_);

        transmittance_.generate({ 256, 256 }, atmos_);
        mulscatter_.generate(
            { 256, 256 }, transmittance_.getSRV(), Float3(0.3f), atmos_);

        loadMesh("./asset/terrain.obj", Float3(0.3f), Mat4::identity());
    }
    
    void frame() override
    {
        if(keyboard_->isDown(KEY_ESCAPE))
            window_->setCloseFlag(true);

        if(ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::SliderFloat("Sun Angle", &sunAngle_, 0, 90);
            ImGui::InputFloat("Sun Intensity", &sunIntensity_);

            if(ImGui::Checkbox("Enable Multi Scattering", &enableMultiScatter_))
                skyLUT_.enableMultiScattering(enableMultiScatter_);
        }
        ImGui::End();

        const float sunRad        = -agz::math::deg2rad(sunAngle_);
        const Float3 sunDirection = Float3(cos(sunRad), sin(sunRad), 0).normalize();
        const Float3 sunRadiance  = Float3(sunIntensity_);

        const Mat4 sunViewProj =
            Trans4::look_at(-sunDirection * 10, { 0, 0, 0 }, { 0, 1, 0 }) *
            Trans4::orthographic(-10, 10, 10, -10, 0.1f, 40);

        shadowMap_.setLight(sunViewProj);
        shadowMap_.begin();
        for(auto &m : meshes_)
            shadowMap_.render(m.vertexBuffer, m.world);
        shadowMap_.end();

        skyLUT_.setCamera({ 0, 20, 0 });
        skyLUT_.generate(
            atmos_, sunDirection, sunRadiance,
            transmittance_.getSRV(), mulscatter_.getSRV());

        window_->useDefaultRTVAndDSV();
        window_->useDefaultViewport();
        window_->clearDefaultDepth(1);
        window_->clearDefaultRenderTarget({ 0, 1, 1, 0 });

        Mat4 view = Trans4::look_at({ 1, -0.5f, 0.2f }, { 0, 0, 0 }, { 0, 1, 0 });
        Mat4 proj = Trans4::perspective(
            agz::math::deg2rad(60.0f), window_->getClientWOverH(), 0.1f, 20.0f);
        final_.setCamera((view * proj).inv());
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
                vertices.push_back({ v.position, v.normal });
        }

        Mesh mesh;
        mesh.vertexBuffer.initialize(vertices.size(), vertices.data());
        mesh.albedo = albedo;
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

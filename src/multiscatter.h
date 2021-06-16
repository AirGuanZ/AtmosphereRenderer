#pragma once

#include "./medium.h"

class MultiScatteringTable
{
public:

    void generate(
        const Int2                      &res,
        ComPtr<ID3D11ShaderResourceView> transmittance,
        const Float3                    &terrainAlbedo,
        const AtmosphereProperties      &atmos);

    ComPtr<ID3D11ShaderResourceView> getSRV() const;

private:

    Shader<CS>                       shader_;
    ComPtr<ID3D11ShaderResourceView> srv_;
};

#pragma once

#include "./medium.h"

class TransmittanceLUT
{
public:

    void generate(const Int2 &res, const AtmosphereProperties &atmosphere);

    ComPtr<ID3D11ShaderResourceView> getSRV() const;

private:

    Shader<CS>                       shader_;
    ComPtr<ID3D11ShaderResourceView> srv_;
};

# Atmosphere Renderer

A C++/DirectX 11 implementation of "[A Scalable and Production Ready Sky and Atmosphere Rendering Technique](https://sebh.github.io/publications/egsr2020.pdf)"

## Features

* interactive edition of atmosphere model parameters
* efficient generation of low-resolution sky-view LUT and aerial perspective LUT 
* volumetric shadow

## Build

```powershell
git clone --recursive https://github.com/AirGuanZ/AtmosphereRenderer.git
cd AtmosphereRenderers
mkdir build
cd build
cmake ..
```

## Control

* move: `W, A, S, D, Space, LShift`
* show/hide cursor: `Ctrl`.

## Performance

Crucial shader performance measured on NVIDIA GTX 1060:

* Sky-view LUT generation (64 * 64): 0.036 ms
* Aerial perspective LUT generation without volume shadow (200 * 150 * 32): 0.13 ms
* Aerial perspective LUT generation with volume shadow (200 * 150 * 32): 0.2 ms

Shaders can actually be further optimized by replacing feature flags in constant buffers with compile-time macros.

## Screenshots

![](./gallery/00.png)

![](./gallery/01.png)

![](./gallery/02.png)

## Tips

* Planet-scale rendering are not considered. Thus the sky may become strange when camera is out of the atmosphere.
* Terrain renderer is just used to show the aerial perspective effect (transmittance and in-scattering), so multi-scattering related to terrain is simply ignored.
* Terrain occlusion is ignored when computing multi-scattering LUT.
* Sunlight is approximated as a directional light when computing atmosphere scattering. Thus a very big sun disk will always have an unnatural appearance.


# THIS ENTIRE REPO IS OUTDATED!

[The upstream repository](https://github.com/Dao-AILab/flash-attention) works on GFX11/RDNA3 now. It is faster and more up-to-date by several years. This is unlikely to even build correctly on recent ROCm versions.

If you need headdim > 256, the develop branch of [ROCm/xformers](https://github.com/ROCm/xformers) now also builds and works correctly on GFX11/RDNA3 as well.

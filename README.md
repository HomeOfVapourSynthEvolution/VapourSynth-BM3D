# VapourSynth-BM3D

Copyright© 2015 mawen1250

BM3D denoising filter for VapourSynth

## Description

BM3D is a state-of-the-art image denoising algorithm. It can be extended to video denoising, named V-BM3D, which is also implemented in this plugin.

Requirements: libfftw3f-3.dll should be in the search path http://www.fftw.org/install/windows.html

namespace: bm3d

functions: RGB2OPP, OPP2RGB, Basic, Final, VBasic, VFinal, VAggregate

## Supported Formats

sample type & bps: 8-16 bit integer, 32 bit float.

color family: Gray, RGB, YUV or YCoCg.

sub sampling: when chroma is processed, no sub-sampling is supported, only YUV444.

## Important Note

- The denoising quality is best when filtering in opponent color space (abbr. OPP, a kind of YUV color space with simple and intuitive matrix coefficients), which significantly outperforms the quality when filtering in RGB, YCbCr, YCgCo, etc. Thus RGB input is recommended, this filter will convert it to OPP internally and convert back to RGB for output.

- Sub-sampling support is not implemented for simplicity and also for the sake of quality. In fact, the computational cost of conversion between YUV4xx and RGB can be ignored when comparing with that of the filtering kernel.

- Alternatively, call bm3d.RGB2OPP to apply the conversion first to avoid frequently converting between RGB and OPP, and call bm3d.OPP2RGB at the end to convert it back to RGB. However, compared to the computational cost of the BM3D kernel, those conversion costs can barely affect the speed of the entire filtering procedure, and may lead to more memory consumption (especially if you set sample=1, employing 32bit float clip in the VS processing chain).

- For V-BM3D, the filtered output is always OPP for RGB input, and you should manually call bm3d.OPP2RGB afterwards.

## Usage

### Helper Functions

#### RGB color space to opponent color space.

```python
bm3d.RGB2OPP(clip input[, int sample=0])
```

- input:<br />
    The input clip, must be of RGB color family.<br />
    The output clip is of YUV color family, 16 bit integer or 32 bit float.

- sample:<br />
  - 0 - 16 bit integer output (default)
  - 1 - 32 bit float output

#### opponent color space to RGB color space.

```python
bm3d.OPP2RGB(clip input[, int sample=0])
```

- input:<br />
    The input clip, must be of YUV color family.<br />
    The output clip is of RGB color family, 16 bit integer or 32 bit float.

- sample:<br />
  - 0 - 16 bit integer output (default)
  - 1 - 32 bit float output

### BM3D Functions

BM3D is a spatial domain denoising (image denoising) filter.

#### basic estimate of BM3D denoising filter

The input clip is processed in 3-step stages, For each reference block:<br />

- Grouping.<br />
    Use block-matching within the noisy image to find locations of blocks similar to the reference one (alternatively, if clip "ref" is assigned, then the block-matching will be applied on the reference clip). Then form a groups by stacking together the matched blocks in noisy image.
- Collaborative hard-thresholding.<br />
    Apply a 3D transform to the formed group. Attenuate the noise by hard-thresholding of the transform coefficients. Invert the 3D transform to produce estimates of all grouped blocks.
- Aggregation.<br />
    Compute a basic estimate by aggregating the obtained block-wise estimates in each group using a weighted average.

This basic estimate produces a decent estimate of the noise-free image, as a reference for final estimate.

```python
bm3d.Basic(clip input[, clip ref=input, string profile="fast", float[] sigma=[10,10,10], int block_size, int block_step, int group_size, int bm_range, int bm_step, float th_mse, float hard_thr, int matrix=2])
```

- input:<br />
    The input clip, the clip to be filtered with collaborative hard-thresholding.<br />
    The output clip is of the same format, width and height as the input clip.

- ref:<br />
    The reference clip, this clip is used in block-matching.<br />
    If not specified, the input clip is used instead.

- profile:<br />
    Preset profiles.<br />
    A table below shows the default parameters for each profile.<br />
  - "fast" - Fast Profile (default)
  - "lc" - Low Complexity Profile
  - "np" - Normal Profile
  - "high" - High Profile
  - "vn" - Very Noisy Profile

- sigma:<br />
    The strength of denoising, valid range [0, +inf), default [10,10,10].<br />
    Technically, this is the standard deviation of i.i.d. zero mean additive white Gaussian noise in 8 bit scale. BM3D denoising filter is designed based on this noise model, and best fit for attenuating it.<br />
    An array up to 3 elements can be assigned to set different sigma for Y,U,V channels. If less than 3 elements assigned, the last element's value will be assigned to the undefined elements.<br />
    0 will disable the processing for corresponding channel.

- block_size:<br />
    The size of a block is block_size x block_size (the 1st and the 2nd dimension), valid range [1,64].<br />
    A block is the basic processing unit of BM3D, representing a local patch.<br />
    Generally, larger block will be slower, especially in the DCT/IDCT part. While at the same time, larger block_size allows you to set larger block_step, resulting in less block to be processed.<br />
    8 is a well-balanced value, both for quality and speed.

- block_step:<br />
    Sliding step to process every next reference block, valid range [1,block_size].<br />
    Total number of reference blocks to be processed can be calculated approximately by (width / block_step) * (height / block_step).<br />
    Smaller step results in processing more reference blocks, and is slower.

- group_size:<br />
    Maximum number of similar blocks in each group (the 3rd dimension), valid range [1,256].<br />
    Larger value allows more blocks in a single group. Thus, the sparsity in a transformed group raises, the filtering will be stronger, and also slower in the DCT/IDCT part.<br />
    When set to 1, no block-matching will be performed and each group only consists of the referenc block, then basic estimate stage of BM3D will behave similar to the DFTTest denoising filter (without temporal filtering).

- bm_range:<br />
    Length of the side of the search neighborhood for block-matching, valid range [1, +inf).<br />
    The size of search window is (bm_range * 2 + 1) x (bm_range * 2 + 1).<br />
    Larger is slower, with more chances to find similar patches.

- bm_step:<br />
    Step between two search locations for block-matching, valid range [1, bm_range].<br />
    Total number of search locations for each reference block is (bm_range / bm_step * 2 + 1) ^ 2.<br />
    Smaller is slower, 1 is equivalent to full-search block-matching.

- th_mse:<br />
    Threshold of mean square error for block-matching, valid range [0, +inf).<br />
    Larger the value, more blocks will be matched. Too large value will lead to more dissimilar blocks being matched into a group, resulting in losses to fine structures and details.<br />
    Increase it if the noise in reference clip is strong. The default value is automatically adjusted according to sigma[0].

- hard_thr:<br />
    The threshold parameter for the hard-thresholding in 3D transformed domain, in 8 bit scale, valid range (0, +inf).<br />
    Larger results in stronger hard-threshold filtering in frequency domain.<br />
    Usually, to tweak denoising strength, it's better to adjust "sigma" rather than "hard_thr".

- matrix:<br />
    Matrix coefficients for Gray, YUV or YCoCg input, default 2.<br />
    Since the YUV color space is unnormalized, the actual sigma used inside BM3D will be normalized according to the matrix coefficients. This is important! It can significantly affect the final results.<br />
    This normalization only plays a part when initializing the filter, thus I cannot employ the frame properties such as "_Matrix" for it.<br />
    In case matrix is not properly set for BM3D with OPP input, bm3d.RGB2OPP attachs a property "BM3D_OPP=1" to its output frame. If this property is presented but matrix is not set to 100, the GetFrame function of BM3D will return an error message.<br />
    The number is as specified in ISO/IEC 14496-10, with an additional one for OPP.<br />
      - 0 - GBR
      - 1 - bt709
      - 2 - Unspecified, will automatically choose the matrix between smpte170m, bt709 and bt2020nc according to width and height of input clip
      - 4 - fcc
      - 5 - bt470bg
      - 6 - smpte170m
      - 7 - smpte240m
      - 8 - YCgCo, always set when color family is YCoCg
      - 9 - bt2020nc
      - 10 - bt2020c
      - 100 - OPP, opponent color space converted by bm3d.RGB2OPP, always set when color family is RGB

#### final estimate of BM3D denoising filter

It takes the basic estimate as a reference.

The input clip is processed in 3-step stages, For each reference block:<br />

- Grouping.<br />
    Use block-matching within the basic estimate to find locations of blocks similar to the reference one. Then form two groups, one from the noisy image and one from the basic estimate.
- Collaborative Wiener filtering.<br />
    Apply a 3D transform on both groups. Perform empirical Wiener filtering on the noisy one guided by the basic estimate.
- Aggregation.<br />
    Compute a final estimate by aggregating the obtained block-wise estimates in each group using a weighted average.

This final estimate can be realized as a refinement. It can significantly improve the denoising quality, keeping more details and fine structures that were removed in basic estimate.

```python
bm3d.Final(clip input, clip ref[, string profile="fast", float[] sigma=[10,10,10], int block_size, int block_step, int group_size, int bm_range, int bm_step, float th_mse, int matrix=2])
```

- input:<br />
    The input clip, the clip to be filtered.<br />
    The output clip is of the same format, width and height as the input clip.

- ref:<br />
    The reference clip, this clip is used in block-matching and as the reference in empirical Wiener filtering.<br />
    It must be specified. In original BM3D algorithm, it is the basic estimate.<br />
    Alternatively, you can choose any other decent denoising filter as basic estimate, and take this final estimate as a refinement.

- profile, sigma, block_size, block_step, group_size, bm_range, bm_step, th_mse, matrix:<br />
    Same as those in bm3d.Basic.

### V-BM3D Functions

V-BM3D extends the BM3D to spatial-temporal domain denoising (video denoising).

The algorithm is much the same as BM3D, except that it applies block-matching and collaborative filtering across multiple frames in a video.

For the current frame, is still applies a full-search(when BMstep=1) block-matching to it.

For the backward frames and forward frames, it applies predictive-search block-matching, whose search window is centered on several matched locations in the previous processed frame.

The obtained block-wise estimates are also aggregated into multiple frames.

However, since the estimates are returned into multiple frames, I have to divide it into 2 functions: bm3d.VBasic or bm3d.VFinal as the first stage and bm3d.VAggregate as the second stage.

*Always call bm3d.VAggregate after bm3d.VBasic or bm3d.VFinal.*

For RGB color family input, the output clip is of opponent color space in YUV color family. You should manually call bm3d.OPP2RGB after bm3d.VAggregate if you want to convert it back to RGB color space.

The output clip is an intermediate processed buffer. It is of 32 bit float format, and (radius * 2 + 1) * 2 times the height of input.

Due to the float format and multiple times height of the output clip, as well as the multiple frames requested by each function, those frame cache leads to very high memory consumption of this V-BM3D implementation.

#### basic estimate of V-BM3D denoising filter

```python
bm3d.VBasic(clip input[, clip ref=input, string profile="fast", float[] sigma=[10,10,10], int radius, int block_size, int block_step, int group_size, int bm_range, int bm_step, int ps_num, int ps_range, int ps_step, float th_mse, float hard_thr, int matrix=2])
```

- input, ref:<br />
    Same as those in bm3d.Basic.

- profile, sigma, block_size, block_step, group_size, bm_range, bm_step, th_mse, matrix:<br />
    Same as those in bm3d.Basic.

- radius:<br />
    The temporal radius for denoising, valid range [1, 16].<br />
    For each processed frame, (radius * 2 + 1) frames will be requested, and the filtering result will be returned to these frames by bm3d.VAggregate.<br />
    Increasing radius only increases tiny computational cost in block-matching and aggregation, and will not affect collaborative filtering, but the memory consumption can grow quadratically.<br />
    Thus, feel free to use large radius as long as your RAM is large enough :D

- ps_num:<br />
    The number of matched locations used for predictive search, valid range [1, group_size].<br />
    Larger value increases the possibility to match more similar blocks, with tiny increasing in computational cost. But in the original MATLAB implementation of V-BM3D, it's fixed to 2 for all profiles except "lc", perhaps larger value is not always good for quality?

- ps_range:<br />
    Length of the side of the search neighborhood for predictive-search block-matching, valid range [1, +inf)

- ps_step:<br />
    Step between two search locations for predictive-search block-matching, valid range [1, ps_range].<br />
    The maximum number of predictive-search locations for each reference block in a frame is (ps_range / ps_step * 2 + 1) ^ 2 * ps_num.

#### final estimate of V-BM3D denoising filter

```python
bm3d.VFinal(clip input, clip ref[, string profile="fast", float[] sigma=[10,10,10], int radius, int block_size, int block_step, int group_size, int bm_range, int bm_step, int ps_num, int ps_range, int ps_step, float th_mse, int matrix=2])
```

- input, ref:<br />
    Same as those in bm3d.Final.

- profile, sigma, block_size, block_step, group_size, bm_range, bm_step, th_mse, matrix:<br />
    Same as those in bm3d.Basic.

- radius, ps_num, ps_range, ps_step:<br />
    Same as those in bm3d.VBasic.

#### aggregation of V-BM3D denoising filter

*If your input clip of bm3d.VBasic or bm3d.VFinal is of RGB color family, you will need to manually call bm3d.OPP2RGB after bm3d.VAggregate to convert it back to RGB.*

```python
bm3d.VAggregate(clip input[, int radius=1, int sample=0])
```

- input:<br />
    The clip output by bm3d.VBasic or bm3d.VFinal.

- radius:<br />
    Must be of the same radius as used in previous function bm3d.VBasic or bm3d.VFinal.

- sample:<br />
  - 0 - 16 bit integer output (default)
  - 1 - 32 bit float output

## Profile Default

```
bm3d.Basic / bm3d.Final / bm3d.VBasic / bm3d.VFinal
----------------------------------------------------------------------------
| profile || block_size | block_step | group_size  | bm_range    | bm_step |
----------------------------------------------------------------------------
| "fast"  || 8/8/8/8    | 8/7/8/7    | 16/16/8/8   | 9/9/7/7     | 1/1/1/1 |
| "lc"    || 8/8/8/8    | 6/5/6/5    | 16/16/8/8   | 9/9/9/9     | 1/1/1/1 |
| "np"    || 8/8/8/8    | 4/3/4/3    | 16/32/8/8   | 16/16/12/12 | 1/1/1/1 |
| "high"  || 8/8/8/8    | 3/2/3/2    | 16/32/8/8   | 16/16/16/16 | 1/1/1/1 |
| "vn"    || 8/11/8/11  | 4/6/4/6    | 32/32/16/16 | 16/16/12/12 | 1/1/1/1 |
----------------------------------------------------------------------------
```

```
bm3d.VBasic / bm3d.VFinal
---------------------------------------------------
| profile || radius | ps_num | ps_range | ps_step |
--------------------------------------------------
| "fast"  || 1/1    | 2/2    | 4/5      | 1/1/1/1 |
| "lc"    || 2/2    | 2/2    | 4/5      | 1/1/1/1 |
| "np"    || 3/3    | 2/2    | 5/6      | 1/1/1/1 |
| "high"  || 4/4    | 2/2    | 7/8      | 1/1/1/1 |
| "vn"    || 4/4    | 2/2    | 5/6      | 1/1/1/1 |
---------------------------------------------------
```

```
bm3d.Basic & bm3d.VBasic / bm3d.Final & bm3d.VFinal
--------------------------------------------------------------
| profile || th_mse                              | hard_thr  |
--------------------------------------------------------------
| "fast"  || sigma[0]*80+400   / sigma[0]*10+200 | 2.2 / NUL |
| "lc"    || sigma[0]*80+400   / sigma[0]*10+200 | 2.2 / NUL |
| "np"    || sigma[0]*80+400   / sigma[0]*10+200 | 2.2 / NUL |
| "high"  || sigma[0]*80+400   / sigma[0]*10+200 | 2.2 / NUL |
| "vn"    || sigma[0]*150+1000 / sigma[0]*40+400 | 2.3 / NUL |
--------------------------------------------------------------
```

## Example

### BM3D Example

- basic estimate only, specify different sigma for Y,U,V planes

```python
flt = core.bm3d.Basic(src, sigma=[10,6,8])
```

- basic estimate + final estimate, sigma=10 for Y and sigma=7 for U,V

```python
ref = core.bm3d.Basic(src, sigma=[10,7])
flt = core.bm3d.Final(src, ref, sigma=[10,7])
```

- additional pre-filtered clip as the reference for block-matching of basic estimate, sigma=10 for Y,U,V

```python
pre = haf.sbr(src, 3)
ref = core.bm3d.Basic(src, pre, sigma=10)
flt = core.bm3d.Final(src, ref, sigma=10)
```

- apply the RGB<->OPP conversions separately

```python
src = core.bm3d.RGB2OPP(src) # The output is of 16bit opponent color space
ref = core.bm3d.Basic(src, matrix=100) # Specify the matrix of opponent color space
flt = core.bm3d.Final(src, ref, matrix=100) # Specify the matrix of opponent color space
flt = core.bm3d.OPP2RGB(flt) # The output is of 16bit RGB color space
```

### V-BM3D Example

- basic estimate + final estimate

```python
src = core.bm3d.RGB2OPP(src)
ref = core.bm3d.VBasic(src, radius=1, matrix=100).bm3d.VAggregate(radius=1)
flt = core.bm3d.VFinal(src, ref, radius=1, matrix=100).bm3d.VAggregate(radius=1)
flt = core.bm3d.OPP2RGB(flt)
```

- use bm3d.Basic instead of bm3d.VBasic, faster, less memory consumption

```python
src = core.bm3d.RGB2OPP(src)
ref = core.bm3d.Basic(src, matrix=100)
flt = core.bm3d.VFinal(src, ref, radius=1, matrix=100).bm3d.VAggregate(radius=1)
flt = core.bm3d.OPP2RGB(flt)
```

- Employ custom denoising filter as basic estimate, refined with V-BM3D final estimate.<br />
    May compensate the shortages of both denoising filters: SMDegrain is effective at spatial-temporal smoothing but can lead to blending and detail loss, V-BM3D preserves details well but is not very effective for large noise pattern (such as heavy grain).

```python
src = core.bm3d.RGB2OPP(src)
ref = haf.SMDegrain(src)
flt = core.bm3d.VFinal(src, ref, radius=1, matrix=100).bm3d.VAggregate(radius=1)
flt = core.bm3d.OPP2RGB(flt)
```

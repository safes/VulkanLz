/******************************************************************************
 * Copyright (c) 2019-2023, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/

#if !defined(MDL_RENDERER_RUNTIME_HLSLI)
#define MDL_RENDERER_RUNTIME_HLSLI

// compiler constants defined from outside:
// - MDL_TARGET_REGISTER_SPACE
// - MDL_TARGET_RO_DATA_SEGMENT_SLOT
//
// - MDL_MATERIAL_REGISTER_SPACE
// - MDL_MATERIAL_ARGUMENT_BLOCK_SLOT
// - MDL_MATERIAL_TEXTURE_INFO_SLOT
// - MDL_MATERIAL_MBSDF_INFO_SLOT

// - MDL_MATERIAL_TEXTURE_2D_REGISTER_SPACE
// - MDL_MATERIAL_TEXTURE_3D_REGISTER_SPACE
// - MDL_MATERIAL_TEXTURE_SLOT_BEGIN
// 
// - MDL_MATERIAL_BUFFER_REGISTER_SPACE
// - MDL_MATERIAL_BUFFER_SLOT_BEGIN
//
// - MDL_TEXTURE_SAMPLER_SLOT
// - MDL_LIGHT_PROFILE_SAMPLER_SLOT
// - MDL_MBSDF_SAMPLER_SLOT


/// Information passed to GPU for mapping id requested in the runtime functions to texture
/// views of the corresponding type.
struct Mdl_texture_info
{
    // index into the tex2d, tex3d, ... buffers, depending on the type requested
    uint gpu_resource_array_start;

    // number resources (e.g. uv-tiles) that belong to this resource
    uint gpu_resource_array_size;

    // frame number of the first texture/uv-tile
    int gpu_resource_frame_first;

    // coordinate of the left bottom most uv-tile (also bottom left corner)
    int2 gpu_resource_uvtile_min;

    // in case of uv-tiled textures, required to calculate a linear index (u + v * width
    uint gpu_resource_uvtile_width;
    uint gpu_resource_uvtile_height;

    // get the last frame of an animated texture
    int get_last_frame()
    {
        return gpu_resource_array_size / (gpu_resource_uvtile_width * gpu_resource_uvtile_height)
            + gpu_resource_frame_first - 1;
    }

    // return the resource view index for a given uv-tile id. (see compute_uvtile_and_update_uv(...))
    // returning of -1 indicates out of bounds, 0 refers to the invalid resource.
    int compute_uvtile_id(float frame, int2 uv_tile)
    {
        if (gpu_resource_array_size == 1) // means no uv-tiles
            return int(gpu_resource_array_start);

        // simplest handling possible
        int frame_number = floor(frame) - gpu_resource_frame_first;

        uv_tile -= gpu_resource_uvtile_min;
        const int offset = uv_tile.x +
                           uv_tile.y * int(gpu_resource_uvtile_width) +
                           frame_number * int(gpu_resource_uvtile_width) * int(gpu_resource_uvtile_height);
        if (frame_number < 0 || uv_tile.x < 0 || uv_tile.y < 0 ||
            uv_tile.x >= int(gpu_resource_uvtile_width) ||
            uv_tile.y >= int(gpu_resource_uvtile_height) ||
            offset >= gpu_resource_array_size)
            return -1; // out of bounds

        return int(gpu_resource_array_start) + offset;
    }

    // for uv-tiles, uv coordinate implicitly specifies which resource to use
    // the index of the resource is returned while the uv mapped into the uv-tile
    // if uv-tiles are not used, the data is just passed through
    // returning of -1 indicates out of bounds, 0 refers to the invalid resource.
    int compute_uvtile_and_update_uv(float frame, inout float2 uv)
    {
        if(gpu_resource_array_size == 1) // means no uv-tiles
            return int(gpu_resource_array_start);

        // uv-coordinate in the tile
        const int2 uv_tile = int2(floor(uv)); // floor
        uv = frac(uv);

        // compute a linear index
        return compute_uvtile_id(frame, uv_tile);
    }

    // for texel fetches the uv tile is given explicitly
    int compute_uvtile_and_update_uv(float frame, int2 uv_tile)
    {
        if (gpu_resource_array_size == 1) // means no uv-tiles
            return int(gpu_resource_array_start);

        // compute a linear index
        return compute_uvtile_id(frame, uv_tile);
    }
};

/// Information passed to the GPU for each light profile resource
struct Mdl_light_profile_info
{
    // angular resolution of the grid and its inverse
    uint2 angular_resolution;
    float2 inv_angular_resolution;

    // starting angles of the grid
    float2 theta_phi_start;

    // angular step size and its inverse
    float2 theta_phi_delta;
    float2 theta_phi_inv_delta;

    // factor to rescale the normalized data
    // also represents the maximum candela value of the data
    float candela_multiplier;

    // power (radiant flux)
    float total_power;

    // index into the textures_2d array
    // -  texture contains normalized data sampled on grid
    uint eval_data_index;

    // index into the buffers
    // - CDFs for sampling a light profile
    uint sample_data_index;
};

/// Information passed to the GPU for each BSDF measurement resource
struct Mdl_mbsdf_info
{
    // if the MBSDF has data for reflection (0) and transmission (1)
    uint2 has_data;

    // index into the texture_3d array for both parts
    // - texture contains the measurement values for evaluation
    uint2 eval_data_index;

    // indices into the buffers array for both parts
    // - sample_data buffer contains CDFs for sampling
    // - albedo_data buffer contains max albedos for each theta (isotropic)
    uint2 sample_data_index;
    uint2 albedo_data_index;

    // maximum albedo values for both parts, used for limiting the multiplier
    float2 max_albedo;

    // discrete angular resolution for both parts
    uint2 angular_resolution_theta;
    uint2 angular_resolution_phi;

    // number of color channels (1 for scalar, 3 for rgb) for both parts
    uint2 num_channels;
};

// per target data
ByteAddressBuffer mdl_ro_data_segment : register(MDL_TARGET_RO_DATA_SEGMENT_SLOT);

//StructuredBuffer<Mdl_texture_info> mdl_texture_infos : register(MDL_MATERIAL_TEXTURE_INFO_SLOT, MDL_MATERIAL_REGISTER_SPACE);

Texture2D mdl_textures_2d[NUM_TEXTURES] : register(MDL_MATERIAL_TEXTURE_SLOT_BEGIN);

SamplerState mdl_sampler_tex : register(MDL_TEXTURE_SAMPLER_SLOT);


#if USE_RES_DATA

struct Res_data
{
    uint dummy;
};

#define RES_DATA_PARAM_DECL     Res_data res_data,
#define RES_DATA_PARAM          res_data,

#else

#define RES_DATA_PARAM_DECL
#define RES_DATA_PARAM

#endif

// ------------------------------------------------------------------------------------------------
// Argument block access for dynamic parameters in class compilation mode
// ------------------------------------------------------------------------------------------------

//float mdl_read_argblock_as_float(int offs)
//{
//    return asfloat(mdl_argument_block.Load(offs));
//}
//
//double mdl_read_argblock_as_double(int offs)
//{
//    return asdouble(mdl_argument_block.Load(offs), mdl_argument_block.Load(offs + 4));
//}
//
//int mdl_read_argblock_as_int(int offs)
//{
//    return asint(mdl_argument_block.Load(offs));
//}
//
//uint mdl_read_argblock_as_uint(int offs)
//{
//    return mdl_argument_block.Load(offs);
//}
//
//bool mdl_read_argblock_as_bool(int offs)
//{
//    uint val = mdl_argument_block.Load(offs & ~3);
//    return (val & (0xffU << (8 * (offs & 3)))) != 0;
//}

float mdl_read_rodata_as_float(int offs)
{
    return asfloat(mdl_ro_data_segment.Load(offs));
}

double mdl_read_rodata_as_double(int offs)
{
    return asdouble(mdl_ro_data_segment.Load(offs), mdl_ro_data_segment.Load(offs + 4));
}

int mdl_read_rodata_as_int(int offs)
{
    return asint(mdl_ro_data_segment.Load(offs));
}

int mdl_read_rodata_as_uint(int offs)
{
    return mdl_ro_data_segment.Load(offs);
}

bool mdl_read_rodata_as_bool(int offs)
{
    uint val = mdl_ro_data_segment.Load(offs & ~3);
    return (val & (0xffU << (8 * (offs & 3)))) != 0;
}

// ------------------------------------------------------------------------------------------------
// Texturing functions, check if valid
// ------------------------------------------------------------------------------------------------

// corresponds to ::tex::texture_isvalid(uniform texture_2d tex)
// corresponds to ::tex::texture_isvalid(uniform texture_3d tex)
// corresponds to ::tex::texture_isvalid(uniform texture_cube tex) // not supported by this example
// corresponds to ::tex::texture_isvalid(uniform texture_ptex tex) // not supported by this example
bool tex_texture_isvalid(RES_DATA_PARAM_DECL int tex)
{
    // assuming that there is no indexing out of bounds of the resource_infos and the view arrays
    return tex != 0; // invalid texture
}

// helper function to realize wrap and crop.
// Out of bounds case for TEX_WRAP_CLIP must already be handled.
float apply_wrap_and_crop(
    float coord,
    int wrap,
    float2 crop,
    int res)
{
    if (wrap != TEX_WRAP_REPEAT || any(crop != float2(0, 1)))
    {
        if (wrap == TEX_WRAP_REPEAT)
        {
            coord -= floor(coord);
        }
        else
        {
            if (wrap == TEX_WRAP_MIRRORED_REPEAT)
            {
                float floored_val = floor(coord);
                if ((int(floored_val) & 1) != 0)
                    coord = 1 - (coord - floored_val);
                else
                    coord -= floored_val;
            }
            float inv_hdim = 0.5f / float(res);
            coord = clamp(coord, inv_hdim, 1.f - inv_hdim);
        }
        coord = coord * (crop.y - crop.x) + crop.x;
    }
    return coord;
}

// Modify texture coordinates to get better texture filtering,
// see http://www.iquilezles.org/www/articles/texture/texture.htm
float2 apply_smootherstep_filter(float2 uv, uint2 size)
{
    float2 res;
    res = uv * size + 0.5f;

    float u_i = floor(res.x), v_i = floor(res.y);
    float u_f = res.x - u_i;
    float v_f = res.y - v_i;
    u_f = u_f * u_f * u_f * (u_f * (u_f * 6.f - 15.f) + 10.f);
    v_f = v_f * v_f * v_f * (v_f * (v_f * 6.f - 15.f) + 10.f);
    res.x = u_i + u_f;
    res.y = v_i + v_f;

    res = (res - 0.5f) / size;
    return res;
}

// ------------------------------------------------------------------------------------------------
// Texturing functions, 2D
// ------------------------------------------------------------------------------------------------

// corresponds to ::tex::lookup_float4(uniform texture_2d tex, float2 coord, ...) when derivatives are enabled
// corresponds to ::tex::lookup_float4(uniform texture_2d tex, float2 coord, ...)
//float4 tex_lookup_float4_2d(
//    RES_DATA_PARAM_DECL
//    int tex,
//    float2 coord,
//    int wrap_u,
//    int wrap_v,
//    float2 crop_u,
//    float2 crop_v,
//    float frame)
//{
//    if (tex == 0) return float4(0, 0, 0, 0); // invalid texture
//
//    // fetch the infos about this resource
//    Mdl_texture_info info = mdl_texture_infos[tex - 1]; // assuming this is in bounds
//
//    // handle uv-tiles and/or get texture array index
//    int array_index = info.compute_uvtile_and_update_uv(frame, coord);
//    if (array_index < 0) return float4(0, 0, 0, 0); // out of bounds or no uv-tile
//
//    if (wrap_u == TEX_WRAP_CLIP && (coord.x < 0.0 || coord.x >= 1.0))
//        return float4(0, 0, 0, 0);
//    if (wrap_v == TEX_WRAP_CLIP && (coord.y < 0.0 || coord.y >= 1.0))
//        return float4(0, 0, 0, 0);
//
//    uint2 res;
//    mdl_textures_2d[NonUniformResourceIndex(array_index)].GetDimensions(res.x, res.y);
//    coord.x = apply_wrap_and_crop(coord.x, wrap_u, crop_u, res.x);
//    coord.y = apply_wrap_and_crop(coord.y, wrap_v, crop_v, res.y);
//
//    coord = apply_smootherstep_filter(coord, res);
//
//    // Note, since we don't have ddx and ddy in the compute pipeline, TextureObject::Sample() is not
//    // available, we use SampleLevel instead and go for the most detailed level. Therefore, we don't
//    // need mipmaps. Manual mip level computation is possible though.
//    return mdl_textures_2d[NonUniformResourceIndex(array_index)].SampleLevel(
//        mdl_sampler_tex, coord, /*lod=*/ 0.0f, /*offset=*/ int2(0, 0));
//}

float4 tex_lookup_float4_2d(
    int tex,
    float2 coord,
    int wrap_u,
    int wrap_v,
    float2 crop_u,
    float2 crop_v,
    float frame)
{
    if (tex == 0) return float4(0.f,0.f,0.f,0.f);
    return mdl_textures_2d[NonUniformResourceIndex(tex-1)].SampleLevel(
        mdl_sampler_tex, coord, /*lod=*/ 0.0f, /*offset=*/ int2(0, 0));
    
}



#endif // MDL_RENDERER_RUNTIME_HLSLI

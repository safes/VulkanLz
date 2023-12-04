
//Texture2D mdl_textures_2d[NUM_TEXTURES] : register(t1);
//SamplerState samplerColorMap : register(s1);

struct VS_OUTPUT {
    float4 pos : SV_POSITION;
    [[vk::location(0)]] float3 color : COLOR0;
    [[vk::location(1)]] float2 texcoord : TEXCOORD0;
};

struct user_data {
    // Material pattern as chosen by the user.
    unsigned int material_pattern;

    // Current time in seconds since the start of the render loop.
    float animation_time;
};

[[vk::push_constant]] user_data pushConsts;

void setup_mdl_shading_state(
    in VS_OUTPUT input,
    out Shading_state_material mdl_state
    )
{

    //State state = State(
    //    /*normal=*/                 vec3(0.0, 0.0, 1.0),
    //    /*geometry_normal=*/        vec3(0.0, 0.0, 1.0),
    //    /*position=*/               -vPosition,
    //    /*animation_time=*/         user_data.animation_time,
    //    /*text_coords=*/            vec3[1](vTexCoord),
    //    /*texture_tangent_u=*/      vec3[1](vec3(1.0, 0.0, 0.0)),
    //    /*texture_tangent_v=*/      vec3[1](vec3(0.0, 1.0, 0.0)),
    //    /*ro_data_segment_offset=*/ 0,
    //    /*world_to_object=*/        mat4(1.0),
    //    /*object_to_world=*/        mat4(1.0),
    //    /*object_id=*/              0,
    //    /*meters_per_scene_unit=*/  1.0,
    //    /*arg_block_offset=*/       0
    //);

    // fill the actual state fields used by MD
    mdl_state.normal = float3(0.f,0.f,1.f);
    mdl_state.geom_normal = float3(0.f,0.f,1.f);
#if defined(USE_DERIVS)
    // currently not supported
    mdl_state.position.val = -input.pos.xyz;
    mdl_state.position.dx = float3(0, 0, 0);
    mdl_state.position.dy = float3(0, 0, 0);
#else
    mdl_state.position = -input.pos.xyz;
#endif
    mdl_state.animation_time = pushConsts.animation_time;
    mdl_state.tangent_u[0] = float3(1.f, 0.f, 0.f); //world_tangent;
    mdl_state.tangent_v[0] = float3(0.f, 1.f, 0.f);//world_binormal;
    // #if defined(USE_TEXTURE_RESULTS)
    // filling the buffer with zeros not required
    //     mdl_state.text_results = (float4[MDL_NUM_TEXTURE_RESULTS]) 0;
    // #endif
    mdl_state.ro_data_segment_offset = 0;
    mdl_state.world_to_object = float4x4(float4(1.f,0.f,0.f,0.f),float4(0.f,1.f,0.f,0.f),
        float4(0.f,0.f,1.f,0.f),float4(0.f,0.f,0.f,1.f));

    mdl_state.object_to_world = float4x4(float4(1.f, 0.f, 0.f, 0.f), float4(0.f, 1.f, 0.f, 0.f),
        float4(0.f, 0.f, 1.f, 0.f), float4(0.f, 0.f, 0.f, 1.f));
    mdl_state.object_id = 0;
    mdl_state.meters_per_scene_unit = 1.0f;
    mdl_state.arg_block_offset = 0;

    // fill the renderer state information
    //mdl_state.renderer_state.scene_data_info_offset = geometry_scene_data_info_offset;
    //mdl_state.renderer_state.scene_data_geometry_byte_offset = geometry_vertex_buffer_byte_offset;
    //mdl_state.renderer_state.hit_vertex_indices = vertex_indices;
    //mdl_state.renderer_state.barycentric = barycentric;

    // get texture coordinates using a manually added scene data element with the scene data id
    // defined as `SCENE_DATA_ID_TEXCOORD_0`
    // (see end of target code generation on application side)
    /*float2 texcoord0 = scene_data_lookup_float2(
        mdl_state, SCENE_DATA_ID_TEXCOORD_0, float2(0.0f, 0.0f), false);*/

    // apply uv transformatins
      

#if defined(USE_DERIVS)
    // would make sense in a rasterizer. for a ray tracers this is not straight forward
    mdl_state.text_coords[0].val = float3(input.texcoord, 0);
    mdl_state.text_coords[0].dx = float3(0, 0, 0); // float3(ddx(texcoord0), 0);
    mdl_state.text_coords[0].dy = float3(0, 0, 0); // float3(ddy(texcoord0), 0);
#else
    mdl_state.text_coords[0] = float3(input.texcoord, 0);
#endif

}


float4 main(VS_OUTPUT input) : SV_TARGET
{
	float4 color;
    Shading_state_material mdl_state;
    setup_mdl_shading_state(input,mdl_state );
	//color = texturecolor.Sample(samplerColorMap, input.texcoord);
    color = float4( tint(mdl_state).xyz,1.f);
	//return color;
	return float4(color.rgb * input.color ,1.f);
}
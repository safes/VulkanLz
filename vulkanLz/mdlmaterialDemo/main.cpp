#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

//#include <mi/mdl_sdk.h>
#include <iostream>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <algorithm>
#include "shader.h"
#include "Sample01Application.h"

#define NUM_TEXTURE_SPACES 1
#define NUM_TEXTURE_RESULTS 16
namespace mi {
    namespace lz {
        namespace mdl {
            extern mi::base::Handle<mi::base::ILogger> g_logger;
        }
    }
}

// required for loading and unloading the SDK
#ifdef _WIN32
    HMODULE g_dso_handle = 0;
#else
    void* g_dso_handle = 0;
#endif
    class Default_logger : public mi::base::Interface_implement<mi::base::ILogger>
    {
    public:
        void message(mi::base::Message_severity level,
            const char* /* module_category */,
            const mi::base::Message_details& /* details */,
            const char* message) override
        {
            const char* severity = 0;

            switch (level)
            {
            case mi::base::MESSAGE_SEVERITY_FATAL:
                severity = "FATAL: ";
                assert(!"Default_logger() fatal error.");
                break;
            case mi::base::MESSAGE_SEVERITY_ERROR:
                severity = "ERROR: ";
                assert(!"Default_logger() error.");
                break;
            case mi::base::MESSAGE_SEVERITY_WARNING:
                severity = "WARN:  ";
                break;
            case mi::base::MESSAGE_SEVERITY_INFO:
                //return; // DEBUG No info messages.
                severity = "INFO:  ";
                break;
            case mi::base::MESSAGE_SEVERITY_VERBOSE:
                return; // DEBUG No verbose messages.
            case mi::base::MESSAGE_SEVERITY_DEBUG:
                return; // DEBUG No debug messages.
            case mi::base::MESSAGE_SEVERITY_FORCE_32_BIT:
                return;
            }

            std::cerr << severity << message << '\n';
        }

        void message(mi::base::Message_severity level,
            const char* module_category,
            const char* message) override
        {
            this->message(level, module_category, mi::base::Message_details(), message);
        }
    };
static mi::base::Handle<mi::neuraylib::INeuray> s_neuray;
static mi::base::Handle<mi::neuraylib::IMdl_compiler>   s_mdl_compiler;
static mi::base::Handle<mi::neuraylib::IMdl_configuration> s_mdl_config;
static mi::base::Handle<mi::neuraylib::IDatabase> s_database;
static mi::base::Handle<mi::neuraylib::IScope> s_global_scope;
static mi::base::Handle<mi::neuraylib::IMdl_factory> s_mdl_factory;
static mi::base::Handle<mi::neuraylib::IMdl_execution_context> s_execution_context;
static mi::base::Handle<mi::neuraylib::IMdl_backend> s_mdl_backend;
static mi::base::Handle<mi::neuraylib::IImage_api> s_image_api;
static mi::base::Handle<mi::neuraylib::IMdl_impexp_api> s_mdl_impexp_api;
static mi::base::Handle<mi::neuraylib::ITransaction> s_transaction;
static mi::base::Handle<mi::neuraylib::IMdl_backend_api> s_mdl_backend_api;

mi::neuraylib::INeuray* load_and_get_ineuray(const char* filename)
{
    if (!filename)
    {
        //#ifdef IRAY_SDK
        //    filename = "libneuray" MI_BASE_DLL_FILE_EXT;
        //#else
        filename = "libmdl_sdk" MI_BASE_DLL_FILE_EXT;
        //#endif
    }

#ifdef _WIN32

    HMODULE handle = LoadLibraryA(filename);
    //if (!handle)
    //{
    //  // fall back to libraries in a relative lib folder, relevant for install targets
    //  std::string fallback = std::string("../../../lib/") + filename;
    //  handle = LoadLibraryA(fallback.c_str());
    //}
    if (!handle)
    {
        DWORD error_code = GetLastError();
        std::cerr << "ERROR: LoadLibraryA(" << filename << ") failed with error code " << error_code << '\n';
        return 0;
    }

    void* symbol = GetProcAddress(handle, "mi_factory");
    if (!symbol)
    {
        DWORD error_code = GetLastError();
        std::cerr << "ERROR: GetProcAddress(handle, \"mi_factory\") failed with error " << error_code << '\n';
        return 0;
    }

#else // MI_PLATFORM_WINDOWS

    void* handle = dlopen(filename, RTLD_LAZY);
    //if (!handle)
    //{
    //  // fall back to libraries in a relative lib folder, relevant for install targets
    //  std::string fallback = std::string("../../../lib/") + filename;
    //  handle = dlopen(fallback.c_str(), RTLD_LAZY);
    //}
    if (!handle)
    {
        std::cerr << "ERROR: dlopen(" << filename << " , RTLD_LAZY) failed with error code " << dlerror() << '\n';
        return 0;
    }

    void* symbol = dlsym(handle, "mi_factory");
    if (!symbol)
    {
        std::cerr << "ERROR: dlsym(handle, \"mi_factory\") failed with error " << dlerror() << '\n';
        return 0;
    }

#endif // MI_PLATFORM_WINDOWS

    g_dso_handle = handle;

    mi::neuraylib::INeuray* neuray = mi::neuraylib::mi_factory<mi::neuraylib::INeuray>(symbol);
    if (!neuray)
    {
        mi::base::Handle<mi::neuraylib::IVersion> version(mi::neuraylib::mi_factory<mi::neuraylib::IVersion>(symbol));
        if (!version)
        {
            std::cerr << "ERROR: Incompatible library. Could not determine version.\n";
        }
        else
        {
            std::cerr << "ERROR: Library version " << version->get_product_version() << " does not match header version " << MI_NEURAYLIB_PRODUCT_VERSION_STRING << '\n';
        }
        return 0;
    }

    //#ifdef IRAY_SDK
    //  if (authenticate(neuray) != 0)
    //  {
    //    std::cerr << "ERROR: Iray SDK Neuray Authentication failed.\n";
    //    unload();
    //    return 0;
    //  }
    //#endif

    return neuray;
}

mi::Sint32 load_plugin(mi::neuraylib::INeuray* neuray, const char* path)
{
    mi::base::Handle<mi::neuraylib::IPlugin_configuration> plugin_conf(neuray->get_api_component<mi::neuraylib::IPlugin_configuration>());

    // Try loading the requested plugin before adding any special handling
    mi::Sint32 res = plugin_conf->load_plugin_library(path);
    if (res == 0)
    {
        //std::cerr << "load_plugin(" << path << ") succeeded.\n"; // DEBUG The logger prints this.
        return 0;
    }

    // Special handling for freeimage in the open source release.
    // In the open source version of the plugin we are linking against a dynamic vanilla freeimage library.
    // In the binary release, you can download from the MDL website, freeimage is linked statically and 
    // thereby requires no special handling.
#if defined(_WIN32) && defined(MDL_SOURCE_RELEASE)
    if (strstr(path, "nv_freeimage" MI_BASE_DLL_FILE_EXT) != nullptr)
    {
        // Load the freeimage (without nv_ prefix) first.
        std::string freeimage_3rd_party_path = mi::lz::strings::replace(path,
            "nv_freeimage" MI_BASE_DLL_FILE_EXT, "freeimage" MI_BASE_DLL_FILE_EXT);
        HMODULE handle_tmp = LoadLibraryA(freeimage_3rd_party_path.c_str());
        if (!handle_tmp)
        {
            DWORD error_code = GetLastError();
            std::cerr << "ERROR: load_plugin(" << freeimage_3rd_party_path << " failed with error " << error_code << '\n';
        }
        else
        {
            std::cerr << "Pre-loading library " << freeimage_3rd_party_path << " succeeded\n";
        }

        // Try to load the plugin itself now
        res = plugin_conf->load_plugin_library(path);
        if (res == 0)
        {
            std::cerr << "load_plugin(" << path << ") succeeded.\n"; // DAR FIXME The logger prints this as info anyway.
            return 0;
        }
    }
#endif

    // return the failure code
    std::cerr << "ERROR: load_plugin(" << path << ") failed with error " << res << '\n';

    return res;
}


bool InitMDL(const std::vector<std::string>& searchPaths)
{
    s_neuray = load_and_get_ineuray(nullptr);
    s_mdl_compiler = s_neuray->get_api_component<mi::neuraylib::IMdl_compiler>();
    s_mdl_config = s_neuray->get_api_component<mi::neuraylib::IMdl_configuration>();
    mi::lz::mdl::g_logger = mi::base::make_handle(new Default_logger());
    s_mdl_config->set_logger(mi::lz::mdl::g_logger.get());
    s_mdl_config->add_mdl_system_paths();
    s_mdl_config->add_mdl_user_paths();

    for (auto const& path : searchPaths)
    {
        mi::Sint32 result = s_mdl_config->add_mdl_path(path.c_str());
        if (result != 0)
        {
            std::cerr << "WARNING: add_mdl_path( " << path << ") failed with " << result << '\n';
        }

        result = s_mdl_config->add_resource_path(path.c_str());
        if (result != 0)
        {
            std::cerr << "WARNING: add_resource_path( " << path << ") failed with " << result << '\n';
        }
    }
    // Load plugins.
#if USE_OPENIMAGEIO_PLUGIN
    if (load_plugin(s_neuray.get(), "nv_openimageio" MI_BASE_DLL_FILE_EXT) != 0)
    {
        std::cerr << "FATAL: Failed to load nv_openimageio plugin\n";
        return false;
    }
#else
    if (load_plugin(s_neuray.get(), "nv_freeimage" MI_BASE_DLL_FILE_EXT) != 0)
    {
        std::cerr << "FATAL: Failed to load nv_freeimage plugin\n";
        return false;
    }
#endif

    if (load_plugin(s_neuray.get(), "dds" MI_BASE_DLL_FILE_EXT) != 0)
    {
        std::cerr << "FATAL: Failed to load dds plugin\n";
        return false;
    }

    if (s_neuray->start() != 0)
    {
        std::cerr << "FATAL: Starting MDL SDK failed.\n";
        return false;
    }

    s_database = s_neuray->get_api_component<mi::neuraylib::IDatabase>();

    s_global_scope = s_database->get_global_scope();

    s_mdl_factory = s_neuray->get_api_component<mi::neuraylib::IMdl_factory>();
    
    // Configure the execution context.
    // Used for various configurable operations and for querying warnings and error messages.
    // It is possible to have more than one, in order to use different settings.
    s_execution_context = s_mdl_factory->create_execution_context();
    s_execution_context->set_option("mdl_next", false);
    s_execution_context->set_option("fold_ternary_on_df", false);
    s_execution_context->set_option("internal_space", "coordinate_world");
    //s_execution_context->set_option("internal_space", "coordinate_world");  // equals default
    //s_execution_context->set_option("bundle_resources", false);             // equals default
    //s_execution_context->set_option("meters_per_scene_unit", 1.0f);         // equals default
    //s_execution_context->set_option("mdl_wavelength_min", 380.0f);          // equals default
    //s_execution_context->set_option("mdl_wavelength_max", 780.0f);          // equals default
    //// If true, the "geometry.normal" field will be applied to the MDL state prior to evaluation of the given DF.
    //s_execution_context->set_option("include_geometry_normal", true);       // equals default 

    mi::base::Handle<mi::neuraylib::IMdl_backend_api> mdl_backend_api(s_neuray->get_api_component<mi::neuraylib::IMdl_backend_api>());

    s_mdl_backend = mdl_backend_api->get_backend(mi::neuraylib::IMdl_backend_api::MB_HLSL);

    // Hardcoded values!
    assert(NUM_TEXTURE_SPACES == 1 || NUM_TEXTURE_SPACES == 2);
    // The renderer only supports one or two texture spaces.
    // The hair BSDF requires two texture coordinates! 
    // If you do not use the hair BSDF, NUM_TEXTURE_SPACES should be set to 1 for performance reasons.

    if (s_mdl_backend->set_option("num_texture_spaces", std::to_string(NUM_TEXTURE_SPACES).c_str()) != 0)
    {
        return false;
    }

    if (s_mdl_backend->set_option("num_texture_results", std::to_string(NUM_TEXTURE_RESULTS).c_str()) != 0)
    {
        return false;
    }

    //if (s_mdl_backend->set_option("texture_runtime_with_derivs", "on" ) != 0)
    //    return false;

    //if (s_mdl_backend->set_option("enable_auxiliary", "on" ) != 0)
    //    return false;


    // The HLSL backend supports no pointers, which means we need use fixed size arrays
    if (s_mdl_backend->set_option("df_handle_slot_mode", "none") != 0)
    {
       // log_error("Backend option 'df_handle_slot_mode' invalid.", SRC);
        return false;
    }

    // Use SM 5.0 for Maxwell and above.
 /*   if (s_mdl_backend->set_option("sm_version", "50") != 0)
    {
        return false;
    }*/

 /*   if (s_mdl_backend->set_option("tex_lookup_call_mode", "direct_call") != 0)
    {
        return false;
    }*/

    //if (enable_derivatives) // == false. Not supported in this renderer
    //{
    //  // Option "texture_runtime_with_derivs": Default is disabled.
    //  // We enable it to get coordinates with derivatives for texture lookup functions.
    //  if (m_mdl_backend->set_option("texture_runtime_with_derivs", "on") != 0)
    //  {
    //    return false;
    //  }
    //}

  /*  if (s_mdl_backend->set_option("inline_aggressively", "on") != 0)
    {
        return false;
    }*/

    // FIXME Determine what scene data the renderer needs to provide here.
    // FIXME scene_data_names is not a supported option anymore!
    //if (m_mdl_backend->set_option("scene_data_names", "*") != 0)
    //{
    //  return false;
    //}

    // PERF Disable code generation for distribution pdf functions.
    // The unidirectional light transport in this renderer never calls them.
    // The sample and evaluate functions return the necessary pdf values.
    if (s_mdl_backend->set_option("enable_pdf", "off") != 0)
    {
        std::cerr << "WARNING: Raytracer::initMDL() Setting backend option enable_pdf to off failed.\n";
        // Not a fatal error if this cannot be set.
        // return false;
    }
    s_image_api = s_neuray->get_api_component<mi::neuraylib::IImage_api>();
   
    s_mdl_impexp_api = s_neuray->get_api_component<mi::neuraylib::IMdl_impexp_api>();
    //mi::base::Handle<mi::neuraylib::IScope> scope(s_database->get_global_scope());
    //mi::base::Handle<mi::neuraylib::ITransaction> transaction(scope->create_transaction());

    s_transaction = s_global_scope->create_transaction();
   // s_transaction = s_neuray->get_api_component<mi::neuraylib::ITransaction>();
    s_mdl_backend_api = s_neuray->get_api_component<mi::neuraylib::IMdl_backend_api>();

    return true;

}

inline bool parse_cmd_argument_material_name(
    const std::string& argument,
    std::string& out_module_name,
    std::string& out_material_name,
    bool prepend_colons_if_missing)
{
    out_module_name = "";
    out_material_name = "";
    std::size_t p_left_paren = argument.rfind('(');
    if (p_left_paren == std::string::npos)
        p_left_paren = argument.size();
    std::size_t p_last = argument.rfind("::", p_left_paren - 1);

    bool starts_with_colons = argument.length() > 2 && argument[0] == ':' && argument[1] == ':';

    // check for mdle
    if (!starts_with_colons)
    {
        std::string potential_path = argument;
        std::string potential_material_name = "main";

        // input already has ::main attached (optional)
        if (p_last != std::string::npos)
        {
            potential_path = argument.substr(0, p_last);
            potential_material_name = argument.substr(p_last + 2, argument.size() - p_last);
        }

        // is it an mdle?
        if (mi::lz::strings::ends_with(potential_path, ".mdle"))
        {
            if (potential_material_name != "main")
            {
                fprintf(stderr, "Error: Material and module name cannot be extracted from "
                    "'%s'.\nThe module was detected as MDLE but the selected material is "
                    "different from 'main'.\n", argument.c_str());
                return false;
            }
            out_module_name = potential_path;
            out_material_name = potential_material_name;
            return true;
        }
    }

    if (p_last == std::string::npos ||
        p_last == 0 ||
        p_last == argument.length() - 2 ||
        (!starts_with_colons && !prepend_colons_if_missing))
    {
        fprintf(stderr, "Error: Material and module name cannot be extracted from '%s'.\n"
            "An absolute fully-qualified material name of form "
            "'[::<package>]::<module>::<material>' is expected.\n", argument.c_str());
        return false;
    }

    if (!starts_with_colons && prepend_colons_if_missing)
    {
        fprintf(stderr, "Warning: The provided argument '%s' is not an absolute fully-qualified"
            " material name, a leading '::' has been added.\n", argument.c_str());
        out_module_name = "::";
    }

    out_module_name.append(argument.substr(0, p_last));
    out_material_name = argument.substr(p_last + 2, argument.size() - p_last);
    return true;
}

inline void print_message(
    mi::base::details::Message_severity severity,
    mi::neuraylib::IMessage::Kind kind,
    const char* msg)
{
    std::string s_kind = mi::lz::strings::to_string(kind);

    if (mi::lz::mdl::g_logger) {
        mi::lz::mdl::g_logger->message(severity, s_kind.c_str(), msg);
    }
    else {
        std::string s_severity = mi::lz::strings::to_string(severity);

        fprintf(stderr, "%s: %s %s\n", s_severity.c_str(), s_kind.c_str(), msg);
    }
}

/// Prints the messages of the given context.
/// Returns true, if the context does not contain any error messages, false otherwise.
inline bool print_messages(mi::neuraylib::IMdl_execution_context* context)
{
    for (mi::Size i = 0, n = context->get_messages_count(); i < n; ++i) {
        mi::base::Handle<const mi::neuraylib::IMessage> message(context->get_message(i));
        print_message(message->get_severity(), message->get_kind(), message->get_string());
    }
    return context->get_error_messages_count() == 0;
}

mi::neuraylib::ICompiled_material* compile_material_instance(
    mi::neuraylib::IFunction_call* material_instance,
    mi::neuraylib::IMdl_execution_context* context,
    bool class_compilation)
{
    mi::base::Handle<const mi::neuraylib::IMaterial_instance> material_instance2(
        material_instance->get_interface<mi::neuraylib::IMaterial_instance>());

    mi::Uint32 compile_flags = class_compilation
        ? mi::neuraylib::IMaterial_instance::CLASS_COMPILATION
        : mi::neuraylib::IMaterial_instance::DEFAULT_OPTIONS;

    mi::base::Handle<mi::neuraylib::ICompiled_material> compiled_material(
        material_instance2->create_compiled_material(compile_flags, context));
    check_success(print_messages(context));

    compiled_material->retain();
    return compiled_material.get();
}

mi::neuraylib::IFunction_call* create_material_instance(
    mi::neuraylib::IMdl_impexp_api* mdl_impexp_api,
    mi::neuraylib::IMdl_factory* mdl_factory,
    mi::neuraylib::ITransaction* transaction,
    mi::neuraylib::IMdl_execution_context* context,
    const std::string& material_name)
{
    // Split material name into module and simple material name
    std::string module_name, material_simple_name;
    //parse_cmd_argument_material_name(
    //    material_name, module_name, material_simple_name,true);
    module_name = material_name; //"::bsdf_microfacet_beckmann_smith_reflect";
    // Load module
    mdl_impexp_api->load_module(transaction, module_name.c_str(), context);
    if (!print_messages(context))
        exit_failure("Loading module '%s' failed.", module_name.c_str());

    // Get the database name for the module we loaded and check if
    // the module exists in the database.
    mi::base::Handle<const mi::IString> module_db_name(
        mdl_factory->get_db_definition_name(module_name.c_str()));
    mi::base::Handle<const mi::neuraylib::IModule> module(
        transaction->access<mi::neuraylib::IModule>(module_db_name->get_c_str()));
    if (!module)
        exit_failure("Failed to access the loaded module.");

    material_simple_name = "example_execution2";//"bsdf_diffuse_reflection";
    // To access the material in the database we need to know the exact material
    // signature, so we append the arguments to the full name (with module).
    std::string material_db_name
        = std::string(module_db_name->get_c_str()) + "::" + material_simple_name;
    material_db_name = mi::lz::mdl::add_missing_material_signature(
        module.get(), material_db_name.c_str());
    module.reset();
    mi::base::Handle<const mi::neuraylib::IFunction_definition> material_definition(
        transaction->access<mi::neuraylib::IFunction_definition>(material_db_name.c_str()));
    if (!material_definition)
        exit_failure("Failed to access material definition '%s'.", material_db_name.c_str());

    // Create material instance
    mi::Sint32 result;
    mi::base::Handle<mi::neuraylib::IFunction_call> material_instance(
        material_definition->create_function_call(nullptr, &result));
    if (result != 0)
        exit_failure("Failed to instantiate material '%s'.", material_db_name.c_str());

    material_instance->retain();
    return material_instance.get();
}

const mi::neuraylib::ITarget_code* generate_hlsl_code(
    mi::neuraylib::ICompiled_material* compiled_material,
    mi::neuraylib::IMdl_backend_api* mdl_backend_api,
    mi::neuraylib::ITransaction* transaction,
    mi::neuraylib::IMdl_execution_context* context)
{
    // Add compiled material to link unit
 /*   mi::base::Handle<mi::neuraylib::IMdl_backend> be_shader(
        mdl_backend_api->get_backend(mi::neuraylib::IMdl_backend_api::MB_HLSL));*/

 /*   check_success(be_shader->set_option("glsl_version", "450") == 0);
    check_success(be_shader->set_option("glsl_place_uniforms_into_ssbo", "on") == 0);
    check_success(be_shader->set_option("glsl_max_const_data", "0") == 0);
    check_success(be_shader->set_option("glsl_uniform_ssbo_binding",
        std::to_string(g_binding_ro_data_buffer).c_str()) == 0);
    check_success(be_shader->set_option("glsl_uniform_ssbo_set",
        std::to_string(g_set_ro_data_buffer).c_str()) == 0);
    check_success(be_shader->set_option("num_texture_spaces", "1") == 0);
    check_success(be_shader->set_option("enable_auxiliary", "on") == 0);
    check_success(be_shader->set_option("df_handle_slot_mode", "none") == 0);*/

    mi::base::Handle<mi::neuraylib::ILink_unit> link_unit(
        s_mdl_backend->create_link_unit(transaction, context));

    // Specify which functions to generate
    std::vector<mi::neuraylib::Target_function_description> function_descs;
  
    function_descs.emplace_back("surface.scattering.tint", "tint");
  /*  function_descs.emplace_back("surface.emission.emission", "mdl_edf");
    function_descs.emplace_back("surface.emission.intensity", "mdl_emission_intensity");
    function_descs.emplace_back("volume.absorption_coefficient", "mdl_absorption_coefficient");
    function_descs.emplace_back("geometry.cutout_opacity", "mdl_cutout_opacity");*/

    // Try to determine if the material is thin walled so we can check
    // if backface functions need to be generated.
    
    bool is_thin_walled_function = true;
    bool thin_walled_value = false;
    mi::base::Handle<const mi::neuraylib::IExpression> thin_walled_expr(
        compiled_material->lookup_sub_expression("thin_walled"));
    if (thin_walled_expr->get_kind() == mi::neuraylib::IExpression::EK_CONSTANT)
    {
        mi::base::Handle<const mi::neuraylib::IExpression_constant> thin_walled_const(
            thin_walled_expr->get_interface<const mi::neuraylib::IExpression_constant>());
        mi::base::Handle<const mi::neuraylib::IValue_bool> thin_walled_bool(
            thin_walled_const->get_value<mi::neuraylib::IValue_bool>());

        is_thin_walled_function = false;
        thin_walled_value = thin_walled_bool->get_value();
    }

    // Back faces could be different for thin walled materials
    bool need_backface_bsdf = false;
    bool need_backface_edf = false;
    bool need_backface_emission_intensity = false;

    if (is_thin_walled_function || thin_walled_value)
    {
        // First, backfacs DFs are only considered for thin_walled materials

        // Second, we only need to generate new code if surface and backface are different
        need_backface_bsdf =
            compiled_material->get_slot_hash(mi::neuraylib::SLOT_SURFACE_SCATTERING)
            != compiled_material->get_slot_hash(mi::neuraylib::SLOT_BACKFACE_SCATTERING);
        need_backface_edf =
            compiled_material->get_slot_hash(mi::neuraylib::SLOT_SURFACE_EMISSION_EDF_EMISSION)
            != compiled_material->get_slot_hash(mi::neuraylib::SLOT_BACKFACE_EMISSION_EDF_EMISSION);
        need_backface_emission_intensity =
            compiled_material->get_slot_hash(mi::neuraylib::SLOT_SURFACE_EMISSION_INTENSITY)
            != compiled_material->get_slot_hash(mi::neuraylib::SLOT_BACKFACE_EMISSION_INTENSITY);

        // Third, either the bsdf or the edf need to be non-default (black)
        mi::base::Handle<const mi::neuraylib::IExpression> scattering_expr(
            compiled_material->lookup_sub_expression("backface.scattering"));
        mi::base::Handle<const mi::neuraylib::IExpression> emission_expr(
            compiled_material->lookup_sub_expression("backface.emission.emission"));

        if (scattering_expr->get_kind() == mi::neuraylib::IExpression::EK_CONSTANT
            && emission_expr->get_kind() == mi::neuraylib::IExpression::EK_CONSTANT)
        {
            mi::base::Handle<const mi::neuraylib::IExpression_constant> scattering_expr_constant(
                scattering_expr->get_interface<mi::neuraylib::IExpression_constant>());
            mi::base::Handle<const mi::neuraylib::IValue> scattering_value(
                scattering_expr_constant->get_value());

            mi::base::Handle<const mi::neuraylib::IExpression_constant> emission_expr_constant(
                emission_expr->get_interface<mi::neuraylib::IExpression_constant>());
            mi::base::Handle<const mi::neuraylib::IValue> emission_value(
                emission_expr_constant->get_value());

            if (scattering_value->get_kind() == mi::neuraylib::IValue::VK_INVALID_DF
                && emission_value->get_kind() == mi::neuraylib::IValue::VK_INVALID_DF)
            {
                need_backface_bsdf = false;
                need_backface_edf = false;
                need_backface_emission_intensity = false;
            }
        }
    }

    if (need_backface_bsdf)
        function_descs.emplace_back("backface.scattering", "mdl_backface_bsdf");

    if (need_backface_edf)
        function_descs.emplace_back("backface.emission.emission", "mdl_backface_edf");

    if (need_backface_emission_intensity)
        function_descs.emplace_back("backface.emission.intensity", "mdl_backface_emission_intensity");

    link_unit->add_material_expression(
        compiled_material, function_descs[0].path, function_descs[0].base_fname, context);
    check_success(print_messages(context));

    // Generate GLSL code
    mi::base::Handle<const mi::neuraylib::ITarget_code> target_code(
        s_mdl_backend->translate_link_unit(link_unit.get(), context));
    check_success(print_messages(context));
    check_success(target_code);

    target_code->retain();
    return target_code.get();
}


int main()
{
	glm::mat4 matrix;
	glm::vec4 vec;
	auto test = matrix * vec;
    std::vector<std::string> paths{ "./mdl" };
    {
        if (!InitMDL(paths)) {
            return -1;
        }

        mi::base::Handle<mi::neuraylib::IFunction_call> moduleIns(
            create_material_instance(s_mdl_impexp_api.get(), s_mdl_factory.get(),
                s_transaction.get(), s_execution_context.get(),
                "::tutorials"));
        mi::base::Handle<mi::neuraylib::ICompiled_material> compiledMat(
            compile_material_instance(moduleIns.get(),
                s_execution_context.get(), false));

        mi::base::Handle<const mi::neuraylib::ITarget_code> target_code(
            generate_hlsl_code(compiledMat.get(), s_mdl_backend_api.get(),
                s_transaction.get(), s_execution_context.get()));

        {
            Sample01Application app{ s_transaction,s_mdl_impexp_api,s_image_api,
                target_code };
            app.run();
        }
        
    }
    s_transaction->commit();
    s_mdl_compiler.reset();
    s_mdl_config.reset();
    s_database.reset();
    s_global_scope.reset();
    s_mdl_factory.reset();
     s_execution_context.reset();
    s_mdl_backend.reset();
    s_image_api.reset();
    s_mdl_impexp_api.reset();
    s_transaction.reset();
     s_mdl_backend_api.reset();
    
    
    // Shut down the MDL SDK
    mi::lz::mdl::g_logger.reset();
    if (s_neuray->shutdown() != 0)
        exit_failure("Failed to shutdown the SDK.");
   
    // Unload the MDL SDK
    s_neuray.reset();
    s_neuray = nullptr;
    if (!mi::lz::mdl::unload())
        exit_failure("Failed to unload the SDK.");
	
    
	return 0;

}
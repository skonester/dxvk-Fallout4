dxvk_shaders = files([
  'shaders/dxvk_cursor_vert.vert',
  'shaders/dxvk_cursor_frag.frag',

  'shaders/dxvk_dummy_frag.frag',

  'shaders/dxvk_mipgen.comp',

  'shaders/dxvk_present_frag.frag',
  'shaders/dxvk_present_frag_blit.frag',
  'shaders/dxvk_present_frag_ms.frag',
  'shaders/dxvk_present_frag_ms_blit.frag',
  'shaders/dxvk_present_vert.vert',

  'hud/shaders/hud_chunk_frag_background.frag',
  'hud/shaders/hud_chunk_frag_visualize.frag',
  'hud/shaders/hud_chunk_vert_background.vert',
  'hud/shaders/hud_chunk_vert_visualize.vert',

  'hud/shaders/hud_frame_time_eval.comp',

  'hud/shaders/hud_graph_frag.frag',
  'hud/shaders/hud_graph_vert.vert',

  'hud/shaders/hud_simd_graph_frag.frag',

  'hud/shaders/hud_text_frag.frag',
  'hud/shaders/hud_text_vert.vert',
])

dxvk_src = [
  'dxvk_access.cpp',
  'dxvk_adapter.cpp',
  'dxvk_barrier.cpp',
  'dxvk_buffer.cpp',
  'dxvk_cmdlist.cpp',
  'dxvk_compute.cpp',
  'dxvk_context.cpp',
  'dxvk_cs.cpp',
  'dxvk_descriptor_heap.cpp',
  'dxvk_descriptor_pool.cpp',
  'dxvk_descriptor_worker.cpp',
  'dxvk_device.cpp',
  'dxvk_device_filter.cpp',
  'dxvk_device_info.cpp',
  'dxvk_fence.cpp',
  'dxvk_format.cpp',
  'dxvk_framebuffer.cpp',
  'dxvk_gpu_event.cpp',
  'dxvk_gpu_query.cpp',
  'dxvk_graphics.cpp',
  'dxvk_image.cpp',
  'dxvk_implicit_resolve.cpp',
  'dxvk_instance.cpp',
  'dxvk_latency_builtin.cpp',
  'dxvk_latency_reflex.cpp',
  'dxvk_memory.cpp',
  'dxvk_meta_blit.cpp',
  'dxvk_meta_copy.cpp',
  'dxvk_meta_mipgen.cpp',
  'dxvk_meta_resolve.cpp',
  'dxvk_options.cpp',
  'dxvk_pipelayout.cpp',
  'dxvk_platform_exts.cpp',
  'dxvk_presenter.cpp',
  'dxvk_queue.cpp',
  'dxvk_sampler.cpp',
  'dxvk_shader.cpp',
  'dxvk_shader_cache.cpp',
  'dxvk_shader_io.cpp',
  'dxvk_shader_ir.cpp',
  'dxvk_shader_spirv.cpp',
  'dxvk_signal.cpp',
  'dxvk_sparse.cpp',
  'dxvk_staging.cpp',
  'dxvk_swapchain_blitter.cpp',
  'dxvk_unbound.cpp',

  'hud/dxvk_hud.cpp',
  'hud/dxvk_hud_font.cpp',
  'hud/dxvk_hud_item.cpp',
  'hud/dxvk_hud_renderer.cpp',
]

if platform == 'windows'
  dxvk_src += [
    'dxvk_openvr.cpp',
    'dxvk_openxr.cpp',
  ]
endif

dxvk_extra_deps = [ dependency('threads') ]
if platform == 'linux'
  dxvk_extra_deps += [ cpp.find_library('dl', required: false) ]
endif

dxvk_link_libs = [ util_lib, spirv_lib, wsi_lib ]

if get_option('dxvk_experimental_simd')
  dxvk_allocator_lib = static_library('dxvk_allocator_simd', files('dxvk_allocator.cpp'),
    dependencies        : [ dxbc_spirv_dep, vkcommon_dep ] + dxvk_extra_deps,
    include_directories : [ dxvk_include_path ],
    link_with           : dxvk_link_libs,
    cpp_args            : dxvk_experimental_simd_args,
  )

  dxvk_constant_state_lib = static_library('dxvk_constant_state_simd', files('dxvk_constant_state.cpp'),
    dependencies        : [ dxbc_spirv_dep, vkcommon_dep ] + dxvk_extra_deps,
    include_directories : [ dxvk_include_path ],
    link_with           : dxvk_link_libs,
    cpp_args            : dxvk_experimental_simd_args,
  )

  dxvk_descriptor_info_lib = static_library('dxvk_descriptor_info_simd', files('dxvk_descriptor_info.cpp'),
    dependencies        : [ dxbc_spirv_dep, vkcommon_dep ] + dxvk_extra_deps,
    include_directories : [ dxvk_include_path ],
    link_with           : dxvk_link_libs,
    cpp_args            : dxvk_experimental_simd_args,
  )

  dxvk_util_lib = static_library('dxvk_util_simd', files('dxvk_util.cpp'),
    dependencies        : [ dxbc_spirv_dep, vkcommon_dep ] + dxvk_extra_deps,
    include_directories : [ dxvk_include_path ],
    link_with           : dxvk_link_libs,
    cpp_args            : dxvk_experimental_simd_args,
  )

  dxvk_stats_lib = static_library('dxvk_stats_simd', files('dxvk_stats.cpp'),
    dependencies        : [ dxbc_spirv_dep, vkcommon_dep ] + dxvk_extra_deps,
    include_directories : [ dxvk_include_path ],
    link_with           : dxvk_link_libs,
    cpp_args            : dxvk_experimental_simd_args,
  )

  dxvk_shader_key_lib = static_library('dxvk_shader_key_simd', files('dxvk_shader_key.cpp'),
    dependencies        : [ dxbc_spirv_dep, vkcommon_dep ] + dxvk_extra_deps,
    include_directories : [ dxvk_include_path ],
    link_with           : dxvk_link_libs,
    cpp_args            : dxvk_experimental_simd_args,
  )

  dxvk_meta_clear_lib = static_library('dxvk_meta_clear_simd', files('dxvk_meta_clear.cpp'),
    dependencies        : [ dxbc_spirv_dep, vkcommon_dep ] + dxvk_extra_deps,
    include_directories : [ dxvk_include_path ],
    link_with           : dxvk_link_libs,
    cpp_args            : dxvk_experimental_simd_args,
  )

  dxvk_pipemanager_lib = static_library('dxvk_pipemanager_simd', files('dxvk_pipemanager.cpp'),
    dependencies        : [ dxbc_spirv_dep, vkcommon_dep ] + dxvk_extra_deps,
    include_directories : [ dxvk_include_path ],
    link_with           : dxvk_link_libs,
    cpp_args            : dxvk_experimental_simd_args,
  )

  dxvk_shader_builtin_lib = static_library('dxvk_shader_builtin_simd', files('dxvk_shader_builtin.cpp'),
    dependencies        : [ dxbc_spirv_dep, vkcommon_dep ] + dxvk_extra_deps,
    include_directories : [ dxvk_include_path ],
    link_with           : dxvk_link_libs,
    cpp_args            : dxvk_experimental_simd_args,
  )

  dxvk_link_libs += [
    dxvk_allocator_lib,
    dxvk_constant_state_lib,
    dxvk_descriptor_info_lib,
    dxvk_util_lib,
    dxvk_stats_lib,
    dxvk_shader_key_lib,
    dxvk_meta_clear_lib,
    dxvk_pipemanager_lib,
    dxvk_shader_builtin_lib
  ]
else
  dxvk_src += [
    'dxvk_allocator.cpp',
    'dxvk_constant_state.cpp',
    'dxvk_descriptor_info.cpp',
    'dxvk_util.cpp',
    'dxvk_stats.cpp',
    'dxvk_shader_key.cpp',
    'dxvk_meta_clear.cpp',
    'dxvk_pipemanager.cpp',
    'dxvk_shader_builtin.cpp'
  ]
endif

dxvk_lib = static_library('dxvk', dxvk_src, glsl_generator.process(dxvk_shaders), dxvk_version,
  link_with           : dxvk_link_libs,
  dependencies        : [ dxbc_spirv_dep, vkcommon_dep ] + dxvk_extra_deps,
  include_directories : [ dxvk_include_path ],
)

dxvk_dep = declare_dependency(
  link_with           : [ dxvk_lib ] + dxvk_link_libs,
  include_directories : [ dxvk_include_path ],
)

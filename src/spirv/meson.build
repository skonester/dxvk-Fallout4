if get_option('dxvk_experimental_simd')
  spirv_compression_lib = static_library('spirv_compression_simd', files('spirv_compression.cpp'),
    include_directories : [ dxvk_include_path ],
    cpp_args            : dxvk_experimental_simd_args,
  )
  spirv_src = files([
    'spirv_code_buffer.cpp',
    'spirv_module.cpp',
  ])
  spirv_link_libs = [ spirv_compression_lib ]
else
  spirv_src = files([
    'spirv_code_buffer.cpp',
    'spirv_compression.cpp',
    'spirv_module.cpp',
  ])
  spirv_link_libs = []
endif

spirv_lib = static_library('spirv', spirv_src,
  include_directories : [ dxvk_include_path ],
  link_with           : spirv_link_libs,
)


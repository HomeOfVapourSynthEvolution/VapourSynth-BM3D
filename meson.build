project('BM3D', 'cpp',
  default_options : ['buildtype=release', 'b_ndebug=if-release', 'cpp_std=c++14'],
  meson_version : '>=0.48.0',
  version : '8'
)

add_project_arguments('-Wno-unused-local-typedefs', language : 'cpp')

sources = [
  'include/Block.h',
  'include/BM3D.h',
  'include/BM3D_Base.h',
  'include/BM3D_Basic.h',
  'include/BM3D_Final.h',
  'include/Conversion.hpp',
  'include/fftw3_helper.hpp',
  'include/Helper.h',
  'include/OPP2RGB.h',
  'include/RGB2OPP.h',
  'include/Specification.h',
  'include/Type.h',
  'include/VAggregate.h',
  'include/VBM3D_Base.h',
  'include/VBM3D_Basic.h',
  'include/VBM3D_Final.h',
  'source/BM3D.cpp',
  'source/BM3D_Base.cpp',
  'source/BM3D_Basic.cpp',
  'source/BM3D_Final.cpp',
  'source/VAggregate.cpp',
  'source/VBM3D_Base.cpp',
  'source/VBM3D_Basic.cpp',
  'source/VBM3D_Final.cpp',
  'source/VSPlugin.cpp'
]

vapoursynth_dep = dependency('vapoursynth').partial_dependency(compile_args : true, includes : true)

fftw3f_dep = dependency('fftw3f')

if host_machine.cpu_family().startswith('x86')
  add_project_arguments('-mfpmath=sse', '-msse2', language : 'cpp')
endif

shared_module('bm3d', sources,
  dependencies : [vapoursynth_dep, fftw3f_dep],
  include_directories : include_directories('include'),
  install : true,
  install_dir : join_paths(vapoursynth_dep.get_pkgconfig_variable('libdir'), 'vapoursynth'),
  gnu_symbol_visibility : 'hidden'
)

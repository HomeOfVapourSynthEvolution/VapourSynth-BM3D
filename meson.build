project('BM3D', 'cpp',
    default_options: ['buildtype=release', 'b_lto=true', 'b_ndebug=if-release', 'cpp_std=c++17'],
    license: 'MIT',
    license_files: 'LICENSE',
    meson_version: '>=1.2.3',
    version: '10.0',
)

add_project_arguments('-Wno-unused-local-typedefs', language: 'cpp')

py = import('python').find_installation(pure: false)

incdir = include_directories(
    'include',
    run_command(py, '-c', 'import vapoursynth as vs; print(vs.get_include())', check: true).stdout().strip(),
)

shared_module('bm3d',
    files(
        'source/BM3D.cpp',
        'source/BM3D_Base.cpp',
        'source/BM3D_Basic.cpp',
        'source/BM3D_Final.cpp',
        'source/VAggregate.cpp',
        'source/VBM3D_Base.cpp',
        'source/VBM3D_Basic.cpp',
        'source/VBM3D_Final.cpp',
        'source/VSPlugin.cpp',
    ),
    dependencies: dependency('fftw3f'),
    gnu_symbol_visibility: 'hidden',
    include_directories: incdir,
    install: true,
    install_dir: py.get_install_dir() / 'vapoursynth/plugins',
    name_prefix: '',
)

project(
  'fabric',
  'cpp',
  version: '0.1',
  default_options: ['warning_level=3', 'cpp_std=c++14'],
)

includes = include_directories('include')

exe = executable(
  'fabric',
  'src/fabric.cpp',
  include_directories: includes,
  install: false,
)

test('basic', exe)

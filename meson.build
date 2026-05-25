project('vdj-quickfilter-pro', 'cpp',
  version: '0.1',
  default_options: ['cpp_std=c++11'],
)

cf_dep = dependency('appleframeworks', modules: ['CoreFoundation'])

plugin = shared_module('QFPro',
  'src/Main.cpp',
  'src/MyPlugin8.cpp',
  include_directories: include_directories('external/sdk'),
  dependencies: [cf_dep],
  name_prefix: '',
  install: true,
  install_dir: 'Contents/MacOS',
  cpp_args: ['-fvisibility=hidden'],
)

plist = configure_file(
  input: 'data/Info.plist.in',
  output: 'Info.plist',
  configuration: {
    'NAME': 'QFPro.dylib',
    'IDENTIFIER': 'com.me.vdj-quickfilter-pro',
    'VERSION': meson.project_version(),
  },
)

install_data(plist, install_dir: 'Contents')

custom_target('codesign',
  input: plugin,
  output: 'QFPro.signed',
  command: ['codesign', '--force', '--sign', '-', '@INPUT@'],
  build_by_default: true,
)

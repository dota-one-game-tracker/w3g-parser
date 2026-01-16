# These arguments are only used to build the shared library
# not the executables that use the library.
lib_args = ['-DBUILDING_BAJE_PARSER']

zdep = dependency('zlib', version : '>=1.2.8')

lib = library(
  'baje_parser',
  'baje_parser.cpp',
  install : true,
  include_directories : includes,
  cpp_shared_args : lib_args,
  gnu_symbol_visibility : 'hidden',
  dependencies : dependencies,
)

# Make this library usable as a Meson subproject.
baje_parser_dep = declare_dependency(
  include_directories : include_directories('.'),
  dependencies : dependencies,
  link_with : lib,
)

application = executable(
  'baje-parser-app',
  'main.cpp',
  install: true,
  include_directories: includes,
  dependencies: dependencies + zdep,
)

meson.override_dependency('baje-parser', baje_parser_dep)
dependencies += baje_parser_dep

pkg_mod = import('pkgconfig')
pkg_mod.generate(
  lib,
  description : 'Parser for original dota replay files.',
  subdirs : 'baje_parser',
)

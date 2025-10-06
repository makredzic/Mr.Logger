project('MRLogger', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3', 'cpp_std=c++20', 'buildtype=release']
)

# --- Global Setup ---
incdir = include_directories('include')
deps = [
  dependency('threads'),
  dependency('liburing'),
  dependency('fmt')
]

# Compile arguments
compile_args = ['-Werror=return-type']
if get_option('sequence_tracking')
  message('LOGGER_TEST_SEQUENCE_TRACKING: ENABLED')
  compile_args += '-DLOGGER_TEST_SEQUENCE_TRACKING'
else
  message('LOGGER_TEST_SEQUENCE_TRACKING: DISABLED')
endif

# --- Subdirectories ---
subdir('src')

# --- Library Target ---
mrlogger_lib = static_library('mrlogger',
  lib_src,
  include_directories: incdir,
  dependencies: deps,
  cpp_args: compile_args,
  install: true
)

# --- Declare dependency for use as subproject ---
mrlogger_dep = declare_dependency(
  link_with: mrlogger_lib,
  include_directories: incdir,
  dependencies: deps
)

# Make this available to parent projects when used as subproject
meson.override_dependency('mrlogger', mrlogger_dep)

# --- Main executable (development only) ---
executable('main',
  files('src/main.cpp'),
  include_directories: incdir,
  dependencies: deps,
  link_with: mrlogger_lib,
  cpp_args: compile_args
)

# --- Tests and Benchmarks ---
subdir('test')
subdir('Benchmarks')

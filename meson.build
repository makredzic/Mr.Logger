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
compile_args = []
if get_option('sequence_tracking')
  message('LOGGER_TEST_SEQUENCE_TRACKING: ENABLED')
  compile_args += '-DLOGGER_TEST_SEQUENCE_TRACKING'
else
  message('LOGGER_TEST_SEQUENCE_TRACKING: DISABLED')
endif

# --- Subdirectories ---
subdir('src')
subdir('test')
subdir('Benchmarks')

# --- Main executable ---
executable('main',
  [lib_src, files('src/main.cpp')],
  include_directories: incdir,
  dependencies: deps,
  cpp_args: compile_args
)

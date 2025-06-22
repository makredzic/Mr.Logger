project('IOUring', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3', 'cpp_std=c++20', 'buildtype=debug']
)

# --- Global Setup ---
incdir = include_directories('include')
deps = [
  dependency('threads'),
  dependency('liburing')
]

# --- Subdirectories ---
subdir('src')
subdir('test')

# --- Main executable ---
executable('main',
  [lib_src, files('src/main.cpp')],
  include_directories: incdir,
  dependencies: deps
)

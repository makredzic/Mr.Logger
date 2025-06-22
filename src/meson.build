# files() makes Meson remember the path to the
# provided files so that when the variable 
# source_files gets hoisted to the root meson.build
# these source files can actually be used

iouring_sources = []

subdir('IO')
subdir('Logger')

# Expose library sources to parent
lib_src = iouring_sources 
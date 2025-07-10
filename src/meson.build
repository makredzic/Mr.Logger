# files() makes Meson remember the path to the
# provided files so that when the variable 
# source_files gets hoisted to the root meson.build
# these source files can actually be used

src_files = []

subdir('IO')
subdir('Logger')
subdir('Memory')

# Expose library sources to parent
lib_src = src_files 
# Dependencies
gtest_dep = dependency('gtest_main', static: true)
gmock_dep = dependency('gmock', static: true)


# This is a regular list of strings and not files()
# because the string file name is modified
# (split, replaced, etc.) for the creation
# of individual test executables dynamically
# in the foreach loop below
test_filenames = [
  'Integration/LoggerIntegrationTest.cpp'
]

# Build and test each one
foreach fname : test_filenames

  # Remove file extension using split
  test_name = fname.split('.')[0].replace('/', '.')

  test_exe = executable(
    test_name,
    [lib_src, files(fname)],
    include_directories: [incdir],
    dependencies: [gtest_dep, gmock_dep, deps],
  )

  test(test_name, test_exe)
endforeach



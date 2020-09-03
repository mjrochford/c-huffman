project('huffc', 'c',
  version : '0.1',
  default_options : ['warning_level=3'])

src = ['src/huff.c',
       'src/h_tree.c', 'src/h_tree.h',
       'src/b_heap.c', 'src/b_heap.h',
       'src/bitstream.c', 'src/bitstream.h']
deps = []

huff = executable('huff', sources: src, dependencies: deps)
bitstream_test = executable('bitstream_test',
                            sources: ['tests/bitstream.test.c',
                                      'src/bitstream.c',
                                      'src/bitstream.h'])
test('bitstream test', bitstream_test)

pandoc = find_program('pandoc', required : false, disabler : true)

if not pandoc.found()
  warning('pandoc not found, man pages will not be built or installed')
endif

foreach source: ['recoverjpeg', 'recovermov', 'remove-duplicates', 'sort-pictures']
  custom_target(source + '.1',
    input : source + '.md',
    output : '@BASENAME@.1',
    command : [pandoc, '-s', '-w', 'man', '@INPUT@', '-o', '@OUTPUT@'],
    install : true,
    install_dir : join_paths(get_option('mandir'), 'man1'))
endforeach

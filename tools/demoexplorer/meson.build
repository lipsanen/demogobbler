cli_files = ['main.c']
curses_dep = dependency('curses', required: true)
cli = executable('demoexplorer', cli_files, link_with : [goblin], dependencies: [curses_dep], include_directories : goblin_export)
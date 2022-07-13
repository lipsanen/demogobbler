project('demolime', ['c', 'cpp'], default_options : ['c_std=c11', 'cpp_std=c++17'])

goblin_export = include_directories(['include'])
goblin_all =  include_directories(['include', 'src'])

subdir('src')
subdir('test')
subdir('bench')
subdir('tools/copydemo')
subdir('tools/demodump')
subdir('tools/listgobbler')
google_benchmark_proj = subproject('google-benchmark')
google_benchmark_dep = google_benchmark_proj.get_variable('google_benchmark_dep')
google_benchmark_main_dep = google_benchmark_proj.get_variable('google_benchmark_main_dep')

bench_files = ['main.cpp', 'empty.cpp', 'e2e.cpp']
bench = executable('goblin_benchmark', bench_files, link_with : [goblin], include_directories : goblin_export, dependencies : [google_benchmark_dep, google_benchmark_main_dep])
benchmark('Benchmark', bench, workdir : meson.source_root())

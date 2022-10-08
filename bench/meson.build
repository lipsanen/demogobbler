google_benchmark_proj = subproject('google-benchmark')
google_benchmark_dep = google_benchmark_proj.get_variable('google_benchmark_dep')
google_benchmark_main_dep = google_benchmark_proj.get_variable('google_benchmark_main_dep')

bench_files = ['arena.cpp', 'bitstream.cpp', 'main.cpp', 'e2e.cpp', 'eprops.cpp', 'hashtable.cpp']
bench = executable('gobbler_benchmark', bench_files, link_with : [goblin, test_utils], include_directories : [goblin_all, test_utils_includes], dependencies : [google_benchmark_dep, google_benchmark_main_dep])
benchmark('Benchmark', bench, workdir : meson.source_root(), timeout: 0)

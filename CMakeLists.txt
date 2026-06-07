cmake_minimum_required(VERSION 3.20)
project(kdna_opencl C)

option(KALYX_FORCE_OPENCL_RUNTIME_TESTS "Force-enable vendor OpenCL runtime tests. OFF by default because ICD crashes must never break CPU/KIE validation." OFF)
set(KALYX_ENABLE_OPENCL_TESTS OFF CACHE BOOL "Deprecated; ignored. Use KALYX_FORCE_OPENCL_RUNTIME_TESTS=ON for unsafe OpenCL runtime tests." FORCE)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(kdna STATIC
    src/kdna_cpu.c
    src/kdna_opencl.c
    src/kdna_ksoa.c
    src/kdna_kreg.c
    src/kdna_klib.c
    src/kdna_kgram.c
    src/kdna_krun.c
    src/kdna_kgen.c
    src/kdna_kvar.c
    src/kdna_kexp.c
    src/kdna_kmap.c
)
target_include_directories(kdna PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
if(WIN32)
    target_link_libraries(kdna PUBLIC)
else()
    target_link_libraries(kdna PUBLIC m dl)
endif()

add_executable(kdna_demo tools/kdna_demo.c)
target_link_libraries(kdna_demo PRIVATE kdna)

add_executable(kdna_scan tools/kdna_scan.c)
target_link_libraries(kdna_scan PRIVATE kdna)

add_executable(kdna_inspect tools/kdna_inspect.c)
target_link_libraries(kdna_inspect PRIVATE kdna)


add_executable(kdna_region tools/kdna_region.c)
target_link_libraries(kdna_region PRIVATE kdna)


add_executable(kdna_library tools/kdna_library.c)
target_link_libraries(kdna_library PRIVATE kdna)

add_executable(kdna_query tools/kdna_query.c)
target_link_libraries(kdna_query PRIVATE kdna)

add_executable(kdna_gram tools/kdna_gram.c)
target_link_libraries(kdna_gram PRIVATE kdna)

add_executable(kdna_run tools/kdna_run.c)
target_link_libraries(kdna_run PRIVATE kdna)

add_executable(kdna_reinject tools/kdna_reinject.c)
target_link_libraries(kdna_reinject PRIVATE kdna)

add_executable(kdna_genesis tools/kdna_genesis.c)
target_link_libraries(kdna_genesis PRIVATE kdna)

add_executable(kdna_variants tools/kdna_variants.c)
target_link_libraries(kdna_variants PRIVATE kdna)

add_executable(kdna_genesis_export tools/kdna_genesis_export.c)
target_link_libraries(kdna_genesis_export PRIVATE kdna)

add_executable(kdna_variant_gram tools/kdna_variant_gram.c)
target_link_libraries(kdna_variant_gram PRIVATE kdna)

add_executable(kdna_probe tools/kdna_probe.c)
target_link_libraries(kdna_probe PRIVATE kdna)

add_executable(kdna_project tools/kdna_project.c)
target_link_libraries(kdna_project PRIVATE kdna)

add_executable(kdna_symbol_gram tools/kdna_symbol_gram.c)
target_link_libraries(kdna_symbol_gram PRIVATE kdna)

add_executable(kdna_genome_matrix tools/kdna_genome_matrix.c)
target_link_libraries(kdna_genome_matrix PRIVATE kdna)

add_executable(kdna_fasta_symbols tools/kdna_fasta_symbols.c)
target_link_libraries(kdna_fasta_symbols PRIVATE kdna)

add_executable(kdna_manifest tools/kdna_manifest.c)
target_link_libraries(kdna_manifest PRIVATE kdna)

add_executable(kdna_experiment tools/kdna_experiment.c)
target_link_libraries(kdna_experiment PRIVATE kdna)

add_executable(kdna_benchmark tools/kdna_benchmark.c)
target_link_libraries(kdna_benchmark PRIVATE kdna)

add_executable(kdna_sweep tools/kdna_sweep.c)
target_link_libraries(kdna_sweep PRIVATE kdna)

add_executable(kdna_dynamics tools/kdna_dynamics.c)
target_link_libraries(kdna_dynamics PRIVATE kdna)

add_executable(test_cpu_invariants tests/test_cpu_invariants.c)
target_link_libraries(test_cpu_invariants PRIVATE kdna)

add_executable(test_opencl_diff tests/test_opencl_diff.c)
target_link_libraries(test_opencl_diff PRIVATE kdna)

add_executable(test_scan_ksoa tests/test_scan_ksoa.c)
target_link_libraries(test_scan_ksoa PRIVATE kdna)


add_executable(test_kreg tests/test_kreg.c)
target_link_libraries(test_kreg PRIVATE kdna)

add_executable(test_klib tests/test_klib.c)
target_link_libraries(test_klib PRIVATE kdna)

add_executable(test_kgram tests/test_kgram.c)
target_link_libraries(test_kgram PRIVATE kdna)

add_executable(test_krun tests/test_krun.c)
target_link_libraries(test_krun PRIVATE kdna)

add_executable(test_reinject tests/test_reinject.c)
target_link_libraries(test_reinject PRIVATE kdna)

add_executable(test_kgen tests/test_kgen.c)
target_link_libraries(test_kgen PRIVATE kdna)

add_executable(test_kgen_opencl_diff tests/test_kgen_opencl_diff.c)
target_link_libraries(test_kgen_opencl_diff PRIVATE kdna)

add_executable(test_kvar tests/test_kvar.c)
target_link_libraries(test_kvar PRIVATE kdna)

add_executable(test_kgen_export_ksoa tests/test_kgen_export_ksoa.c)
target_link_libraries(test_kgen_export_ksoa PRIVATE kdna)

add_executable(test_kexp tests/test_kexp.c)
target_link_libraries(test_kexp PRIVATE kdna)

add_executable(test_kexp_sweep tests/test_kexp_sweep.c)
target_link_libraries(test_kexp_sweep PRIVATE kdna)

add_executable(test_ksieve tests/test_ksieve.c)
target_link_libraries(test_ksieve PRIVATE kdna)

add_executable(test_kmap tests/test_kmap.c)
target_link_libraries(test_kmap PRIVATE kdna)

add_executable(test_symbol_gram tests/test_symbol_gram.c)
target_link_libraries(test_symbol_gram PRIVATE kdna)

add_executable(test_kgenome tests/test_kgenome.c)
target_link_libraries(test_kgenome PRIVATE kdna)

add_executable(test_fasta_symbols tests/test_fasta_symbols.c)
target_link_libraries(test_fasta_symbols PRIVATE kdna)

enable_testing()
add_test(NAME cpu_invariants COMMAND $<TARGET_FILE:test_cpu_invariants>)
if(KALYX_FORCE_OPENCL_RUNTIME_TESTS)
add_test(NAME cpu_opencl_diff COMMAND $<TARGET_FILE:test_opencl_diff> ${CMAKE_CURRENT_SOURCE_DIR}/kernels/kdna_eval.cl)
else()
add_test(NAME cpu_opencl_diff COMMAND ${CMAKE_COMMAND} -E echo "cpu_opencl_diff disabled: pass -DKALYX_FORCE_OPENCL_RUNTIME_TESTS=ON to run unsafe vendor OpenCL runtime test")
endif()
set(KDNA_SCAN_CPU_TEST_FILE "${CMAKE_CURRENT_BINARY_DIR}/scan_cpu_test.ksoa")
add_test(NAME scan_cpu_file
    COMMAND $<TARGET_FILE:kdna_scan> --min -1 --max 1 --n 17 --backend cpu --out ${KDNA_SCAN_CPU_TEST_FILE})
add_test(NAME scan_ksoa_header_payload
    COMMAND $<TARGET_FILE:test_scan_ksoa> ${KDNA_SCAN_CPU_TEST_FILE})
set_tests_properties(scan_ksoa_header_payload PROPERTIES DEPENDS scan_cpu_file)

add_test(NAME inspect_cpu_file
    COMMAND $<TARGET_FILE:kdna_inspect> --a ${KDNA_SCAN_CPU_TEST_FILE} --max-events 8)
set_tests_properties(inspect_cpu_file PROPERTIES DEPENDS scan_cpu_file)

add_test(NAME inspect_cpu_self_compare
    COMMAND $<TARGET_FILE:kdna_inspect> --a ${KDNA_SCAN_CPU_TEST_FILE} --b ${KDNA_SCAN_CPU_TEST_FILE} --max-events 8)
set_tests_properties(inspect_cpu_self_compare PROPERTIES DEPENDS scan_cpu_file)

set(KDNA_REGION_TEST_FILE "${CMAKE_CURRENT_BINARY_DIR}/regions_cpu_test.kreg")
add_test(NAME region_cpu_file
    COMMAND $<TARGET_FILE:kdna_region> --in ${KDNA_SCAN_CPU_TEST_FILE} --out ${KDNA_REGION_TEST_FILE} --split both)
set_tests_properties(region_cpu_file PROPERTIES DEPENDS scan_cpu_file)

add_test(NAME region_kreg_header_records
    COMMAND $<TARGET_FILE:test_kreg> ${KDNA_REGION_TEST_FILE} 17)
set_tests_properties(region_kreg_header_records PROPERTIES DEPENDS region_cpu_file)


set(KDNA_LIBRARY_TEST_FILE "${CMAKE_CURRENT_BINARY_DIR}/library_cpu_test.klib")
add_test(NAME library_cpu_file
    COMMAND $<TARGET_FILE:kdna_library> --ksoa ${KDNA_SCAN_CPU_TEST_FILE} --kreg ${KDNA_REGION_TEST_FILE} --out ${KDNA_LIBRARY_TEST_FILE})
set_tests_properties(library_cpu_file PROPERTIES DEPENDS "scan_cpu_file;region_cpu_file")

add_test(NAME library_klib_header_records
    COMMAND $<TARGET_FILE:test_klib> ${KDNA_LIBRARY_TEST_FILE} 17)
set_tests_properties(library_klib_header_records PROPERTIES DEPENDS library_cpu_file)

add_test(NAME query_library_smoke
    COMMAND $<TARGET_FILE:kdna_query> --lib ${KDNA_LIBRARY_TEST_FILE} --top 8)
set_tests_properties(query_library_smoke PROPERTIES DEPENDS library_cpu_file)

add_test(NAME query_library_compression
    COMMAND $<TARGET_FILE:kdna_query> --lib ${KDNA_LIBRARY_TEST_FILE} --dom K4 --top 8)
set_tests_properties(query_library_compression PROPERTIES DEPENDS library_cpu_file)

set(KDNA_GRAMMAR_TEST_FILE "${CMAKE_CURRENT_BINARY_DIR}/grammar_cpu_test.kgram")
add_test(NAME grammar_cpu_file
    COMMAND $<TARGET_FILE:kdna_gram> --lib ${KDNA_LIBRARY_TEST_FILE} --out ${KDNA_GRAMMAR_TEST_FILE} --top 8)
set_tests_properties(grammar_cpu_file PROPERTIES DEPENDS library_cpu_file)

add_test(NAME grammar_kgram_header_rules
    COMMAND $<TARGET_FILE:test_kgram> ${KDNA_GRAMMAR_TEST_FILE} 17)
set_tests_properties(grammar_kgram_header_rules PROPERTIES DEPENDS grammar_cpu_file)

add_test(NAME grammar_query_causal
    COMMAND $<TARGET_FILE:kdna_gram> --gram ${KDNA_GRAMMAR_TEST_FILE} --causal --top 8)
set_tests_properties(grammar_query_causal PROPERTIES DEPENDS grammar_cpu_file)

add_test(NAME grammar_query_transition
    COMMAND $<TARGET_FILE:kdna_gram> --gram ${KDNA_GRAMMAR_TEST_FILE} --changes --min-strength 0.1 --top 8)
set_tests_properties(grammar_query_transition PROPERTIES DEPENDS grammar_cpu_file)

set(KDNA_RUN_TEST_FILE "${CMAKE_CURRENT_BINARY_DIR}/run_causal_test.krun")
add_test(NAME run_causal_file
    COMMAND $<TARGET_FILE:kdna_run> --gram ${KDNA_GRAMMAR_TEST_FILE} --out ${KDNA_RUN_TEST_FILE} --causal --top 8)
set_tests_properties(run_causal_file PROPERTIES DEPENDS grammar_cpu_file)

add_test(NAME run_krun_header_steps
    COMMAND $<TARGET_FILE:test_krun> ${KDNA_RUN_TEST_FILE} 17)
set_tests_properties(run_krun_header_steps PROPERTIES DEPENDS run_causal_file)

add_test(NAME run_query_smoke
    COMMAND $<TARGET_FILE:kdna_run> --run ${KDNA_RUN_TEST_FILE} --top 8)
set_tests_properties(run_query_smoke PROPERTIES DEPENDS run_causal_file)

set(KDNA_REINJECT_TEST_FILE "${CMAKE_CURRENT_BINARY_DIR}/reinject_causal_test.ksoa")
add_test(NAME reinject_causal_file
    COMMAND $<TARGET_FILE:kdna_reinject> --run ${KDNA_RUN_TEST_FILE} --out ${KDNA_REINJECT_TEST_FILE} --samples-per-step 4 --mode boundary)
set_tests_properties(reinject_causal_file PROPERTIES DEPENDS run_causal_file)

add_test(NAME reinject_ksoa_trace
    COMMAND $<TARGET_FILE:test_reinject> ${KDNA_REINJECT_TEST_FILE} 20)
set_tests_properties(reinject_ksoa_trace PROPERTIES DEPENDS reinject_causal_file)

add_test(NAME reinject_inspect_smoke
    COMMAND $<TARGET_FILE:kdna_inspect> --a ${KDNA_REINJECT_TEST_FILE} --max-events 8)
set_tests_properties(reinject_inspect_smoke PROPERTIES DEPENDS reinject_causal_file)


set(KDNA_KGEN_CPU_TEST_FILE "${CMAKE_CURRENT_BINARY_DIR}/genesis_cpu_test.kgen")
add_test(NAME kgen_cpu_file
    COMMAND $<TARGET_FILE:kdna_genesis> --out ${KDNA_KGEN_CPU_TEST_FILE} --backend cpu --n 257 --steps 24 --dt 0.01 --seed 0x4b47454e)
add_test(NAME kgen_header_payload
    COMMAND $<TARGET_FILE:test_kgen> ${KDNA_KGEN_CPU_TEST_FILE} 257)
set_tests_properties(kgen_header_payload PROPERTIES DEPENDS kgen_cpu_file)

add_test(NAME kgen_inspect_smoke
    COMMAND $<TARGET_FILE:kdna_genesis> --a ${KDNA_KGEN_CPU_TEST_FILE})
set_tests_properties(kgen_inspect_smoke PROPERTIES DEPENDS kgen_cpu_file)

if(KALYX_FORCE_OPENCL_RUNTIME_TESTS)
add_test(NAME kgen_opencl_diff
    COMMAND $<TARGET_FILE:test_kgen_opencl_diff> ${CMAKE_CURRENT_SOURCE_DIR}/kernels/kdna_genesis.cl)
else()
add_test(NAME kgen_opencl_diff COMMAND ${CMAKE_COMMAND} -E echo "kgen_opencl_diff disabled: pass -DKALYX_FORCE_OPENCL_RUNTIME_TESTS=ON to run unsafe vendor OpenCL runtime test")
endif()


set(KDNA_KVAR_TEST_FILE "${CMAKE_CURRENT_BINARY_DIR}/variants_cpu_test.kvar")
add_test(NAME kvar_cpu_file
    COMMAND $<TARGET_FILE:kdna_variants> --kgen ${KDNA_KGEN_CPU_TEST_FILE} --out ${KDNA_KVAR_TEST_FILE} --top 8)
set_tests_properties(kvar_cpu_file PROPERTIES DEPENDS kgen_cpu_file)

add_test(NAME kvar_header_records
    COMMAND $<TARGET_FILE:test_kvar> ${KDNA_KVAR_TEST_FILE} 257)
set_tests_properties(kvar_header_records PROPERTIES DEPENDS kvar_cpu_file)

add_test(NAME kvar_query_resonant
    COMMAND $<TARGET_FILE:kdna_variants> --a ${KDNA_KVAR_TEST_FILE} --resonant --top 8)
set_tests_properties(kvar_query_resonant PROPERTIES DEPENDS kvar_cpu_file)

add_test(NAME kvar_query_dom
    COMMAND $<TARGET_FILE:kdna_variants> --a ${KDNA_KVAR_TEST_FILE} --dom K4 --top 8)
set_tests_properties(kvar_query_dom PROPERTIES DEPENDS kvar_cpu_file)


set(KDNA_KGEN_EXPORT_KSOA_TEST_FILE "${CMAKE_CURRENT_BINARY_DIR}/genesis_export_test.ksoa")
add_test(NAME kgen_export_ksoa_file
    COMMAND $<TARGET_FILE:kdna_genesis_export> --kgen ${KDNA_KGEN_CPU_TEST_FILE} --out ${KDNA_KGEN_EXPORT_KSOA_TEST_FILE})
set_tests_properties(kgen_export_ksoa_file PROPERTIES DEPENDS kgen_cpu_file)

add_test(NAME kgen_export_ksoa_header
    COMMAND $<TARGET_FILE:test_kgen_export_ksoa> ${KDNA_KGEN_EXPORT_KSOA_TEST_FILE} 257)
set_tests_properties(kgen_export_ksoa_header PROPERTIES DEPENDS kgen_export_ksoa_file)

set(KDNA_KVAR_GRAMMAR_TEST_FILE "${CMAKE_CURRENT_BINARY_DIR}/variant_grammar_test.kgram")
add_test(NAME kvar_variant_grammar_file
    COMMAND $<TARGET_FILE:kdna_variant_gram> --kvar ${KDNA_KVAR_TEST_FILE} --out ${KDNA_KVAR_GRAMMAR_TEST_FILE} --top 8)
set_tests_properties(kvar_variant_grammar_file PROPERTIES DEPENDS kvar_cpu_file)

add_test(NAME kvar_variant_grammar_probe
    COMMAND $<TARGET_FILE:kdna_probe> ${KDNA_KVAR_GRAMMAR_TEST_FILE})
set_tests_properties(kvar_variant_grammar_probe PROPERTIES DEPENDS kvar_variant_grammar_file)

set(KDNA_KVAR_RUN_TEST_FILE "${CMAKE_CURRENT_BINARY_DIR}/run_variant_test.krun")
add_test(NAME kvar_variant_run_file
    COMMAND $<TARGET_FILE:kdna_run> --gram ${KDNA_KVAR_GRAMMAR_TEST_FILE} --out ${KDNA_KVAR_RUN_TEST_FILE} --causal --top 8)
set_tests_properties(kvar_variant_run_file PROPERTIES DEPENDS kvar_variant_grammar_file)

add_test(NAME probe_all_artifacts
    COMMAND $<TARGET_FILE:kdna_probe> ${KDNA_KGEN_CPU_TEST_FILE} ${KDNA_KVAR_TEST_FILE} ${KDNA_KGEN_EXPORT_KSOA_TEST_FILE} ${KDNA_KVAR_GRAMMAR_TEST_FILE} ${KDNA_KVAR_RUN_TEST_FILE})
set_tests_properties(probe_all_artifacts PROPERTIES DEPENDS "kgen_cpu_file;kvar_cpu_file;kgen_export_ksoa_file;kvar_variant_grammar_file;kvar_variant_run_file")

set(KDNA_MANIFEST_TEST_FILE "${CMAKE_CURRENT_BINARY_DIR}/lineage_manifest.json")
add_test(NAME manifest_lineage_file
    COMMAND $<TARGET_FILE:kdna_manifest> --out ${KDNA_MANIFEST_TEST_FILE} ${KDNA_KGEN_CPU_TEST_FILE} ${KDNA_KVAR_TEST_FILE} ${KDNA_KGEN_EXPORT_KSOA_TEST_FILE} ${KDNA_KVAR_GRAMMAR_TEST_FILE} ${KDNA_KVAR_RUN_TEST_FILE})
set_tests_properties(manifest_lineage_file PROPERTIES DEPENDS probe_all_artifacts)


set(KDNA_DYN_VARIANT_CPU_TEST_FILE "${CMAKE_CURRENT_BINARY_DIR}/dyn_variant_cpu_test.kdyn")
add_test(NAME dyn_from_variants_cpu_file
    COMMAND $<TARGET_FILE:kdna_dynamics> --run ${KDNA_KVAR_RUN_TEST_FILE} --out ${KDNA_DYN_VARIANT_CPU_TEST_FILE} --backend cpu --n 128 --steps 12 --dt 0.02 --seed 0x12345678)
set_tests_properties(dyn_from_variants_cpu_file PROPERTIES DEPENDS kvar_variant_run_file)

add_test(NAME dyn_from_variants_inspect
    COMMAND $<TARGET_FILE:kdna_dynamics> --a ${KDNA_DYN_VARIANT_CPU_TEST_FILE})
set_tests_properties(dyn_from_variants_inspect PROPERTIES DEPENDS dyn_from_variants_cpu_file)

if(KALYX_FORCE_OPENCL_RUNTIME_TESTS)
set(KDNA_DYN_VARIANT_OPENCL_TEST_FILE "${CMAKE_CURRENT_BINARY_DIR}/dyn_variant_opencl_test.kdyn")
add_test(NAME dyn_from_variants_opencl_file
    COMMAND $<TARGET_FILE:kdna_dynamics> --run ${KDNA_KVAR_RUN_TEST_FILE} --out ${KDNA_DYN_VARIANT_OPENCL_TEST_FILE} --backend opencl --kernel ${CMAKE_CURRENT_SOURCE_DIR}/kernels/kdna_dynamics.cl --n 128 --steps 12 --dt 0.02 --seed 0x12345678)
set_tests_properties(dyn_from_variants_opencl_file PROPERTIES DEPENDS kvar_variant_run_file)

add_test(NAME dyn_from_variants_diff
    COMMAND $<TARGET_FILE:kdna_dynamics> --a ${KDNA_DYN_VARIANT_CPU_TEST_FILE} --b ${KDNA_DYN_VARIANT_OPENCL_TEST_FILE})
set_tests_properties(dyn_from_variants_diff PROPERTIES DEPENDS "dyn_from_variants_cpu_file;dyn_from_variants_opencl_file")
else()
add_test(NAME dyn_from_variants_opencl_file COMMAND ${CMAKE_COMMAND} -E echo "dyn_from_variants_opencl_file disabled: pass -DKALYX_FORCE_OPENCL_RUNTIME_TESTS=ON to run unsafe vendor OpenCL runtime test")
add_test(NAME dyn_from_variants_diff COMMAND ${CMAKE_COMMAND} -E echo "dyn_from_variants_diff disabled because OpenCL runtime test is disabled")
set_tests_properties(dyn_from_variants_diff PROPERTIES DEPENDS dyn_from_variants_cpu_file)
endif()

add_test(NAME probe_kdyn_artifact
    COMMAND $<TARGET_FILE:kdna_probe> ${KDNA_DYN_VARIANT_CPU_TEST_FILE})
set_tests_properties(probe_kdyn_artifact PROPERTIES DEPENDS dyn_from_variants_cpu_file)


set(KDNA_KEXP_DATA_TEST_FILE "${CMAKE_CURRENT_BINARY_DIR}/kexp_hidden_test.kdat")
set(KDNA_KEXP_REPORT_TEST_FILE "${CMAKE_CURRENT_BINARY_DIR}/kexp_hidden_test.krep")
add_test(NAME kexp_hidden_file
    COMMAND $<TARGET_FILE:kdna_experiment> --kind hidden --n 1024 --seed 0x4b455850 --out-data ${KDNA_KEXP_DATA_TEST_FILE} --out-report ${KDNA_KEXP_REPORT_TEST_FILE} --train 0.70 --bins 32)

add_test(NAME kexp_header_metrics
    COMMAND $<TARGET_FILE:test_kexp> ${KDNA_KEXP_DATA_TEST_FILE} ${KDNA_KEXP_REPORT_TEST_FILE})
set_tests_properties(kexp_header_metrics PROPERTIES DEPENDS kexp_hidden_file)

add_test(NAME kexp_report_inspect
    COMMAND $<TARGET_FILE:kdna_experiment> --a ${KDNA_KEXP_REPORT_TEST_FILE})
set_tests_properties(kexp_report_inspect PROPERTIES DEPENDS kexp_hidden_file)

add_test(NAME kexp_data_inspect
    COMMAND $<TARGET_FILE:kdna_experiment> --data ${KDNA_KEXP_DATA_TEST_FILE})
set_tests_properties(kexp_data_inspect PROPERTIES DEPENDS kexp_hidden_file)

set(KDNA_KEXP_BENCH_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/kexp_bench")
add_test(NAME kexp_benchmark_suite
    COMMAND $<TARGET_FILE:kdna_benchmark> --out-prefix ${KDNA_KEXP_BENCH_PREFIX} --n 1024 --seed 0x4b455850)


set(KDNA_KEXP_SWEEP_REPORT "${CMAKE_CURRENT_BINARY_DIR}/kexp_v2_sweep.krep")
set(KDNA_KEXP_SWEEP_CSV "${CMAKE_CURRENT_BINARY_DIR}/kexp_v2_sweep.csv")
set(KDNA_KEXP_SWEEP_MD "${CMAKE_CURRENT_BINARY_DIR}/kexp_v2_sweep.md")
add_test(NAME kexp_v2_sweep_file
    COMMAND $<TARGET_FILE:kdna_sweep> --out-report ${KDNA_KEXP_SWEEP_REPORT} --out-csv ${KDNA_KEXP_SWEEP_CSV} --out-md ${KDNA_KEXP_SWEEP_MD} --n 768 --seeds 2 --seed 0x4b455850 --train 0.70 --bins 32)

add_test(NAME kexp_v2_sweep_validate
    COMMAND $<TARGET_FILE:test_kexp_sweep> ${KDNA_KEXP_SWEEP_REPORT} ${KDNA_KEXP_SWEEP_CSV} ${KDNA_KEXP_SWEEP_MD} 12)
set_tests_properties(kexp_v2_sweep_validate PROPERTIES DEPENDS kexp_v2_sweep_file)

add_test(NAME kexp_v2_sweep_probe
    COMMAND $<TARGET_FILE:kdna_probe> ${KDNA_KEXP_SWEEP_REPORT})
set_tests_properties(kexp_v2_sweep_probe PROPERTIES DEPENDS kexp_v2_sweep_file)


set(KDNA_KSIEVE_REPORT_TEST_FILE "${CMAKE_CURRENT_BINARY_DIR}/ksieve_hidden_vs_variant_grammar.krep")
add_test(NAME ksieve_kgram_filter_file
    COMMAND $<TARGET_FILE:kdna_probe> --sieve --data ${KDNA_KEXP_DATA_TEST_FILE} --gram ${KDNA_KVAR_GRAMMAR_TEST_FILE} --out ${KDNA_KSIEVE_REPORT_TEST_FILE} --train 0.70 --bins 32)
set_tests_properties(ksieve_kgram_filter_file PROPERTIES DEPENDS "kexp_hidden_file;kvar_variant_grammar_file")

add_test(NAME ksieve_report_validate
    COMMAND $<TARGET_FILE:test_ksieve> ${KDNA_KSIEVE_REPORT_TEST_FILE})
set_tests_properties(ksieve_report_validate PROPERTIES DEPENDS ksieve_kgram_filter_file)

add_test(NAME ksieve_report_probe
    COMMAND $<TARGET_FILE:kdna_probe> ${KDNA_KSIEVE_REPORT_TEST_FILE})
set_tests_properties(ksieve_report_probe PROPERTIES DEPENDS ksieve_kgram_filter_file)


set(KDNA_KMAP_TEST_SYMBOLS "${CMAKE_CURRENT_BINARY_DIR}/kmap_symbols.u64")
set(KDNA_KMAP_TEST_OUT_SYMBOLS "${CMAKE_CURRENT_BINARY_DIR}/kmap_projected.u64")
set(KDNA_KMAP_TEST_FILE "${CMAKE_CURRENT_BINARY_DIR}/kmap_projected.kmap")

add_test(NAME kmap_project_file
    COMMAND $<TARGET_FILE:test_kmap> ${KDNA_KMAP_TEST_SYMBOLS} ${KDNA_KMAP_TEST_OUT_SYMBOLS} ${KDNA_KMAP_TEST_FILE})

add_test(NAME kmap_project_probe
    COMMAND $<TARGET_FILE:kdna_probe> ${KDNA_KMAP_TEST_FILE})
set_tests_properties(kmap_project_probe PROPERTIES DEPENDS kmap_project_file)

add_test(NAME kmap_project_tool_cpu
    COMMAND $<TARGET_FILE:kdna_project> --symbols ${KDNA_KMAP_TEST_SYMBOLS} --n 1024 --out-symbols ${CMAKE_CURRENT_BINARY_DIR}/kmap_tool_symbols.u64 --out-kmap ${CMAKE_CURRENT_BINARY_DIR}/kmap_tool.kmap --mode affine --k 16 --backend cpu --chunk 128)
set_tests_properties(kmap_project_tool_cpu PROPERTIES DEPENDS kmap_project_file)


set(KDNA_SYMBOL_GRAM_TEST_FILE "${CMAKE_CURRENT_BINARY_DIR}/symbol_self_grammar.kgram")
set(KDNA_SYMBOL_GRAM_REPORT_FILE "${CMAKE_CURRENT_BINARY_DIR}/ksieve_symbol_self.krep")

add_test(NAME symbol_gram_from_kmap_file
    COMMAND $<TARGET_FILE:kdna_symbol_gram> --symbols ${KDNA_KMAP_TEST_OUT_SYMBOLS} --n 1024 --out ${KDNA_SYMBOL_GRAM_TEST_FILE} --train 0.70)
set_tests_properties(symbol_gram_from_kmap_file PROPERTIES DEPENDS kmap_project_file)

add_test(NAME symbol_gram_header_records
    COMMAND $<TARGET_FILE:test_symbol_gram> ${KDNA_SYMBOL_GRAM_TEST_FILE} 1024)
set_tests_properties(symbol_gram_header_records PROPERTIES DEPENDS symbol_gram_from_kmap_file)

add_test(NAME symbol_gram_probe
    COMMAND $<TARGET_FILE:kdna_probe> ${KDNA_SYMBOL_GRAM_TEST_FILE})
set_tests_properties(symbol_gram_probe PROPERTIES DEPENDS symbol_gram_from_kmap_file)

add_test(NAME symbol_gram_self_sieve
    COMMAND $<TARGET_FILE:kdna_probe> --sieve --symbols ${KDNA_KMAP_TEST_OUT_SYMBOLS} --n 1024 --gram ${KDNA_SYMBOL_GRAM_TEST_FILE} --out ${KDNA_SYMBOL_GRAM_REPORT_FILE} --train 0.70 --bins 32)
set_tests_properties(symbol_gram_self_sieve PROPERTIES DEPENDS symbol_gram_from_kmap_file)

add_test(NAME symbol_gram_self_sieve_report
    COMMAND $<TARGET_FILE:test_ksieve> ${KDNA_SYMBOL_GRAM_REPORT_FILE})
set_tests_properties(symbol_gram_self_sieve_report PROPERTIES DEPENDS symbol_gram_self_sieve)


set(KDNA_KGENOME_TEST_FILE "${CMAKE_CURRENT_BINARY_DIR}/genome_matrix.kgenome")
set(KDNA_KGENOME_TEST_CSV "${CMAKE_CURRENT_BINARY_DIR}/genome_matrix.csv")
add_test(NAME kgenome_matrix_file
    COMMAND $<TARGET_FILE:kdna_genome_matrix>
        --entry chrA ${KDNA_KMAP_TEST_OUT_SYMBOLS} ${KDNA_SYMBOL_GRAM_TEST_FILE}
        --entry chrB ${KDNA_KMAP_TEST_OUT_SYMBOLS} ${KDNA_SYMBOL_GRAM_TEST_FILE}
        --out ${KDNA_KGENOME_TEST_FILE}
        --csv ${KDNA_KGENOME_TEST_CSV}
        --train 0.70
        --bins 32)
set_tests_properties(kgenome_matrix_file PROPERTIES DEPENDS symbol_gram_from_kmap_file)

add_test(NAME kgenome_header_records
    COMMAND $<TARGET_FILE:test_kgenome> ${KDNA_KGENOME_TEST_FILE} ${KDNA_KGENOME_TEST_CSV} 4)
set_tests_properties(kgenome_header_records PROPERTIES DEPENDS kgenome_matrix_file)

add_test(NAME kgenome_probe
    COMMAND $<TARGET_FILE:kdna_probe> ${KDNA_KGENOME_TEST_FILE})
set_tests_properties(kgenome_probe PROPERTIES DEPENDS kgenome_matrix_file)


# KGENOME v3 FASTA streaming tests
set(KDNA_FASTA_TEST_FILE "${CMAKE_CURRENT_BINARY_DIR}/fasta_symbols_test.fa")
set(KDNA_FASTA_TEST_SYMBOLS "${CMAKE_CURRENT_BINARY_DIR}/fasta_symbols_test.u64")
set(KDNA_FASTA_TEST_SUMMARY "${CMAKE_CURRENT_BINARY_DIR}/fasta_symbols_test.kfsum")
file(WRITE ${KDNA_FASTA_TEST_FILE} ">test\nACGTAC\nNN\nACG\n")
add_test(NAME fasta_symbols_file
    COMMAND $<TARGET_FILE:kdna_fasta_symbols> --in ${KDNA_FASTA_TEST_FILE} --out ${KDNA_FASTA_TEST_SYMBOLS} --k 3 --max-symbols 4 --summary ${KDNA_FASTA_TEST_SUMMARY})
add_test(NAME fasta_symbols_values
    COMMAND $<TARGET_FILE:test_fasta_symbols> ${KDNA_FASTA_TEST_SYMBOLS})
set_tests_properties(fasta_symbols_values PROPERTIES DEPENDS fasta_symbols_file)
add_test(NAME fasta_symbols_probe
    COMMAND $<TARGET_FILE:kdna_probe> ${KDNA_FASTA_TEST_SUMMARY})
set_tests_properties(fasta_symbols_probe PROPERTIES DEPENDS fasta_symbols_file)

# KGENOME Enterprise Library / KGLIB001
add_executable(kdna_genome_library tools/kdna_genome_library.c)
target_link_libraries(kdna_genome_library PRIVATE kdna)

add_executable(test_kglib tests/test_kglib.c)
target_link_libraries(test_kglib PRIVATE kdna)

add_test(NAME kglib_abi COMMAND $<TARGET_FILE:test_kglib>)
add_test(NAME kglib_help COMMAND $<TARGET_FILE:kdna_genome_library> --help)

add_executable(kdna_null_matrix tools/kdna_null_matrix.c)
target_link_libraries(kdna_null_matrix PRIVATE kdna)


# KSTREAM/KFIELD v1 generic stream adapters
add_executable(kdna_stream_bytes tools/kdna_stream_bytes.c)
target_include_directories(kdna_stream_bytes PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(kdna_stream_bytes PRIVATE kdna)

add_executable(kdna_stream_tokens tools/kdna_stream_tokens.c)
target_include_directories(kdna_stream_tokens PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(kdna_stream_tokens PRIVATE kdna)

add_executable(kdna_stream_csv tools/kdna_stream_csv.c)
target_include_directories(kdna_stream_csv PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(kdna_stream_csv PRIVATE kdna)

add_executable(kdna_stream_probe tools/kdna_stream_probe.c)
target_include_directories(kdna_stream_probe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(kdna_stream_probe PRIVATE kdna)

add_executable(test_kstream tests/test_kstream.c)
target_link_libraries(test_kstream PRIVATE kdna)

# KSTREAM/KFIELD v1 tests
set(KDNA_KSTREAM_TEST_BYTES "${CMAKE_CURRENT_BINARY_DIR}/kstream_bytes_input.bin")
set(KDNA_KSTREAM_TEST_BYTES_U64 "${CMAKE_CURRENT_BINARY_DIR}/kstream_bytes.u64")
set(KDNA_KSTREAM_TEST_BYTES_SUM "${CMAKE_CURRENT_BINARY_DIR}/kstream_bytes.kstream")
file(WRITE ${KDNA_KSTREAM_TEST_BYTES} "ABC\n")
add_test(NAME kstream_bytes_file
  COMMAND $<TARGET_FILE:kdna_stream_bytes>
    --in ${KDNA_KSTREAM_TEST_BYTES}
    --out ${KDNA_KSTREAM_TEST_BYTES_U64}
    --summary ${KDNA_KSTREAM_TEST_BYTES_SUM}
    --mode byte)
add_test(NAME kstream_bytes_values
  COMMAND $<TARGET_FILE:test_kstream> b ${KDNA_KSTREAM_TEST_BYTES_U64})
set_tests_properties(kstream_bytes_values PROPERTIES DEPENDS kstream_bytes_file)
add_test(NAME kstream_bytes_probe
  COMMAND $<TARGET_FILE:kdna_stream_probe> ${KDNA_KSTREAM_TEST_BYTES_SUM})
set_tests_properties(kstream_bytes_probe PROPERTIES DEPENDS kstream_bytes_file)

set(KDNA_KSTREAM_TEST_TOKENS "${CMAKE_CURRENT_BINARY_DIR}/kstream_tokens_input.txt")
set(KDNA_KSTREAM_TEST_TOKENS_U64 "${CMAKE_CURRENT_BINARY_DIR}/kstream_tokens.u64")
set(KDNA_KSTREAM_TEST_TOKENS_SUM "${CMAKE_CURRENT_BINARY_DIR}/kstream_tokens.kstream")
file(WRITE ${KDNA_KSTREAM_TEST_TOKENS} "alpha beta alpha\n")
add_test(NAME kstream_tokens_file
  COMMAND $<TARGET_FILE:kdna_stream_tokens>
    --in ${KDNA_KSTREAM_TEST_TOKENS}
    --out ${KDNA_KSTREAM_TEST_TOKENS_U64}
    --summary ${KDNA_KSTREAM_TEST_TOKENS_SUM})
add_test(NAME kstream_tokens_values
  COMMAND $<TARGET_FILE:test_kstream> t ${KDNA_KSTREAM_TEST_TOKENS_U64})
set_tests_properties(kstream_tokens_values PROPERTIES DEPENDS kstream_tokens_file)
add_test(NAME kstream_tokens_probe
  COMMAND $<TARGET_FILE:kdna_stream_probe> ${KDNA_KSTREAM_TEST_TOKENS_SUM})
set_tests_properties(kstream_tokens_probe PROPERTIES DEPENDS kstream_tokens_file)

set(KDNA_KSTREAM_TEST_CSV "${CMAKE_CURRENT_BINARY_DIR}/kstream_csv_input.csv")
set(KDNA_KSTREAM_TEST_CSV_U64 "${CMAKE_CURRENT_BINARY_DIR}/kstream_csv.u64")
set(KDNA_KSTREAM_TEST_CSV_SUM "${CMAKE_CURRENT_BINARY_DIR}/kstream_csv.kstream")
file(WRITE ${KDNA_KSTREAM_TEST_CSV} "x\n0\n3\n7\n10\n")
add_test(NAME kstream_csv_file
  COMMAND $<TARGET_FILE:kdna_stream_csv>
    --in ${KDNA_KSTREAM_TEST_CSV}
    --out ${KDNA_KSTREAM_TEST_CSV_U64}
    --summary ${KDNA_KSTREAM_TEST_CSV_SUM}
    --column 0
    --bins 11
    --header 1
    --min 0
    --max 10)
add_test(NAME kstream_csv_values
  COMMAND $<TARGET_FILE:test_kstream> c ${KDNA_KSTREAM_TEST_CSV_U64})
set_tests_properties(kstream_csv_values PROPERTIES DEPENDS kstream_csv_file)
add_test(NAME kstream_csv_probe
  COMMAND $<TARGET_FILE:kdna_stream_probe> ${KDNA_KSTREAM_TEST_CSV_SUM})
set_tests_properties(kstream_csv_probe PROPERTIES DEPENDS kstream_csv_file)


# KLANG v1 / CoNLL-U language adapter

add_executable(kdna_klang_library tools/kdna_klang_library.c)
target_link_libraries(kdna_klang_library PRIVATE kdna)
add_executable(kdna_stream_conllu tools/kdna_stream_conllu.c)
target_include_directories(kdna_stream_conllu PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(kdna_stream_conllu PRIVATE kdna)

add_executable(kdna_field_null_matrix tools/kdna_field_null_matrix.c)
target_link_libraries(kdna_field_null_matrix PRIVATE kdna)

set(KDNA_KLANG_TEST_CONLLU "${CMAKE_CURRENT_BINARY_DIR}/klang_test.conllu")
set(KDNA_KLANG_TEST_U64 "${CMAKE_CURRENT_BINARY_DIR}/klang_test.u64")
set(KDNA_KLANG_TEST_SUM "${CMAKE_CURRENT_BINARY_DIR}/klang_test.kstream")
file(WRITE ${KDNA_KLANG_TEST_CONLLU} "# sent_id = 1\n1\tThe\tthe\tDET\tDT\tDefinite=Def|PronType=Art\t2\tdet\t_\t_\n2\tdog\tdog\tNOUN\tNN\tNumber=Sing\t3\tnsubj\t_\t_\n3\truns\trun\tVERB\tVBZ\tMood=Ind|Tense=Pres\t0\troot\t_\t_\n4\t.\t.\tPUNCT\t.\t_\t3\tpunct\t_\t_\n\n# sent_id = 2\n1\tI\tI\tPRON\tPRP\tPerson=1\t2\tnsubj\t_\t_\n2\tsee\tsee\tVERB\tVBP\tMood=Ind\t0\troot\t_\t_\n")
add_test(NAME klang_conllu_file
  COMMAND $<TARGET_FILE:kdna_stream_conllu>
    --in ${KDNA_KLANG_TEST_CONLLU}
    --out ${KDNA_KLANG_TEST_U64}
    --summary ${KDNA_KLANG_TEST_SUM}
    --field upos
    --window 1
    --symbol-bits 32
    --sentence-boundary reset)
add_test(NAME klang_conllu_values
  COMMAND $<TARGET_FILE:test_kstream> l ${KDNA_KLANG_TEST_U64})
set_tests_properties(klang_conllu_values PROPERTIES DEPENDS klang_conllu_file)
add_test(NAME klang_conllu_probe
  COMMAND $<TARGET_FILE:kdna_stream_probe> ${KDNA_KLANG_TEST_SUM})
set_tests_properties(klang_conllu_probe PROPERTIES DEPENDS klang_conllu_file)
add_test(NAME klang_field_null_help
  COMMAND $<TARGET_FILE:kdna_field_null_matrix> --help)




add_test(NAME klang_library_help
    COMMAND $<TARGET_FILE:kdna_klang_library> --help)


# KALYX-PLANT v0.1 / declared planted-structure positive control
add_executable(kalyx_plant tools/kalyx_plant.c)
target_link_libraries(kalyx_plant PRIVATE kdna)

add_test(NAME kalyx_plant_help
  COMMAND $<TARGET_FILE:kalyx_plant> --help)

add_executable(kalyx_origin tools/kalyx_origin.c)
target_link_libraries(kalyx_origin PRIVATE kdna)


add_test(NAME kalyx_origin_help COMMAND kalyx_origin --help)
set_tests_properties(kalyx_origin_help PROPERTIES PASS_REGULAR_EXPRESSION "KALYX-ORIGIN|kalyx_origin")

add_executable(kalyx_repeat_context tools/kalyx_repeat_context.c)
target_link_libraries(kalyx_repeat_context PRIVATE kdna)
add_test(NAME kalyx_repeat_context_help COMMAND kalyx_repeat_context --help)

add_executable(kalyx_motif_context tools/kalyx_motif_context.c)
target_link_libraries(kalyx_motif_context PRIVATE kdna)
if(UNIX)
  target_link_libraries(kalyx_motif_context PRIVATE m)
endif()

add_test(NAME kalyx_motif_context_help COMMAND kalyx_motif_context --help)

add_executable(kalyx_motif_family tools/kalyx_motif_family.c)
target_link_libraries(kalyx_motif_family PRIVATE kdna)
if(UNIX)
  target_link_libraries(kalyx_motif_family PRIVATE m)
endif()

add_test(NAME kalyx_motif_family_help COMMAND kalyx_motif_family --help)

add_executable(kalyx_signature_scan tools/kalyx_signature_scan.c)
target_link_libraries(kalyx_signature_scan PRIVATE kdna)

add_test(NAME kalyx_signature_scan_help COMMAND kalyx_signature_scan --help)

# KALYX Interaction Envelope / Response / Audit substrate
add_library(kalyx_core STATIC
    src/kalyx_common.c
    src/kalyx_error.c
    src/kalyx_domain.c
    src/kalyx_json.c
    src/kalyx_action.c
    src/kalyx_policy.c
    src/kalyx_host.c
    src/kalyx_dispatch.c
    src/kalyx_hash.c
    src/kalyx_interaction.c
    src/kalyx_response.c
    src/kalyx_audit.c
)
target_include_directories(kalyx_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(kalyx_make_envelope tools/kalyx_make_envelope.c)
target_link_libraries(kalyx_make_envelope PRIVATE kalyx_core)

add_executable(kalyx_validate_response tools/kalyx_validate_response.c)
target_link_libraries(kalyx_validate_response PRIVATE kalyx_core)

add_executable(kalyx_audit_print tools/kalyx_audit_print.c)
target_link_libraries(kalyx_audit_print PRIVATE kalyx_core)

add_executable(kalyx_host_dispatch_demo tools/kalyx_host_dispatch_demo.c)
target_link_libraries(kalyx_host_dispatch_demo PRIVATE kalyx_core)

add_executable(kalyx_prompt_author tools/kalyx_prompt_author.c)
target_link_libraries(kalyx_prompt_author PRIVATE kalyx_core)

add_executable(test_kalyx_envelope_determinism tests/test_kalyx_envelope_determinism.c)
target_link_libraries(test_kalyx_envelope_determinism PRIVATE kalyx_core)

add_executable(test_kalyx_response_validation tests/test_kalyx_response_validation.c)
target_link_libraries(test_kalyx_response_validation PRIVATE kalyx_core)

add_executable(test_kalyx_forbidden_actions tests/test_kalyx_forbidden_actions.c)
target_link_libraries(test_kalyx_forbidden_actions PRIVATE kalyx_core)

add_executable(test_kalyx_allowed_actions tests/test_kalyx_allowed_actions.c)
target_link_libraries(test_kalyx_allowed_actions PRIVATE kalyx_core)

add_executable(test_kalyx_audit_hashes tests/test_kalyx_audit_hashes.c)
target_link_libraries(test_kalyx_audit_hashes PRIVATE kalyx_core)

add_executable(test_kalyx_domains tests/test_kalyx_domains.c)
target_link_libraries(test_kalyx_domains PRIVATE kalyx_core)

add_executable(test_kalyx_json tests/test_kalyx_json.c)
target_link_libraries(test_kalyx_json PRIVATE kalyx_core)

add_executable(test_kalyx_action_args tests/test_kalyx_action_args.c)
target_link_libraries(test_kalyx_action_args PRIVATE kalyx_core)

add_executable(test_kalyx_policy_rules tests/test_kalyx_policy_rules.c)
target_link_libraries(test_kalyx_policy_rules PRIVATE kalyx_core)

add_executable(test_kalyx_confirmation_policy tests/test_kalyx_confirmation_policy.c)
target_link_libraries(test_kalyx_confirmation_policy PRIVATE kalyx_core)

add_executable(test_kalyx_host_sdk tests/test_kalyx_host_sdk.c)
target_link_libraries(test_kalyx_host_sdk PRIVATE kalyx_core)

add_executable(test_kalyx_safe_section_parser tests/test_kalyx_safe_section_parser.c)
target_link_libraries(test_kalyx_safe_section_parser PRIVATE kalyx_core)

add_executable(test_kalyx_dispatch_sandbox tests/test_kalyx_dispatch_sandbox.c)
target_link_libraries(test_kalyx_dispatch_sandbox PRIVATE kalyx_core)

add_executable(test_kalyx_dispatch_strict_gate tests/test_kalyx_dispatch_strict_gate.c)
target_link_libraries(test_kalyx_dispatch_strict_gate PRIVATE kalyx_core)

add_executable(test_kalyx_workflow_sandbox tests/test_kalyx_workflow_sandbox.c)
target_link_libraries(test_kalyx_workflow_sandbox PRIVATE kalyx_core)

add_executable(test_kalyx_workflow_policy_engine tests/test_kalyx_workflow_policy_engine.c)
target_link_libraries(test_kalyx_workflow_policy_engine PRIVATE kalyx_core)

add_test(NAME kalyx_envelope_determinism COMMAND $<TARGET_FILE:test_kalyx_envelope_determinism>)
add_test(NAME kalyx_response_validation COMMAND $<TARGET_FILE:test_kalyx_response_validation>)
add_test(NAME kalyx_forbidden_actions COMMAND $<TARGET_FILE:test_kalyx_forbidden_actions>)
add_test(NAME kalyx_allowed_actions COMMAND $<TARGET_FILE:test_kalyx_allowed_actions>)
add_test(NAME kalyx_audit_hashes COMMAND $<TARGET_FILE:test_kalyx_audit_hashes> ${CMAKE_CURRENT_BINARY_DIR})
add_test(NAME kalyx_domains COMMAND $<TARGET_FILE:test_kalyx_domains>)
add_test(NAME kalyx_json COMMAND $<TARGET_FILE:test_kalyx_json>)
add_test(NAME kalyx_action_args COMMAND $<TARGET_FILE:test_kalyx_action_args>)
add_test(NAME kalyx_policy_rules COMMAND $<TARGET_FILE:test_kalyx_policy_rules>)
add_test(NAME kalyx_confirmation_policy COMMAND $<TARGET_FILE:test_kalyx_confirmation_policy>)
add_test(NAME kalyx_host_sdk COMMAND $<TARGET_FILE:test_kalyx_host_sdk>)
add_test(NAME kalyx_safe_section_parser COMMAND $<TARGET_FILE:test_kalyx_safe_section_parser>)
add_test(NAME kalyx_dispatch_sandbox COMMAND $<TARGET_FILE:test_kalyx_dispatch_sandbox> ${CMAKE_CURRENT_BINARY_DIR})
add_test(NAME kalyx_dispatch_strict_gate COMMAND $<TARGET_FILE:test_kalyx_dispatch_strict_gate> ${CMAKE_CURRENT_BINARY_DIR})
add_test(NAME kalyx_workflow_sandbox COMMAND $<TARGET_FILE:test_kalyx_workflow_sandbox> ${CMAKE_CURRENT_BINARY_DIR})
add_test(NAME kalyx_workflow_policy_engine COMMAND $<TARGET_FILE:test_kalyx_workflow_policy_engine> ${CMAKE_CURRENT_BINARY_DIR})
add_test(NAME kalyx_document_input_adapter COMMAND $<TARGET_FILE:kalyx_make_envelope> --domain app --intent summarize_document --document ${CMAKE_CURRENT_SOURCE_DIR}/README.md --document-type markdown --request ${CMAKE_CURRENT_SOURCE_DIR}/examples/app_request.txt --out ${CMAKE_CURRENT_BINARY_DIR}/readme_doc.kie.md)
add_test(NAME kalyx_make_envelope_help COMMAND $<TARGET_FILE:kalyx_make_envelope> --help)
add_test(NAME kalyx_validate_response_help COMMAND $<TARGET_FILE:kalyx_validate_response> --help)
add_test(NAME kalyx_audit_print_help COMMAND $<TARGET_FILE:kalyx_audit_print> --help)
add_test(NAME kalyx_host_dispatch_demo_help COMMAND $<TARGET_FILE:kalyx_host_dispatch_demo> --help)
add_test(NAME kalyx_prompt_author_help COMMAND $<TARGET_FILE:kalyx_prompt_author> --help)
add_test(NAME kalyx_prompt_author_list_profiles COMMAND $<TARGET_FILE:kalyx_prompt_author> --list-profiles)
add_test(NAME kalyx_prompt_author_pack COMMAND $<TARGET_FILE:kalyx_prompt_author> --domain app --intent summarize_and_export --goal "Fasse README.md zusammen und exportiere Markdown in die Sandbox" --pack ${CMAKE_CURRENT_BINARY_DIR}/prompt_author_pack)
add_test(NAME kalyx_prompt_author_profile_pack COMMAND $<TARGET_FILE:kalyx_prompt_author> --profile app.summary_export --goal "Fasse README.md zusammen und exportiere Markdown in die Sandbox" --pack ${CMAKE_CURRENT_BINARY_DIR}/prompt_profile_pack)
add_test(NAME kalyx_prompt_author_research_profile COMMAND $<TARGET_FILE:kalyx_prompt_author> --profile research.hypothesis_report --pack ${CMAKE_CURRENT_BINARY_DIR}/prompt_research_profile_pack)
add_test(NAME kalyx_prompt_pack_envelope COMMAND $<TARGET_FILE:kalyx_make_envelope> --prompt-pack ${CMAKE_CURRENT_BINARY_DIR}/prompt_author_pack --domain app --intent summarize_and_export --out ${CMAKE_CURRENT_BINARY_DIR}/prompt_pack_envelope.kie.md)
add_test(NAME kalyx_prompt_profile_pack_envelope COMMAND $<TARGET_FILE:kalyx_make_envelope> --prompt-pack ${CMAKE_CURRENT_BINARY_DIR}/prompt_profile_pack --domain app --intent summarize_and_export --out ${CMAKE_CURRENT_BINARY_DIR}/prompt_profile_pack_envelope.kie.md)
set_tests_properties(kalyx_prompt_pack_envelope PROPERTIES DEPENDS kalyx_prompt_author_pack)
set_tests_properties(kalyx_prompt_profile_pack_envelope PROPERTIES DEPENDS kalyx_prompt_author_profile_pack)
add_test(NAME kalyx_prompt_author_list_custom_profiles COMMAND $<TARGET_FILE:kalyx_prompt_author> --list-profiles --profile-dir ${CMAKE_CURRENT_SOURCE_DIR}/profiles)
add_test(NAME kalyx_prompt_author_custom_profile_file COMMAND $<TARGET_FILE:kalyx_prompt_author> --profile-file ${CMAKE_CURRENT_SOURCE_DIR}/profiles/custom.summary_export.kprofile.json --pack ${CMAKE_CURRENT_BINARY_DIR}/prompt_custom_profile_file_pack)
add_test(NAME kalyx_prompt_author_custom_profile_dir COMMAND $<TARGET_FILE:kalyx_prompt_author> --profile-dir ${CMAKE_CURRENT_SOURCE_DIR}/profiles --profile custom.summary_export --pack ${CMAKE_CURRENT_BINARY_DIR}/prompt_custom_profile_dir_pack)
add_test(NAME kalyx_prompt_custom_profile_pack_envelope COMMAND $<TARGET_FILE:kalyx_make_envelope> --prompt-pack ${CMAKE_CURRENT_BINARY_DIR}/prompt_custom_profile_dir_pack --domain app --intent custom_summary_export --out ${CMAKE_CURRENT_BINARY_DIR}/prompt_custom_profile_envelope.kie.md)
set_tests_properties(kalyx_prompt_custom_profile_pack_envelope PROPERTIES DEPENDS kalyx_prompt_author_custom_profile_dir)

add_test(NAME kalyx_prompt_author_version COMMAND $<TARGET_FILE:kalyx_prompt_author> --version)
add_test(NAME kalyx_make_envelope_version COMMAND $<TARGET_FILE:kalyx_make_envelope> --version)
add_test(NAME kalyx_validate_response_version COMMAND $<TARGET_FILE:kalyx_validate_response> --version)
add_test(NAME kalyx_host_dispatch_demo_version COMMAND $<TARGET_FILE:kalyx_host_dispatch_demo> --version)



# KALYX-KIE v1.0 Final documentation/manifest smoke checks
add_test(NAME kalyx_final_manifest_present COMMAND ${CMAKE_COMMAND} -E compare_files ${CMAKE_CURRENT_SOURCE_DIR}/KALYX_KIE_v1_0_Final_MANIFEST.json ${CMAKE_CURRENT_SOURCE_DIR}/KALYX_KIE_v1_0_Final_MANIFEST.json)
add_test(NAME kalyx_final_quickstart_present COMMAND ${CMAKE_COMMAND} -E compare_files ${CMAKE_CURRENT_SOURCE_DIR}/docs/KALYX_KIE_v1_0_Final_QUICKSTART.md ${CMAKE_CURRENT_SOURCE_DIR}/docs/KALYX_KIE_v1_0_Final_QUICKSTART.md)
add_test(NAME kalyx_final_test_matrix_present COMMAND ${CMAKE_COMMAND} -E compare_files ${CMAKE_CURRENT_SOURCE_DIR}/docs/KALYX_KIE_v1_0_Final_TEST_MATRIX.md ${CMAKE_CURRENT_SOURCE_DIR}/docs/KALYX_KIE_v1_0_Final_TEST_MATRIX.md)
add_test(NAME kalyx_final_known_issues_present COMMAND ${CMAKE_COMMAND} -E compare_files ${CMAKE_CURRENT_SOURCE_DIR}/docs/KALYX_KIE_v1_0_Final_KNOWN_ISSUES.md ${CMAKE_CURRENT_SOURCE_DIR}/docs/KALYX_KIE_v1_0_Final_KNOWN_ISSUES.md)
add_test(NAME kalyx_final_release_notes_present COMMAND ${CMAKE_COMMAND} -E compare_files ${CMAKE_CURRENT_SOURCE_DIR}/KALYX_KIE_v1_0_Final_RELEASE_NOTES.md ${CMAKE_CURRENT_SOURCE_DIR}/KALYX_KIE_v1_0_Final_RELEASE_NOTES.md)

#include "regex.h"
#include <benchmark/benchmark.h>

static void BM_RegexCompilation(benchmark::State& state) {
  for (auto _ : state)
    regex::compile(".*");
}
// Register the function as a benchmark
BENCHMARK(BM_RegexCompilation);

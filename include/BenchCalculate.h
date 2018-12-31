#ifndef BENCH_CALCULATE_H
#define BENCH_CALCULATE_H

#include <string>

#include "Benchmark.h"


class BenchCalculate : public Benchmark {
public:

    BenchCalculate();

    double DoBenchmark(const std::string&, long);
};

#endif

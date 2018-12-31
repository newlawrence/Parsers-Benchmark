#include <vector>
#include <cmath>

#include "BenchCalculate.h"
#include "calculate/calculate.hpp"


BenchCalculate::BenchCalculate() : Benchmark() {
    m_sName = "Calculate 2.1.1rc10";
}

double BenchCalculate::DoBenchmark(const std::string& sExpr, long iCount) {
    // Adapted lexer to accept only one-character length symbols
    auto lexer = calculate::Lexer<double>(
        calculate::defaults::number<double>,
        calculate::defaults::name,
        R"(^(?:[^A-Za-z\d.(),_\s]|(?:\.(?!\d)))$)",
        calculate::defaults::left,
        calculate::defaults::right,
        calculate::defaults::separator
    );

    // Create parser instance and add missing operators for the benchmarks
    calculate::Parser parser{lexer};
    parser.operators.emplace(
        "<",
        calculate::Parser::Operator{
            [](double x, double y) noexcept { return static_cast<double>(x < y); },
            calculate::defaults::Precedence::very_low,
            calculate::Parser::Associativity::LEFT
        }
    );
    parser.optimize = true;

    // Initialize
    auto expression = parser.parse("0");
    std::vector<double> initial_values = {
        1.1,       // a
        2.2,       // b
        3.3,       // c
        2.123456,  // x
        3.123456,  // y
        4.123456,  // z
        5.123456   // w
    };

    // Perform basic tests for the variables used in the expressions
    {
        auto test_result = true;
        auto tests_list = test_expressions();
        auto& tests_values = initial_values;
        auto test_value = tests_values.begin();

        for (auto test : tests_list) {
            try {
                auto test_expression = parser.parse(test.first);
                if (!is_equal(test.second, test_expression(*test_value))) {
                    test_result = false;
                    break;
                }
                ++test_value;
            }
            catch (const calculate::BaseError& exception) {
                StopTimerAndReport(exception.what());
                return m_fTime1;
            }
        }

        if (!test_result) {
            StopTimerAndReport("Failed variable test");
            return m_fTime1;
        }
    }
    
    // Test given expression
    {
        try {
            expression = parser.parse(sExpr);
        }
        catch (const calculate::BaseError& exception) {
            StopTimerAndReport(exception.what());
            return m_fTime1;
        }
    }
    
    // Extract vector variables and get values to evaluate
    auto variables = expression.variables();
    std::sort(variables.begin(), variables.end());
    expression = parser.from_postfix(expression.postfix(), variables);  // Order variables

    std::vector<bool> in_expression{};
    for (auto& variable : std::vector<std::string>{"a", "b", "c", "w", "x", "y", "z"})
        in_expression.push_back(
            std::find(variables.begin(), variables.end(), variable) != variables.end()
        );

    // Order values to match variables names previously ordered
    std::swap(initial_values[3], initial_values[4]);  // x --> y
    std::swap(initial_values[3], initial_values[5]);  // y --> z
    std::swap(initial_values[3], initial_values[6]);  // z --> w

    // Load values
    std::vector<double> values{};
    int a_pos{-1}, b_pos{-1}, x_pos{-1}, y_pos{-1};
    for (std::size_t i = 0; i < initial_values.size(); ++i) {
        if (in_expression[i]) {
            switch (i) {
            case 0:
                a_pos = values.size();  // Set a position in values
                break;
            case 1:
                b_pos = values.size();  // Set b position in values
                break;
            case 4:
                x_pos = values.size();  // Set x position in values
                break;
            case 5:
                y_pos = values.size();  // Set y position in values
                break;
            }
            values.push_back(initial_values[i]);
        }

    }

    // Prime the I and D caches for the expression
    {
        double d0 = 0.0;
        double d1 = 0.0;

        for (std::size_t i = 0; i < priming_rounds; ++i) {
            if (i & 1)
                d0 += expression(values);
            else
                d1 += expression(values);
        }

        if (
            (d0 == std::numeric_limits<double>::infinity()) &&
            (d1 == std::numeric_limits<double>::infinity())
        )
            printf("\n");
    }

    // Generate references
    double& a = a_pos >= 0 ? values[a_pos] : initial_values[0];
    double& b = b_pos >= 0 ? values[b_pos] : initial_values[1];
    double& x = x_pos >= 0 ? values[x_pos] : initial_values[4];
    double& y = y_pos >= 0 ? values[y_pos] : initial_values[5];

    // Perform benchmark then return results
    double fRes = expression(values), fSum = 0.;
    StartTimer();
    for (int j = 0; j < iCount; ++j) {
        fSum += expression(values);
        std::swap(a, b);
        std::swap(x, y);
    }
    StopTimer(fRes, fSum, iCount);

    return m_fTime1;
}

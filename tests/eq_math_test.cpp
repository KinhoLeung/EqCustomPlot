#include "eq.h"

#include <cmath>
#include <iostream>
#include <limits>

namespace
{
constexpr int kSampleRate = 384000;
constexpr double kToleranceDb = 0.02;

QVector<double> makeLogFrequencies(int count)
{
    QVector<double> result;
    result.resize(count);

    constexpr double lower = 16.0;
    constexpr double upper = 25000.0;
    const double step = (std::log10(upper) - std::log10(lower)) / static_cast<double>(count - 1);

    for (int i = 0; i < count; ++i)
        result[i] = lower * std::pow(10.0, step * i);

    return result;
}

bool sameSignInfinity(double a, double b)
{
    return std::isinf(a) && std::isinf(b) && std::signbit(a) == std::signbit(b);
}

bool closeEnough(double reference, double actual)
{
    if (sameSignInfinity(reference, actual))
        return true;
    if (!std::isfinite(reference) || !std::isfinite(actual))
        return false;
    return std::abs(reference - actual) <= kToleranceDb;
}

bool compareVectors(const QVector<double> &reference, const QVector<double> &actual, const char *caseName)
{
    if (reference.size() != actual.size())
    {
        std::cerr << caseName << ": size mismatch\n";
        return false;
    }

    for (int i = 0; i < reference.size(); ++i)
    {
        if (!closeEnough(reference.at(i), actual.at(i)))
        {
            std::cerr << caseName << ": index " << i << " reference=" << reference.at(i)
                      << " actual=" << actual.at(i) << '\n';
            return false;
        }
    }

    return true;
}
} // namespace

int main()
{
    const QVector<double> frequencies = makeLogFrequencies(257);
    const eq::FrequencyGrid grid = eq::makeFrequencyGrid(kSampleRate, frequencies);

    const QVector<eq::FilterType> types = {
        eq::FilterType::Peak,     eq::FilterType::Lowshelf, eq::FilterType::Highshelf,
        eq::FilterType::Lowpass,  eq::FilterType::Highpass, eq::FilterType::Bandpass,
        eq::FilterType::Notch,    eq::FilterType::Allpass,
    };
    const QVector<int> centerFrequencies = {20, 997, 20000};
    const QVector<double> gains = {-18.0, 0.0, 18.0};
    const QVector<double> qValues = {0.1, 0.7, 1.0, 10.0, 30.0};

    QVector<double> fast;
    int checked = 0;

    for (eq::FilterType type : types)
    {
        for (int fc : centerFrequencies)
        {
            for (double gain : gains)
            {
                for (double q : qValues)
                {
                    const eq::Coeff coeff = eq::getSectionsMatrix(gain, fc, q, type, false, kSampleRate);
                    const QVector<double> reference = eq::getFreqzn(coeff, kSampleRate, frequencies);
                    eq::getFreqznFast(coeff, grid, fast);
                    if (!compareVectors(reference, fast, "single-section"))
                        return 1;
                    ++checked;
                }
            }
        }
    }

    const QVector<eq::Coeff> coeffList = {
        eq::getSectionsMatrix(6.0, 80, 0.7, eq::FilterType::Highpass, false, kSampleRate),
        eq::getSectionsMatrix(-4.5, 997, 3.5, eq::FilterType::Peak, false, kSampleRate),
        eq::getSectionsMatrix(3.0, 12000, 0.8, eq::FilterType::Highshelf, false, kSampleRate),
    };
    eq::getFreqznFast(coeffList, grid, fast);
    if (!compareVectors(eq::getFreqzn(coeffList, kSampleRate, frequencies), fast, "multi-section"))
        return 1;

    const eq::Coeff bypassCoeff =
        eq::getSectionsMatrix(18.0, 997, 30.0, eq::FilterType::Peak, true, kSampleRate);
    eq::getFreqznFast(bypassCoeff, grid, fast);
    for (double value : fast)
    {
        if (!std::isfinite(value) || std::abs(value) > 1e-9)
        {
            std::cerr << "bypass: expected 0 dB, got " << value << '\n';
            return 1;
        }
    }

    std::cout << "Checked " << checked << " single-section cases plus multi-section and bypass cases.\n";
    return 0;
}

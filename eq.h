#pragma once

#include <QVector>

namespace eq
{

enum class FilterType : int
{
    Unknown = -1,
    Peak,
    Lowshelf,
    Highshelf,
    Lowpass,
    Highpass,
    Bandpass,
    Notch,
    Allpass
};

struct Coeff
{
    double A0;
    double A1;
    double A2;
    double B0;
    double B1;
    double B2;

    Coeff(double a0 = 1, double a1 = 0, double a2 = 0, double b0 = 1, double b1 = 0, double b2 = 0)
        : A0(a0), A1(a1), A2(a2), B0(b0), B1(b1), B2(b2)
    {
    }
};

struct FrequencyGrid
{
    int sampleRate = 0;
    QVector<double> frequencyHz;
    QVector<double> cosW;
    QVector<double> sinW;
    QVector<double> cos2W;
    QVector<double> sin2W;

    int size() const;
    bool isValid() const;
};

QVector<double> getFreqzn(const Coeff &coeff, int fs, const QVector<double> &f);
QVector<double> getFreqzn(const QVector<Coeff> &coeffList, int fs, const QVector<double> &f);
FrequencyGrid makeFrequencyGrid(int fs, const QVector<double> &f);
void getFreqznFast(const Coeff &coeff, const FrequencyGrid &grid, QVector<double> &outDb);
void getFreqznFast(const QVector<Coeff> &coeffList, const FrequencyGrid &grid, QVector<double> &outDb);
Coeff getSectionsMatrix(double gain, int fc, double q, FilterType type, bool bypass, int fs);

} // namespace eq

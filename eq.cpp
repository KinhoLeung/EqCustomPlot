#include "eq.h"
#include <cmath>
#include <complex>

namespace eq
{

namespace
{
/**
 * @brief 计算某组滤波器下的频响
 *
 * @param coeff 传递函数的系数 b0,b1,b2,a0,a1,a2
 * @param w 角频率
 * @return std::complex<double> 频率响应 （复数类型）
 */
std::complex<double> getFreqw(const Coeff &coeff, double w)
{
    std::complex<double> complex(std::cos(w), -std::sin(w));
    std::complex<double> b2 = complex * complex * coeff.B2;
    std::complex<double> b1 = complex * coeff.B1;
    std::complex<double> hup = b2 + b1 + coeff.B0;
    std::complex<double> a2 = complex * complex * coeff.A2;
    std::complex<double> a1 = complex * coeff.A1;
    std::complex<double> hdown = a2 + a1 + coeff.A0;
    std::complex<double> res = hup / hdown;
    return res;
}

/**
 * @brief 计算多组滤波器作用下的频响
 *
 * @param coeffList 系数组
 * @param w 频率
 * @return std::complex<double> 频响
 */
std::complex<double> getFreqw(const QVector<Coeff> &coeffList, double w)
{
    if (coeffList.empty())
        return std::complex<double>(0, 0);
    std::complex<double> h = getFreqw(coeffList.at(0), w);
    for (int i = 1; i < coeffList.size(); i++)
    {
        h = h * getFreqw(coeffList.at(i), w);
    }
    return h;
}

/**
 * @brief 计算幅值
 *
 * @param complex 频率响应
 * @return double 幅值
 */
double getAmplitude(const std::complex<double> &complex)
{
    return std::abs(complex);
}

/**
 * @brief 计算相位
 *
 * @param complex 频率响应
 * @return double 相位（-pi~pi）
 */
double getPhase(const std::complex<double> &complex)
{
    return std::atan2(complex.imag(), complex.real());
}
} // namespace

/**
 * @brief 计算n个点的频率响应
 *
 * @param coeff 系数
 * @param fs 采样频率
 * @param f 频率数组
 * @return QVector<double> n个点的频率响应 （dB）
 */
QVector<double> getFreqzn(const Coeff &coeff, int fs, const QVector<double> &f)
{
    if (f.empty())
        return QVector<double>();
    int n = f.size();
    QVector<double> h(n);
    for (int i = 0; i < n; i++)
    {
        double w = 2 * M_PI * (f.at(i)) / fs;
        std::complex<double> complex = getFreqw(coeff, w);
        h[i] = 20 * std::log10(getAmplitude(complex));
    }
    return h;
}

/**
 * @brief 计算多个滤波器作用下n个点的频率响应
 *
 * @param coeffList 滤波器系数组
 * @param fs 采样频率
 * @param f 频率数组
 * @return QVector<double> n个点的频率响应 （dB）
 */
QVector<double> getFreqzn(const QVector<Coeff> &coeffList, int fs, const QVector<double> &f)
{
    if (coeffList.empty() || f.empty())
        return QVector<double>();
    int n = f.size();
    QVector<double> h(n);
    for (int i = 0; i < n; i++)
    {
        double w = 2 * M_PI * (f.at(i)) / fs;
        std::complex<double> complex = getFreqw(coeffList, w);
        h[i] = 20 * std::log10(getAmplitude(complex));
    }
    return h;
}

/**
 * @brief 计算滤波器的截面矩阵系数
 *
 * @param gain Gain
 * @param fc Fc
 * @param q Q
 * @param type
 * Type：peak、lowshelf、highshelf、lowpass、highpass、bandpass、notch、allpass（0-7）
 * @param bypass bypass
 * @param fs 采样频率
 * @return EQ::Coeff 截面矩阵系数sos
 */
Coeff getSectionsMatrix(double gain, int fc, double q, FilterType type, bool bypass, int fs)
{
    double w = 2.0 * fc / fs;
    FilterType bqtype = bypass ? FilterType::Unknown : type;
    double sA = std::pow(10, gain / 40);
    double sqrt_sA = std::sqrt(sA);
    double sin_w = std::sin(M_PI * w);
    double cos_w = std::cos(M_PI * w);
    double alpha = sin_w / (2 * q);

    double b0, b1, b2, a0, a1, a2;

    switch (bqtype)
    {
    case FilterType::Peak:
        b0 = 1 + alpha * sA;
        b1 = -2 * cos_w;
        b2 = 1 - alpha * sA;
        a0 = 1 + alpha / sA;
        a1 = b1;
        a2 = 1 - alpha / sA;
        break;
    case FilterType::Lowshelf:
        b0 = sA * ((sA + 1) - (sA - 1) * cos_w + 2 * sqrt_sA * alpha);
        b1 = 2 * sA * ((sA - 1) - (sA + 1) * cos_w);
        b2 = sA * ((sA + 1) - (sA - 1) * cos_w - 2 * sqrt_sA * alpha);
        a0 = (sA + 1) + (sA - 1) * cos_w + 2 * sqrt_sA * alpha;
        a1 = -2 * ((sA - 1) + (sA + 1) * cos_w);
        a2 = (sA + 1) + (sA - 1) * cos_w - 2 * sqrt_sA * alpha;
        break;
    case FilterType::Highshelf:
        b0 = sA * ((sA + 1) + (sA - 1) * cos_w + 2 * sqrt_sA * alpha);
        b1 = -2 * sA * ((sA - 1) + (sA + 1) * cos_w);
        b2 = sA * ((sA + 1) + (sA - 1) * cos_w - 2 * sqrt_sA * alpha);
        a0 = (sA + 1) - (sA - 1) * cos_w + 2 * sqrt_sA * alpha;
        a1 = 2 * ((sA - 1) - (sA + 1) * cos_w);
        a2 = (sA + 1) - (sA - 1) * cos_w - 2 * sqrt_sA * alpha;
        break;
    case FilterType::Lowpass:
        b0 = (1 - cos_w) / 2;
        b1 = 1 - cos_w;
        b2 = b0;
        a0 = 1 + alpha;
        a1 = -2 * cos_w;
        a2 = 1 - alpha;
        break;
    case FilterType::Highpass:
        b0 = (1 + cos_w) / 2;
        b1 = -(1 + cos_w);
        b2 = b0;
        a0 = 1 + alpha;
        a1 = -2 * cos_w;
        a2 = 1 - alpha;
        break;
    case FilterType::Bandpass:
        b0 = alpha;
        b1 = 0;
        b2 = -alpha;
        a0 = 1 + alpha;
        a1 = -2 * cos_w;
        a2 = 1 - alpha;
        break;
    case FilterType::Notch:
        b0 = 1;
        b1 = -2 * cos_w;
        b2 = 1;
        a0 = 1 + alpha;
        a1 = -2 * cos_w;
        a2 = 1 - alpha;
        break;
    case FilterType::Allpass:
        b0 = 1 - alpha;
        b1 = -2 * cos_w;
        b2 = 1 + alpha;
        a0 = b2;
        a1 = b1;
        a2 = b0;
        break;
    default:
        b0 = 1;
        b1 = 0;
        b2 = 0;
        a0 = 1;
        a1 = 0;
        a2 = 0;
        break;
    }

    return Coeff(a0 / a0, a1 / a0, a2 / a0, b0 / a0, b1 / a0, b2 / a0);
}

} // namespace eq
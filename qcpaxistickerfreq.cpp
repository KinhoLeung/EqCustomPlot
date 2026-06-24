#include "qcpaxistickerfreq.h"
#include <algorithm>
#include <cmath>

QCPAxisTickerFreq::SubTickRule::SubTickRule(double from, double to, const QVector<double> &minorMantissas)
    : fromMantissa(from), toMantissa(to), mantissas(minorMantissas)
{
}

QCPAxisTickerFreq::UnitFormat::UnitFormat(double threshold, double divisor, const QString &unitSuffix)
    : thresholdValue(threshold), scaleDivisor(divisor), suffix(unitSuffix)
{
}

QCPAxisTickerFreq::LabelFormat::LabelFormat()
{
    formatChar = QLatin1Char('g');
    units = {
        UnitFormat(1000000000.0, 1000000000.0, QStringLiteral("G")),
        UnitFormat(1000000.0, 1000000.0, QStringLiteral("M")),
        UnitFormat(1000.0, 1000.0, QStringLiteral("k")),
    };
}

QCPAxisTickerFreq::FrequencyTickSpec::FrequencyTickSpec()
{
    // 默认刻度适配常见音频频率轴：20、50、100、200、500、1k、2k、5k...
    majorMantissas = {2.0, 5.0, 10.0};
    minorRules = {
        SubTickRule(2.0, 5.0, {3.0, 4.0}),
        SubTickRule(5.0, 10.0, {6.0, 7.0, 8.0, 9.0}),
    };
}

QCPAxisTickerFreq::QCPAxisTickerFreq()
{
    normalizeSpec();
}

QCPAxisTickerFreq::FrequencyTickSpec QCPAxisTickerFreq::tickSpec() const
{
    return mSpec;
}

void QCPAxisTickerFreq::setTickSpec(const FrequencyTickSpec &spec)
{
    mSpec = spec;
    normalizeSpec();
}

void QCPAxisTickerFreq::resetTickSpec()
{
    mSpec = FrequencyTickSpec();
    normalizeSpec();
}

void QCPAxisTickerFreq::generate(const QCPRange &range, const QLocale &locale, QChar formatChar, int precision,
                                 QVector<double> &ticks, QVector<double> *subTicks, QVector<QString> *tickLabels)
{
    // 次刻度依赖相邻主刻度区间，先保留视野外各一颗主刻度，再裁剪最终可见主刻度。
    const QVector<double> ticksWithOutliers = createMajorTicks(range, true);
    ticks = ticksWithOutliers;
    trimTicks(range, ticks, false);

    if (subTicks)
        *subTicks = createMinorTicks(ticksWithOutliers, range);
    if (tickLabels)
        *tickLabels = createLabels(ticks, locale, formatChar, precision);
}

bool QCPAxisTickerFreq::approxEqual(double a, double b, double relTol)
{
    if (a == b)
        return true;
    const double denom = std::max(std::abs(a), std::abs(b));
    if (denom == 0)
        return std::abs(a - b) < 1e-12;
    return std::abs(a - b) / denom <= relTol;
}

double QCPAxisTickerFreq::ipow10(int p)
{
    return std::pow(10.0, p);
}

int QCPAxisTickerFreq::floorDecade(double v)
{
    if (v <= 0)
        return -9;
    return (int)std::floor(std::log10(v));
}

int QCPAxisTickerFreq::ceilDecade(double v)
{
    if (v <= 0)
        return -9;
    return (int)std::ceil(std::log10(v));
}

QVector<double> QCPAxisTickerFreq::cleanMantissas(const QVector<double> &mantissas, double relTol)
{
    QVector<double> result;
    result.reserve(mantissas.size());

    for (double mantissa : mantissas)
    {
        if (std::isfinite(mantissa) && mantissa > 0)
            result.push_back(mantissa);
    }

    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end(),
                             [&](double a, double b) { return approxEqual(a, b, relTol); }),
                 result.end());

    return result;
}

QVector<QCPAxisTickerFreq::SubTickRule>
QCPAxisTickerFreq::cleanSubTickRules(const QVector<SubTickRule> &rules, double relTol)
{
    QVector<SubTickRule> result;
    result.reserve(rules.size());

    for (const auto &rule : rules)
    {
        if (!std::isfinite(rule.fromMantissa) || !std::isfinite(rule.toMantissa) ||
            rule.fromMantissa <= 0 || rule.toMantissa <= rule.fromMantissa)
            continue;

        SubTickRule cleanedRule(rule.fromMantissa, rule.toMantissa, cleanMantissas(rule.mantissas, relTol));
        // 次刻度 mantissa 必须严格落在规则区间内，避免和主刻度重复。
        cleanedRule.mantissas.erase(
            std::remove_if(cleanedRule.mantissas.begin(), cleanedRule.mantissas.end(),
                           [&](double m) {
                               return m <= cleanedRule.fromMantissa || m >= cleanedRule.toMantissa;
                           }),
            cleanedRule.mantissas.end());

        if (!cleanedRule.mantissas.isEmpty())
            result.push_back(cleanedRule);
    }

    std::sort(result.begin(), result.end(), [](const SubTickRule &a, const SubTickRule &b) {
        if (a.fromMantissa == b.fromMantissa)
            return a.toMantissa < b.toMantissa;
        return a.fromMantissa < b.fromMantissa;
    });
    result.erase(std::unique(result.begin(), result.end(),
                             [&](const SubTickRule &a, const SubTickRule &b) {
                                 return approxEqual(a.fromMantissa, b.fromMantissa, relTol) &&
                                        approxEqual(a.toMantissa, b.toMantissa, relTol);
                             }),
                 result.end());

    return result;
}

QVector<QCPAxisTickerFreq::UnitFormat>
QCPAxisTickerFreq::cleanUnitFormats(const QVector<UnitFormat> &units, double relTol)
{
    QVector<UnitFormat> result;
    result.reserve(units.size());

    for (const auto &unit : units)
    {
        if (std::isfinite(unit.thresholdValue) && std::isfinite(unit.scaleDivisor) &&
            unit.thresholdValue > 0 && unit.scaleDivisor > 0)
            result.push_back(unit);
    }

    std::sort(result.begin(), result.end(), [](const UnitFormat &a, const UnitFormat &b) {
        return a.thresholdValue > b.thresholdValue;
    });
    result.erase(std::unique(result.begin(), result.end(),
                             [&](const UnitFormat &a, const UnitFormat &b) {
                                 return approxEqual(a.thresholdValue, b.thresholdValue, relTol);
                             }),
                 result.end());

    return result;
}

void QCPAxisTickerFreq::normalizeSpec()
{
    // 配置对象来自外部，进入生成流程前统一过滤非法值、排序和去重。
    if (!std::isfinite(mSpec.relTol) || mSpec.relTol <= 0)
        mSpec.relTol = 1e-3;

    mSpec.majorMantissas = cleanMantissas(mSpec.majorMantissas, mSpec.relTol);
    mSpec.minorRules = cleanSubTickRules(mSpec.minorRules, mSpec.relTol);
    mSpec.labelFormat.units = cleanUnitFormats(mSpec.labelFormat.units, mSpec.relTol);

    if (mSpec.minDecade > mSpec.maxDecade)
        std::swap(mSpec.minDecade, mSpec.maxDecade);
}

void QCPAxisTickerFreq::normalizeMantissa(double v, double &base, double &m) const
{
    int d = floorDecade(v);
    base = ipow10(d);
    m = v / base;

    // 把 1*10^n 归一成 10*10^(n-1)，这样 1000 可匹配 10*100。
    if (approxEqual(m, 1.0, mSpec.relTol))
    {
        d -= 1;
        base /= 10.0;
        m = 10.0;
    }
}

QVector<double> QCPAxisTickerFreq::createMajorTicks(const QCPRange &range, bool keepOneOutlier) const
{
    QVector<double> ticks;
    if (range.size() <= 0 || range.upper <= 0 || mSpec.majorMantissas.isEmpty())
        return ticks;

    // 多生成前后一层 decade，给边界附近的次刻度计算留出相邻主刻度。
    const int dFirst = floorDecade(range.lower) - 1;
    const int dLast = ceilDecade(range.upper) + 1;
    const int dMin = mSpec.boundDecades ? std::max(dFirst, mSpec.minDecade) : dFirst;
    const int dMax = mSpec.boundDecades ? std::min(dLast, mSpec.maxDecade) : dLast;

    for (int d = dMin; d <= dMax; ++d)
    {
        const double decadeBase = ipow10(d);
        for (double m : mSpec.majorMantissas)
        {
            const double v = m * decadeBase;
            if (std::isfinite(v))
                ticks.push_back(v);
        }
    }

    std::sort(ticks.begin(), ticks.end());
    ticks.erase(std::unique(ticks.begin(), ticks.end(),
                            [&](double a, double b) { return approxEqual(a, b, mSpec.relTol); }),
                ticks.end());
    trimTicks(range, ticks, keepOneOutlier);

    return ticks;
}

QVector<double> QCPAxisTickerFreq::createMinorTicks(const QVector<double> &ticks, const QCPRange &range) const
{
    QVector<double> subs;
    if (ticks.size() < 2)
        return subs;

    for (int i = 0; i + 1 < ticks.size(); ++i)
    {
        const double lower = ticks[i];
        const double upper = ticks[i + 1];

        double baseL, mL;
        normalizeMantissa(lower, baseL, mL);

        for (const auto &rule : mSpec.minorRules)
        {
            if (!approxEqual(mL, rule.fromMantissa, mSpec.relTol) ||
                !approxEqual(upper, rule.toMantissa * baseL, mSpec.relTol))
                continue;

            for (double m : rule.mantissas)
            {
                const double v = m * baseL;
                if (v >= range.lower && v <= range.upper)
                    subs.push_back(v);
            }
            break;
        }
    }

    std::sort(subs.begin(), subs.end());
    subs.erase(std::unique(subs.begin(), subs.end(),
                           [&](double a, double b) { return approxEqual(a, b, mSpec.relTol); }),
               subs.end());
    trimTicks(range, subs, false);

    return subs;
}

QVector<QString> QCPAxisTickerFreq::createLabels(const QVector<double> &ticks, const QLocale &locale,
                                                 QChar formatChar, int precision) const
{
    QVector<QString> labels;
    labels.reserve(ticks.size());

    for (double v : ticks)
        labels.push_back(formatFrequency(v, locale, formatChar, precision));

    return labels;
}

const QCPAxisTickerFreq::UnitFormat *QCPAxisTickerFreq::selectUnitFormat(double value) const
{
    const double absValue = std::abs(value);
    // units 在 normalizeSpec 中按阈值降序排列，优先选择最大匹配单位。
    for (const auto &unit : mSpec.labelFormat.units)
    {
        if (absValue >= unit.thresholdValue)
            return &unit;
    }
    return nullptr;
}

QString QCPAxisTickerFreq::formatNumber(double value, const QLocale &locale, QChar formatChar, int precision,
                                        bool roundNearInteger) const
{
    const double rounded = std::round(value);
    if (roundNearInteger && approxEqual(value, rounded, mSpec.relTol))
        return locale.toString(rounded, 'f', 0);

    char fmt = 'g';
    if (!formatChar.isNull() && formatChar.toLatin1() != '\0')
        fmt = formatChar.toLatin1();
    const int digits = precision >= 0 ? precision : 6;
    return locale.toString(value, fmt, digits);
}

QString QCPAxisTickerFreq::formatFrequency(double f, const QLocale &locale, QChar formatChar, int precision) const
{
    const QChar fmt = mSpec.labelFormat.formatChar.isNull() ? formatChar : mSpec.labelFormat.formatChar;
    const int digits = mSpec.labelFormat.precision >= 0 ? mSpec.labelFormat.precision : precision;

    if (mSpec.labelFormat.useUnits)
    {
        if (const UnitFormat *unit = selectUnitFormat(f))
        {
            const double scaledValue = f / unit->scaleDivisor;
            return formatNumber(scaledValue, locale, fmt, digits, mSpec.labelFormat.roundUnitValues) + unit->suffix;
        }
    }

    return formatNumber(f, locale, fmt, digits, mSpec.labelFormat.roundBaseValues);
}

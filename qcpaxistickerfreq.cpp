#include "qcpaxistickerfreq.h"
#include <QtMath>
#include <algorithm>

QCPAxisTickerFreq::QCPAxisTickerFreq()
{
    setTickStepStrategy(QCPAxisTicker::tssReadability);
}

void QCPAxisTickerFreq::setMantissasMain(const QVector<double> &mantissas)
{
    mMantissasMain = mantissas;
}
void QCPAxisTickerFreq::setSubMantissas_2to5(const QVector<double> &mantissas)
{
    mSubMantissas_2to5 = mantissas;
}
void QCPAxisTickerFreq::setSubMantissas_5to10(const QVector<double> &mantissas)
{
    mSubMantissas_5to10 = mantissas;
}
void QCPAxisTickerFreq::setRelTol(double relTol)
{
    mRelTol = relTol;
}
void QCPAxisTickerFreq::setFormatUseHzUnit(bool on)
{
    mUseHzUnit = on;
}
void QCPAxisTickerFreq::setKiloRounding(bool on)
{
    mKiloRound = on;
}
void QCPAxisTickerFreq::setDecadeBounds(double minDecade, double maxDecade)
{
    mBoundDecades = true;
    mMinDecade = minDecade;
    mMaxDecade = maxDecade;
}

bool QCPAxisTickerFreq::approxEqual(double a, double b, double relTol)
{
    if (a == b)
        return true;
    const double denom = qMax(qAbs(a), qAbs(b));
    if (denom == 0)
        return qAbs(a - b) < 1e-12;
    return qAbs(a - b) / denom <= relTol;
}

double QCPAxisTickerFreq::ipow10(int p)
{
    return qPow(10.0, p);
}

int QCPAxisTickerFreq::floorDecade(double v)
{
    if (v <= 0)
        return -9;
    return (int)qFloor(qLn(v) / qLn(10.0));
}

int QCPAxisTickerFreq::ceilDecade(double v)
{
    if (v <= 0)
        return -9;
    return (int)qCeil(qLn(v) / qLn(10.0));
}

void QCPAxisTickerFreq::normalizeMantissa(double v, int &d, double &base, double &m) const
{
    d = floorDecade(v);
    base = ipow10(d);
    m = v / base;

    if (approxEqual(m, 1.0, mRelTol))
    {
        d -= 1;
        base /= 10.0;
        m = 10.0;
    }
}

QString QCPAxisTickerFreq::formatHz(double f) const
{
    if (!mUseHzUnit)
        return QLocale().toString(f, 'g', 6);

    if (f >= 1000.0)
    {
        double k = f / 1000.0;
        if (mKiloRound)
            k = qRound(k);
        return QString::number(k, 'g', 6) + "k";
    }
    return QString::number(qRound(f));
}

double QCPAxisTickerFreq::getTickStep(const QCPRange &range)
{
    Q_UNUSED(range)
    return 1.0;
}

QVector<double> QCPAxisTickerFreq::createTickVector(double tickStep, const QCPRange &range)
{
    Q_UNUSED(tickStep)
    QVector<double> ticks;
    if (range.size() <= 0)
        return ticks;

    const int dFirst = floorDecade(range.lower);
    const int dLast = ceilDecade(range.upper);

    const int dMin = mBoundDecades ? qMax(dFirst, (int)mMinDecade) : dFirst;
    const int dMax = mBoundDecades ? qMin(dLast, (int)mMaxDecade) : dLast;

    for (int d = dMin; d <= dMax; ++d)
    {
        const double decadeBase = ipow10(d);
        for (double m : mMantissasMain)
        {
            const double v = m * decadeBase;
            if (v >= range.lower && v <= range.upper)
                ticks.push_back(v);
        }
    }
    std::sort(ticks.begin(), ticks.end());
    ticks.erase(std::unique(ticks.begin(), ticks.end(), [&](double a, double b) { return approxEqual(a, b, mRelTol); }),
                ticks.end());

    return ticks;
}

QVector<double> QCPAxisTickerFreq::createSubTickVector(int subTickCount, const QVector<double> &ticks)
{
    Q_UNUSED(subTickCount)
    QVector<double> subs;
    if (ticks.size() < 2)
        return subs;

    for (int i = 0; i + 1 < ticks.size(); ++i)
    {
        const double lower = ticks[i];
        const double upper = ticks[i + 1];

        int dL;
        double baseL, mL;
        normalizeMantissa(lower, dL, baseL, mL);

        if (approxEqual(mL, 2.0, mRelTol) && approxEqual(upper, 5.0 * baseL, mRelTol))
        {
            for (double m : mSubMantissas_2to5)
                subs.push_back(m * baseL);
        }
        else if (approxEqual(mL, 5.0, mRelTol))
        {
            const double upperAs10 = 10.0 * baseL;
            if (approxEqual(upper, upperAs10, mRelTol))
            {
                for (double m : mSubMantissas_5to10)
                    subs.push_back(m * baseL);
            }
        }
    }
    return subs;
}

QVector<QString> QCPAxisTickerFreq::createLabelVector(const QVector<double> &ticks, const QLocale &locale,
                                                      QChar formatChar, int precision)
{
    Q_UNUSED(locale)
    Q_UNUSED(formatChar)
    Q_UNUSED(precision)

    QVector<QString> labels;
    labels.reserve(ticks.size());

    for (double v : ticks)
    {
        int d;
        double base, m;
        normalizeMantissa(v, d, base, m);

        QString label;
        for (double mm : mMantissasMain)
        {
            if (approxEqual(m, mm, mRelTol))
            {
                label = formatHz(v);
                break;
            }
        }
        labels.push_back(label);
    }
    return labels;
}
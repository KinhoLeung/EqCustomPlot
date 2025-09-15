#pragma once
#include "qcustomplot.h"

class QCPAxisTickerFreq : public QCPAxisTicker
{
  public:
    QCPAxisTickerFreq();

    // 可配置项
    void setMantissasMain(const QVector<double> &mantissas);      // 默认 {2,5,10}
    void setSubMantissas_2to5(const QVector<double> &mantissas);  // 默认 {3,4}
    void setSubMantissas_5to10(const QVector<double> &mantissas); // 默认 {6,7,8,9}
    void setRelTol(double relTol);                                // 默认 1e-3
    void setFormatUseHzUnit(bool on);                             // 默认 true
    void setKiloRounding(bool on);                                // 默认 true（1k/2k/5k 取整）
    void setDecadeBounds(double minDecade, double maxDecade);     // 可选限制 decade 范围

  protected:
    // 不等距刻度下 getTickStep 无实际意义，仅返回正数
    double getTickStep(const QCPRange &range) override;
    QVector<double> createTickVector(double tickStep, const QCPRange &range) override;
    QVector<double> createSubTickVector(int subTickCount, const QVector<double> &ticks) override;
    QVector<QString> createLabelVector(const QVector<double> &ticks, const QLocale &locale, QChar formatChar,
                                       int precision) override;

  private:
    // 工具函数
    static bool approxEqual(double a, double b, double relTol);
    static double ipow10(int p); // 10^p
    static int floorDecade(double v);
    static int ceilDecade(double v);

    // 关键修复：将 m≈1 的点（如 100, 10000）归一到上一 decade 的 m=10
    void normalizeMantissa(double v, int &d, double &base, double &m) const;

    QString formatHz(double f) const;

  private:
    QVector<double> mMantissasMain{2.0, 5.0, 10.0};
    QVector<double> mSubMantissas_2to5{3.0, 4.0};
    QVector<double> mSubMantissas_5to10{6.0, 7.0, 8.0, 9.0};
    double mRelTol = 1e-3;

    bool mUseHzUnit = true;
    bool mKiloRound = true;

    bool mBoundDecades = false;
    double mMinDecade = 0; // 10^0
    double mMaxDecade = 6; // 10^6
};
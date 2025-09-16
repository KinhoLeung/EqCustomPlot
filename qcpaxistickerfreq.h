#pragma once
#include "qcustomplot.h"

class QCPAxisTickerFreq : public QCPAxisTicker
{
  public:
    QCPAxisTickerFreq();

    void setMantissasMain(const QVector<double> &mantissas);      
    void setSubMantissas_2to5(const QVector<double> &mantissas);  
    void setSubMantissas_5to10(const QVector<double> &mantissas); 
    void setRelTol(double relTol);                                
    void setFormatUseHzUnit(bool on);                             
    void setKiloRounding(bool on);                                
    void setDecadeBounds(double minDecade, double maxDecade);     

  protected:
    double getTickStep(const QCPRange &range) override;
    QVector<double> createTickVector(double tickStep, const QCPRange &range) override;
    QVector<double> createSubTickVector(int subTickCount, const QVector<double> &ticks) override;
    QVector<QString> createLabelVector(const QVector<double> &ticks, const QLocale &locale, QChar formatChar,
                                       int precision) override;

  private:
    static bool approxEqual(double a, double b, double relTol);
    static double ipow10(int p);
    static int floorDecade(double v);
    static int ceilDecade(double v);

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
    double mMinDecade = 0;
    double mMaxDecade = 6;
};
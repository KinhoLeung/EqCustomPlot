#pragma once
#include "eq.h"
#include "qcustomplot.h"

class QCPAxisTickerFreq;

class EqCustomPlot : public QCustomPlot
{
    Q_OBJECT
  public:
    explicit EqCustomPlot(QWidget *parent = nullptr);
    ~EqCustomPlot();

  private:
    enum EqIndex
    {
        Invalid = -1,
        EqHp,
        Eq1,
        Eq2,
        Eq3,
        Eq4,
        Eq5,
        Eq6,
        Eq7,
        Eq8,
        Eq9,
        EqLp,
        EqCount
    };

    const int MIN_XAXIS_VALUE = 16;
    const int MAX_XAXIS_VALUE = 25000;
    const double MIN_YAXIS_VALUE = -18.0;
    const double MAX_YAXIS_VALUE = 18.0;
    const int FREQ_COUNT = 1024;
    const int SAMPLE_RATE = 384000;
    const int MIN_FREQ = 20;
    const int MAX_FREQ = 20000;
    const double MIN_Q = 0.1;
    const double MAX_Q = 30.0;
    const double MIN_GAIN = -18.0;
    const double MAX_GAIN = 18.0;
    const double Q_SINGLE_STEP = 0.1;
    const double DEFAULT_GAIN = 0.0;
    const double DEFAULT_Q = 1.0;
    const double DEFAULT_HIGHPASS_Q = 0.7;
    const double DEFAULT_LOWPASS_Q = 0.7;
    const bool DEFAULT_BYPASS = false;

    QCPItemText *mEqParamText;

    QVector<QIcon> mSelectedIcon;
    QVector<QIcon> mUnselectedIcon;

    Qt::MouseButton mMouseButton;

    QVector<QCPGraph *> mScatter;
    QVector<double> mFreqData;
    QVector<eq::Coeff> mOverallCoeff;

    QCPGraph *mSelectedScatter;
    QCPGraph *mOverallCurve;
    QCPGraph *mSoloScattrer;

    QMap<QCPGraph *, QCPGraph *> mScatterCurve;
    QMap<QCPGraph *, QCPScatterStyle> mSelectedStyle;
    QMap<QCPGraph *, QCPScatterStyle> mUnselectedStyle;

    QMenu *mMenu;
    QMenu *mFilterTypeMenu;
    QAction *mFilterTypeAction;
    QAction *mBypassAction;
    QAction *mSoloAction;
    QActionGroup *mFilterActionGroup;

    void drawEqCurve(QCPGraph *scatter, bool solo = false);
    void updateEqParamText(QCPGraph *scatter);

  protected:
    void resizeEvent(QResizeEvent *event) override;
};
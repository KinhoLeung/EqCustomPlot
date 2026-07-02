#pragma once

#include "eq.h"

#include <QColor>
#include <QElapsedTimer>
#include <QIcon>
#include <QPainterPath>
#include <QPixmap>
#include <QRectF>
#include <QVector>
#include <QWidget>
#include <array>

class EqCustomPlot : public QWidget
{
    Q_OBJECT

  public:
    enum BandIndex
    {
        InvalidBand = -1,
        EqHp = 0,
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
        BandCount
    };

    struct Band
    {
        int frequencyHz = 1000;
        double gainDb = 0.0;
        double q = 1.0;
        eq::FilterType type = eq::FilterType::Peak;
        bool bypass = false;
    };

    explicit EqCustomPlot(QWidget *parent = nullptr);
    ~EqCustomPlot() override;

    Band band(int index) const;
    QVector<Band> bands() const;
    int selectedBand() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

  public slots:
    void setBand(int index, const Band &band);
    void setBands(const QVector<Band> &bands);
    void resetBand(int index);
    void resetAll();
    void setSelectedBand(int index);

  signals:
    void bandChanged(int index, EqCustomPlot::Band band);
    void bandsChanged();
    void selectedBandChanged(int index);
    void editStarted(int index);
    void editFinished(int index);

  protected:
    bool event(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

  private:
    enum class DragTarget
    {
        None,
        BandHandle,
        FrequencySlider,
        GainSlider,
        QSlider
    };

    struct BandState
    {
        Band value;
        Band defaults;
        eq::Coeff coeff;
        QVector<double> responseDb;
        QPixmap selectedIcon;
        QPixmap unselectedIcon;
        bool dirty = true;
    };

    struct SliderGeometry
    {
        QRectF bounds;
        QRectF groove;
        QRectF handle;
        QString label;
        QString value;
        bool enabled = true;
    };

    static constexpr int kFinePointCount = 1024;
    static constexpr int kLivePointCount = 512;
    static constexpr int kSampleRate = 384000;
    static constexpr int kMinAxisFrequency = 16;
    static constexpr int kMaxAxisFrequency = 25000;
    static constexpr int kMinBandFrequency = 20;
    static constexpr int kMaxBandFrequency = 20000;
    static constexpr double kMinGainDb = -18.0;
    static constexpr double kMaxGainDb = 18.0;
    static constexpr double kMinQ = 0.1;
    static constexpr double kMaxQ = 30.0;
    static constexpr double kDefaultQ = 1.0;
    static constexpr double kDefaultPassQ = 0.7;

    void initializeBands();
    void initializeFrequencyData();
    void refreshTheme();
    void rebuildIconCache();
    void rebuildLayout();
    void invalidateBackground();
    void invalidateResponses(bool allBands);
    void ensureResponses();
    void rebuildPaths();
    void requestFrame(bool finalFrame = false);

    void drawBackground(QPainter &painter);
    void drawCurves(QPainter &painter);
    void drawHandles(QPainter &painter);
    void drawPanel(QPainter &painter);
    void drawSlider(QPainter &painter, const SliderGeometry &slider, double t);
    void drawButton(QPainter &painter, const QRectF &rect, const QString &text, bool checked, bool enabled);

    bool beginInteraction(const QPointF &pos);
    void updateInteraction(const QPointF &pos);
    void finishInteraction();
    bool handlePanelPress(const QPointF &pos);
    void updateSliderFromPosition(DragTarget slider, const QPointF &pos);
    int hitBandHandle(const QPointF &pos) const;
    QPointF bandPoint(int index) const;

    Band normalizedBand(int index, Band band) const;
    bool isValidBand(int index) const;
    bool isPassBand(int index) const;
    bool isEditableType(eq::FilterType type) const;
    QVector<eq::FilterType> editableTypes() const;
    QString bandLabel(int index) const;
    QString typeLabel(eq::FilterType type) const;

    double xForFrequency(double frequencyHz) const;
    double yForGain(double gainDb) const;
    int frequencyForX(double x) const;
    double gainForY(double y) const;
    double sliderTForFrequency(int frequencyHz) const;
    int frequencyForSliderT(double t) const;
    double sliderTForQ(double q) const;
    double qForSliderT(double t) const;
    double clampDisplayDb(double value) const;

    const QVector<double> &activeFrequencies() const;
    const eq::FrequencyGrid &activeGrid() const;

    std::array<BandState, BandCount> mBandStates;
    QVector<double> mFineFrequencies;
    QVector<double> mLiveFrequencies;
    QVector<double> mOverallDb;
    eq::FrequencyGrid mFineGrid;
    eq::FrequencyGrid mLiveGrid;

    QRectF mPlotRect;
    QRectF mPanelRect;
    SliderGeometry mFrequencySlider;
    SliderGeometry mGainSlider;
    SliderGeometry mQSlider;
    QRectF mBypassButtonRect;
    QRectF mSoloButtonRect;
    QRectF mResetButtonRect;
    QRectF mTypeButtonRect;
    QVector<QRectF> mTypeChipRects;
    QVector<eq::FilterType> mTypeChipTypes;

    QPixmap mBackgroundCache;
    QSize mBackgroundCacheSize;
    QPainterPath mOverallPath;
    QPainterPath mSelectedPath;

    QColor mBackgroundColor;
    QColor mPanelColor;
    QColor mTextColor;
    QColor mMutedTextColor;
    QColor mGridColor;
    QColor mSubGridColor;
    QColor mAccentColor;
    QColor mAccentFillColor;
    QColor mDisabledColor;

    int mSelectedIndex = InvalidBand;
    int mSoloIndex = InvalidBand;
    DragTarget mDragTarget = DragTarget::None;
    bool mUsingLiveGrid = false;
    bool mEditing = false;
    bool mBackgroundDirty = true;
    bool mPathDirty = true;
    int mHandleSize = 34;
    QElapsedTimer mFrameTimer;
};

Q_DECLARE_METATYPE(EqCustomPlot::Band)

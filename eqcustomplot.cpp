#include "eqcustomplot.h"

#include <QApplication>
#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPalette>
#include <QResizeEvent>
#include <QStyleHints>
#include <QTouchEvent>
#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
constexpr int kLocalMinAxisFrequency = 16;
constexpr int kLocalMaxAxisFrequency = 25000;
constexpr int kLocalMinBandFrequency = 20;
constexpr int kLocalMaxBandFrequency = 20000;
const double kAxisMinLog = std::log10(kLocalMinAxisFrequency);
const double kAxisMaxLog = std::log10(kLocalMaxAxisFrequency);
const double kBandMinLog = std::log10(kLocalMinBandFrequency);
const double kBandMaxLog = std::log10(kLocalMaxBandFrequency);
constexpr int kFrameIntervalMs = 16;

double clamped(double value, double lower, double upper)
{
    return std::min(std::max(value, lower), upper);
}

bool fuzzySame(double a, double b)
{
    return std::abs(a - b) <= 1e-9;
}

bool sameBand(const EqCustomPlot::Band &lhs, const EqCustomPlot::Band &rhs)
{
    return lhs.frequencyHz == rhs.frequencyHz && fuzzySame(lhs.gainDb, rhs.gainDb) && fuzzySame(lhs.q, rhs.q) &&
           lhs.type == rhs.type && lhs.bypass == rhs.bypass;
}

QVector<double> makeLogFrequencies(int count)
{
    QVector<double> result;
    result.resize(count);

    const double step = (kAxisMaxLog - kAxisMinLog) / static_cast<double>(count - 1);
    for (int i = 0; i < count; ++i)
        result[i] = kLocalMinAxisFrequency * std::pow(10.0, step * i);

    return result;
}

QString formatFrequency(int frequencyHz)
{
    if (frequencyHz >= 1000)
    {
        const double khz = frequencyHz / 1000.0;
        return QStringLiteral("%1k").arg(khz, 0, khz >= 10.0 || std::fmod(khz, 1.0) == 0.0 ? 'f' : 'f',
                                        khz >= 10.0 || std::fmod(khz, 1.0) == 0.0 ? 0 : 1);
    }
    return QStringLiteral("%1").arg(frequencyHz);
}

QPointF mouseEventPosition(const QMouseEvent *event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return event->position();
#else
    return event->localPos();
#endif
}

bool firstTouchPosition(const QTouchEvent *event, QPointF &position)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (event->points().isEmpty())
        return false;
    position = event->points().first().position();
#else
    if (event->touchPoints().isEmpty())
        return false;
    position = event->touchPoints().first().pos();
#endif
    return true;
}

bool isDarkPalette(const QPalette &palette)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    return QApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
#else
    return palette.color(QPalette::Window).lightness() < 128;
#endif
}

QColor accentColor(const QPalette &palette)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    return palette.color(QPalette::Accent);
#else
    return palette.color(QPalette::Highlight);
#endif
}
} // namespace

EqCustomPlot::EqCustomPlot(QWidget *parent) : QWidget(parent)
{
    qRegisterMetaType<EqCustomPlot::Band>("EqCustomPlot::Band");

    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_AcceptTouchEvents);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    initializeBands();
    initializeFrequencyData();
    refreshTheme();
    rebuildLayout();
}

EqCustomPlot::~EqCustomPlot() = default;

EqCustomPlot::Band EqCustomPlot::band(int index) const
{
    if (!isValidBand(index))
        return Band();
    return mBandStates[index].value;
}

QVector<EqCustomPlot::Band> EqCustomPlot::bands() const
{
    QVector<Band> result;
    result.reserve(BandCount);
    for (const BandState &state : mBandStates)
        result.push_back(state.value);
    return result;
}

int EqCustomPlot::selectedBand() const
{
    return mSelectedIndex;
}

QSize EqCustomPlot::sizeHint() const
{
    return QSize(900, 560);
}

QSize EqCustomPlot::minimumSizeHint() const
{
    return QSize(480, 340);
}

void EqCustomPlot::setBand(int index, const Band &band)
{
    if (!isValidBand(index))
        return;

    const Band normalized = normalizedBand(index, band);
    BandState &state = mBandStates[index];
    if (sameBand(state.value, normalized))
        return;

    state.value = normalized;
    state.dirty = true;
    mPathDirty = true;

    emit bandChanged(index, state.value);
    emit bandsChanged();
    requestFrame();
}

void EqCustomPlot::setBands(const QVector<Band> &bands)
{
    bool anyChanged = false;
    const int count = std::min(static_cast<int>(BandCount), static_cast<int>(bands.size()));

    for (int i = 0; i < count; ++i)
    {
        const Band normalized = normalizedBand(i, bands.at(i));
        BandState &state = mBandStates[i];
        if (sameBand(state.value, normalized))
            continue;

        state.value = normalized;
        state.dirty = true;
        anyChanged = true;
        emit bandChanged(i, state.value);
    }

    if (!anyChanged)
        return;

    mPathDirty = true;
    emit bandsChanged();
    requestFrame(true);
}

void EqCustomPlot::resetBand(int index)
{
    if (!isValidBand(index))
        return;
    setBand(index, mBandStates[index].defaults);
}

void EqCustomPlot::resetAll()
{
    bool anyChanged = false;

    for (int i = 0; i < BandCount; ++i)
    {
        BandState &state = mBandStates[i];
        if (sameBand(state.value, state.defaults))
            continue;

        state.value = state.defaults;
        state.dirty = true;
        anyChanged = true;
        emit bandChanged(i, state.value);
    }

    if (!anyChanged)
        return;

    mSoloIndex = InvalidBand;
    mPathDirty = true;
    emit bandsChanged();
    requestFrame(true);
}

void EqCustomPlot::setSelectedBand(int index)
{
    const int normalizedIndex = isValidBand(index) ? index : InvalidBand;
    if (mSelectedIndex == normalizedIndex)
        return;

    mSelectedIndex = normalizedIndex;
    mPathDirty = true;
    emit selectedBandChanged(mSelectedIndex);
    requestFrame();
}

bool EqCustomPlot::event(QEvent *event)
{
    switch (event->type())
    {
    case QEvent::ApplicationPaletteChange:
    case QEvent::PaletteChange:
    case QEvent::StyleChange:
        refreshTheme();
        return QWidget::event(event);
    case QEvent::TouchBegin:
    {
        auto *touch = static_cast<QTouchEvent *>(event);
        QPointF position;
        if (firstTouchPosition(touch, position))
        {
            event->accept();
            beginInteraction(position);
            return true;
        }
        break;
    }
    case QEvent::TouchUpdate:
    {
        auto *touch = static_cast<QTouchEvent *>(event);
        QPointF position;
        if (firstTouchPosition(touch, position))
        {
            event->accept();
            updateInteraction(position);
            return true;
        }
        break;
    }
    case QEvent::TouchEnd:
    case QEvent::TouchCancel:
        event->accept();
        finishInteraction();
        return true;
    default:
        break;
    }

    return QWidget::event(event);
}

void EqCustomPlot::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    ensureResponses();
    if (mPathDirty)
        rebuildPaths();

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    drawBackground(painter);
    drawCurves(painter);
    drawHandles(painter);
    drawPanel(painter);
}

void EqCustomPlot::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    rebuildLayout();
    invalidateBackground();
    mPathDirty = true;
    requestFrame(true);
}

void EqCustomPlot::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
    {
        QWidget::mousePressEvent(event);
        return;
    }

    if (!beginInteraction(mouseEventPosition(event)))
        setSelectedBand(InvalidBand);

    event->accept();
}

void EqCustomPlot::mouseMoveEvent(QMouseEvent *event)
{
    updateInteraction(mouseEventPosition(event));
    event->accept();
}

void EqCustomPlot::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        finishInteraction();
        event->accept();
        return;
    }

    QWidget::mouseReleaseEvent(event);
}

void EqCustomPlot::mouseDoubleClickEvent(QMouseEvent *event)
{
    const int hit = hitBandHandle(mouseEventPosition(event));
    if (isValidBand(hit))
    {
        setSelectedBand(hit);
        resetBand(hit);
        event->accept();
        return;
    }

    QWidget::mouseDoubleClickEvent(event);
}

void EqCustomPlot::initializeBands()
{
    const double step = (kBandMaxLog - kBandMinLog) / static_cast<double>(BandCount - 1);

    for (int i = 0; i < BandCount; ++i)
    {
        Band band;
        band.frequencyHz = static_cast<int>(std::lround(kMinBandFrequency * std::pow(10.0, step * i)));
        band.gainDb = 0.0;
        band.q = kDefaultQ;
        band.type = eq::FilterType::Peak;
        band.bypass = false;

        if (i == EqHp)
        {
            band.frequencyHz = kMinBandFrequency;
            band.q = kDefaultPassQ;
            band.type = eq::FilterType::Highpass;
        }
        else if (i == EqLp)
        {
            band.frequencyHz = kMaxBandFrequency;
            band.q = kDefaultPassQ;
            band.type = eq::FilterType::Lowpass;
        }

        band = normalizedBand(i, band);
        mBandStates[i].value = band;
        mBandStates[i].defaults = band;
        mBandStates[i].dirty = true;
    }

    mSelectedIndex = Eq1;
}

void EqCustomPlot::initializeFrequencyData()
{
    mFineFrequencies = makeLogFrequencies(kFinePointCount);
    mLiveFrequencies = makeLogFrequencies(kLivePointCount);
    mFineGrid = eq::makeFrequencyGrid(kSampleRate, mFineFrequencies);
    mLiveGrid = eq::makeFrequencyGrid(kSampleRate, mLiveFrequencies);
}

void EqCustomPlot::refreshTheme()
{
    const QPalette pal = palette();
    const bool dark = isDarkPalette(pal);

    mBackgroundColor = dark ? QColor(9, 11, 13) : QColor(250, 251, 252);
    mPanelColor = dark ? QColor(24, 27, 31) : QColor(238, 241, 245);
    mTextColor = pal.color(QPalette::WindowText);
    mMutedTextColor = mTextColor;
    mMutedTextColor.setAlpha(150);
    mGridColor = mTextColor;
    mGridColor.setAlpha(dark ? 54 : 46);
    mSubGridColor = mTextColor;
    mSubGridColor.setAlpha(dark ? 26 : 22);
    mAccentColor = accentColor(pal);
    mAccentFillColor = mAccentColor;
    mAccentFillColor.setAlpha(45);
    mDisabledColor = mTextColor;
    mDisabledColor.setAlpha(70);

    invalidateBackground();
    requestFrame(true);
}

void EqCustomPlot::rebuildLayout()
{
    const qreal w = width();
    const qreal h = height();
    if (w <= 0 || h <= 0)
        return;

    const qreal panelHeight = clamped(h * 0.33, 170.0, 214.0);
    mPanelRect = QRectF(0.0, h - panelHeight, w, panelHeight);

    const qreal left = clamped(w * 0.07, 50.0, 68.0);
    const qreal right = 18.0;
    const qreal top = 18.0;
    const qreal bottomGap = 14.0;
    mPlotRect = QRectF(left, top, std::max(80.0, w - left - right),
                       std::max(90.0, mPanelRect.top() - top - bottomGap));

    mHandleSize = static_cast<int>(clamped(std::min(w, h) / 15.0, 30.0, 48.0));

    const qreal padding = 12.0;
    const qreal sliderHeight = 30.0;
    const qreal rowGap = 8.0;
    const qreal labelWidth = 86.0;
    const qreal valueWidth = 92.0;
    const qreal panelWidth = std::max(0.0, mPanelRect.width() - padding * 2.0);
    qreal y = mPanelRect.top() + padding;

    auto setupSlider = [&](SliderGeometry &slider, const QString &label) {
        slider.bounds = QRectF(padding, y, panelWidth, sliderHeight);
        slider.label = label;
        const qreal grooveLeft = slider.bounds.left() + labelWidth;
        const qreal grooveRight = slider.bounds.right() - valueWidth;
        slider.groove = QRectF(grooveLeft, slider.bounds.center().y() - 5.0, std::max(40.0, grooveRight - grooveLeft),
                               10.0);
        slider.handle = QRectF();
        y += sliderHeight + rowGap;
    };

    setupSlider(mFrequencySlider, QStringLiteral("Freq"));
    setupSlider(mGainSlider, QStringLiteral("Gain"));
    setupSlider(mQSlider, QStringLiteral("Q"));

    const qreal buttonHeight = 38.0;
    const qreal buttonGap = 8.0;
    const qreal smallButtonWidth = clamped((panelWidth - buttonGap * 5.0) * 0.14, 76.0, 102.0);
    qreal x = padding;
    mBypassButtonRect = QRectF(x, y, smallButtonWidth, buttonHeight);
    x += smallButtonWidth + buttonGap;
    mSoloButtonRect = QRectF(x, y, smallButtonWidth, buttonHeight);
    x += smallButtonWidth + buttonGap;
    mResetButtonRect = QRectF(x, y, smallButtonWidth, buttonHeight);
    x += smallButtonWidth + buttonGap;

    const QRectF typeArea(x, y, std::max(0.0, mPanelRect.right() - padding - x), buttonHeight);
    mTypeChipRects.clear();
    mTypeChipTypes.clear();
    mTypeButtonRect = QRectF();

    const QVector<eq::FilterType> types = editableTypes();
    const qreal chipGap = 6.0;
    const qreal chipWidth = types.isEmpty() ? 0.0 : (typeArea.width() - chipGap * (types.size() - 1)) / types.size();
    if (chipWidth >= 66.0)
    {
        qreal chipX = typeArea.left();
        for (eq::FilterType type : types)
        {
            mTypeChipRects.push_back(QRectF(chipX, typeArea.top(), chipWidth, typeArea.height()));
            mTypeChipTypes.push_back(type);
            chipX += chipWidth + chipGap;
        }
    }
    else
    {
        mTypeButtonRect = typeArea;
    }
}

void EqCustomPlot::invalidateBackground()
{
    mBackgroundDirty = true;
    mBackgroundCache = QPixmap();
}

void EqCustomPlot::invalidateResponses(bool allBands)
{
    if (allBands)
    {
        for (BandState &state : mBandStates)
            state.dirty = true;
    }

    mPathDirty = true;
}

void EqCustomPlot::ensureResponses()
{
    const eq::FrequencyGrid &grid = activeGrid();
    const int pointCount = grid.size();
    if (pointCount <= 0)
        return;

    if (mOverallDb.size() != pointCount)
        mOverallDb.resize(pointCount);
    std::fill(mOverallDb.begin(), mOverallDb.end(), 0.0);

    for (int i = 0; i < BandCount; ++i)
    {
        BandState &state = mBandStates[i];
        if (state.dirty || state.responseDb.size() != pointCount)
        {
            const Band &band = state.value;
            state.coeff = eq::getSectionsMatrix(band.gainDb, band.frequencyHz, band.q, band.type, band.bypass,
                                                kSampleRate);
            eq::getFreqznFast(state.coeff, grid, state.responseDb);
            state.dirty = false;
        }

        if (mSoloIndex != InvalidBand && mSoloIndex != i)
            continue;

        for (int j = 0; j < pointCount; ++j)
            mOverallDb[j] += state.responseDb.at(j);
    }
}

void EqCustomPlot::rebuildPaths()
{
    const QVector<double> &frequencies = activeFrequencies();
    const int pointCount = std::min(frequencies.size(), mOverallDb.size());

    auto buildPath = [&](const QVector<double> &values) {
        QPainterPath path;
        const int count = std::min(frequencies.size(), values.size());
        for (int i = 0; i < count; ++i)
        {
            const QPointF point(xForFrequency(frequencies.at(i)), yForGain(clampDisplayDb(values.at(i))));
            if (i == 0)
                path.moveTo(point);
            else
                path.lineTo(point);
        }
        return path;
    };

    auto buildFillPath = [&](const QVector<double> &values) {
        QPainterPath path;
        const int count = std::min(frequencies.size(), values.size());
        if (count <= 0)
            return path;

        const QPointF firstPoint(xForFrequency(frequencies.at(0)), yForGain(clampDisplayDb(values.at(0))));
        path.moveTo(firstPoint);

        QPointF lastPoint = firstPoint;
        for (int i = 1; i < count; ++i)
        {
            lastPoint = QPointF(xForFrequency(frequencies.at(i)), yForGain(clampDisplayDb(values.at(i))));
            path.lineTo(lastPoint);
        }

        path.lineTo(lastPoint.x(), mPlotRect.bottom());
        path.lineTo(firstPoint.x(), mPlotRect.bottom());
        path.closeSubpath();
        return path;
    };

    mOverallPath = pointCount > 0 ? buildPath(mOverallDb) : QPainterPath();
    if (isValidBand(mSelectedIndex))
    {
        const QVector<double> &selectedResponse = mBandStates[mSelectedIndex].responseDb;
        mSelectedPath = buildPath(selectedResponse);
        mSelectedFillPath = buildFillPath(selectedResponse);
    }
    else
    {
        mSelectedPath = QPainterPath();
        mSelectedFillPath = QPainterPath();
    }
    mPathDirty = false;
}

void EqCustomPlot::requestFrame(bool finalFrame)
{
    if (finalFrame)
    {
        update();
        mFrameTimer.restart();
        return;
    }

    if (!mFrameTimer.isValid() || mFrameTimer.elapsed() >= kFrameIntervalMs)
    {
        update();
        mFrameTimer.restart();
    }
}

void EqCustomPlot::drawBackground(QPainter &painter)
{
    const QSize cacheSize = size();
    const qreal dpr = devicePixelRatioF();
    if (mBackgroundDirty || mBackgroundCacheSize != cacheSize || mBackgroundCache.isNull())
    {
        mBackgroundCacheSize = cacheSize;
        mBackgroundCache = QPixmap(cacheSize * dpr);
        mBackgroundCache.setDevicePixelRatio(dpr);
        mBackgroundCache.fill(mBackgroundColor);

        QPainter cachePainter(&mBackgroundCache);
        cachePainter.setRenderHint(QPainter::Antialiasing, false);
        cachePainter.fillRect(rect(), mBackgroundColor);
        cachePainter.fillRect(mPanelRect, mPanelColor);

        cachePainter.setPen(QPen(mSubGridColor, 1.0));
        for (int decade = 1; decade <= 4; ++decade)
        {
            const double base = std::pow(10.0, decade);
            for (int m = 2; m < 10; ++m)
            {
                const double f = base * m;
                if (f < kMinAxisFrequency || f > kMaxAxisFrequency)
                    continue;
                const qreal x = xForFrequency(f);
                cachePainter.drawLine(QPointF(x, mPlotRect.top()), QPointF(x, mPlotRect.bottom()));
            }
        }

        cachePainter.setPen(QPen(mGridColor, 1.0));
        const QVector<int> majorFrequencies = {20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000};
        for (int frequency : majorFrequencies)
        {
            const qreal x = xForFrequency(frequency);
            cachePainter.drawLine(QPointF(x, mPlotRect.top()), QPointF(x, mPlotRect.bottom()));
        }

        for (double gain = kMinGainDb; gain <= kMaxGainDb + 0.1; gain += 6.0)
        {
            const qreal y = yForGain(gain);
            cachePainter.drawLine(QPointF(mPlotRect.left(), y), QPointF(mPlotRect.right(), y));
        }

        cachePainter.setPen(QPen(mTextColor, 1.0));
        cachePainter.drawRect(mPlotRect);

        QFont labelFont = font();
        labelFont.setPointSizeF(std::max(8.0, labelFont.pointSizeF() - 1.0));
        cachePainter.setFont(labelFont);
        cachePainter.setPen(mMutedTextColor);
        for (int frequency : majorFrequencies)
        {
            const qreal x = xForFrequency(frequency);
            const QString label = formatFrequency(frequency);
            cachePainter.drawText(QRectF(x - 30.0, mPlotRect.bottom() + 4.0, 60.0, 18.0), Qt::AlignHCenter,
                                  label);
        }

        for (double gain = kMinGainDb; gain <= kMaxGainDb + 0.1; gain += 6.0)
        {
            const qreal y = yForGain(gain);
            cachePainter.drawText(QRectF(4.0, y - 9.0, mPlotRect.left() - 10.0, 18.0), Qt::AlignRight | Qt::AlignVCenter,
                                  QStringLiteral("%1").arg(gain, 0, 'f', 0));
        }

        mBackgroundDirty = false;
    }

    painter.drawPixmap(0, 0, mBackgroundCache);
}

void EqCustomPlot::drawCurves(QPainter &painter)
{
    painter.save();
    painter.setClipRect(mPlotRect.adjusted(1.0, 1.0, -1.0, -1.0));

    if (isValidBand(mSelectedIndex) && !mSelectedFillPath.isEmpty())
    {
        QLinearGradient fillGradient(mPlotRect.topLeft(), mPlotRect.bottomLeft());
        QColor top = mAccentColor;
        QColor bottom = mAccentColor;
        top.setAlpha(58);
        bottom.setAlpha(8);
        fillGradient.setColorAt(0.0, top);
        fillGradient.setColorAt(1.0, bottom);

        painter.setPen(Qt::NoPen);
        painter.setBrush(fillGradient);
        painter.drawPath(mSelectedFillPath);
    }

    if (isValidBand(mSelectedIndex) && !mSelectedPath.isEmpty())
    {
        QColor selectedCurve = mAccentColor;
        selectedCurve.setAlpha(95);
        painter.setPen(QPen(selectedCurve, 1.4));
        painter.drawPath(mSelectedPath);
    }

    painter.setPen(QPen(mAccentColor, 2.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(mOverallPath);
    painter.restore();
}

void EqCustomPlot::drawHandles(QPainter &painter)
{
    painter.save();
    painter.setClipRect(mPlotRect.adjusted(-mHandleSize, -mHandleSize, mHandleSize, mHandleSize));

    for (int i = 0; i < BandCount; ++i)
    {
        const bool selected = i == mSelectedIndex;
        const bool mutedBySolo = mSoloIndex != InvalidBand && mSoloIndex != i;
        const bool bypassed = mBandStates[i].value.bypass;
        const QPointF center = bandPoint(i);
        const qreal size = selected ? mHandleSize * 1.12 : mHandleSize;
        const QRectF target(center.x() - size / 2.0, center.y() - size / 2.0, size, size);

        painter.save();
        painter.setOpacity(mutedBySolo ? 0.32 : (bypassed ? 0.45 : 1.0));

        QColor fill = selected ? mAccentColor : mPanelColor;
        QColor border = selected ? mAccentColor : mTextColor;
        QColor labelColor = selected ? mBackgroundColor : mTextColor;

        if (bypassed)
        {
            fill = mPanelColor;
            border = mDisabledColor;
            labelColor = mDisabledColor;
        }

        if (selected)
        {
            QColor halo = mAccentColor;
            halo.setAlpha(55);
            painter.setPen(Qt::NoPen);
            painter.setBrush(halo);
            painter.drawEllipse(target.adjusted(-4.0, -4.0, 4.0, 4.0));
        }

        painter.setBrush(fill);
        painter.setPen(QPen(border, selected ? 2.0 : 1.4));
        painter.drawEllipse(target);

        QFont labelFont = painter.font();
        labelFont.setBold(selected);
        labelFont.setPointSizeF(std::max(8.0, target.height() * 0.34));
        painter.setFont(labelFont);
        painter.setPen(labelColor);
        painter.drawText(target, Qt::AlignCenter, bandLabel(i));

        painter.restore();
    }

    painter.restore();
}

void EqCustomPlot::drawPanel(QPainter &painter)
{
    painter.save();

    const bool hasSelection = isValidBand(mSelectedIndex);
    const Band current = hasSelection ? mBandStates[mSelectedIndex].value : Band();
    const bool gainEnabled = hasSelection && !isPassBand(mSelectedIndex);
    const bool typeEnabled = hasSelection && !isPassBand(mSelectedIndex);

    mFrequencySlider.enabled = hasSelection;
    mFrequencySlider.value = hasSelection ? QStringLiteral("%1 Hz").arg(current.frequencyHz) : QStringLiteral("--");
    mGainSlider.enabled = gainEnabled;
    mGainSlider.value = hasSelection ? QStringLiteral("%1 dB").arg(current.gainDb, 0, 'f', 1) : QStringLiteral("--");
    mQSlider.enabled = hasSelection;
    mQSlider.value = hasSelection ? QStringLiteral("%1").arg(current.q, 0, 'f', 1) : QStringLiteral("--");

    drawSlider(painter, mFrequencySlider, hasSelection ? sliderTForFrequency(current.frequencyHz) : 0.0);
    drawSlider(painter, mGainSlider, hasSelection ? (current.gainDb - kMinGainDb) / (kMaxGainDb - kMinGainDb) : 0.5);
    drawSlider(painter, mQSlider, hasSelection ? sliderTForQ(current.q) : 0.0);

    drawButton(painter, mBypassButtonRect, QStringLiteral("Bypass"), hasSelection && current.bypass, hasSelection);
    drawButton(painter, mSoloButtonRect, QStringLiteral("Solo"), hasSelection && mSoloIndex == mSelectedIndex,
               hasSelection);
    drawButton(painter, mResetButtonRect, QStringLiteral("Reset"), false, hasSelection);

    if (!mTypeChipRects.isEmpty())
    {
        for (int i = 0; i < mTypeChipRects.size(); ++i)
        {
            const eq::FilterType type = mTypeChipTypes.at(i);
            drawButton(painter, mTypeChipRects.at(i), typeLabel(type), hasSelection && current.type == type,
                       typeEnabled);
        }
    }
    else if (!mTypeButtonRect.isNull())
    {
        const QString text = hasSelection ? QStringLiteral("Type: %1").arg(typeLabel(current.type))
                                          : QStringLiteral("Type");
        drawButton(painter, mTypeButtonRect, text, false, typeEnabled);
    }

    painter.restore();
}

void EqCustomPlot::drawSlider(QPainter &painter, const SliderGeometry &slider, double t)
{
    painter.save();

    const double clampedT = clamped(t, 0.0, 1.0);
    const QColor textColor = slider.enabled ? mTextColor : mDisabledColor;
    const QColor trackColor = slider.enabled ? mMutedTextColor : mDisabledColor;
    QColor fillColor = slider.enabled ? mAccentColor : mDisabledColor;
    fillColor.setAlpha(slider.enabled ? 210 : 80);

    painter.setPen(textColor);
    painter.drawText(QRectF(slider.bounds.left(), slider.bounds.top(), 78.0, slider.bounds.height()),
                     Qt::AlignLeft | Qt::AlignVCenter, slider.label);
    painter.drawText(QRectF(slider.bounds.right() - 88.0, slider.bounds.top(), 88.0, slider.bounds.height()),
                     Qt::AlignRight | Qt::AlignVCenter, slider.value);

    painter.setPen(Qt::NoPen);
    painter.setBrush(trackColor);
    painter.drawRoundedRect(slider.groove, 5.0, 5.0);

    QRectF fill = slider.groove;
    fill.setWidth(slider.groove.width() * clampedT);
    painter.setBrush(fillColor);
    painter.drawRoundedRect(fill, 5.0, 5.0);

    const qreal handleDiameter = 24.0;
    const QPointF center(slider.groove.left() + slider.groove.width() * clampedT, slider.groove.center().y());
    QRectF handle(center.x() - handleDiameter / 2.0, center.y() - handleDiameter / 2.0, handleDiameter,
                  handleDiameter);
    painter.setBrush(slider.enabled ? mAccentColor : mPanelColor);
    painter.setPen(QPen(slider.enabled ? mBackgroundColor : mDisabledColor, 2.0));
    painter.drawEllipse(handle);

    painter.restore();
}

void EqCustomPlot::drawButton(QPainter &painter, const QRectF &rect, const QString &text, bool checked, bool enabled)
{
    if (rect.isNull() || rect.width() <= 0.0 || rect.height() <= 0.0)
        return;

    painter.save();

    QColor fill = checked ? mAccentColor : mBackgroundColor;
    QColor pen = checked ? mAccentColor : mMutedTextColor;
    QColor textColor = checked ? mBackgroundColor : mTextColor;

    if (!enabled)
    {
        fill = mPanelColor;
        pen = mDisabledColor;
        textColor = mDisabledColor;
    }

    painter.setPen(QPen(pen, 1.2));
    painter.setBrush(fill);
    painter.drawRoundedRect(rect, 8.0, 8.0);
    painter.setPen(textColor);
    painter.drawText(rect.adjusted(4.0, 0.0, -4.0, 0.0), Qt::AlignCenter, text);

    painter.restore();
}

bool EqCustomPlot::beginInteraction(const QPointF &pos)
{
    if (handlePanelPress(pos))
        return true;

    const int hit = hitBandHandle(pos);
    if (isValidBand(hit))
    {
        setSelectedBand(hit);
        mDragTarget = DragTarget::BandHandle;
        mUsingLiveGrid = true;
        mEditing = true;
        invalidateResponses(true);
        emit editStarted(hit);
        updateInteraction(pos);
        return true;
    }

    return false;
}

void EqCustomPlot::updateInteraction(const QPointF &pos)
{
    if (mDragTarget == DragTarget::None || !isValidBand(mSelectedIndex))
        return;

    if (mDragTarget == DragTarget::BandHandle)
    {
        Band next = mBandStates[mSelectedIndex].value;
        next.frequencyHz = frequencyForX(pos.x());
        if (!isPassBand(mSelectedIndex))
            next.gainDb = gainForY(pos.y());
        setBand(mSelectedIndex, next);
        return;
    }

    updateSliderFromPosition(mDragTarget, pos);
}

void EqCustomPlot::finishInteraction()
{
    if (mEditing && isValidBand(mSelectedIndex))
        emit editFinished(mSelectedIndex);

    mDragTarget = DragTarget::None;
    mEditing = false;
    if (mUsingLiveGrid)
    {
        mUsingLiveGrid = false;
        mPathDirty = true;
    }
    requestFrame(true);
}

bool EqCustomPlot::handlePanelPress(const QPointF &pos)
{
    if (!mPanelRect.contains(pos) || !isValidBand(mSelectedIndex))
        return false;

    Band current = mBandStates[mSelectedIndex].value;

    if (mBypassButtonRect.contains(pos))
    {
        current.bypass = !current.bypass;
        setBand(mSelectedIndex, current);
        return true;
    }

    if (mSoloButtonRect.contains(pos))
    {
        mSoloIndex = mSoloIndex == mSelectedIndex ? InvalidBand : mSelectedIndex;
        mPathDirty = true;
        requestFrame(true);
        return true;
    }

    if (mResetButtonRect.contains(pos))
    {
        resetBand(mSelectedIndex);
        return true;
    }

    if (!isPassBand(mSelectedIndex))
    {
        for (int i = 0; i < mTypeChipRects.size(); ++i)
        {
            if (!mTypeChipRects.at(i).contains(pos))
                continue;
            current.type = mTypeChipTypes.at(i);
            setBand(mSelectedIndex, current);
            return true;
        }

        if (!mTypeButtonRect.isNull() && mTypeButtonRect.contains(pos))
        {
            const QVector<eq::FilterType> types = editableTypes();
            int nextIndex = 0;
            for (int i = 0; i < types.size(); ++i)
            {
                if (types.at(i) == current.type)
                {
                    nextIndex = (i + 1) % types.size();
                    break;
                }
            }
            current.type = types.at(nextIndex);
            setBand(mSelectedIndex, current);
            return true;
        }
    }

    if (mFrequencySlider.bounds.contains(pos))
    {
        mDragTarget = DragTarget::FrequencySlider;
    }
    else if (mGainSlider.bounds.contains(pos) && !isPassBand(mSelectedIndex))
    {
        mDragTarget = DragTarget::GainSlider;
    }
    else if (mQSlider.bounds.contains(pos))
    {
        mDragTarget = DragTarget::QSlider;
    }
    else
    {
        return true;
    }

    mUsingLiveGrid = true;
    mEditing = true;
    invalidateResponses(true);
    emit editStarted(mSelectedIndex);
    updateSliderFromPosition(mDragTarget, pos);
    return true;
}

void EqCustomPlot::updateSliderFromPosition(DragTarget slider, const QPointF &pos)
{
    if (!isValidBand(mSelectedIndex))
        return;

    Band next = mBandStates[mSelectedIndex].value;
    const auto sliderT = [&](const SliderGeometry &geometry) {
        if (geometry.groove.width() <= 0.0)
            return 0.0;
        return clamped((pos.x() - geometry.groove.left()) / geometry.groove.width(), 0.0, 1.0);
    };

    switch (slider)
    {
    case DragTarget::FrequencySlider:
        next.frequencyHz = frequencyForSliderT(sliderT(mFrequencySlider));
        break;
    case DragTarget::GainSlider:
        if (!isPassBand(mSelectedIndex))
            next.gainDb = kMinGainDb + sliderT(mGainSlider) * (kMaxGainDb - kMinGainDb);
        break;
    case DragTarget::QSlider:
        next.q = qForSliderT(sliderT(mQSlider));
        break;
    default:
        return;
    }

    setBand(mSelectedIndex, next);
}

int EqCustomPlot::hitBandHandle(const QPointF &pos) const
{
    int bestIndex = InvalidBand;
    double bestDistance = std::numeric_limits<double>::max();
    const double tolerance = std::max(30.0, mHandleSize * 0.85);

    for (int i = 0; i < BandCount; ++i)
    {
        const QPointF point = bandPoint(i);
        const double dx = point.x() - pos.x();
        const double dy = point.y() - pos.y();
        const double distance = std::sqrt(dx * dx + dy * dy);
        if (distance <= tolerance && distance < bestDistance)
        {
            bestDistance = distance;
            bestIndex = i;
        }
    }

    return bestIndex;
}

QPointF EqCustomPlot::bandPoint(int index) const
{
    if (!isValidBand(index))
        return QPointF();

    const Band &band = mBandStates[index].value;
    const double gain = isPassBand(index) ? 0.0 : band.gainDb;
    return QPointF(xForFrequency(band.frequencyHz), yForGain(gain));
}

EqCustomPlot::Band EqCustomPlot::normalizedBand(int index, Band band) const
{
    band.frequencyHz = static_cast<int>(clamped(band.frequencyHz, kMinBandFrequency, kMaxBandFrequency));
    band.gainDb = clamped(band.gainDb, kMinGainDb, kMaxGainDb);
    band.q = clamped(band.q, kMinQ, kMaxQ);

    if (index == EqHp)
    {
        band.type = eq::FilterType::Highpass;
        band.gainDb = 0.0;
    }
    else if (index == EqLp)
    {
        band.type = eq::FilterType::Lowpass;
        band.gainDb = 0.0;
    }
    else if (!isEditableType(band.type))
    {
        band.type = eq::FilterType::Peak;
    }

    band.q = std::round(band.q * 10.0) / 10.0;
    band.gainDb = std::round(band.gainDb * 10.0) / 10.0;

    return band;
}

bool EqCustomPlot::isValidBand(int index) const
{
    return index >= 0 && index < BandCount;
}

bool EqCustomPlot::isPassBand(int index) const
{
    return index == EqHp || index == EqLp;
}

bool EqCustomPlot::isEditableType(eq::FilterType type) const
{
    const QVector<eq::FilterType> types = editableTypes();
    return std::find(types.begin(), types.end(), type) != types.end();
}

QVector<eq::FilterType> EqCustomPlot::editableTypes() const
{
    return {
        eq::FilterType::Peak,      eq::FilterType::Lowshelf, eq::FilterType::Highshelf,
        eq::FilterType::Bandpass,  eq::FilterType::Notch,    eq::FilterType::Allpass,
    };
}

QString EqCustomPlot::bandLabel(int index) const
{
    if (index == EqHp)
        return QStringLiteral("HP");
    if (index == EqLp)
        return QStringLiteral("LP");
    if (index > EqHp && index < EqLp)
        return QString::number(index);
    return QStringLiteral("--");
}

QString EqCustomPlot::typeLabel(eq::FilterType type) const
{
    switch (type)
    {
    case eq::FilterType::Peak:
        return QStringLiteral("Peak");
    case eq::FilterType::Lowshelf:
        return QStringLiteral("LowShelf");
    case eq::FilterType::Highshelf:
        return QStringLiteral("HighShelf");
    case eq::FilterType::Lowpass:
        return QStringLiteral("LowPass");
    case eq::FilterType::Highpass:
        return QStringLiteral("HighPass");
    case eq::FilterType::Bandpass:
        return QStringLiteral("BandPass");
    case eq::FilterType::Notch:
        return QStringLiteral("Notch");
    case eq::FilterType::Allpass:
        return QStringLiteral("AllPass");
    default:
        return QStringLiteral("Unknown");
    }
}

double EqCustomPlot::xForFrequency(double frequencyHz) const
{
    if (mPlotRect.width() <= 0.0)
        return mPlotRect.left();

    const double safeFrequency = clamped(frequencyHz, kMinAxisFrequency, kMaxAxisFrequency);
    const double t = (std::log10(safeFrequency) - kAxisMinLog) / (kAxisMaxLog - kAxisMinLog);
    return mPlotRect.left() + t * mPlotRect.width();
}

double EqCustomPlot::yForGain(double gainDb) const
{
    if (mPlotRect.height() <= 0.0)
        return mPlotRect.center().y();

    const double clampedGain = clamped(gainDb, kMinGainDb, kMaxGainDb);
    const double t = (clampedGain - kMinGainDb) / (kMaxGainDb - kMinGainDb);
    return mPlotRect.bottom() - t * mPlotRect.height();
}

int EqCustomPlot::frequencyForX(double x) const
{
    if (mPlotRect.width() <= 0.0)
        return kMinBandFrequency;

    const double t = clamped((x - mPlotRect.left()) / mPlotRect.width(), 0.0, 1.0);
    const double frequency = kMinAxisFrequency * std::pow(10.0, t * (kAxisMaxLog - kAxisMinLog));
    return static_cast<int>(std::lround(clamped(frequency, kMinBandFrequency, kMaxBandFrequency)));
}

double EqCustomPlot::gainForY(double y) const
{
    if (mPlotRect.height() <= 0.0)
        return 0.0;

    const double t = clamped((mPlotRect.bottom() - y) / mPlotRect.height(), 0.0, 1.0);
    return kMinGainDb + t * (kMaxGainDb - kMinGainDb);
}

double EqCustomPlot::sliderTForFrequency(int frequencyHz) const
{
    const double safeFrequency = clamped(frequencyHz, kMinBandFrequency, kMaxBandFrequency);
    return (std::log10(safeFrequency) - kBandMinLog) / (kBandMaxLog - kBandMinLog);
}

int EqCustomPlot::frequencyForSliderT(double t) const
{
    const double frequency = kMinBandFrequency * std::pow(10.0, clamped(t, 0.0, 1.0) * (kBandMaxLog - kBandMinLog));
    return static_cast<int>(std::lround(frequency));
}

double EqCustomPlot::sliderTForQ(double q) const
{
    const double minLog = std::log10(kMinQ);
    const double maxLog = std::log10(kMaxQ);
    return (std::log10(clamped(q, kMinQ, kMaxQ)) - minLog) / (maxLog - minLog);
}

double EqCustomPlot::qForSliderT(double t) const
{
    const double minLog = std::log10(kMinQ);
    const double maxLog = std::log10(kMaxQ);
    return std::pow(10.0, minLog + clamped(t, 0.0, 1.0) * (maxLog - minLog));
}

double EqCustomPlot::clampDisplayDb(double value) const
{
    if (std::isinf(value))
        return value > 0 ? kMaxGainDb : kMinGainDb;
    if (!std::isfinite(value))
        return 0.0;
    return clamped(value, kMinGainDb, kMaxGainDb);
}

const QVector<double> &EqCustomPlot::activeFrequencies() const
{
    return mUsingLiveGrid ? mLiveFrequencies : mFineFrequencies;
}

const eq::FrequencyGrid &EqCustomPlot::activeGrid() const
{
    return mUsingLiveGrid ? mLiveGrid : mFineGrid;
}

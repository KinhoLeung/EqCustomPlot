#include "eqcustomplot.h"
#include "qcpaxistickerfreq.h"

EqCustomPlot::EqCustomPlot(QWidget *parent) : QCustomPlot(parent)
{
    Qt::ColorScheme theme = QApplication::styleHints()->colorScheme();
    QColor themeColor = theme == Qt::ColorScheme::Dark ? Qt::white : Qt::black;
    QColor curvePenColor = QApplication::palette().color(QPalette::Accent);
    QColor curveBurshColor = curvePenColor;
    curveBurshColor.setAlphaF(0.25);

    QString prefixStr = theme == Qt::ColorScheme::Dark ? ":/image/dark/" : ":/image/light/";

    mMouseButton = Qt::NoButton;

    mMenu = new QMenu(this);
    mBypassAction = mMenu->addAction("Bypass");
    mSoloAction = mMenu->addAction("Solo");
    mFilterTypeMenu = new QMenu("Filter Type", mMenu);
    mFilterTypeAction = mMenu->addMenu(mFilterTypeMenu);
    mBypassAction->setCheckable(true);
    mSoloAction->setCheckable(true);
    connect(mBypassAction, &QAction::triggered, this, [=]() {
        bool bypass = mSelectedScatter->property("bypass").toBool();
        bypass = !bypass;
        mSelectedScatter->setProperty("bypass", bypass);
        mBypassAction->setChecked(bypass);
        drawEqCurve(mSelectedScatter);
    });

    connect(mSoloAction, &QAction::triggered, this, [=]() {
        static bool bypass[EqCount];
        if (mSoloAction->isChecked())
        {
            for (int i = 0; i < EqCount; i++)
            {
                if (mSelectedScatter != mScatter[i])
                {
                    bypass[i] = mScatter[i]->property("bypass").toBool();
                    mScatter[i]->setProperty("bypass", true);
                    drawEqCurve(mScatter[i], true);
                    mScatter[i]->setVisible(false);
                }
            }
            mSoloScattrer = mSelectedScatter;
            drawEqCurve(mSelectedScatter);
        }
        else
        {
            for (int i = 0; i < EqCount; i++)
            {
                if (mSelectedScatter != mScatter[i])
                {
                    mScatter[i]->setProperty("bypass", bypass[i]);
                    drawEqCurve(mScatter[i], true);
                    mScatter[i]->setVisible(true);
                }
            }
            mSoloScattrer = nullptr;
            drawEqCurve(mSelectedScatter);
        }
    });

    QList<QString> typeStr{"Peak", "LowShelf", "HighShelf", "LowPass", "HighPass", "BandPass", "Notch", "AllPass"};
    mFilterActionGroup = new QActionGroup(this);
    mFilterActionGroup->setExclusive(true);

    for (int i = 0; i < typeStr.count(); i++)
    {
        QAction *action = new QAction(typeStr[i], mFilterActionGroup);
        action->setCheckable(true);
        action->setProperty("type", i);
    }
    mFilterTypeMenu->addActions(mFilterActionGroup->actions());

    connect(mFilterActionGroup, &QActionGroup::triggered, this, [=](QAction *action) {
        int type = action->property("type").toInt();
        mSelectedScatter->setProperty("type", type);
        drawEqCurve(mSelectedScatter);
    });

    mEqParamText = new QCPItemText(this);
    mEqParamText->position->setType(QCPItemPosition::ptPlotCoords);
    mEqParamText->setPositionAlignment(Qt::AlignCenter);
    mEqParamText->setPen(Qt::NoPen);
    mEqParamText->setBrush(Qt::NoBrush);
    mEqParamText->setColor(themeColor);
    mEqParamText->setVisible(false);

    QList<QString> selectedSvgPaths = {prefixStr+"hp_filled.svg", prefixStr+"1_filled.svg", prefixStr+"2_filled.svg",
                                       prefixStr+"3_filled.svg",  prefixStr+"4_filled.svg", prefixStr+"5_filled.svg",
                                       prefixStr+"6_filled.svg",  prefixStr+"7_filled.svg", prefixStr+"8_filled.svg",
                                       prefixStr+"9_filled.svg",  prefixStr+"lp_filled.svg"};
    QList<QString> unselectedSvgPaths = {prefixStr+"hp_regular.svg", prefixStr+"1_regular.svg", prefixStr+"2_regular.svg",
                                         prefixStr+"3_regular.svg",  prefixStr+"4_regular.svg", prefixStr+"5_regular.svg",
                                         prefixStr+"6_regular.svg",  prefixStr+"7_regular.svg", prefixStr+"8_regular.svg",
                                         prefixStr+"9_regular.svg",  prefixStr+"lp_regular.svg"};

    for (int i = 0; i < selectedSvgPaths.size(); i++)
    {
        mSelectedIcon.append(QIcon(selectedSvgPaths[i]));
    }

    for (int i = 0; i < unselectedSvgPaths.size(); i++)
    {
        mUnselectedIcon.append(QIcon(unselectedSvgPaths[i]));
    }

    this->xAxis2->setVisible(true);
    this->xAxis2->setTickLabels(false);
    auto xTicker = QSharedPointer<QCPAxisTickerFreq>::create();
    QList<QCPAxis *> xAxes = {this->xAxis, this->xAxis2};
    for (auto axis : xAxes)
    {
        axis->setTicker(xTicker);
        axis->setScaleType(QCPAxis::stLogarithmic);
        axis->setRange(MIN_XAXIS_VALUE, MAX_XAXIS_VALUE);
    }

    this->yAxis2->setVisible(true);
    this->yAxis2->setTickLabels(false);
    QMap<double, QString> ticks;
    for (double i = MIN_YAXIS_VALUE; i <= MAX_YAXIS_VALUE; i += (MAX_YAXIS_VALUE - MIN_YAXIS_VALUE) / 6)
    {
        ticks.insert(i, QString::number(i, 'f', 0));
    }
    auto yTicker = QSharedPointer<QCPAxisTickerText>::create();
    yTicker->addTicks(ticks);
    yTicker->setSubTickCount(1);

    QList<QCPAxis *> yAxes = {this->yAxis, this->yAxis2};
    for (auto axis : yAxes)
    {
        axis->setTicker(yTicker);
        axis->setRange(MIN_YAXIS_VALUE, MAX_YAXIS_VALUE);
    }

    QList<QCPAxis *> axes = {this->xAxis, this->xAxis2, this->yAxis, this->yAxis2};
    for (auto axis : axes)
    {
        axis->setBasePen(QPen(QBrush(themeColor), 1));
        axis->setTickPen(QPen(QBrush(themeColor), 1));
        axis->setSubTickPen(QPen(QBrush(themeColor), 1));
        axis->setTickLabelColor(QColor(themeColor));
        axis->setTickLength(0, 0);
        axis->setSubTickLength(0, 0);
        axis->grid()->setPen(QPen(QBrush(themeColor), 1));
        axis->grid()->setSubGridPen(QPen(QBrush(themeColor), 1));
        axis->grid()->setSubGridVisible(true);
        axis->grid()->setZeroLinePen(Qt::NoPen);
    }

    this->setBackground(QBrush(theme == Qt::ColorScheme::Dark ? QColor(Qt::black) : QColor(Qt::white)));
    this->setInteractions(QCP::iSelectPlottables);
    this->setAntialiasedElements(QCP::aeAll);

    mFreqData.resize(FREQ_COUNT);
    double xAxisLogStep = (qLn(MAX_XAXIS_VALUE) / M_LN10 - qLn(MIN_XAXIS_VALUE) / M_LN10) / FREQ_COUNT;
    double freqLogStep = (qLn(MAX_FREQ) / M_LN10 - qLn(MIN_FREQ) / M_LN10) / (EqCount - 1);
    for (int i = 0; i < FREQ_COUNT; i++)
    {
        mFreqData[i] = (MIN_XAXIS_VALUE * qPow(10, i * xAxisLogStep));
    }

    for (int i = 0; i < EqCount; i++)
    {
        QCPGraph *curve = this->addGraph(this->xAxis, this->yAxis);
        QCPGraph *scatter = this->addGraph(this->xAxis, this->yAxis);

        mScatter.append(scatter);
        mScatterCurve.insert(scatter, curve);

        curve->setPen(QPen(curvePenColor));
        curve->setBrush(QBrush(curveBurshColor));
        curve->setSelectable(QCP::stNone);

        scatter->setProperty("defaultGain", DEFAULT_GAIN);
        scatter->setProperty("defaultBypass", false);
        scatter->setProperty("defaultFreq", (MIN_FREQ * qPow(10, i * freqLogStep)));
        if (i == EqHp)
        {
            scatter->setProperty("defaultQ", DEFAULT_HIGHPASS_Q);
            scatter->setProperty("defaultType", static_cast<int>(eq::FilterType::Highpass));
        }
        else if (i == EqLp)
        {
            scatter->setProperty("defaultQ", DEFAULT_LOWPASS_Q);
            scatter->setProperty("defaultType", static_cast<int>(eq::FilterType::Lowpass));
        }
        else
        {
            scatter->setProperty("defaultQ", DEFAULT_Q);
            scatter->setProperty("defaultType", static_cast<int>(eq::FilterType::Peak));
        }

        double gain = scatter->property("defaultGain").toDouble();
        double q = scatter->property("defaultQ").toDouble();
        int freq = scatter->property("defaultFreq").toInt();
        int type = scatter->property("defaultType").toInt();
        bool bypass = scatter->property("defaultBypass").toBool();

        scatter->setProperty("gain", gain);
        scatter->setProperty("q", q);
        scatter->setProperty("freq", freq);
        scatter->setProperty("type", type);
        scatter->setProperty("bypass", bypass);

        scatter->addData(freq, gain);
        scatter->setLayer(QLatin1String("overlay"));

        mOverallCoeff.append(
            eq::getSectionsMatrix(gain, freq, q, static_cast<eq::FilterType>(type), bypass, SAMPLE_RATE));
    }

    mOverallCurve = this->addGraph(this->xAxis, this->yAxis);
    QVector<double> hoverallH = eq::getFreqzn(mOverallCoeff, SAMPLE_RATE, mFreqData);
    mOverallCurve->setData(mFreqData, hoverallH);
    mOverallCurve->setPen(QPen(curvePenColor, 2));
    mOverallCurve->setSelectable(QCP::stNone);

    double yBottom = this->yAxis->range().lower;
    QVector<double> bottomY(mFreqData.size(), yBottom);
    QCPGraph *bottomGraph = this->addGraph(this->xAxis, this->yAxis);
    bottomGraph->setData(mFreqData, bottomY);
    bottomGraph->setPen(Qt::NoPen);
    bottomGraph->setSelectable(QCP::stNone);

    mScatterCurve[mScatter[EqHp]]->setChannelFillGraph(bottomGraph);
    mScatterCurve[mScatter[EqLp]]->setChannelFillGraph(bottomGraph);

    connect(this, &QCustomPlot::plottableClick, this,
            [this](QCPAbstractPlottable *plottable, int dataIndex, QMouseEvent *event) {
                if (event->type() == QEvent::MouseButtonPress &&
                    (event->button() == Qt::LeftButton || event->button() == Qt::RightButton))
                {
                    QCPGraph *graph = qobject_cast<QCPGraph *>(plottable);
                    if (graph == mOverallCurve)
                        return;
                    mSelectedScatter = graph;
                    mMouseButton = event->button();
                    mSelectedScatter->setScatterStyle(mSelectedStyle[mSelectedScatter]);
                    mScatterCurve[mSelectedScatter]->setVisible(true);
                    if (event->button() == Qt::LeftButton)
                    {
                        updateEqParamText(mSelectedScatter);
                    }
                    else if (event->button() == Qt::RightButton)
                    {
                        if ((mSoloAction->isChecked() && mSoloScattrer == mSelectedScatter) ||
                            mSoloAction->isChecked() == false)
                        {
                            mBypassAction->setChecked(mSelectedScatter->property("bypass").toBool());
                            int type = mSelectedScatter->property("type").toInt();
                            mFilterActionGroup->actions().at(type)->setChecked(true);
                            if (mSelectedScatter == mScatter[EqHp] || mSelectedScatter == mScatter[EqLp])
                                mFilterTypeAction->setVisible(false);
                            else
                                mFilterTypeAction->setVisible(true);
                            mMenu->exec(QCursor::pos());
                        }
                    }
                }
            });

    connect(this, &QCustomPlot::plottableDoubleClick, this,
            [this](QCPAbstractPlottable *plottable, int dataIndex, QMouseEvent *event) {
                if (event->type() != QEvent::MouseButtonDblClick || event->button() != Qt::LeftButton)
                    return;
                QCPGraph *graph = qobject_cast<QCPGraph *>(plottable);

                bool isChanged = false;

                double defaultGain = graph->property("defaultGain").toDouble();
                double defaultQ = graph->property("defaultQ").toDouble();
                int defaultFreq = graph->property("defaultFreq").toInt();

                double gain = graph->property("gain").toDouble();
                double q = graph->property("q").toDouble();
                int freq = graph->property("freq").toInt();

                if (gain != defaultGain)
                {
                    isChanged = true;
                    graph->setProperty("gain", defaultGain);
                }

                if (q != defaultQ)
                {
                    isChanged = true;
                    graph->setProperty("q", defaultQ);
                }

                if (freq != defaultFreq)
                {
                    isChanged = true;
                    graph->setProperty("freq", defaultFreq);
                }

                if (isChanged)
                {
                    drawEqCurve(graph);
                }
            });

    connect(this, &QCustomPlot::mousePress, this, [this](QMouseEvent *event) {
        for (int i = 0; i < EqCount; i++)
        {
            mScatter[i]->setScatterStyle(mUnselectedStyle[mScatter[i]]);
            mScatterCurve[mScatter[i]]->setVisible(false);
        }
        mEqParamText->setVisible(false);
        mSelectedScatter = nullptr;
    });

    connect(this, &QCustomPlot::mouseMove, this, [this](QMouseEvent *event) {
        if (mMouseButton == Qt::LeftButton)
        {
            double newFreq = this->xAxis->pixelToCoord(event->pos().x());
            double newGain = this->yAxis->pixelToCoord(event->pos().y());
            if (newFreq < MIN_FREQ)
                newFreq = MIN_FREQ;
            if (newFreq > MAX_FREQ)
                newFreq = MAX_FREQ;

            if (newGain < MIN_GAIN)
                newGain = MIN_GAIN;
            if (newGain > MAX_GAIN)
                newGain = MAX_GAIN;

            bool isChanged = false;
            double gain = mSelectedScatter->property("gain").toDouble();
            int freq = mSelectedScatter->property("freq").toInt();

            if (newFreq != freq)
            {
                isChanged = true;
                mSelectedScatter->setProperty("freq", newFreq);
            }

            if (newGain != gain)
            {
                isChanged = true;
                mSelectedScatter->setProperty("gain", newGain);
            }

            if (isChanged)
            {
                drawEqCurve(mSelectedScatter);
            }
        }
    });
    connect(this, &QCustomPlot::mouseRelease, this, [this](QMouseEvent *event) { mMouseButton = Qt::NoButton; });
    connect(this, &QCustomPlot::mouseWheel, this, [this](QWheelEvent *event) {
        if (this->selectedGraphs().count() <= 0)
            return;
        int step = 0;
        if (!event->angleDelta().isNull())
        {
            step = event->angleDelta().y() > 0 ? 1 : -1;
        }
        else if (!event->pixelDelta().isNull())
        {
            step = event->pixelDelta().y() > 0 ? 1 : -1;
        }

        double curQ = mSelectedScatter->property("q").toDouble();
        double newQ = curQ + step * Q_SINGLE_STEP;

        if (newQ < MIN_Q)
            newQ = MIN_Q;
        if (newQ > MAX_Q)
            newQ = MAX_Q;

        if (curQ != newQ)
        {
            mSelectedScatter->setProperty("q", newQ);
            drawEqCurve(mSelectedScatter);
        }
    });
}

EqCustomPlot::~EqCustomPlot()
{
}

void EqCustomPlot::drawEqCurve(QCPGraph *scatter, bool solo)
{
    if (scatter == nullptr)
        return;
    double gain = scatter->property("gain").toDouble();
    double q = scatter->property("q").toDouble();
    int freq = scatter->property("freq").toInt();
    int type = scatter->property("type").toInt();
    bool bypass = scatter->property("bypass").toBool();

    eq::Coeff coeff = eq::getSectionsMatrix(gain, freq, q, static_cast<eq::FilterType>(type), bypass, SAMPLE_RATE);
    QVector<double> h = eq::getFreqzn(coeff, SAMPLE_RATE, mFreqData);
    mScatterCurve[scatter]->setData(mFreqData, h);

    QVector<double> key(1, freq), value(1, gain);
    scatter->setData(key, value);
    int index = mScatter.indexOf(scatter);
    mOverallCoeff[index] = eq::getSectionsMatrix(gain, freq, q, static_cast<eq::FilterType>(type), bypass, SAMPLE_RATE);
    QVector<double> overallH = eq::getFreqzn(mOverallCoeff, SAMPLE_RATE, mFreqData);
    mOverallCurve->setData(mFreqData, overallH);

    for (int i = 0; i < EqCount; i++)
    {
        mScatter[i]->setScatterStyle(mUnselectedStyle[mScatter[i]]);
        mScatterCurve[mScatter[i]]->setVisible(false);
    }

    scatter->setScatterStyle(mSelectedStyle[scatter]);
    mScatterCurve[scatter]->setVisible(true);
    if (solo == false)
        updateEqParamText(scatter);

    this->replot();
}

void EqCustomPlot::updateEqParamText(QCPGraph *scatter)
{
    if (scatter == nullptr)
        return;

    int freq = scatter->property("freq").toInt();
    double gain = scatter->property("gain").toDouble();
    double q = scatter->property("q").toDouble();

    mEqParamText->setText(QString("F:%1Hz\nG:%2dB\nQ:%3").arg(freq).arg(gain, 0, 'f', 1).arg(q, 0, 'f', 1));

    QFontMetrics fm(mEqParamText->font());
    QRect rect = fm.boundingRect(QRect(), Qt::TextWordWrap, mEqParamText->text());

    double gainHeight =
        this->yAxis->pixelToCoord(0) - this->yAxis->pixelToCoord(rect.height() + this->selectionTolerance() * 2);

    QPointF textPos = gain > 0 ? QPointF(freq, gain - gainHeight / 2) : QPointF(freq, gain + gainHeight / 2);
    mEqParamText->position->setCoords(textPos);
    mEqParamText->setVisible(true);
}

void EqCustomPlot::resizeEvent(QResizeEvent *event)
{
    QCustomPlot::resizeEvent(event);
    int width = this->width() / 25;
    int height = this->height() / 25;
    QSize scatterSize = width > height ? QSize(height, height) : QSize(width, width);
    mEqParamText->setFont(QFont(this->font().family(), scatterSize.height() / 2));
    this->setSelectionTolerance(scatterSize.width() / 2);
    int index = mScatter.indexOf(mSelectedScatter);
    qreal dpr = devicePixelRatio();

    mSelectedStyle.clear();
    mUnselectedStyle.clear();

    for (int i = 0; i < EqCount; i++)
    {
        QPixmap selectedPixmap = mSelectedIcon[i].pixmap(mSelectedIcon[i].actualSize(scatterSize));
        QPixmap unselectedPixmap = mUnselectedIcon[i].pixmap(mUnselectedIcon[i].actualSize(scatterSize));
        selectedPixmap.setDevicePixelRatio(dpr);
        unselectedPixmap.setDevicePixelRatio(dpr);
        QCPScatterStyle selectedStyle(selectedPixmap);
        QCPScatterStyle unselectedStyle(unselectedPixmap);

        mSelectedStyle.insert(mScatter[i], selectedStyle);
        mUnselectedStyle.insert(mScatter[i], unselectedStyle);
        if (index == i)
        {
            mScatter[i]->setScatterStyle(mSelectedStyle[mScatter[i]]);
            updateEqParamText(mSelectedScatter);
        }
        else
        {
            mScatter[i]->setScatterStyle(mUnselectedStyle[mScatter[i]]);
        }
    }

    this->replot();
}

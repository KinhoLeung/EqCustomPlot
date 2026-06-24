#pragma once
#include "qcustomplot.h"

// 频率轴专用 ticker：用 mantissa * 10^n 的方式生成对数频率刻度。
class QCPAxisTickerFreq : public QCPAxisTicker
{
  public:
    // 描述一个次刻度区间。例如 2->5 内补 3、4，对应 3*10^n 和 4*10^n。
    struct SubTickRule
    {
        SubTickRule() = default;
        SubTickRule(double from, double to, const QVector<double> &minorMantissas);

        double fromMantissa = 0;
        double toMantissa = 0;
        QVector<double> mantissas;
    };

    // 描述一个标签单位。例如 threshold=1000, divisor=1000, suffix="k" 表示 1000Hz 起显示为 kHz。
    struct UnitFormat
    {
        UnitFormat() = default;
        UnitFormat(double threshold, double divisor, const QString &unitSuffix);

        double thresholdValue = 0;
        double scaleDivisor = 1;
        QString suffix;
    };

    // 控制刻度标签的数值格式和单位缩放。
    struct LabelFormat
    {
        LabelFormat();

        bool useUnits = true;
        bool roundUnitValues = true;
        bool roundBaseValues = true;
        QChar formatChar;
        int precision = 6;
        QVector<UnitFormat> units;
    };

    // 完整刻度配置。调用方只需要构造这个对象，再通过 setTickSpec 一次性应用。
    struct FrequencyTickSpec
    {
        FrequencyTickSpec();

        // 主刻度 mantissa，默认生成 2/5/10 * 10^n。
        QVector<double> majorMantissas;
        // 次刻度规则，按相邻主刻度区间匹配。
        QVector<SubTickRule> minorRules;
        // 浮点比较容差，用于去重和判断 mantissa 是否匹配。
        double relTol = 1e-3;

        // 可选 decade 限制，decade 指 10^n 的指数 n。
        bool boundDecades = false;
        int minDecade = 0;
        int maxDecade = 6;

        LabelFormat labelFormat;
    };

    // 使用默认 FrequencyTickSpec 初始化：音频频率轴常用的 2/5/10 主刻度。
    QCPAxisTickerFreq();

    // 返回当前已归一化后的完整配置，可用于读取后局部修改再 setTickSpec。
    FrequencyTickSpec tickSpec() const;
    // 应用完整刻度配置。传入配置会被清洗、排序、去重后再用于生成刻度。
    void setTickSpec(const FrequencyTickSpec &spec);
    // 恢复默认频率轴配置。
    void resetTickSpec();

    // QCPAxisTicker 的主入口：一次生成主刻度、次刻度和标签。
    void generate(const QCPRange &range, const QLocale &locale, QChar formatChar, int precision,
                  QVector<double> &ticks, QVector<double> *subTicks, QVector<QString> *tickLabels) override;

  private:
    // 相对容差比较，避免 log10/pow 带来的浮点误差影响刻度匹配和去重。
    static bool approxEqual(double a, double b, double relTol);
    // 计算 10^p，集中封装方便表达 decade 语义。
    static double ipow10(int p);
    // 取得不大于 v 的 decade 指数，例如 250 -> 2，因为 10^2 <= 250。
    static int floorDecade(double v);
    // 取得不小于 v 的 decade 指数，例如 250 -> 3，因为 10^3 >= 250。
    static int ceilDecade(double v);
    // 清洗 mantissa 列表：只保留正有限数，并按容差排序去重。
    static QVector<double> cleanMantissas(const QVector<double> &mantissas, double relTol);
    // 清洗次刻度规则：过滤非法区间、非法 mantissa，并去掉重复规则。
    static QVector<SubTickRule> cleanSubTickRules(const QVector<SubTickRule> &rules, double relTol);
    // 清洗单位格式：过滤非法阈值/缩放值，并按阈值从大到小排序。
    static QVector<UnitFormat> cleanUnitFormats(const QVector<UnitFormat> &units, double relTol);

    // 统一归一化外部配置，保证后续生成逻辑只面对合法、稳定的配置。
    void normalizeSpec();
    // 把频率拆成 base 和 mantissa，例如 2000 -> base=1000, m=2。
    void normalizeMantissa(double v, double &base, double &m) const;

    // 生成主刻度。keepOneOutlier=true 时保留视野外一颗主刻度，供次刻度计算边界区间。
    QVector<double> createMajorTicks(const QCPRange &range, bool keepOneOutlier) const;
    // 根据相邻主刻度和 minorRules 生成次刻度。
    QVector<double> createMinorTicks(const QVector<double> &ticks, const QCPRange &range) const;
    // 为可见主刻度生成显示文本。
    QVector<QString> createLabels(const QVector<double> &ticks, const QLocale &locale, QChar formatChar,
                                  int precision) const;

    // 根据数值大小选择最合适的单位，例如 1000 以上选 k。
    const UnitFormat *selectUnitFormat(double value) const;
    // 格式化纯数字，支持接近整数时按整数显示。
    QString formatNumber(double value, const QLocale &locale, QChar formatChar, int precision,
                         bool roundNearInteger) const;
    // 按 LabelFormat 选择单位并格式化最终频率标签。
    QString formatFrequency(double f, const QLocale &locale, QChar formatChar, int precision) const;

  private:
    FrequencyTickSpec mSpec;
};

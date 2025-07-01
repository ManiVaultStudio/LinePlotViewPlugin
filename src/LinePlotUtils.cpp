#include "LinePlotUtils.h"
#include <QDebug>
#include <float.h>
#include <QColor>
#include <QString>
#include <array>

// includes for mv::Dataset, Points, Clusters, NormalizationType, SmoothingType
#include "PointData/PointData.h"
#include "ClusterData/ClusterData.h"
#include "LinePlotViewPlugin.h" // for NormalizationType, SmoothingType

using namespace mv;

void customMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    const char* colorCode = "\033[0m"; // Default

    switch (type) {
    case QtDebugMsg:
        colorCode = "\033[1;36m"; // Cyan
        break;
    case QtInfoMsg:
        colorCode = "\033[1;32m"; // Green
        break;
    case QtWarningMsg:
        colorCode = "\033[1;33m"; // Yellow
        break;
    case QtCriticalMsg:
        colorCode = "\033[1;31m"; // Red
        break;
    case QtFatalMsg:
        colorCode = "\033[1;41m"; // Red background
        break;
    }

    fprintf(stderr, "%s[%s] %s (%s:%u, %s)%s\n",
        colorCode,
        (type == QtDebugMsg ? "DEBUG" :
            type == QtInfoMsg ? "INFO" :
            type == QtWarningMsg ? "WARNING" :
            type == QtCriticalMsg ? "CRITICAL" : "FATAL"),
        localMsg.constData(),
        context.file ? context.file : "",
        context.line,
        context.function ? context.function : "",
        "\033[0m"); // Reset color

    if (type == QtFatalMsg)
        abort();
}


FunctionTimer::FunctionTimer(const QString& functionName)
    : _functionName(functionName)
{
    _timer.start();
}

FunctionTimer::~FunctionTimer()
{
    std::cout
        << _functionName.toStdString()
        << ":"
        << QString::number(_timer.elapsed() / 1000.0, 'f', 3).toStdString()
        << "sec";
}

static const std::array<QColor, 10> kQualitative10 = {
    QColor("#8dd3c7"), QColor("#ffffb3"), QColor("#bebada"), QColor("#fb8072"),
    QColor("#80b1d3"), QColor("#fdb462"), QColor("#b3de69"), QColor("#fccde5"),
    QColor("#d9d9d9"), QColor("#bc80bd")
};

static const std::array<QColor, 9> kYlGn = {
    QColor("#ffffe5"), QColor("#f7fcb9"), QColor("#d9f0a3"), QColor("#addd8e"),
    QColor("#78c679"), QColor("#41ab5d"), QColor("#238443"), QColor("#006837"),
    QColor("#004529")
};

static const std::array<QColor, 11> kRdYlGn = {
    QColor("#a50026"), QColor("#d73027"), QColor("#f46d43"), QColor("#fdae61"),
    QColor("#fee08b"), QColor("#ffffbf"), QColor("#d9ef8b"), QColor("#a6d96a"),
    QColor("#66bd63"), QColor("#1a9850"), QColor("#006837")
};

static const std::array<QColor, 9> kGnBu = {
    QColor("#f7fcf0"), QColor("#e0f3db"), QColor("#ccebc5"), QColor("#a8ddb5"),
    QColor("#7bccc4"), QColor("#4eb3d3"), QColor("#2b8cbe"), QColor("#0868ac"),
    QColor("#084081")
};

static const std::array<QColor, 9> kYlGnBu = {
    QColor("#ffffd9"), QColor("#edf8b1"), QColor("#c7e9b4"), QColor("#7fcdbb"),
    QColor("#41b6c4"), QColor("#1d91c0"), QColor("#225ea8"), QColor("#253494"),
    QColor("#081d58")
};

static const std::array<QColor, 11> kSpectral = {
    QColor("#9e0142"), QColor("#d53e4f"), QColor("#f46d43"), QColor("#fdae61"),
    QColor("#fee08b"), QColor("#e6f598"), QColor("#abdda4"), QColor("#66c2a5"),
    QColor("#3288bd"), QColor("#5e4fa2"), QColor("#000000")
};

static const std::array<QColor, 11> kBrBg = {
    QColor("#543005"), QColor("#8c510a"), QColor("#bf812d"), QColor("#dfc27d"),
    QColor("#f6e8c3"), QColor("#f5f5f5"), QColor("#c7eae5"), QColor("#80cdc1"),
    QColor("#35978f"), QColor("#01665e"), QColor("#003c30")
};

static const std::array<QColor, 9> kYlOrBr = {
    QColor("#ffffe5"), QColor("#fff7bc"), QColor("#fee391"), QColor("#fec44f"),
    QColor("#fe9929"), QColor("#ec7014"), QColor("#cc4c02"), QColor("#993404"),
    QColor("#662506")
};

static const std::array<QColor, 11> kRdBu = {
    QColor("#67001f"), QColor("#b2182b"), QColor("#d6604d"), QColor("#f4a582"),
    QColor("#f7f7f7"), QColor("#92c5de"), QColor("#4393c3"), QColor("#2166ac"),
    QColor("#053061"), QColor("#ffffff"), QColor("#000000")
};

static const std::array<QColor, 9> kRdPu = {
    QColor("#fff7f3"), QColor("#fde0dd"), QColor("#fcc5c0"), QColor("#fa9fb5"),
    QColor("#f768a1"), QColor("#dd3497"), QColor("#ae017e"), QColor("#7a0177"),
    QColor("#49006a")
};

static const std::array<QColor, 9> kPlasma = {
    QColor("#0d0887"), QColor("#5b02a3"), QColor("#9a179b"), QColor("#cb4679"),
    QColor("#ed7953"), QColor("#fb9f3a"), QColor("#fdca26"), QColor("#f0f921"),
    QColor("#ffffff")
};

static const std::array<QColor, 11> kPuOr = {
    QColor("#7f3b08"), QColor("#b35806"), QColor("#e08214"), QColor("#fdb863"),
    QColor("#fee0b6"), QColor("#f7f7f7"), QColor("#d8daeb"), QColor("#b2abd2"),
    QColor("#8073ac"), QColor("#542788"), QColor("#2d004b")
};

static const std::array<QColor, 9> kBuPu = {
    QColor("#f7fcfd"), QColor("#e0ecf4"), QColor("#bfd3e6"), QColor("#9ebcda"),
    QColor("#8c96c6"), QColor("#8c6bb1"), QColor("#88419d"), QColor("#810f7c"),
    QColor("#4d004b")
};

static const std::array<QColor, 9> kReds = {
    QColor("#fff5f0"), QColor("#fee0d2"), QColor("#fcbba1"), QColor("#fc9272"),
    QColor("#fb6a4a"), QColor("#ef3b2c"), QColor("#cb181d"), QColor("#a50f15"),
    QColor("#67000d")
};

static const std::array<QColor, 9> kViridis = {
    QColor("#440154"), QColor("#482777"), QColor("#3e4989"), QColor("#31688e"),
    QColor("#26828e"), QColor("#1f9e89"), QColor("#35b779"), QColor("#6ece58"),
    QColor("#b5de2b")
};

static const std::array<QColor, 9> kQ_BiGrRd = {
    QColor("#40004b"), QColor("#762a83"), QColor("#9970ab"), QColor("#c2a5cf"),
    QColor("#e7d4e8"), QColor("#d9f0d3"), QColor("#a6dba0"), QColor("#5aae61"),
    QColor("#1b7837")
};

static const std::array<QColor, 9> kMagma = {
    QColor("#000004"), QColor("#1c1044"), QColor("#51127c"), QColor("#833790"),
    QColor("#b63679"), QColor("#ee605e"), QColor("#fb8761"), QColor("#f9c86a"),
    QColor("#fcfdbf")
};

static const std::array<QColor, 11> kPiYG = {
    QColor("#8e0152"), QColor("#c51b7d"), QColor("#de77ae"), QColor("#f1b6da"),
    QColor("#fde0ef"), QColor("#f7f7f7"), QColor("#e6f5d0"), QColor("#b8e186"),
    QColor("#7fbc41"), QColor("#4d9221"), QColor("#276419")
};

QColor getColorFromColormap(float t, ColormapTypeValue type) {
    t = std::clamp(t, 0.0f, 1.0f);
    auto pick = [t](const auto& arr) {
        int idx = static_cast<int>(t * (arr.size() - 1) + 0.5f);
        return arr[std::clamp(idx, 0, (int)arr.size() - 1)];
        };
    switch (type) {
    case ColormapTypeValue::BlackWhite:
        return QColor::fromRgbF(t, t, t);
    case ColormapTypeValue::Qualitative10:
        return pick(kQualitative10);
    case ColormapTypeValue::RdYlBu:
        // Simple 3-color interpolation: red -> yellow -> blue
        if (t < 0.5f) {
            float s = t * 2.0f;
            return QColor::fromRgbF(1.0f, s, 0.0f); // red to yellow
        }
        else {
            float s = (t - 0.5f) * 2.0f;
            return QColor::fromRgbF(1.0f - s, 1.0f - s, s); // yellow to blue
        }
    case ColormapTypeValue::YlGn:
        return pick(kYlGn);
    case ColormapTypeValue::RdYlGn:
        return pick(kRdYlGn);
    case ColormapTypeValue::GnBu:
        return pick(kGnBu);
    case ColormapTypeValue::YlGnBu:
        return pick(kYlGnBu);
    case ColormapTypeValue::Spectral:
        return pick(kSpectral);
    case ColormapTypeValue::BrBg:
        return pick(kBrBg);
    case ColormapTypeValue::YlOrBr:
        return pick(kYlOrBr);
    case ColormapTypeValue::RdBu:
        return pick(kRdBu);
    case ColormapTypeValue::RdPu:
        return pick(kRdPu);
    case ColormapTypeValue::Plasma:
        return pick(kPlasma);
    case ColormapTypeValue::PuOr:
        return pick(kPuOr);
    case ColormapTypeValue::BuPu:
        return pick(kBuPu);
    case ColormapTypeValue::Reds:
        return pick(kReds);
    case ColormapTypeValue::Viridis:
        return pick(kViridis);
    case ColormapTypeValue::Q_BiGrRd:
        return pick(kQ_BiGrRd);
    case ColormapTypeValue::Magma:
        return pick(kMagma);
    case ColormapTypeValue::PiYG:
        return pick(kPiYG);
    case ColormapTypeValue::Constant:
        return QColor::fromHsl(240, 175, 159);
    default:
        return QColor::fromHsvF(t, 1.0, 1.0); // fallback
    }
}

QVector<QPair<float, float>> applyNormalization(
    const QVector<QPair<float, float>>& data,
    NormalizationType type)
{
    if (type == NormalizationType::None) return data;

    //FunctionTimer timer(Q_FUNC_INFO);
    int n = data.size();
    QVector<QPair<float, float>> result;
    result.reserve(n);

    float xMean = 0, yMean = 0, xMin = FLT_MAX, xMax = -FLT_MAX, yMin = FLT_MAX, yMax = -FLT_MAX;
    for (const auto& pt : data) {
        xMean += pt.first;
        yMean += pt.second;
        if (pt.first < xMin) xMin = pt.first;
        if (pt.first > xMax) xMax = pt.first;
        if (pt.second < yMin) yMin = pt.second;
        if (pt.second > yMax) yMax = pt.second;
    }
    xMean /= n; yMean /= n;

    float xStd = 1, yStd = 1;
    if (type == NormalizationType::ZScore) {
        float xVar = 0, yVar = 0;
        for (const auto& pt : data) {
            xVar += (pt.first - xMean) * (pt.first - xMean);
            yVar += (pt.second - yMean) * (pt.second - yMean);
        }
        xStd = std::max(std::sqrt(xVar / n), 1e-6f);
        yStd = std::max(std::sqrt(yVar / n), 1e-6f);
    }

    int jx = 1, jy = 1;
    if (type == NormalizationType::DecimalScaling) {
        float maxAbsX = 0, maxAbsY = 0;
        for (const auto& pt : data) {
            maxAbsX = std::max(maxAbsX, std::fabs(pt.first));
            maxAbsY = std::max(maxAbsY, std::fabs(pt.second));
        }
        jx = (int)std::ceil(std::log10(maxAbsX + 1e-6f));
        jy = (int)std::ceil(std::log10(maxAbsY + 1e-6f));
    }

    for (const auto& pt : data) {
        switch (type) {
        case NormalizationType::ZScore:
            result.append({ (pt.first - xMean) / xStd, (pt.second - yMean) / yStd });
            break;
        case NormalizationType::MinMax:
            result.append({ (pt.first - xMin) / (xMax - xMin + 1e-6f), (pt.second - yMin) / (yMax - yMin + 1e-6f) });
            break;
        case NormalizationType::DecimalScaling:
            result.append({ pt.first / std::pow(10, jx), pt.second / std::pow(10, jy) });
            break;
        default:
            result.append(pt);
            break;
        }
    }
    return result;
}

QVector<QPair<float, float>> applyMovingAverage(const QVector<QPair<float, float>>& data, int windowSize) {
    QVector<QPair<float, float>> smoothed;
    int n = data.size();
    if (windowSize < 1 || n < windowSize) return data;

    //FunctionTimer timer(Q_FUNC_INFO);
    smoothed.reserve(n - windowSize + 1);

    float sumX = 0, sumY = 0;
    for (int i = 0; i < windowSize; ++i) {
        sumX += data[i].first;
        sumY += data[i].second;
    }
    smoothed.append({ sumX / windowSize, sumY / windowSize });

    for (int i = windowSize; i < n; ++i) {
        sumX += data[i].first - data[i - windowSize].first;
        sumY += data[i].second - data[i - windowSize].second;
        smoothed.append({ sumX / windowSize, sumY / windowSize });
    }
    return smoothed;
}

QVector<QPair<float, float>> applySavitzkyGolay(const QVector<QPair<float, float>>& data, int windowSize) {
    QVector<QPair<float, float>> smoothed;
    if (data.size() < windowSize || windowSize % 2 == 0) return data;

    //FunctionTimer timer(Q_FUNC_INFO);
    int half = windowSize / 2;
    for (int i = half; i < data.size() - half; ++i) {
        float sumY = 0;
        for (int j = -half; j <= half; ++j) sumY += data[i + j].second;
        smoothed.append({ data[i].first, sumY / windowSize });
    }
    return smoothed;
}

QVector<QPair<float, float>> applyGaussian(const QVector<QPair<float, float>>& data, int windowSize) {
    QVector<QPair<float, float>> smoothed;
    int n = data.size();
    if (n < windowSize || windowSize % 2 == 0) return data;

    //FunctionTimer timer(Q_FUNC_INFO);
    smoothed.reserve(n - windowSize + 1);

    int half = windowSize / 2;
    QVector<float> kernel(windowSize);
    float sigma = windowSize / 6.0f;
    float sum = 0.0f;
    for (int i = 0; i < windowSize; ++i) {
        int x = i - half;
        kernel[i] = expf(-0.5f * (x * x) / (sigma * sigma));
        sum += kernel[i];
    }
    for (float& val : kernel) val /= sum;

    for (int i = half; i < n - half; ++i) {
        float y = 0.0f;
        for (int j = -half; j <= half; ++j)
            y += data[i + j].second * kernel[j + half];
        smoothed.append({ data[i].first, y });
    }
    return smoothed;
}

QVector<QPair<float, float>> applyExponentialMovingAverage(const QVector<QPair<float, float>>& data, float alpha) {
    QVector<QPair<float, float>> smoothed;
    if (data.isEmpty()) return data;

    //FunctionTimer timer(Q_FUNC_INFO);
    float ema = data[0].second;
    for (const auto& point : data) {
        ema = alpha * point.second + (1 - alpha) * ema;
        smoothed.append({ point.first, ema });
    }
    return smoothed;
}

QVector<QPair<float, float>> applyRunningMedian(const QVector<QPair<float, float>>& data, int windowSize) {
    QVector<QPair<float, float>> smoothed;
    int n = data.size();
    if (n < windowSize || windowSize % 2 == 0) return data;

   // FunctionTimer timer(Q_FUNC_INFO);
    smoothed.reserve(n - windowSize + 1);

    std::multiset<float> window;
    for (int i = 0; i < windowSize; ++i)
        window.insert(data[i].second);

    auto mid = std::next(window.begin(), windowSize / 2);
    for (int i = windowSize; i <= n; ++i) {
        smoothed.append({ data[i - windowSize / 2 - 1].first, *mid });
        if (i == n) break;
        window.erase(window.find(data[i - windowSize].second));
        window.insert(data[i].second);
        mid = std::next(window.begin(), windowSize / 2);
    }
    return smoothed;
}

QVector<QPair<float, float>> applyLinearInterpolation(const QVector<QPair<float, float>>& data, int step) {
    //FunctionTimer timer(Q_FUNC_INFO);
    QVector<QPair<float, float>> interpolated;
    for (int i = 0; i < data.size() - step; i += step) {
        interpolated.append(data[i]);
        float midX = (data[i].first + data[i + step].first) / 2.0f;
        float midY = (data[i].second + data[i + step].second) / 2.0f;
        interpolated.append({ midX, midY });
    }
    interpolated.append(data.last());
    return interpolated;
}

QVector<QPair<float, float>> applyCubicSplineApproximation(const QVector<QPair<float, float>>& data) {
    QVector<QPair<float, float>> smoothed;
    if (data.size() < 3) return data;

    //FunctionTimer timer(Q_FUNC_INFO);
    smoothed.append(data.first());
    for (int i = 1; i < data.size() - 1; ++i) {
        float x = (data[i - 1].first + data[i].first + data[i + 1].first) / 3.0f;
        float y = (data[i - 1].second + data[i].second + data[i + 1].second) / 3.0f;
        smoothed.append({ x, y });
    }
    smoothed.append(data.last());
    return smoothed;
}

QVector<QPair<float, float>> applyMinMaxSampling(const QVector<QPair<float, float>>& data, int windowSize) {
    //FunctionTimer timer(Q_FUNC_INFO);
    QVector<QPair<float, float>> result;
    for (int i = 0; i < data.size(); i += windowSize) {
        int end = std::min(i + windowSize, static_cast<int>(data.size()));
        if (end - i < 2) {
            result.append(data[i]);
            continue;
        }
        auto minIt = std::min_element(data.begin() + i, data.begin() + end, [](auto a, auto b) { return a.second < b.second; });
        auto maxIt = std::max_element(data.begin() + i, data.begin() + end, [](auto a, auto b) { return a.second < b.second; });
        result.append(*minIt);
        if (minIt != maxIt) result.append(*maxIt);
    }
    return result;
}

void sortDataAndCategories(
    const QVector<QPair<float, float>>& rawData,
    const QVector<QPair<QString, QColor>>& categoryValues,
    QVector<QPair<float, float>>& sortedData,
    QVector<QPair<QString, QColor>>& sortedCategories,
    QString axis = "X")
{
    bool alreadySorted = true;
    for (int i = 1; i < rawData.size(); ++i) {
        if ((axis == "X" && rawData[i - 1].first > rawData[i].first) ||
            (axis == "Y" && rawData[i - 1].second > rawData[i].second)) {
            alreadySorted = false;
            break;
        }
    }
    bool hasCategories = !categoryValues.isEmpty();

    if (alreadySorted) {
        sortedData = rawData;
        if (hasCategories) {
            sortedCategories = categoryValues;
        }
    }
    else {
        QVector<int> indices(rawData.size());
        std::iota(indices.begin(), indices.end(), 0);
        std::sort(indices.begin(), indices.end(), [&](int a, int b) {
            return (axis =="X")
                ? rawData[a].first < rawData[b].first
                : rawData[a].second < rawData[b].second;
            });

        sortedData.reserve(rawData.size());
        if (hasCategories) {
            sortedCategories.reserve(categoryValues.size());
        }

        for (int idx : indices) {
            sortedData.append(rawData[idx]);
            if (hasCategories && idx < categoryValues.size()) {
                sortedCategories.append(categoryValues[idx]);
            }
        }
    }
}

QVariantMap calculateStatLine(const QVector<QPair<float, float>>& normalizedData)
{
    QVariantMap statLine;
    if (normalizedData.size() >= 2) {
        const int n = normalizedData.size();
        const int n_half = (n + 1) / 2;  // Round up for odd numbers

        float sumStartX = 0, sumStartY = 0;
        float sumEndX = 0, sumEndY = 0;

        const int endStart = std::min(n_half, n);
        const int startEnd = std::max(n - n_half, 0);

        for (int i = 0; i < endStart; ++i) {
            sumStartX += normalizedData[i].first;
            sumStartY += normalizedData[i].second;
        }
        for (int i = startEnd; i < n; ++i) {
            sumEndX += normalizedData[i].first;
            sumEndY += normalizedData[i].second;
        }

        const float actualStartCount = endStart;
        const float actualEndCount = n - startEnd;

        float meanStartX = sumStartX / actualStartCount;
        float meanStartY = sumStartY / actualStartCount;
        float meanEndX = sumEndX / actualEndCount;
        float meanEndY = sumEndY / actualEndCount;

        statLine["start_x"] = meanStartX;
        statLine["start_y"] = meanStartY;
        statLine["end_x"] = meanEndX;
        statLine["end_y"] = meanEndY;
        statLine["start_label"] = QString("Mean first %1").arg(n_half);
        statLine["end_label"] = QString("Mean last %1").arg(n_half);
        statLine["color"] = "#000000";
        statLine["n_start"] = actualStartCount;
        statLine["n_end"] = actualEndCount;
    }
    return statLine;
}

QVariantList buildPayload(
    const QVector<QPair<float, float>>& smoothedData,
    const QVector<QPair<QString, QColor>>& sortedCategories)
{
    QVariantList payload;
    bool hasCategories = !sortedCategories.isEmpty();
    payload.reserve(smoothedData.size());
    for (int i = 0; i < smoothedData.size(); ++i) {
        QVariantMap entry;
        entry["x"] = smoothedData[i].first;
        entry["y"] = smoothedData[i].second;
        if (hasCategories && i < sortedCategories.size() && !sortedCategories[i].first.isEmpty()) {
            const auto& cat = sortedCategories[i];
            entry["category"] = QVariantList{ cat.second.name(), cat.first };
        }
        payload.append(entry);
    }
    return payload;
}

QVariant prepareData(
    QVector<float>& coordvalues,
    QVector<QPair<QString, QColor>>& categoryValues,
    SmoothingType smoothing,
    int smoothingParam,
    NormalizationType normalization,
    const QString& selectedDimensionX,
    const QString& selectedDimensionY,
    const QString& titleText,
    const QString& sortAxisValue
)
{
    //qDebug() << "prepareData: called";
    //qDebug() << "  coordvalues.size() =" << coordvalues.size();
    //qDebug() << "  categoryValues.size() =" << categoryValues.size();
    //qDebug() << "  smoothing =" << static_cast<int>(smoothing) << " smoothingParam =" << smoothingParam << " normalization =" << static_cast<int>(normalization);

    if (coordvalues.isEmpty() || coordvalues.size() % 2 != 0) {
        qInfo() << "prepareData: Invalid input data";
        return QVariant();
    }

    //FunctionTimer timer(Q_FUNC_INFO);

    // Convert flat coordvalues to point pairs
    QVector<QPair<float, float>> rawData;
    rawData.reserve(coordvalues.size() / 2);
    for (int i = 0; i < coordvalues.size(); i += 2) {
        rawData.append({ coordvalues[i], coordvalues[i + 1] });
    }
    //qDebug() << "prepareData: rawData.size() =" << rawData.size();
    //if (!rawData.isEmpty()) {
       // qDebug() << "prepareData: rawData sample:" << rawData.first() << (rawData.size() > 1 ? rawData[1] : QPair<float,float>());
    //}

    // Sort by X, keeping optional categoryValues in sync if they exist
    QVector<QPair<float, float>> sortedData;
    QVector<QPair<QString, QColor>> sortedCategories;
    sortDataAndCategories(rawData, categoryValues, sortedData, sortedCategories, sortAxisValue);
    //if (!sortedData.isEmpty()) {
        //qDebug() << "prepareData: sortedData sample:" << sortedData.first() << (sortedData.size() > 1 ? sortedData[1] : QPair<float,float>());
    //}

    // Apply normalization BEFORE smoothing
    //qDebug() << "prepareData: applying normalization type =" << static_cast<int>(normalization);
    QVector<QPair<float, float>> normalizedData = applyNormalization(sortedData, normalization);
    //if (!normalizedData.isEmpty()) {
        //qDebug() << "prepareData: normalizedData sample:" << normalizedData.first() << (normalizedData.size() > 1 ? normalizedData[1] : QPair<float,float>());
    //}

    // Calculate statLine
    QVariantMap statLine = calculateStatLine(normalizedData);
    //if (!statLine.isEmpty()) {
        //qDebug() << "prepareData: statLine =" << statLine;
    //}

    // Apply smoothing to normalized data
    //qDebug() << "prepareData: applying smoothing type =" << static_cast<int>(smoothing) << " param =" << smoothingParam;
    QVector<QPair<float, float>> smoothedData;
    switch (smoothing) {
    case SmoothingType::MovingAverage:
        smoothedData = applyMovingAverage(normalizedData, smoothingParam);
        break;
    case SmoothingType::SavitzkyGolay:
        smoothedData = applySavitzkyGolay(normalizedData, smoothingParam);
        break;
    case SmoothingType::Gaussian:
        smoothedData = applyGaussian(normalizedData, smoothingParam);
        break;
    case SmoothingType::ExponentialMovingAverage:
        smoothedData = applyExponentialMovingAverage(normalizedData);
        break;
    case SmoothingType::CubicSpline:
        smoothedData = applyCubicSplineApproximation(normalizedData);
        break;
    case SmoothingType::LinearInterpolation:
        smoothedData = applyLinearInterpolation(normalizedData, smoothingParam);
        break;
    case SmoothingType::MinMaxSampling:
        smoothedData = applyMinMaxSampling(normalizedData, smoothingParam);
        break;
    case SmoothingType::RunningMedian:
        smoothedData = applyRunningMedian(normalizedData, smoothingParam);
        break;
    case SmoothingType::None:
    default:
        smoothedData = normalizedData;
        break;
    }
    if (!smoothedData.isEmpty()) {
        //qDebug() << "prepareData: smoothedData sample:" << smoothedData.first() << (smoothedData.size() > 1 ? smoothedData[1] : QPair<float,float>());
    }

    // Convert back to QVariantList with optional categories
    QVariantList payload = buildPayload(smoothedData, sortedCategories);
   // qDebug() << "prepareData: payload.size() =" << payload.size();

    QVariantMap root;
    root["data"] = payload;
    if (!statLine.isEmpty()) {
        root["statLine"] = statLine;
    }
    root["lineColor"] = "#1f77b4";

    root["title"] = titleText.isEmpty()
        ? QString("%1 vs %2").arg(selectedDimensionX, selectedDimensionY)
        : titleText;

    root["xAxisName"] = selectedDimensionX;
    root["yAxisName"] = selectedDimensionY;

    //qDebug() << "prepareData: root keys =" << root.keys();

    return root;
}

ColormapTypeValue getColorMapFromString(const QString& colormapselectedVal)
{
    static const QHash<QString, ColormapTypeValue> colormapMap = {
        { "Black to white", ColormapTypeValue::BlackWhite },
        { "RdYlBu",         ColormapTypeValue::RdYlBu },
        { "qualitative",    ColormapTypeValue::Qualitative10 },
        { "YlGn",           ColormapTypeValue::YlGn },
        { "RdYlGn",         ColormapTypeValue::RdYlGn },
        { "GnBu",           ColormapTypeValue::GnBu },
        { "YlGnBu",         ColormapTypeValue::YlGnBu },
        { "Spectral",       ColormapTypeValue::Spectral },
        { "BrBG",           ColormapTypeValue::BrBg },
        { "YlOrBr",         ColormapTypeValue::YlOrBr },
        { "RdBu",           ColormapTypeValue::RdBu },
        { "RdPu",           ColormapTypeValue::RdPu },
        { "Plasma",         ColormapTypeValue::Plasma },
        { "PuOr",           ColormapTypeValue::PuOr },
        { "BuPu",           ColormapTypeValue::BuPu },
        { "Reds",           ColormapTypeValue::Reds },
        { "Viridis",        ColormapTypeValue::Viridis },
        { "Q_BlGrRd",       ColormapTypeValue::Q_BiGrRd },
        { "Magma",          ColormapTypeValue::Magma },
        { "PiYG",           ColormapTypeValue::PiYG }
    };
    return colormapMap.value(colormapselectedVal, ColormapTypeValue::Constant);
}

void extractLinePlotData(
    const mv::Dataset<Points>& currentDataSet,
    int dimensionXIndex,
    int dimensionYIndex,
    QString colorDatasetID,
    int colorPointDatasetDimensionIndex,
    QString colormapSelectedVal,
    QVector<float>& coordvalues,
    QVector<QPair<QString, QColor>>& categoryValues
) {
    coordvalues.clear();
    categoryValues.clear();
    auto colorDataset= mv::data().getDataset(colorDatasetID);
    if (!currentDataSet.isValid() || dimensionXIndex < 0 || dimensionYIndex < 0)
        return;

    const auto numPoints = currentDataSet->getNumPoints();
    const auto numDimensions = currentDataSet->getNumDimensions();

    coordvalues.reserve(numPoints * 2);
    for (unsigned int i = 0; i < numPoints; ++i) {
        float xValue = currentDataSet->getValueAt(i * numDimensions + dimensionXIndex);
        float yValue = currentDataSet->getValueAt(i * numDimensions + dimensionYIndex);
        coordvalues.push_back(xValue);
        coordvalues.push_back(yValue);
    }

    categoryValues.reserve(numPoints);
    for (unsigned int i = 0; i < numPoints; ++i) {
        categoryValues.push_back({ QString(), QColor() });
    }

    if (colorDataset.isValid()) {
        if (colorDataset->getDataType() == ClusterType)
        {
            Dataset<Clusters> clusterDataset = mv::data().getDataset(colorDatasetID);
            if (clusterDataset.isValid())
            {
                auto clusters = clusterDataset->getClusters();
                for (const auto& cluster : clusters) {
                    auto clusterName = cluster.getName();
                    auto clusterColor = cluster.getColor();
                    auto clusterIndices = cluster.getIndices();
                    if (clusterName.isEmpty() || !clusterColor.isValid() || clusterIndices.empty()) continue;
                    for (const auto& index : clusterIndices) {
                        if (index < numPoints) {
                            categoryValues[index] = { clusterName, clusterColor };
                        }
                    }
                }
            }
            else
            {
                qInfo() << "extractLinePlotData: Invalid cluster dataset:" << colorDatasetID;
            }
        }
        else if (colorDataset->getDataType() == PointType) {
            Dataset<Points> pointDataset = mv::data().getDataset(colorDatasetID);
            if (pointDataset.isValid())
            {
                int numofPoints = pointDataset->getNumPoints();
                if(numofPoints>0)
                { 
                    if (colorPointDatasetDimensionIndex >= 0)
                    {
                        ColormapTypeValue colormap= getColorMapFromString(colormapSelectedVal);
                        
                        
                        std::vector<float> pointsValues(numofPoints);
                        pointDataset->extractDataForDimension(pointsValues, colorPointDatasetDimensionIndex);
                        float minValue = *std::min_element(pointsValues.begin(), pointsValues.end());
                        float maxValue = *std::max_element(pointsValues.begin(), pointsValues.end());
                        float range = maxValue - minValue;
                        if (range == 0) range = 1;
                        

                        for (unsigned int i = 0; i < numofPoints; ++i) {
                            float value = pointsValues[i];
                            float normalizedValue = (value - minValue) / range;
                            QColor color = getColorFromColormap(normalizedValue, colormap);
                            if (i < numPoints) {
                                categoryValues[i] = { QString::number(value), color };
                            }
                        }
                    }
                    else
                    {
                        qInfo() << "extractLinePlotData: Invalid color point dataset dimension index:" << colorPointDatasetDimensionIndex;
                    }
                }
                else
                {
                    qInfo() << "extractLinePlotData: No points in dataset:" << colorDatasetID;
                }
            }
            else
            {
                qInfo() << "extractLinePlotData: Invalid point dataset:" << colorDatasetID;
            }


        } 
        else {
            qInfo() << "extractLinePlotData: Unsupported color dataset type:" << colorDataset->getDataType().getTypeString();
        }

    
    }
}
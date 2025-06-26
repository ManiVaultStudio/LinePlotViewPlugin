#include "LinePlotUtils.h"
#include <QDebug>
#include <float.h>
#include <QColor>
#include <QString>

// includes for mv::Dataset, Points, Clusters, NormalizationType, SmoothingType
#include "PointData/PointData.h"
#include "ClusterData/ClusterData.h"
#include "LinePlotViewPlugin.h" // for NormalizationType, SmoothingType

using namespace mv;

FunctionTimer::FunctionTimer(const QString& functionName)
    : _functionName(functionName)
{
    _timer.start();
}
FunctionTimer::~FunctionTimer()
{
    qDebug() << _functionName << "took"
             << _timer.elapsed() / 1000.0 << "seconds";
}

QVector<QPair<float, float>> applyNormalization(
    const QVector<QPair<float, float>>& data,
    NormalizationType type)
{
    if (type == NormalizationType::None) return data;

    FunctionTimer timer(Q_FUNC_INFO);
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

    FunctionTimer timer(Q_FUNC_INFO);
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

    FunctionTimer timer(Q_FUNC_INFO);
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

    FunctionTimer timer(Q_FUNC_INFO);
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

    FunctionTimer timer(Q_FUNC_INFO);
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

    FunctionTimer timer(Q_FUNC_INFO);
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
    FunctionTimer timer(Q_FUNC_INFO);
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

    FunctionTimer timer(Q_FUNC_INFO);
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
    FunctionTimer timer(Q_FUNC_INFO);
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
    QVector<QPair<QString, QColor>>& sortedCategories)
{
    bool alreadySorted = true;
    for (int i = 1; i < rawData.size(); ++i) {
        if (rawData[i - 1].first > rawData[i].first) {
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
    } else {
        QVector<int> indices(rawData.size());
        std::iota(indices.begin(), indices.end(), 0);
        std::sort(indices.begin(), indices.end(), [&](int a, int b) {
            return rawData[a].first < rawData[b].first;
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
    const QString& titleText
)
{
    qDebug() << "prepareData: called";
    qDebug() << "  coordvalues.size() =" << coordvalues.size();
    qDebug() << "  categoryValues.size() =" << categoryValues.size();
    qDebug() << "  smoothing =" << static_cast<int>(smoothing) << " smoothingParam =" << smoothingParam << " normalization =" << static_cast<int>(normalization);

    if (coordvalues.isEmpty() || coordvalues.size() % 2 != 0) {
        qDebug() << "prepareData: Invalid input data";
        return QVariant();
    }

    FunctionTimer timer(Q_FUNC_INFO);

    // Convert flat coordvalues to point pairs
    QVector<QPair<float, float>> rawData;
    rawData.reserve(coordvalues.size() / 2);
    for (int i = 0; i < coordvalues.size(); i += 2) {
        rawData.append({ coordvalues[i], coordvalues[i + 1] });
    }
    qDebug() << "prepareData: rawData.size() =" << rawData.size();
    if (!rawData.isEmpty()) {
        qDebug() << "prepareData: rawData sample:" << rawData.first() << (rawData.size() > 1 ? rawData[1] : QPair<float,float>());
    }

    // Sort by X, keeping optional categoryValues in sync if they exist
    QVector<QPair<float, float>> sortedData;
    QVector<QPair<QString, QColor>> sortedCategories;
    sortDataAndCategories(rawData, categoryValues, sortedData, sortedCategories);
    if (!sortedData.isEmpty()) {
        qDebug() << "prepareData: sortedData sample:" << sortedData.first() << (sortedData.size() > 1 ? sortedData[1] : QPair<float,float>());
    }

    // Apply normalization BEFORE smoothing
    qDebug() << "prepareData: applying normalization type =" << static_cast<int>(normalization);
    QVector<QPair<float, float>> normalizedData = applyNormalization(sortedData, normalization);
    if (!normalizedData.isEmpty()) {
        qDebug() << "prepareData: normalizedData sample:" << normalizedData.first() << (normalizedData.size() > 1 ? normalizedData[1] : QPair<float,float>());
    }

    // Calculate statLine
    QVariantMap statLine = calculateStatLine(normalizedData);
    if (!statLine.isEmpty()) {
        qDebug() << "prepareData: statLine =" << statLine;
    }

    // Apply smoothing to normalized data
    qDebug() << "prepareData: applying smoothing type =" << static_cast<int>(smoothing) << " param =" << smoothingParam;
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
        qDebug() << "prepareData: smoothedData sample:" << smoothedData.first() << (smoothedData.size() > 1 ? smoothedData[1] : QPair<float,float>());
    }

    // Convert back to QVariantList with optional categories
    QVariantList payload = buildPayload(smoothedData, sortedCategories);
    qDebug() << "prepareData: payload.size() =" << payload.size();

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

    qDebug() << "prepareData: root keys =" << root.keys();

    return root;
}

void extractLinePlotData(
    const mv::Dataset<Points>& currentDataSet,
    int dimensionXIndex,
    int dimensionYIndex,
    const mv::Dataset<Clusters>& clusterDataset,
    QVector<float>& coordvalues,
    QVector<QPair<QString, QColor>>& categoryValues
) {
    coordvalues.clear();
    categoryValues.clear();

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

    if (clusterDataset.isValid()) {
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
}
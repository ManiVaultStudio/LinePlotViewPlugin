#pragma once

#include <QVector>
#include <QPair>
#include <QString>
#include <QElapsedTimer>
#include <QColor>
#include <QVariantMap>
#include <QVariantList>
#include <cmath>
#include <algorithm>
#include <set>

// includes for mv::Dataset, Points, Clusters, NormalizationType, SmoothingType
#include "PointData/PointData.h"
#include "ClusterData/ClusterData.h"
#include "LinePlotViewPlugin.h" // for NormalizationType, SmoothingType
#include  "ColorUtils.h" // for QColor utilities

using namespace mv;

// Utility timer for profiling function durations
class FunctionTimer {
public:
    FunctionTimer(const QString& functionName);
    ~FunctionTimer();
private:
    QString _functionName;
    QElapsedTimer _timer;
};

// Normalization and smoothing utilities
QVector<QPair<float, float>> applyNormalization(
    const QVector<QPair<float, float>>& data,
    NormalizationType type);

QVector<QPair<float, float>> applyMovingAverage(const QVector<QPair<float, float>>& data, int windowSize);
QVector<QPair<float, float>> applySavitzkyGolay(const QVector<QPair<float, float>>& data, int windowSize);
QVector<QPair<float, float>> applyGaussian(const QVector<QPair<float, float>>& data, int windowSize);
QVector<QPair<float, float>> applyExponentialMovingAverage(const QVector<QPair<float, float>>& data, float alpha = 0.2f);
QVector<QPair<float, float>> applyRunningMedian(const QVector<QPair<float, float>>& data, int windowSize);
QVector<QPair<float, float>> applyLinearInterpolation(const QVector<QPair<float, float>>& data, int step);
QVector<QPair<float, float>> applyCubicSplineApproximation(const QVector<QPair<float, float>>& data);
QVector<QPair<float, float>> applyMinMaxSampling(const QVector<QPair<float, float>>& data, int windowSize);

//  utility for sorting and category sync
void sortDataAndCategories(
    const QVector<QPair<float, float>>& rawData,
    const QVector<QPair<QString, QColor>>& categoryValues,
    QVector<QPair<float, float>>& sortedData,
    QVector<QPair<QString, QColor>>& sortedCategories);

//  statLine calculation utility
QVariantMap calculateStatLine(const QVector<QPair<float, float>>& normalizedData);

//  payload construction utility
QVariantList buildPayload(
    const QVector<QPair<float, float>>& smoothedData,
    const QVector<QPair<QString, QColor>>& sortedCategories);

//  general-purpose data preparation utility
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
);

// Utility to extract coordvalues and categoryValues from dataset and cluster info
void extractLinePlotData(
    const mv::Dataset<Points>& currentDataSet,
    int dimensionXIndex,
    int dimensionYIndex,
    QString colorDatasetID,
    int colorPointDatasetDimensionIndex,
    QString colormapSelectedVal, 
    float minValue,
    float maxValue,
    QVector<float>& coordvalues,
    QVector<QPair<QString, QColor>>& categoryValues
);
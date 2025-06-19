#include "LinePlotViewPlugin.h"

#include "ChartWidget.h"

#include <DatasetsMimeData.h>

#include <vector>
#include <random>
#include <algorithm>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <QMimeData>
#include <QDebug>

Q_PLUGIN_METADATA(IID "studio.manivault.LinePlotViewPlugin")

using namespace mv;


QVector<QPair<float, float>> applyNormalization(
    const QVector<QPair<float, float>>& data,
    NormalizationType type)
{
    if (type == NormalizationType::None) return data;

    QVector<QPair<float, float>> result;
    QVector<float> xVals, yVals;
    for (const auto& pt : data) {
        xVals.append(pt.first);
        yVals.append(pt.second);
    }

    float xMean = 0, xStd = 1, xMin = 0, xMax = 1;
    float yMean = 0, yStd = 1, yMin = 0, yMax = 1;
    int n = data.size();

    if (type == NormalizationType::ZScore || type == NormalizationType::MinMax) {
        for (int i = 0; i < n; ++i) {
            xMean += xVals[i];
            yMean += yVals[i];
        }
        xMean /= n;
        yMean /= n;

        xMin = *std::min_element(xVals.begin(), xVals.end());
        xMax = *std::max_element(xVals.begin(), xVals.end());
        yMin = *std::min_element(yVals.begin(), yVals.end());
        yMax = *std::max_element(yVals.begin(), yVals.end());

        if (type == NormalizationType::ZScore) {
            xStd = std::sqrt(std::accumulate(xVals.begin(), xVals.end(), 0.0f,
                [xMean](float acc, float v) { return acc + (v - xMean) * (v - xMean); }) / n);
            yStd = std::sqrt(std::accumulate(yVals.begin(), yVals.end(), 0.0f,
                [yMean](float acc, float v) { return acc + (v - yMean) * (v - yMean); }) / n);
            xStd = std::max(xStd, 1e-6f);
            yStd = std::max(yStd, 1e-6f);
        }
    }

    for (int i = 0; i < n; ++i) {
        float x = xVals[i], y = yVals[i];
        switch (type) {
        case NormalizationType::ZScore:
            result.append({ (x - xMean) / xStd, (y - yMean) / yStd });
            break;
        case NormalizationType::MinMax:
            result.append({ (x - xMin) / (xMax - xMin + 1e-6f), (y - yMin) / (yMax - yMin + 1e-6f) });
            break;
        case NormalizationType::DecimalScaling:
        {
            int jx = (int)std::ceil(std::log10(std::fabs(*std::max_element(xVals.begin(), xVals.end(),
                [](float a, float b) { return std::fabs(a) < std::fabs(b); })) + 1e-6f));
            int jy = (int)std::ceil(std::log10(std::fabs(*std::max_element(yVals.begin(), yVals.end(),
                [](float a, float b) { return std::fabs(a) < std::fabs(b); })) + 1e-6f));
            result.append({ x / std::pow(10, jx), y / std::pow(10, jy) });
            break;
        }
        default:
            result.append({ x, y });
            break;
        }
    }
    return result;
}

QVector<QPair<float, float>> applyMovingAverage(const QVector<QPair<float, float>>& data, int windowSize) {
    QVector<QPair<float, float>> smoothed;
    if (windowSize < 1 || data.size() < windowSize) return data;
    for (int i = 0; i <= data.size() - windowSize; ++i) {
        float sumX = 0, sumY = 0;
        for (int j = 0; j < windowSize; ++j) {
            sumX += data[i + j].first;
            sumY += data[i + j].second;
        }
        smoothed.append({ sumX / windowSize, sumY / windowSize });
    }
    return smoothed;
}

QVector<QPair<float, float>> applySavitzkyGolay(const QVector<QPair<float, float>>& data, int windowSize) {
    QVector<QPair<float, float>> smoothed;
    if (data.size() < windowSize || windowSize % 2 == 0) return data;
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
    if (data.size() < windowSize || windowSize % 2 == 0) return data;
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

    for (int i = half; i < data.size() - half; ++i) {
        float y = 0.0f;
        for (int j = -half; j <= half; ++j)
            y += data[i + j].second * kernel[j + half];
        smoothed.append({ data[i].first, y });
    }
    return smoothed;
}

QVector<QPair<float, float>> applyExponentialMovingAverage(const QVector<QPair<float, float>>& data, float alpha = 0.2f) {
    QVector<QPair<float, float>> smoothed;
    if (data.isEmpty()) return data;
    float ema = data[0].second;
    for (const auto& point : data) {
        ema = alpha * point.second + (1 - alpha) * ema;
        smoothed.append({ point.first, ema });
    }
    return smoothed;
}

QVector<QPair<float, float>> applyRunningMedian(const QVector<QPair<float, float>>& data, int windowSize) {
    QVector<QPair<float, float>> smoothed;
    if (data.size() < windowSize || windowSize % 2 == 0) return data;
    int half = windowSize / 2;
    for (int i = half; i < data.size() - half; ++i) {
        QVector<float> window;
        for (int j = -half; j <= half; ++j)
            window.append(data[i + j].second);
        std::sort(window.begin(), window.end());
        smoothed.append({ data[i].first, window[half] });
    }
    return smoothed;
}

QVector<QPair<float, float>> applyLinearInterpolation(const QVector<QPair<float, float>>& data, int step) {
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


LinePlotViewPlugin::LinePlotViewPlugin(const PluginFactory* factory) :
    ViewPlugin(factory),
    _chartWidget(nullptr),
    _dropWidget(nullptr),
    _settingsAction(*this),
    _currentDataSet(nullptr)
{
    getLearningCenterAction().addVideos(QStringList({ "Practitioner", "Developer" }));
}

void LinePlotViewPlugin::init()
{
    getWidget().setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));

    auto layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);

    _chartWidget = new ChartWidget(this);
    _chartWidget->setPage(":line_chart/line_chart.html", "qrc:/line_chart/");
    auto settings = new QHBoxLayout();
    settings->setContentsMargins(0, 0, 0, 0);
    settings->setSpacing(0);
    settings->addWidget(_settingsAction.getDatasetOptionsHolder().createWidget(&getWidget()));
    settings->addWidget(_settingsAction.getChartOptionsHolder().createCollapsedWidget(&getWidget()));
    layout->addLayout(settings);
    layout->addWidget(_chartWidget,1);

    getWidget().setLayout(layout);

    _dropWidget = new DropWidget(_chartWidget);
    _dropWidget->setDropIndicatorWidget(new DropWidget::DropIndicatorWidget(&getWidget(), "No data loaded", "Drag the LinePlotViewData in this view"));

    _dropWidget->initialize([this](const QMimeData* mimeData) -> DropWidget::DropRegions {

        DropWidget::DropRegions dropRegions;

        const auto datasetsMimeData = dynamic_cast<const DatasetsMimeData*>(mimeData);

        if (datasetsMimeData == nullptr)
            return dropRegions;

        if (datasetsMimeData->getDatasets().count() > 1)
            return dropRegions;

        const auto dataset = datasetsMimeData->getDatasets().first();
        const auto datasetGuiName = dataset->text();
        const auto datasetId = dataset->getId();
        const auto dataType = dataset->getDataType();
        const auto dataTypes = DataTypes({ PointType });

        if (dataTypes.contains(dataType)) {

            if (datasetId == getCurrentDataSetID()) {
                dropRegions << new DropWidget::DropRegion(this, "Warning", "Data already loaded", "exclamation-circle", false);
            }
            else {
                auto candidateDataset = mv::data().getDataset<Points>(datasetId);

                dropRegions << new DropWidget::DropRegion(this, "Points", QString("Visualize %1 as parallel coordinates").arg(datasetGuiName), "map-marker-alt", true, [this, candidateDataset]() {
                    loadData({ candidateDataset });
                    _dropWidget->setShowDropIndicator(false);
                    });

            }
        }
        else {
            dropRegions << new DropWidget::DropRegion(this, "Incompatible data", "This type of data is not supported", "exclamation-circle", false);
        }

        return dropRegions;
        });

    connect(&_currentDataSet, &Dataset<Points>::dataChanged, this, &LinePlotViewPlugin::convertDataAndUpdateChart);

    const auto pointDatasetChanged = [this]() -> void {
        auto dataset = _settingsAction.getDatasetOptionsHolder().getPointDatasetAction().getCurrentDataset();
        if (dataset.isValid()) {

            _currentDataSet = dataset;
            _settingsAction.getDatasetOptionsHolder().getDataDimensionXSelectionAction().setPointsDataset(_currentDataSet);
            _settingsAction.getDatasetOptionsHolder().getDataDimensionYSelectionAction().setPointsDataset(_currentDataSet);
            if (_currentDataSet->getNumDimensions() >= 2)
            {
                _settingsAction.getDatasetOptionsHolder().getDataDimensionXSelectionAction().setCurrentDimensionIndex(0);
                _settingsAction.getDatasetOptionsHolder().getDataDimensionYSelectionAction().setCurrentDimensionIndex(1);
            }
            else if (_currentDataSet->getNumDimensions() == 1)
            {
                _settingsAction.getDatasetOptionsHolder().getDataDimensionXSelectionAction().setCurrentDimensionIndex(0);
                _settingsAction.getDatasetOptionsHolder().getDataDimensionYSelectionAction().setCurrentDimensionIndex(-1);
            }
            else
            {
                _settingsAction.getDatasetOptionsHolder().getDataDimensionXSelectionAction().setCurrentDimensionIndex(-1);
                _settingsAction.getDatasetOptionsHolder().getDataDimensionYSelectionAction().setCurrentDimensionIndex(-1);
            }
            auto children = _currentDataSet->getChildren();
            Datasets clusterDatasets;
            for (const auto& child : children)
            {
                if (child->getDataType() == ClusterType) {
                    clusterDatasets.push_back(child);
                }
            }
            auto parent = _currentDataSet->getParent();
            if (parent.isValid())
            {
                auto others = parent->getChildren();
                for (const auto& other : others)
                {
                    if (other->getDataType() == ClusterType) {
                        clusterDatasets.push_back(other);
                    }
                }
            }
            
            _settingsAction.getChartOptionsHolder().getSmoothingWindowAction().setMaximum(_currentDataSet->getNumPoints() / 2);
            _settingsAction.getChartOptionsHolder().getSmoothingWindowAction().setMinimum(2);
            _settingsAction.getChartOptionsHolder().getSmoothingWindowAction().setValue(
                std::max(2, static_cast<int>(_currentDataSet->getNumPoints()) / 10)
            );

            _settingsAction.getDatasetOptionsHolder().getClusterDatasetAction().setDatasets(clusterDatasets);
            if (clusterDatasets.isEmpty())
            {
                _settingsAction.getDatasetOptionsHolder().getClusterDatasetAction().setCurrentIndex(-1);
            }
            else
            {
                _settingsAction.getDatasetOptionsHolder().getClusterDatasetAction().setCurrentIndex(0);
            }

            
        }
        else
        {
            _currentDataSet = Dataset<Points>();
            _settingsAction.getDatasetOptionsHolder().getDataDimensionXSelectionAction().setPointsDataset(_currentDataSet);
            _settingsAction.getDatasetOptionsHolder().getDataDimensionYSelectionAction().setPointsDataset(_currentDataSet);
            _settingsAction.getDatasetOptionsHolder().getDataDimensionXSelectionAction().setCurrentDimensionIndex(-1);
            _settingsAction.getDatasetOptionsHolder().getDataDimensionYSelectionAction().setCurrentDimensionIndex(-1);
            _settingsAction.getDatasetOptionsHolder().getClusterDatasetAction().setDatasets({});
            
        }
        events().notifyDatasetDataChanged(_currentDataSet);
        };
    connect(&_settingsAction.getDatasetOptionsHolder().getPointDatasetAction(), &DatasetPickerAction::currentIndexChanged, this, pointDatasetChanged);

    const auto variantOptionChanged = [this]() -> void {
        if (_settingsAction.getDatasetOptionsHolder().getDataFromVariantAction().isChecked())
        {
            updateChart();
        }
        else
        {
            convertDataAndUpdateChart();
        }
        };
    connect(&_settingsAction.getDatasetOptionsHolder().getDataFromVariantAction(), &ToggleAction::toggled, this, variantOptionChanged);

    const auto variantChanged = [this]() -> void {
        if (_settingsAction.getDatasetOptionsHolder().getDataFromVariantAction().isChecked())
        {
            updateChart();
        }
        else
        {
            convertDataAndUpdateChart();
        }
        };
    connect(&_settingsAction.getDatasetOptionsHolder().getLineDataVariantAction(), &VariantAction::variantChanged, this, variantChanged);

    connect(&_settingsAction.getDatasetOptionsHolder().getDataDimensionXSelectionAction(), &DimensionPickerAction::currentDimensionIndexChanged, this, &LinePlotViewPlugin::convertDataAndUpdateChart);
    connect(&_settingsAction.getDatasetOptionsHolder().getDataDimensionYSelectionAction(), &DimensionPickerAction::currentDimensionIndexChanged, this, &LinePlotViewPlugin::convertDataAndUpdateChart);
    connect(&_settingsAction.getDatasetOptionsHolder().getClusterDatasetAction(), &DatasetPickerAction::currentIndexChanged, this, &LinePlotViewPlugin::convertDataAndUpdateChart);
    connect(&_settingsAction.getChartOptionsHolder().getNormalizationTypeAction(), &OptionAction::currentTextChanged, this, &LinePlotViewPlugin::convertDataAndUpdateChart);

     connect(&_settingsAction.getChartOptionsHolder().getSmoothingTypeAction(), &OptionAction::currentTextChanged, this, &LinePlotViewPlugin::convertDataAndUpdateChart);
    connect(&_settingsAction.getChartOptionsHolder().getSmoothingWindowAction(), &IntegralAction::valueChanged, this, &LinePlotViewPlugin::convertDataAndUpdateChart);
    
    

    connect(&_chartWidget->getCommunicationObject(), &ChartCommObject::passSelectionToCore, this, &LinePlotViewPlugin::publishSelection);

}

void LinePlotViewPlugin::loadData(const mv::Datasets& datasets)
{
    if (datasets.isEmpty())
        return;

    qDebug() << "LinePlotViewPlugin::loadData: Load data set from ManiVault core";
    if (!datasets.first().isValid()) {
        _settingsAction.getDatasetOptionsHolder().getPointDatasetAction().setCurrentIndex(-1);
        qDebug() << "LinePlotViewPlugin::loadData: Invalid dataset provided";
        return;
    }
    else
    {
        _settingsAction.getDatasetOptionsHolder().getPointDatasetAction().setCurrentDataset(datasets.first());
    }
    
}

void LinePlotViewPlugin::updateChart()
{
    auto variant = _settingsAction.getDatasetOptionsHolder().getLineDataVariantAction().getVariant();
    _currentDataSetMap.clear();
    _currentDataSetMap = variant.toMap();
    emit _chartWidget->getCommunicationObject().qt_js_setDataAndPlotInJS(_currentDataSetMap);
}


void LinePlotViewPlugin::convertDataAndUpdateChart()
{
    QVariant root;
    if (!_currentDataSet.isValid())
    {
        qDebug() << "LinePlotViewPlugin::convertDataAndUpdateChart: No valid dataset to convert";
    }
    else
    {
        qDebug() << "LinePlotViewPlugin::convertDataAndUpdateChart: Prepare payload";
        QVector<float> coordvalues;
        

        const auto numPoints = _currentDataSet->getNumPoints();
        const auto numDimensions = _currentDataSet->getNumDimensions();
        const auto dimensionNames = _currentDataSet->getDimensionNames();

        if (numPoints == 0 || numDimensions < 2) {
            qDebug() << "LinePlotViewPlugin::convertDataAndUpdateChart: No valid data to convert";
            return;
        }
        auto selectedDimensionX = _settingsAction.getDatasetOptionsHolder().getDataDimensionXSelectionAction().getCurrentDimensionName();
        auto selectedDimensionY = _settingsAction.getDatasetOptionsHolder().getDataDimensionYSelectionAction().getCurrentDimensionName();

        int dimensionXIndex = -1;
        int dimensionYIndex = -1;
        if (selectedDimensionX.isEmpty() || selectedDimensionY.isEmpty()) {
            qDebug() << "LinePlotViewPlugin::convertDataAndUpdateChart: No dimensions selected for X or Y axis";
            return;
        }
        for (int i = 0; i < numDimensions; ++i) {
            if (dimensionNames[i] == selectedDimensionX) {
                dimensionXIndex = i;
            }
            if (dimensionNames[i] == selectedDimensionY) {
                dimensionYIndex = i;
            }
        }
        if (dimensionXIndex == -1 || dimensionYIndex == -1) {
            qDebug() << "LinePlotViewPlugin::convertDataAndUpdateChart: Selected dimensions not found in dataset";
            return;
        }
        qDebug() << "LinePlotViewPlugin::convertDataAndUpdateChart: Selected dimensions - X:" << selectedDimensionX << "(Index:" << dimensionXIndex << "), Y:" << selectedDimensionY << "(Index:" << dimensionYIndex << ")";

        coordvalues.reserve(numPoints * 2);
        //categoryValues.reserve(numPoints);
        qDebug() << "LinePlotViewPlugin::convertDataAndUpdateChart: Number of points:" << numPoints << ", Number of dimensions:" << 2;
        //points are stored in row major order std vector float as points and dimensions in the dataset we can get value by getvalue at index
        for (unsigned int i = 0; i < numPoints; ++i) {
            float xValue = _currentDataSet->getValueAt(i * numDimensions + dimensionXIndex);
            float yValue = _currentDataSet->getValueAt(i * numDimensions + dimensionYIndex);
            coordvalues.push_back(xValue);
            coordvalues.push_back(yValue);

        }
        //set the category values as null for now, we can add categories later
        QVector<QPair<QString, QColor>> categoryValues;
        Dataset<Clusters> clusterDataset = _settingsAction.getDatasetOptionsHolder().getClusterDatasetAction().getCurrentDataset();
        if (clusterDataset.isValid())
        {
            categoryValues.reserve(numPoints);
            //fill in temp values in categoryValues
            for (unsigned int i = 0; i < numPoints; ++i) {
                categoryValues.push_back({ QString::number(i), QColor(Qt::black) });
            }
            auto clusters = clusterDataset->getClusters();
            for (const auto& cluster : clusters)
            {
                auto clusterName = cluster.getName();
                auto clusterColor = cluster.getColor();
                auto clusterIndices = cluster.getIndices();
                if (clusterName.isEmpty() || clusterColor.isValid() == false || clusterIndices.empty()) {
                    continue; // Skip invalid clusters
                }
                for (const auto& index : clusterIndices) {
                    if (index < numPoints) {
                        categoryValues[index] = { clusterName, clusterColor };
                    }
                }
            }

        }
        SmoothingType smoothing = SmoothingType::None;
        const QString smoothingText = _settingsAction.getChartOptionsHolder().getSmoothingTypeAction().getCurrentText();
        if (smoothingText == "None") {
            smoothing = SmoothingType::None;
        }
        else if (smoothingText == "Moving Average") {
            smoothing = SmoothingType::MovingAverage;
        }
        else if (smoothingText == "Savitzky-Golay") {
            smoothing = SmoothingType::SavitzkyGolay;
        }
        else if (smoothingText == "Gaussian") {
            smoothing = SmoothingType::Gaussian;
        }
        else if (smoothingText == "Exponential Moving Average") {
            smoothing = SmoothingType::ExponentialMovingAverage;
        }
        else if (smoothingText == "Cubic Spline") {
            smoothing = SmoothingType::CubicSpline;
        }
        else if (smoothingText == "Linear Interpolation") {
            smoothing = SmoothingType::LinearInterpolation;
        }
        else if (smoothingText == "Min-Max Sampling") {
            smoothing = SmoothingType::MinMaxSampling;
        }
        else if (smoothingText == "Running Median") {
            smoothing = SmoothingType::RunningMedian;
        }
        else {
            qDebug() << "LinePlotViewPlugin::convertDataAndUpdateChart: Unknown smoothing type, defaulting to None";
            smoothing = SmoothingType::None;
        }

        int windowSize = _settingsAction.getChartOptionsHolder().getSmoothingWindowAction().getValue();
        NormalizationType normalization = NormalizationType::None;
        const QString normalizationText = _settingsAction.getChartOptionsHolder().getNormalizationTypeAction().getCurrentText();
        if (normalizationText == "None") {
            normalization = NormalizationType::None;
        }
        else if (normalizationText == "Z-Score") {
            normalization = NormalizationType::ZScore;
        }
        else if (normalizationText == "Min-Max") {
            normalization = NormalizationType::MinMax;
        }
        else if (normalizationText == "DecimalScaling") {
            normalization = NormalizationType::DecimalScaling;
        }
        else {
            qDebug() << "LinePlotViewPlugin::convertDataAndUpdateChart: Unknown normalization type, defaulting to None";
            normalization = NormalizationType::None;
        }

        root = prepareData(coordvalues, categoryValues, smoothing, windowSize, normalization);
        _currentDataSetMap.clear();
        _currentDataSetMap = root.toMap();
        qDebug() << "LinePlotViewPlugin::convertDataAndUpdateChart: Send data from Qt cpp to D3 js";
  
    }
    emit _chartWidget->getCommunicationObject().qt_js_setDataAndPlotInJS(_currentDataSetMap);

}

void LinePlotViewPlugin::publishSelection(const std::vector<unsigned int>& selectedIDs)
{
    auto selectionSet = _currentDataSet->getSelection<Points>();
    auto& selectionIndices = selectionSet->indices;

    selectionIndices.clear();
    selectionIndices.reserve(_currentDataSet->getNumPoints());
    for (const auto id : selectedIDs) {
        selectionIndices.push_back(id);
    }

    if (_currentDataSet->isDerivedData())
        events().notifyDatasetDataSelectionChanged(_currentDataSet->getSourceDataset<DatasetImpl>());
    else
        events().notifyDatasetDataSelectionChanged(_currentDataSet);
}

QString LinePlotViewPlugin::getCurrentDataSetID() const
{
    if (_currentDataSet.isValid())
        return _currentDataSet->getId();
    else
        return QString{};
}

QVariant LinePlotViewPlugin::prepareData(
    QVector<float>& coordvalues,
    QVector<QPair<QString, QColor>>& categoryValues,
    SmoothingType smoothing,
    int smoothingParam,
    NormalizationType normalization
)
{
    if (coordvalues.isEmpty() || coordvalues.size() % 2 != 0) {
        qDebug() << "prepareData: Invalid input data";
        return QVariant();
    }

    // Convert flat coordvalues to point pairs
    QVector<QPair<float, float>> rawData;
    for (int i = 0; i < coordvalues.size(); i += 2)
        rawData.append({ coordvalues[i], coordvalues[i + 1] });

    // Sort by X
    std::sort(rawData.begin(), rawData.end(), [](auto a, auto b) { return a.first < b.first; });

    // Apply normalization BEFORE smoothing
    QVector<QPair<float, float>> normalizedData = applyNormalization(rawData, normalization);

    // Compute statLine on normalized (but unsmoothed) data
    QVariantMap statLine;
    if (normalizedData.size() >= 2) {
        int n_half = normalizedData.size() / 2;
        float sx1 = 0, sy1 = 0, sx2 = 0, sy2 = 0;
        for (int i = 0; i < n_half; ++i) {
            sx1 += normalizedData[i].first;
            sy1 += normalizedData[i].second;
        }
        for (int i = normalizedData.size() - n_half; i < normalizedData.size(); ++i) {
            sx2 += normalizedData[i].first;
            sy2 += normalizedData[i].second;
        }
        statLine["start_x"] = sx1 / n_half;
        statLine["start_y"] = sy1 / n_half;
        statLine["end_x"] = sx2 / n_half;
        statLine["end_y"] = sy2 / n_half;
        statLine["label"] = QString("Stat Line (%1)").arg(n_half);
        statLine["color"] = "#d62728";
        statLine["n_start"] = n_half;
        statLine["n_end"] = n_half;
    }

    // Apply smoothing to normalized data
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

    // Convert back to QVariantList with categories
    QVariantList payload;
    for (int i = 0; i < smoothedData.size(); ++i) {
        QVariantMap entry;
        entry["x"] = smoothedData[i].first;
        entry["y"] = smoothedData[i].second;
        if (i < categoryValues.size()) {
            const auto& cat = categoryValues[i];
            if (!cat.first.isEmpty())
                entry["category"] = QVariantList{ cat.second.name(), cat.first };
        }
        payload.append(entry);
    }

    QVariantMap root;
    root["data"] = payload;
    root["statLine"] = statLine;
    root["lineColor"] = "#1f77b4";
    return root;
}

QVariant LinePlotViewPlugin::prepareDataSample()
{

    QVariantList payload;
    {
        QVariantMap entry1;
        entry1["x"] = 1.0;
        entry1["y"] = 5.2;
        entry1["category"] = QVariantList{ "#1f77b4", "Type A" };
        payload << entry1;

        QVariantMap entry2;
        entry2["x"] = 2.0;
        entry2["y"] = 7.8;
        entry2["category"] = QVariantList{ "#1f77b4", "Type A" };
        payload << entry2;

        QVariantMap entry3;
        entry3["x"] = 3.0;
        entry3["y"] = 6.1;
        entry3["category"] = QVariantList{ "#ff7f0e", "Type B" };
        payload << entry3;

        QVariantMap entry4;
        entry4["x"] = 4.0;
        entry4["y"] = 8.3;
        entry4["category"] = QVariantList{ "#ff7f0e", "Type B" };
        payload << entry4;

        QVariantMap entry5;
        entry5["x"] = 5.0;
        entry5["y"] = 4.7;
        entry5["category"] = QVariantList{ "#2ca02c", "Type C" };
        payload << entry5;

        QVariantMap entry6;
        entry6["x"] = 6.0;
        entry6["y"] = 9.0;
        entry6["category"] = QVariantList{ "#2ca02c", "Type C" };
        payload << entry6;

        QVariantMap entry7;
        entry7["x"] = 7.0;
        entry7["y"] = 3.5;
        entry7["category"] = QVariantList{ "#ff7f0e", "Type B" };
        payload << entry7;

        QVariantMap entry8;
        entry8["x"] = 8.0;
        entry8["y"] = 6.8;
        entry8["category"] = QVariantList{ "#1f77b4", "Type A" };
        payload << entry8;

        QVariantMap entry9;
        entry9["x"] = 9.0;
        entry9["y"] = 5.5;
        entry9["category"] = QVariantList{ "#2ca02c", "Type C" };
        payload << entry9;

        QVariantMap entry10;
        entry10["x"] = 10.0;
        entry10["y"] = 7.0;
        entry10["category"] = QVariantList{ "#1f77b4", "Type A" };
        payload << entry10;
    }

    QVariantMap statLine;
    statLine["start_x"] = (1.0 + 2.0 + 3.0 + 4.0 + 5.0) / 5.0;
    statLine["start_y"] = (5.2 + 7.8 + 6.1 + 8.3 + 4.7) / 5.0;
    statLine["end_x"] = (6.0 + 7.0 + 8.0 + 9.0 + 10.0) / 5.0;
    statLine["end_y"] = (9.0 + 3.5 + 6.8 + 5.5 + 7.0) / 5.0;
    statLine["n_start"] = 5;
    statLine["n_end"] = 5;
    statLine["label"] = "Statistical Line (mean first/last 5)";
    statLine["color"] = "#d62728";

    QVariantMap root;
    root["data"] = payload;
    root["statLine"] = statLine;
    root["title"] = "Example Line Chart Title";
    root["lineColor"] = "#1f77b4";

    QVariant data = root;
    return data;
}

// =============================================================================
// Plugin Factory 
// =============================================================================

LinePlotViewPluginFactory::LinePlotViewPluginFactory()
{
    setIconByName("chart-line");

    getPluginMetadata().setDescription("Line Javascript view plugin");
    getPluginMetadata().setSummary("This plugin shows how to implement a basic Javascript-based view plugin in ManiVault Studio.");
    getPluginMetadata().setCopyrightHolder({ "BioVault (Biomedical Visual Analytics Unit LUMC - TU Delft)" });
    getPluginMetadata().setAuthors({
        });
    getPluginMetadata().setOrganizations({
        { "LUMC", "Leiden University Medical Center", "https://www.lumc.nl/en/" },
        { "TU Delft", "Delft university of technology", "https://www.tudelft.nl/" }
        });
    getPluginMetadata().setLicenseText("This plugin is distributed under the [LGPL v3.0](https://www.gnu.org/licenses/lgpl-3.0.en.html) license.");
}

ViewPlugin* LinePlotViewPluginFactory::produce()
{
    return new LinePlotViewPlugin(this);
}

mv::DataTypes LinePlotViewPluginFactory::supportedDataTypes() const
{
    DataTypes supportedTypes;
    supportedTypes.append(PointType);
    return supportedTypes;
}

mv::gui::PluginTriggerActions LinePlotViewPluginFactory::getPluginTriggerActions(const mv::Datasets& datasets) const
{
    PluginTriggerActions pluginTriggerActions;

    const auto getPluginInstance = [this]() -> LinePlotViewPlugin* {
        return dynamic_cast<LinePlotViewPlugin*>(plugins().requestViewPlugin(getKind()));
    };

    const auto numberOfDatasets = datasets.count();

    /*if (numberOfDatasets >= 1 && PluginFactory::areAllDatasetsOfTheSameType(datasets, PointType)) {
        auto pluginTriggerAction = new PluginTriggerAction(const_cast<LinePlotViewPluginFactory*>(this), this, "Line JS", "View JavaScript visualization", icon(), [this, getPluginInstance, datasets](PluginTriggerAction& pluginTriggerAction) -> void {
            for (auto dataset : datasets)
                getPluginInstance()->loadData(Datasets({ dataset }));

        });

        pluginTriggerActions << pluginTriggerAction;
    }*/

    return pluginTriggerActions;
}

void LinePlotViewPlugin::fromVariantMap(const QVariantMap& variantMap)
{
    ViewPlugin::fromVariantMap(variantMap);
    mv::util::variantMapMustContain(variantMap, "LinePlotViewPlugin:Settings");
    _settingsAction.fromVariantMap(variantMap["LinePlotViewPlugin:Settings"].toMap());
}

QVariantMap LinePlotViewPlugin::toVariantMap() const
{
    QVariantMap variantMap = ViewPlugin::toVariantMap();

    _settingsAction.insertIntoVariantMap(variantMap);
    return variantMap;
}
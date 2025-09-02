#include "LinePlotViewPlugin.h"

#include "ChartWidget.h"
#include "../libs/LineChartLib/LineChartWidget.h"
#include "LinePlotUtils.h"

#include <DatasetsMimeData.h>
#include <QApplication> 
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
#include<QtConcurrent>

Q_PLUGIN_METADATA(IID "studio.manivault.LinePlotViewPlugin")

using namespace mv;

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
    qApp->setStyleSheet("QToolTip { color: black; background: #ffffe1; border: 1px solid black; }");
    getWidget().setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));

    auto layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);

    _openGlEnabled = true;

    auto settings = new QHBoxLayout();
    settings->setContentsMargins(0, 0, 0, 0);
    settings->setSpacing(0);
    settings->addWidget(_settingsAction.getDatasetOptionsHolder().createWidget(&getWidget()));
    settings->addWidget(_settingsAction.getChartOptionsHolder().createCollapsedWidget(&getWidget()));
    layout->addLayout(settings);
    if (_openGlEnabled)
    {
        _lineChartWidget = new LineChartWidget(&getWidget());
        layout->addWidget(_lineChartWidget, 1);
        _dropWidget = new DropWidget(_lineChartWidget);
    }
    else
    {
        _chartWidget = new ChartWidget(this);
        _chartWidget->setPage(":line_chart/line_chart.html", "qrc:/line_chart/");
        layout->addWidget(_chartWidget, 1);
        _dropWidget = new DropWidget(_chartWidget);
    }
    
    getWidget().setLayout(layout);

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

     _dimensionXRangeDebounceTimer.setSingleShot(true);
     _dimensionYRangeDebounceTimer.setSingleShot(true);
     _smoothingTypeDebounceTimer.setSingleShot(true);
     _normalizationTypeDebounceTimer.setSingleShot(true);
     _smoothingWindowDebounceTimer.setSingleShot(true);
     _colorDatasetDebounceTimer.setSingleShot(true);
     _colorPointDatasetDimensionDebounceTimer.setSingleShot(true);
     _colorPointDatasetColorMapDebounceTimer.setSingleShot(true);
     _colorMapRangeDebounceTimer.setSingleShot(true);


    const auto dataChanged = [this]() -> void {
        _isUpdating = true;
        dataConvertChartUpdate();
        _isUpdating = false;
        /*QtConcurrent::run([this]() {
            dataConvertChartUpdate();
            _isUpdating = false;
            });*/
        };

    connect(&_currentDataSet, &Dataset<Points>::dataChanged, this, dataChanged);

    const auto pointDatasetChanged = [this]() -> void {
        auto dataset = _settingsAction.getDatasetOptionsHolder().getPointDatasetAction().getCurrentDataset();
        if (dataset.isValid()) {

            _currentDataSet = dataset;
            _dropWidget->setShowDropIndicator(false);
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
            auto parent = _currentDataSet->getParent();
            Datasets clusterDatasets;
            for (const auto& child : children)
            {
                if (child->getDataType() == ClusterType || child->getDataType() == PointType) {
                    clusterDatasets.push_back(child);
                }
            }
            
            if (parent.isValid())
            {
                auto others = parent->getChildren();
                for (const auto& other : others)
                {
                    if (other->getDataType() == ClusterType|| other->getDataType() == PointType) {
                        clusterDatasets.push_back(other);
                    }
                }
                clusterDatasets.push_back(parent);
            }
            
            _settingsAction.getChartOptionsHolder().getSmoothingWindowAction().setMaximum(_currentDataSet->getNumPoints()/2);
            _settingsAction.getChartOptionsHolder().getSmoothingWindowAction().setMinimum(2);
            int lowlimit = 2;
            int highlimit = std::min(static_cast<int>(_currentDataSet->getNumPoints() / 2), static_cast<int>(_currentDataSet->getNumPoints() * 0.1f));
            _settingsAction.getChartOptionsHolder().getSmoothingWindowAction().setValue(
                std::max(lowlimit, highlimit)
            );

            _settingsAction.getDatasetOptionsHolder().getColorDatasetAction().setDatasets(clusterDatasets);
            if (clusterDatasets.isEmpty())
            {
                _settingsAction.getDatasetOptionsHolder().getColorDatasetAction().setCurrentIndex(-1);
            }
            else
            {
                _settingsAction.getDatasetOptionsHolder().getColorDatasetAction().setCurrentIndex(0);
            }

            
        }
        else
        {
            _currentDataSet = Dataset<Points>();
            _settingsAction.getDatasetOptionsHolder().getDataDimensionXSelectionAction().setPointsDataset(_currentDataSet);
            _settingsAction.getDatasetOptionsHolder().getDataDimensionYSelectionAction().setPointsDataset(_currentDataSet);
            _settingsAction.getDatasetOptionsHolder().getDataDimensionXSelectionAction().setCurrentDimensionIndex(-1);
            _settingsAction.getDatasetOptionsHolder().getDataDimensionYSelectionAction().setCurrentDimensionIndex(-1);
            _settingsAction.getDatasetOptionsHolder().getColorDatasetAction().setDatasets({});
            
        }
        events().notifyDatasetDataChanged(_currentDataSet);
        };
    connect(&_settingsAction.getDatasetOptionsHolder().getPointDatasetAction(), &DatasetPickerAction::currentIndexChanged, this, pointDatasetChanged);

    const auto switchAxesChanged = [this]() {updateChartTrigger(); };
    connect(&_settingsAction.getChartOptionsHolder().getSwitchAxesAction(), &ToggleAction::toggled, this, switchAxesChanged);

    const auto showEnvelopeChanged = [this]() {
        if (_lineChartWidget)
        {
            _lineChartWidget->setShowEnvelope(_settingsAction.getChartOptionsHolder().getShowEnvelopeAction().isChecked());
        }
        };
    connect(&_settingsAction.getChartOptionsHolder().getShowEnvelopeAction(), &ToggleAction::toggled, this, showEnvelopeChanged);

    const auto showStatLineChanged = [this]() {
        if (_lineChartWidget)
        {
            _lineChartWidget->setShowStatLine(_settingsAction.getChartOptionsHolder().getShowStatLineAction().isChecked());
        }
        };
    connect(&_settingsAction.getChartOptionsHolder().getShowStatLineAction(), &ToggleAction::toggled, this, showStatLineChanged);

    const auto sortAxesChanged = [this]() {updateChartTrigger(); };
    connect(&_settingsAction.getChartOptionsHolder().getSortByAxisAction(), &OptionAction::currentIndexChanged, this, sortAxesChanged);

    connect(&_settingsAction.getDatasetOptionsHolder().getColorDatasetAction(),
        &DatasetPickerAction::currentIndexChanged,
        this,
        [this]() {
            _colorDatasetDebounceTimer.start(50);
        });

    connect(&_colorDatasetDebounceTimer, &QTimer::timeout, this, [this]() {

        auto colorDataset = _settingsAction.getDatasetOptionsHolder().getColorDatasetAction().getCurrentDataset();
        _settingsAction.getDatasetOptionsHolder().getColorPointDatasetDimensionAction().setCurrentDimensionIndex(-1);
        if (colorDataset.isValid() && colorDataset->getDataType()==PointType)
        {
            _settingsAction.getDatasetOptionsHolder().getColorPointDatasetDimensionAction().setPointsDataset(colorDataset); 
            _settingsAction.getDatasetOptionsHolder().getColorPointDatasetDimensionAction().setEnabled(true);
            _settingsAction.getChartOptionsHolder().getPointDatasetDimensionColorMapAction().setEnabled(true);
        }
        else
        {
            _settingsAction.getDatasetOptionsHolder().getColorPointDatasetDimensionAction().setPointsDataset(Dataset<Points>());
            _settingsAction.getDatasetOptionsHolder().getColorPointDatasetDimensionAction().setDisabled(true);
            _settingsAction.getChartOptionsHolder().getPointDatasetDimensionColorMapAction().setDisabled(true);
        }

        updateChartTrigger();
        });

    connect(&_settingsAction.getDatasetOptionsHolder().getColorPointDatasetDimensionAction(),
        &DimensionPickerAction::currentDimensionIndexChanged,
        this,
        [this]() {
            _colorPointDatasetDimensionDebounceTimer.start(50);
        });

    // In the _colorPointDatasetDimensionDebounceTimer timeout lambda:
    connect(&_colorPointDatasetDimensionDebounceTimer, &QTimer::timeout, this, [this]() {

        _blockcolorRangeTriggerMethod = true;
        auto colorDataset = _settingsAction.getDatasetOptionsHolder().getColorDatasetAction().getCurrentDataset();
        if (colorDataset.isValid() && colorDataset->getDataType() == PointType)
        {
            Dataset<Points> colorPointDataset = _settingsAction.getDatasetOptionsHolder().getColorDatasetAction().getCurrentDataset();
            int colorPointDatasetDimensionIndex = _settingsAction.getDatasetOptionsHolder().getColorPointDatasetDimensionAction().getCurrentDimensionIndex();
            if (colorPointDatasetDimensionIndex >= 0)
            {
                std::vector<float> colorValues(colorPointDataset->getNumPoints());
                colorPointDataset->extractDataForDimension(colorValues, colorPointDatasetDimensionIndex);
                float maxValue = *std::max_element(colorValues.begin(), colorValues.end());
                float minValue = *std::min_element(colorValues.begin(), colorValues.end());
                _settingsAction.getChartOptionsHolder().getUpperColorLimitAction().setMinimum(minValue);
                _settingsAction.getChartOptionsHolder().getUpperColorLimitAction().setMaximum(maxValue);
                _settingsAction.getChartOptionsHolder().getLowerColorLimitAction().setMinimum(minValue);
                _settingsAction.getChartOptionsHolder().getLowerColorLimitAction().setMaximum(maxValue);
                _settingsAction.getChartOptionsHolder().getUpperColorLimitAction().setValue(maxValue);
                _settingsAction.getChartOptionsHolder().getLowerColorLimitAction().setValue(minValue);

            }
            else
            {
                _settingsAction.getChartOptionsHolder().getUpperColorLimitAction().setMinimum(0);
                _settingsAction.getChartOptionsHolder().getUpperColorLimitAction().setMaximum(0);
                _settingsAction.getChartOptionsHolder().getLowerColorLimitAction().setMinimum(0);
                _settingsAction.getChartOptionsHolder().getLowerColorLimitAction().setMaximum(0);
                _settingsAction.getChartOptionsHolder().getUpperColorLimitAction().setValue(0);
                _settingsAction.getChartOptionsHolder().getLowerColorLimitAction().setValue(0);

            }

        }

        _blockcolorRangeTriggerMethod = false;

        updateChartTrigger();
        });

    connect(&_settingsAction.getChartOptionsHolder().getPointDatasetDimensionColorMapAction(),
        &ColorMap1DAction::imageChanged,
        this,
        [this]() {
            _colorPointDatasetColorMapDebounceTimer.start(50);
        });

    connect(&_colorPointDatasetColorMapDebounceTimer, &QTimer::timeout, this, [this]() {

        updateChartTrigger();
        });

    connect(&_settingsAction.getChartOptionsHolder().getUpperColorLimitAction(),
        &DecimalAction::valueChanged,
        this,
        [this]() {
            _colorMapRangeDebounceTimer.start(50);
        });


    connect(&_settingsAction.getChartOptionsHolder().getLowerColorLimitAction(),
        &DecimalAction ::valueChanged,
        this,
        [this]() {
            _colorMapRangeDebounceTimer.start(50);
        });

    connect(&_colorMapRangeDebounceTimer, &QTimer::timeout, this, [this]() {
        if (_blockcolorRangeTriggerMethod)
        {
            //qInfo() << "LinePlotViewPlugin::updateChartTrigger: Skipping color range update due to debounce timer";
            return;
        }
        if (_settingsAction.getChartOptionsHolder().getLowerColorLimitAction().getValue() > _settingsAction.getChartOptionsHolder().getUpperColorLimitAction().getValue())
        {

            _settingsAction.getChartOptionsHolder().getLowerColorLimitAction().setValue(_settingsAction.getChartOptionsHolder().getUpperColorLimitAction().getValue());
            return;
        }
        else
        {
            updateChartTrigger();
        }

        });

    connect(&_settingsAction.getDatasetOptionsHolder().getDataDimensionXSelectionAction(),
        &DimensionPickerAction::currentDimensionIndexChanged,
        this,
        [this]() {
            _dimensionXRangeDebounceTimer.start(50);
        });

    connect(&_dimensionXRangeDebounceTimer, &QTimer::timeout, this, [this]() {
        updateChartTrigger();
        });




    connect(&_settingsAction.getDatasetOptionsHolder().getDataDimensionYSelectionAction(),
        &DimensionPickerAction::currentDimensionIndexChanged,
        this,
        [this]() {
            _dimensionYRangeDebounceTimer.start(50);
        });

    connect(&_dimensionYRangeDebounceTimer, &QTimer::timeout, this, [this]() {
        updateChartTrigger();
        });




    connect(&_settingsAction.getChartOptionsHolder().getNormalizationTypeAction(),
        &OptionAction::currentIndexChanged,
        this,
        [this]() {
            _normalizationTypeDebounceTimer.start(50);
        });

    connect(&_normalizationTypeDebounceTimer, &QTimer::timeout, this, [this]() {
        updateChartTrigger();
        });

    connect(&_settingsAction.getChartOptionsHolder().getSmoothingTypeAction(),
        &OptionAction::currentIndexChanged,
        this,
        [this]() {
            _smoothingTypeDebounceTimer.start(50);
        });

    connect(&_smoothingTypeDebounceTimer, &QTimer::timeout, this, [this]() {
        updateChartTrigger();
        });


     connect(&_settingsAction.getChartOptionsHolder().getSmoothingWindowAction(),
         &IntegralAction::valueChanged,
         this,
         [this]() {
             _smoothingWindowDebounceTimer.start(50);
         });

     connect(&_smoothingWindowDebounceTimer, &QTimer::timeout, this, [this]() {
         updateChartTrigger();
         });


    
    //connect(&_chartWidget->getCommunicationObject(), &ChartCommObject::passSelectionToCore, this, &LinePlotViewPlugin::publishSelection);

}

void LinePlotViewPlugin::initTrigger()
{
    _isUpdating = true;
    dataConvertChartUpdate();
    _isUpdating = false;
    /*QtConcurrent::run([this]() {
        dataConvertChartUpdate();
        _isUpdating = false;
        });*/
}

void LinePlotViewPlugin::updateChartTrigger()
{
    if (_isUpdating)
    {
        //qInfo() << "LinePlotViewPlugin::updateChartTrigger: Already updating, skipping this call";
        return;
    }
    else
    {
        //qInfo() << "LinePlotViewPlugin::updateChartTrigger: Triggering chart update";
        _isUpdating = true;
        dataConvertChartUpdate();
        _isUpdating = false;
       /* QtConcurrent::run([this]() {
            dataConvertChartUpdate();
            _isUpdating = false;
            });*/
    }
}

void LinePlotViewPlugin::loadData(const mv::Datasets& datasets)
{
    if (datasets.isEmpty())
        return;

    //qDebug() << "LinePlotViewPlugin::loadData: Load data set from ManiVault core";
    if (!datasets.first().isValid()) {
        _settingsAction.getDatasetOptionsHolder().getPointDatasetAction().setCurrentIndex(-1);
        qCritical() << "LinePlotViewPlugin::loadData: Invalid dataset provided";
        return;
    }
    else
    {
        _settingsAction.getDatasetOptionsHolder().getPointDatasetAction().setCurrentDataset(datasets.first());
    }
    
}


void LinePlotViewPlugin::dataConvertChartUpdate()
{
    QVariant root;
    if (!_currentDataSet.isValid())
    {
        qWarning() << "LinePlotViewPlugin::convertDataAndUpdateChart: No valid dataset to convert";
    }
    else
    {
        //FunctionTimer timer(Q_FUNC_INFO);

        const auto numPoints = _currentDataSet->getNumPoints();
        const auto numDimensions = _currentDataSet->getNumDimensions();
        const auto dimensionNames = _currentDataSet->getDimensionNames();

        //qDebug() << "dataConvertChartUpdate: numPoints =" << numPoints << " numDimensions =" << numDimensions;
        auto selectedDimensionX = _settingsAction.getDatasetOptionsHolder().getDataDimensionXSelectionAction().getCurrentDimensionName();
        auto selectedDimensionY = _settingsAction.getDatasetOptionsHolder().getDataDimensionYSelectionAction().getCurrentDimensionName();
        //qDebug() << "dataConvertChartUpdate: selectedDimensionX =" << selectedDimensionX << " selectedDimensionY =" << selectedDimensionY;

        int dimensionXIndex = -1;
        int dimensionYIndex = -1;
        if (selectedDimensionX.isEmpty() || selectedDimensionY.isEmpty()) {
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
        //qDebug() << "dataConvertChartUpdate: dimensionXIndex =" << dimensionXIndex << " dimensionYIndex =" << dimensionYIndex;

        if (dimensionXIndex == -1 || dimensionYIndex == -1) {
            qCritical() << "LinePlotViewPlugin::convertDataAndUpdateChart: Selected dimensions not found in dataset";
            return;
        }

        QVector<float> coordvalues;
        QVector<QPair<QString, QColor>> categoryValues;

        Dataset colorDataset = _settingsAction.getDatasetOptionsHolder().getColorDatasetAction().getCurrentDataset();
        int colorPointDatasetDimensionIndex = -1;
        QString colormapselectedVal="";
        float lowerColorLimit = _settingsAction.getChartOptionsHolder().getLowerColorLimitAction().getValue();
        float upperColorLimit = _settingsAction.getChartOptionsHolder().getUpperColorLimitAction().getValue();

        if (colorDataset->getDataType() == PointType)
        {
            colorPointDatasetDimensionIndex = _settingsAction.getDatasetOptionsHolder().getColorPointDatasetDimensionAction().getCurrentDimensionIndex();
            colormapselectedVal = _settingsAction.getChartOptionsHolder().getPointDatasetDimensionColorMapAction().getColorMap();
        }

        extractLinePlotData(
            _currentDataSet,
            dimensionXIndex,
            dimensionYIndex,
            colorDataset->getId(),
            colorPointDatasetDimensionIndex,
            colormapselectedVal,
            lowerColorLimit,
            upperColorLimit,
            coordvalues,
            categoryValues
        );

        if (_settingsAction.getChartOptionsHolder().getSwitchAxesAction().isChecked()) {
            for (int i = 0; i + 1 < coordvalues.size(); i += 2) {
                std::swap(coordvalues[i], coordvalues[i + 1]);
            }
            std::swap(selectedDimensionX, selectedDimensionY);
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
            qCritical() << "LinePlotViewPlugin::convertDataAndUpdateChart: Unknown smoothing type, defaulting to None";
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
            qCritical() << "LinePlotViewPlugin::convertDataAndUpdateChart: Unknown normalization type, defaulting to None";
            normalization = NormalizationType::None;
        }

        QString titleText = _settingsAction.getChartOptionsHolder().getChartTitleAction().getString();
        QString sortAxisValue = _settingsAction.getChartOptionsHolder().getSortByAxisAction().getCurrentText();

        root = ::prepareData(
            coordvalues,
            categoryValues,
            smoothing,
            windowSize,
            normalization,
            selectedDimensionX,
            selectedDimensionY,
            titleText,
            sortAxisValue
        );
    }

    if (_openGlEnabled)
    {
        _lineChartWidget->setData(root.toMap());

    }
    else
    {
        emit _chartWidget->getCommunicationObject().qt_js_setDataAndPlotInJS(root.toMap());
    }
   
}

/*void LinePlotViewPlugin::publishSelection(const std::vector<unsigned int>& selectedIDs)
{
     //FunctionTimer timer(Q_FUNC_INFO);
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
}*/

QString LinePlotViewPlugin::getCurrentDataSetID() const
{
    if (_currentDataSet.isValid())
        return _currentDataSet->getId();
    else
        return QString{};
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
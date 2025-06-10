#include "LinePlotViewPlugin.h"

#include <graphics/Vector2f.h>

#include <DatasetsMimeData.h>

#include <QLabel>
#include <QDebug>

#include <random>
#include <numeric>
#include <QtConcurrent>
#include <QFuture>
#include <QMutexLocker>
#include<QTimer>
#include<QCoreApplication>
#include<QElapsedTimer>
#include<set>


Q_PLUGIN_METADATA(IID "studio.manivault.LinePlotViewPlugin")

using namespace mv;

// -----------------------------------------------------------------------------
// LinePlotViewPlugin
// -----------------------------------------------------------------------------
LinePlotViewPlugin::LinePlotViewPlugin(const PluginFactory* factory) :
    ViewPlugin(factory),
    _currentDataSet(),
    _currentDimensions({0, 1}),
    _dropWidget(nullptr),
    _lineChartWidget(new HighPerfLineChart()),
    _settingsAction(this, "Settings Action")
{
    setObjectName("LinePlot view");

    // Instantiate new drop widget, setting the LinePlot Widget as its parent
    // the parent widget hat to setAcceptDrops(true) for the drop widget to work
    _dropWidget = new DropWidget(_lineChartWidget);
    _lineChartWidget->setAcceptDrops(true);
    // Set the drop indicator widget (the widget that indicates that the view is eligible for data dropping)
    _dropWidget->setDropIndicatorWidget(new DropWidget::DropIndicatorWidget(&getWidget(), "No data loaded", "Drag the LinePlotViewData from the data hierarchy here"));

    // Initialize the drop regions
    _dropWidget->initialize([this](const QMimeData* mimeData) -> DropWidget::DropRegions {
        // A drop widget can contain zero or more drop regions
        DropWidget::DropRegions dropRegions;

        const auto datasetsMimeData = dynamic_cast<const DatasetsMimeData*>(mimeData);

        if (datasetsMimeData == nullptr)
            return dropRegions;

        if (datasetsMimeData->getDatasets().count() > 1)
            return dropRegions;

        // Gather information to generate appropriate drop regions
        const auto dataset = datasetsMimeData->getDatasets().first();
        const auto datasetGuiName = dataset->getGuiName();
        const auto datasetId = dataset->getId();
        const auto dataType = dataset->getDataType();
        const auto dataTypes = DataTypes({ PointType });
        int numOfPointsChildren = 0;
        for (const auto& child : dataset->getChildren()) {
            if (child->getDataType() == PointType)
                numOfPointsChildren++;
        }
        if (dataTypes.contains(dataType) && numOfPointsChildren>0) {

            if (datasetId == getCurrentDataSetID()) {
                dropRegions << new DropWidget::DropRegion(this, "Warning", "Data already loaded", "exclamation-circle", false);
            }
            else {
                auto candidateDataset = mv::data().getDataset<Points>(datasetId);

                dropRegions << new DropWidget::DropRegion(this, "Points", QString("Visualize %1 as line chart").arg(datasetGuiName), "map-marker-alt", true, [this, candidateDataset]() {
                    loadData({ candidateDataset });
                    });

            }
        }
        else {
            dropRegions << new DropWidget::DropRegion(this, "Incompatible data", "This type of data is not supported", "exclamation-circle", false);
        }

        return dropRegions;
    });

    // update data when data set changed
    connect(&_currentDataSet, &Dataset<Points>::dataChanged, this, &LinePlotViewPlugin::updatePlot);

    // update settings UI when data set changed
    connect(&_currentDataSet, &Dataset<Points>::changed, this, [this]() {
        const auto enabled = _currentDataSet.isValid();

        auto& nameString = _settingsAction.getDatasetNameAction();
        auto& xDimPicker = _settingsAction.getXDimensionPickerAction();
        auto& yDimPicker = _settingsAction.getYDimensionPickerAction();
        auto& pointSizeA = _settingsAction.getPointSizeAction();

        xDimPicker.setEnabled(enabled);
        yDimPicker.setEnabled(enabled);
        pointSizeA.setEnabled(enabled);

        if (!enabled)
            return;

        nameString.setString(_currentDataSet->getGuiName());

        xDimPicker.setPointsDataset(_currentDataSet);
        yDimPicker.setPointsDataset(_currentDataSet);

        xDimPicker.setCurrentDimensionIndex(0);

        const auto yIndex = xDimPicker.getNumberOfDimensions() >= 2 ? 1 : 0;
        yDimPicker.setCurrentDimensionIndex(yIndex);

    });

    getLearningCenterAction().addVideos(QStringList({ "Practitioner", "Developer" }));
}

void LinePlotViewPlugin::init()
{
    // Create layout
    auto layout = new QVBoxLayout();

    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(_lineChartWidget, 100);

    // Apply the layout
    getWidget().setLayout(layout);

    addDockingAction(&_settingsAction);


}

void LinePlotViewPlugin::updatePlot()
{
    if (!_currentDataSet.isValid())
    {
        qDebug() << "LinePlotViewPlugin:: dataset is not valid - no data will be displayed";
        return;
    }

    if (_currentDataSet->getNumDimensions() < 2)
    {
        qDebug() << "LinePlotViewPlugin:: dataset must have at least two dimensions";
        return;
    }

    // Retrieve the data that is to be shown from the core
    auto newDimX = _settingsAction.getXDimensionPickerAction().getCurrentDimensionIndex();
    auto newDimY = _settingsAction.getYDimensionPickerAction().getCurrentDimensionIndex();

    if (newDimX >= 0)
        _currentDimensions[0] = static_cast<unsigned int>(newDimX);

    if (newDimY >= 0)
        _currentDimensions[1] = static_cast<unsigned int>(newDimY);

    std::vector<mv::Vector2f> data;
    _currentDataSet->extractDataForDimensions(data, _currentDimensions[0], _currentDimensions[1]);

    /*

    int geneIndex = std::distance(dimensionNames.begin(), it);
    LineData selectedGeneLineData;
    QVector<QColor> colorsExpVals = { QColor("#FDCC0D") };
    std::vector<int> geneIndicesExp = { geneIndex };
    std::vector<float> expresionValues(_filteredSelectionIndices.size() * geneIndicesExp.size());
    QVector<QPointF> points;

    int geneIndicesCoord = 1;
    std::vector<float> coordValues(_filteredSelectionIndices.size() * geneIndicesCoord);


    pointDataset->populateDataForDimensions(expresionValues, geneIndicesExp, _filteredSelectionIndices);
    _rotatedSelectedEmbeddingDataset->extractDataForDimension(coordValues, geneIndicesCoord);

    qDebug() << "Raw data size:" << expresionValues.size();
    if (!expresionValues.empty()) {
        qDebug() << "First 10 elements:";
        for (size_t i = 0; i < std::min(size_t(10), expresionValues.size()); ++i) {
            qDebug() << i << ":" << expresionValues[i];
        }
    }

    // 2. Verify your indices
    qDebug() << "Points per line:" << _filteredSelectionIndices.size();
    qDebug() << "Number of lines:" << geneIndicesExp.size();

    //option to normalize expresionValues 
    if (false) {
        // Normalize expression values to [0, 1] range
        float minVal = *std::min_element(expresionValues.begin(), expresionValues.end());
        float maxVal = *std::max_element(expresionValues.begin(), expresionValues.end());
        if (maxVal - minVal > 0) {
            for (auto& val : expresionValues) {
                val = (val - minVal) / (maxVal - minVal);
            }
        }
    }
    //option to normalize rotatedYCordinates
    if (false) {
        // Normalize rotated Y coordinates to [0, 1] range
        float minY = *std::min_element(coordValues.begin(), coordValues.end());
        float maxY = *std::max_element(coordValues.begin(), coordValues.end());
        if (maxY - minY > 0) {
            for (auto& val : coordValues) {
                val = (val - minY) / (maxY - minY);
            }
        }
    }

    auto lineData = convertToLineData(
        expresionValues,
        coordValues,
        static_cast<int>(_filteredSelectionIndices.size()),
        static_cast<int>(geneIndicesExp.size()),
        colorsExpVals, // colors
        true,              // coordIsX
        true               // sortByX
    );
    // 4. Verify output
    qDebug() << "Converted line data size:" << lineData.size();
    if (!lineData.empty()) {
        qDebug() << "First line points:" << lineData.first().points.size();
        if (!lineData.first().points.empty()) {
            qDebug() << "Sample point:" << lineData.first().points.first();
        }
    }

    // Set data in OpenGL widget
    _lineChartWidget->setLines(lineData);
    _lineChartWidget->setShowPoints(_settingsAction.getDataset1OptionsHolder().getShowDataPointsInChartAction().isChecked());
    _lineChartWidget->setShowLines(_settingsAction.getDataset1OptionsHolder().getShowDataLinesInChartAction().isChecked());

    _numOfPointsInLine1 = lineData.first().points.size();
    _settingsAction.getDataset1OptionsHolder().getLineSmoothingWindowAction().setMaximum(_numOfPointsInLine1);
    _lineChartWidget->setConnectStatsLineN(_numOfPointsInLine1 * _settingsAction.getDataset1OptionsHolder().getConnectStatsLineAction().getValue());
    _lineChartWidget->setConnectStatsLineType(HighPerfLineChart::ConnectStatsType::Mean);
    _lineChartWidget->setConnectStatsLineEnabled(_settingsAction.getDataset1OptionsHolder().getShowConnectedStatsLineInChartAction().isChecked());

    _lineChartWidget->setShowAvgNumPointsLabel(true);



    //_lineChartWidget->setMovingAverageWindow(std::max(1, int(_numOfPointsInLine1 * _settingsAction.getDataset1OptionsHolder().getConnectStatsLineAction().getValue())));
    _lineChartWidget->setMovingAverageWindow(_settingsAction.getDataset1OptionsHolder().getLineSmoothingWindowAction().getValue());
    _lineChartWidget->setShowMovingAverageLine(_settingsAction.getDataset1OptionsHolder().getShowMovingAverageLineInChartAction().isChecked());
    mv::theme().isSystemDarkColorSchemeActive() ?
        _lineChartWidget->setBackgroundColor(Qt::black) :
        _lineChartWidget->setBackgroundColor(Qt::white);
    _lineChartWidget->setAxisFont(QFont("Arial", 12));
    _lineChartWidget->setTickLabelFont(QFont("Arial", 10));
    mv::theme().isSystemDarkColorSchemeActive() ?
        _lineChartWidget->setAxisColor(Qt::white) :
        _lineChartWidget->setAxisColor(Qt::black);
    // Fix tick label color for correct contrast
    mv::theme().isSystemDarkColorSchemeActive() ?
        _lineChartWidget->setTickLabelColor(Qt::white) :
        _lineChartWidget->setTickLabelColor(Qt::black);
    _lineChartWidget->setXAxisName("Rotated Y coordinates");
    _lineChartWidget->setYAxisName("Gene expression " + _selectedGene1);
    _lineChartWidget->setShowLegend(false);
    _lineChartWidget->setShowAvgNumPointsLabel(true);

    */
}


void LinePlotViewPlugin::loadData(const mv::Datasets& datasets)
{
    // Exit if there is nothing to load
    if (datasets.isEmpty())
        return;

    qDebug() << "LinePlotViewPlugin::loadData: Load data set from ManiVault core";
    _dropWidget->setShowDropIndicator(false);

    // Load the first dataset, changes to _currentDataSet are connected with convertDataAndUpdateChart
    _currentDataSet = datasets.first();
    updatePlot();
}

QString LinePlotViewPlugin::getCurrentDataSetID() const
{
    if (_currentDataSet.isValid())
        return _currentDataSet->getId();
    else
        return QString{};
}


QVector<LineData> convertToLineData(const std::vector<float>& vecAll, const std::vector<float>& vecCoord,
    int pointsPerLine,
    int numOfLines,
    const QVector<QColor>& colors,
    bool coordIsX,
    bool sortByX)
{
    QVector<LineData> lines;

    qDebug() << "Conversion started. Total elements:" << vecAll.size()
        << "Points per line:" << pointsPerLine
        << "Number of lines:" << numOfLines
        << "coordIsX:" << coordIsX;

    // Validate input
    if (pointsPerLine < 1 || numOfLines < 1) {
        qWarning() << "Invalid parameters - pointsPerLine:" << pointsPerLine
            << "numOfLines:" << numOfLines;
        return lines;
    }

    const size_t requiredSize = static_cast<size_t>(pointsPerLine * numOfLines);
    if (vecAll.size() < requiredSize || vecCoord.size() < static_cast<size_t>(pointsPerLine)) {
        qWarning() << "Insufficient data - Got:" << vecAll.size()
            << "Need:" << requiredSize << "and vecCoord size:" << vecCoord.size();
        return lines;
    }

    lines.reserve(numOfLines);

    // Debug print first few values to verify data organization
    if (!coordIsX) {
        qDebug() << "First few x values (column order):";
        for (int i = 0; i < std::min(5, pointsPerLine); ++i) {
            for (int j = 0; j < std::min(5, numOfLines); ++j) {
                size_t pos = static_cast<size_t>(i) * numOfLines + j;
                qDebug() << "x[" << i << "][" << j << "]: pos" << pos << "=" << vecAll[pos];
            }
        }
        qDebug() << "First few y values:";
        for (int i = 0; i < std::min(5, pointsPerLine); ++i) {
            qDebug() << "y[" << i << "]:" << vecCoord[i];
        }
    }
    else {
        qDebug() << "First few y values (column order):";
        for (int i = 0; i < std::min(5, pointsPerLine); ++i) {
            for (int j = 0; j < std::min(5, numOfLines); ++j) {
                size_t pos = static_cast<size_t>(i) * numOfLines + j;
                qDebug() << "y[" << i << "][" << j << "]: pos" << pos << "=" << vecAll[pos];
            }
        }
        qDebug() << "First few x values:";
        for (int i = 0; i < std::min(5, pointsPerLine); ++i) {
            qDebug() << "x[" << i << "]:" << vecCoord[i];
        }
    }

    for (int lineIdx = 0; lineIdx < numOfLines; ++lineIdx) {
        LineData line;

        // Set color
        if (!colors.isEmpty() && lineIdx < colors.size()) {
            line.color = colors[lineIdx];
        }
        else {
            static const QVector<QColor> defaultColors = {
                Qt::blue, Qt::green, Qt::red, Qt::cyan,
                Qt::magenta, Qt::yellow, Qt::gray
            };
            line.color = defaultColors[lineIdx % defaultColors.size()];
        }

        line.points.reserve(pointsPerLine);

        for (int pointIdx = 0; pointIdx < pointsPerLine; ++pointIdx) {
            size_t pos = static_cast<size_t>(pointIdx) * numOfLines + lineIdx;
            if (pos >= vecAll.size() || pointIdx >= static_cast<int>(vecCoord.size())) {
                qWarning() << "Index out of bounds at line" << lineIdx
                    << "point" << pointIdx;
                continue;
            }

            float x, y;
            if (!coordIsX) {
                x = vecAll[pos];
                y = vecCoord[pointIdx];
            }
            else {
                x = vecCoord[pointIdx];
                y = vecAll[pos];
            }

            if (!std::isfinite(x) || !std::isfinite(y)) {
                qWarning() << "Non-finite point at line" << lineIdx
                    << "point" << pointIdx << ":" << x << y;
                continue;
            }

            line.points.append(QPointF(x, y));
        }

        // Sort points if requested
        if (sortByX) {
            if (coordIsX) {
                std::sort(line.points.begin(), line.points.end(), [](const QPointF& a, const QPointF& b) {
                    return a.x() < b.x();
                    });
            }
            else {
                std::sort(line.points.begin(), line.points.end(), [](const QPointF& a, const QPointF& b) {
                    return a.y() < b.y();
                    });
            }
        }

        if (!line.points.isEmpty()) {
            lines.append(line);
            //qDebug() << "Added line" << lineIdx << "with" << line.points.size() << "points";
            if (!line.points.isEmpty()) {
                //qDebug() << "  First point:" << line.points.first().x() << line.points.first().y();
                //qDebug() << "  Last point:" << line.points.last().x() << line.points.last().y();
            }
        }
        else {
            //qWarning() << "Skipping empty line" << lineIdx;
        }
    }

    qDebug() << "Conversion complete. Generated" << lines.size() << "lines";
    return lines;
}

// -----------------------------------------------------------------------------
// LinePlotViewPluginFactory
// -----------------------------------------------------------------------------

ViewPlugin* LinePlotViewPluginFactory::produce()
{
    return new LinePlotViewPlugin(this);
}

LinePlotViewPluginFactory::LinePlotViewPluginFactory() :
    _statusBarAction(nullptr),
    _statusBarPopupGroupAction(this, "Popup Group"),
    _statusBarPopupAction(this, "Popup")
{
    setIconByName("cube");

    getPluginMetadata().setDescription("Example OpenGL view");
    getPluginMetadata().setSummary("This example shows how to implement a basic OpenGL-based view plugin in ManiVault Studio.");
    getPluginMetadata().setCopyrightHolder({ "BioVault (Biomedical Visual Analytics Unit LUMC - TU Delft)" });
    getPluginMetadata().setAuthors({
	});
    getPluginMetadata().setOrganizations({
        { "LUMC", "Leiden University Medical Center", "https://www.lumc.nl/en/" },
        { "TU Delft", "Delft university of technology", "https://www.tudelft.nl/" }
    });
    getPluginMetadata().setLicenseText("This plugin is distributed under the [LGPL v3.0](https://www.gnu.org/licenses/lgpl-3.0.en.html) license.");
}

void LinePlotViewPluginFactory::initialize()
{
    ViewPluginFactory::initialize();

    // Configure the status bar popup action
    _statusBarPopupAction.setDefaultWidgetFlags(StringAction::Label);
    _statusBarPopupAction.setString("<p><b>LinePlot View</b></p><p>This is an example of a plugin status bar item</p><p>A concrete example on how this status bar was created can be found <a href='https://github.com/ManiVaultStudio/ExamplePlugins/blob/master/ExampleViewOpenGL/src/LinePlotViewPlugin.cpp'>here</a>.</p>");
    _statusBarPopupAction.setPopupSizeHint(QSize(200, 10));

    _statusBarPopupGroupAction.setShowLabels(false);
    _statusBarPopupGroupAction.setConfigurationFlag(WidgetAction::ConfigurationFlag::NoGroupBoxInPopupLayout);
    _statusBarPopupGroupAction.addAction(&_statusBarPopupAction);
    _statusBarPopupGroupAction.setWidgetConfigurationFunction([](WidgetAction* action, QWidget* widget) -> void {
        auto label = widget->findChild<QLabel*>("Label");

        Q_ASSERT(label != nullptr);

        if (label == nullptr)
            return;

        label->setOpenExternalLinks(true);
    });
    

    _statusBarAction = new PluginStatusBarAction(this, "LinePlot View", getKind());

    _statusBarAction->getConditionallyVisibleAction().setChecked(false);    // The status bar is shown, regardless of the number of instances
    //_statusBarAction->getConditionallyVisibleAction().setChecked(true);   // The status bar is shown when there is at least one instance of the plugin

    // Sets the action that is shown when the status bar is clicked
    _statusBarAction->setPopupAction(&_statusBarPopupGroupAction);

    // Position to the right of the status bar action
    _statusBarAction->setIndex(-1);

    // Assign the status bar action so that it will appear on the main window status bar
    setStatusBarAction(_statusBarAction);
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

    if (numberOfDatasets == 1 && PluginFactory::areAllDatasetsOfTheSameType(datasets, PointType)) {
        auto children = datasets.first()->getChildren();
        int numOfPointsChildren = 0;
        for (const auto& child : children) {
            if (child->getDataType() == PointType)
                numOfPointsChildren++;
        }
        if(numOfPointsChildren>0)
            {   auto pluginTriggerAction = new PluginTriggerAction(const_cast<LinePlotViewPluginFactory*>(this), this, "LinePlot", "LinePlot view data", icon(), [this, getPluginInstance, datasets](PluginTriggerAction& pluginTriggerAction) -> void {
                for (auto& dataset : datasets)
                    getPluginInstance()->loadData(Datasets({ dataset }));
                });

            pluginTriggerActions << pluginTriggerAction;
            }
    }

    return pluginTriggerActions;
}




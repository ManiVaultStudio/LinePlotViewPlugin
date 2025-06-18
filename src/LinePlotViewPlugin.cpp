#include "LinePlotViewPlugin.h"

#include "ChartWidget.h"

#include <DatasetsMimeData.h>

#include <vector>
#include <random>

#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <QMimeData>
#include <QDebug>

Q_PLUGIN_METADATA(IID "studio.manivault.LinePlotViewPlugin")

using namespace mv;

LinePlotViewPlugin::LinePlotViewPlugin(const PluginFactory* factory) :
    ViewPlugin(factory),
    _chartWidget(nullptr),
    _dropWidget(nullptr),
    _currentDataSet(nullptr)
{
    getLearningCenterAction().addVideos(QStringList({ "Practitioner", "Developer" }));
}

void LinePlotViewPlugin::init()
{
    getWidget().setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));

    // Create layout
    auto layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);

    // Create chart widget and set html contents of webpage 
    _chartWidget = new ChartWidget(this);
    _chartWidget->setPage(":line_chart/line_chart.html", "qrc:/line_chart/");

    // Add widget to layout
    layout->addWidget(_chartWidget);

    // Apply the layout
    getWidget().setLayout(layout);

    // Instantiate new drop widget: See LineViewPlugin for details
    _dropWidget = new DropWidget(_chartWidget);
    _dropWidget->setDropIndicatorWidget(new DropWidget::DropIndicatorWidget(&getWidget(), "No data loaded", "Drag the LinePlotViewData in this view"));

    _dropWidget->initialize([this](const QMimeData* mimeData) -> DropWidget::DropRegions {

        // A drop widget can contain zero or more drop regions
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

    // update data when data set changed
    connect(&_currentDataSet, &Dataset<Points>::dataChanged, this, &LinePlotViewPlugin::convertDataAndUpdateChart);

    // Update the selection (coming from PCP) in core
    connect(&_chartWidget->getCommunicationObject(), &ChartCommObject::passSelectionToCore, this, &LinePlotViewPlugin::publishSelection);

    // Create data so that we do not need to load any in this line chart
    createData();

}

void LinePlotViewPlugin::loadData(const mv::Datasets& datasets)
{
    // Exit if there is nothing to load
    if (datasets.isEmpty())
        return;

    qDebug() << "LinePlotViewPlugin::loadData: Load data set from ManiVault core";

    // Load the first dataset, changes to _currentDataSet are connected with convertDataAndUpdateChart
    _currentDataSet = datasets.first();
    events().notifyDatasetDataChanged(_currentDataSet);
}

void LinePlotViewPlugin::convertDataAndUpdateChart()
{
    if (!_currentDataSet.isValid())
        return;

    qDebug() << "LinePlotViewPlugin::convertDataAndUpdateChart: Prepare payload";

    // convert data from ManiVault PointData to a JSON structure
    QVariantList payload;
    QVariantMap entry;

    _currentDataSet->visitFromBeginToEnd([&entry, &payload, this](auto beginOfData, auto endOfData)
        {
            auto pointNames = _currentDataSet->getProperty("PointNames");
            auto dimNames = _currentDataSet->getDimensionNames();
            auto numDims = dimNames.size();

            for (unsigned int pointId = 0; pointId < _currentDataSet->getNumPoints(); pointId++)
            {
                entry.clear();

                entry["className"] = pointNames.isValid() ? pointNames.value<QStringList>()[pointId] : QString::number(pointId);

                QVariantList values;

                for (uint32_t dimId = 0; dimId < numDims; dimId++)
                {
                    QVariantMap axval;
                    axval["axis"] = dimNames[dimId];
                    axval["value"] = static_cast<float>(beginOfData[pointId * numDims + dimId]);
                    values.append(axval);
                }

                entry["axes"] = values;

                payload.append(entry);
            }
        });

    qDebug() << "LinePlotViewPlugin::convertDataAndUpdateChart: Send data from Qt cpp to D3 js";
    emit _chartWidget->getCommunicationObject().qt_js_setDataAndPlotInJS(payload);
}

void LinePlotViewPlugin::publishSelection(const std::vector<unsigned int>& selectedIDs)
{
    // ask core for the selection set for the current data set
    auto selectionSet = _currentDataSet->getSelection<Points>();
    auto& selectionIndices = selectionSet->indices;

    // clear the selection and add the new points
    selectionIndices.clear();
    selectionIndices.reserve(_currentDataSet->getNumPoints());
    for (const auto id : selectedIDs) {
        selectionIndices.push_back(id);
    }

    // notify core about the selection change
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

void LinePlotViewPlugin::createData()
{
    // Here, we create a random data set, so that we do not need 
    // to use other plugins for loading when trying out this line chart
    auto points = mv::data().createDataset<Points>("Points", "LinePlotViewData");

    int numPoints = 2;
    int numDimensions = 5;

    const std::vector<QString> dimNames {"Dim 1", "Dim 2", "Dim 3", "Dim 4", "Dim 5", };
    const QVariant pointNames = QStringList{ "Data point 1", "Data point 2" };
    std::vector<float> lineData;

    qDebug() << "LinePlotViewPlugin::createData: Create some line data. 2 points, each with 5 dimensions";

    // Create random line data
    {
        std::default_random_engine generator;
        std::uniform_real_distribution<float> distribution(0.0, 10.0);

        for (int i = 0; i < numPoints * numDimensions; i++)
        {
            lineData.push_back(distribution(generator));
            qDebug() << "lineData[" << i << "]: " << lineData[i];
        }
    }

    // Passing line data with 1000 points and 2 dimensions
    points->setData(lineData.data(), numPoints, numDimensions);
    points->setDimensionNames(dimNames);

    points->setProperty("PointNames", pointNames);

    // Notify the core system of the new data
    events().notifyDatasetDataChanged(points);
    events().notifyDatasetDataDimensionsChanged(points);
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
    // This line analysis plugin is compatible with points datasets
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

    if (numberOfDatasets >= 1 && PluginFactory::areAllDatasetsOfTheSameType(datasets, PointType)) {
        auto pluginTriggerAction = new PluginTriggerAction(const_cast<LinePlotViewPluginFactory*>(this), this, "Line JS", "View JavaScript visualization", icon(), [this, getPluginInstance, datasets](PluginTriggerAction& pluginTriggerAction) -> void {
            for (auto dataset : datasets)
                getPluginInstance()->loadData(Datasets({ dataset }));

        });

        pluginTriggerActions << pluginTriggerAction;
    }

    return pluginTriggerActions;
}

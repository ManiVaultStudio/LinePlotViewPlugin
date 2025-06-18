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

    auto layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);

    _chartWidget = new ChartWidget(this);
    _chartWidget->setPage(":line_chart/line_chart.html", "qrc:/line_chart/");

    layout->addWidget(_chartWidget);

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

    connect(&_chartWidget->getCommunicationObject(), &ChartCommObject::passSelectionToCore, this, &LinePlotViewPlugin::publishSelection);

    createData();
}

void LinePlotViewPlugin::loadData(const mv::Datasets& datasets)
{
    if (datasets.isEmpty())
        return;

    qDebug() << "LinePlotViewPlugin::loadData: Load data set from ManiVault core";

    _currentDataSet = datasets.first();
    events().notifyDatasetDataChanged(_currentDataSet);
}

void LinePlotViewPlugin::convertDataAndUpdateChart()
{
    if (!_currentDataSet.isValid())
        return;

    qDebug() << "LinePlotViewPlugin::convertDataAndUpdateChart: Prepare payload";

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

    qDebug() << "LinePlotViewPlugin::convertDataAndUpdateChart: Send data from Qt cpp to D3 js";
    emit _chartWidget->getCommunicationObject().qt_js_setDataAndPlotInJS(root);
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

void LinePlotViewPlugin::createData()
{
    auto points = mv::data().createDataset<Points>("Points", "LinePlotViewData");

    int numPoints = 2;
    int numDimensions = 5;

    const std::vector<QString> dimNames {"Dim 1", "Dim 2", "Dim 3", "Dim 4", "Dim 5", };
    const QVariant pointNames = QStringList{ "Data point 1", "Data point 2" };
    std::vector<float> lineData;

    qDebug() << "LinePlotViewPlugin::createData: Create some line data. 2 points, each with 5 dimensions";

    {
        std::default_random_engine generator;
        std::uniform_real_distribution<float> distribution(0.0, 10.0);

        for (int i = 0; i < numPoints * numDimensions; i++)
        {
            lineData.push_back(distribution(generator));
            qDebug() << "lineData[" << i << "]: " << lineData[i];
        }
    }

    points->setData(lineData.data(), numPoints, numDimensions);
    points->setDimensionNames(dimNames);

    points->setProperty("PointNames", pointNames);

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

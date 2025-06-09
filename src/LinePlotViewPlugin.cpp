#include "LinePlotViewPlugin.h"
#include "LinePlotViewWidget.h"

#include "GlobalSettingsAction.h"

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
    _linePlotViewWidget(new LinePlotViewWidget()),
    _settingsAction(this, "Settings Action")
{
    setObjectName("LinePlot view");

    // Instantiate new drop widget, setting the LinePlot Widget as its parent
    // the parent widget hat to setAcceptDrops(true) for the drop widget to work
    _dropWidget = new DropWidget(_linePlotViewWidget);

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

        if (dataTypes.contains(dataType)) {

            if (datasetId == getCurrentDataSetID()) {
                dropRegions << new DropWidget::DropRegion(this, "Warning", "Data already loaded", "exclamation-circle", false);
            }
            else {
                auto candidateDataset = mv::data().getDataset<Points>(datasetId);

                dropRegions << new DropWidget::DropRegion(this, "Points", QString("Visualize %1 as parallel coordinates").arg(datasetGuiName), "map-marker-alt", true, [this, candidateDataset]() {
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

    createData();
    //createDataOptimized();

    getLearningCenterAction().addVideos(QStringList({ "Practitioner", "Developer" }));
}

void LinePlotViewPlugin::init()
{
    // Create layout
    auto layout = new QVBoxLayout();

    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(_linePlotViewWidget, 100);

    // Apply the layout
    getWidget().setLayout(layout);

    addDockingAction(&_settingsAction);

    // Update the data when the scatter plot widget is initialized
    connect(_linePlotViewWidget, &LinePlotViewWidget::initialized, this, []() { qDebug() << "LinePlotViewWidget is initialized."; } );
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

    // Set data in OpenGL widget
    _linePlotViewWidget->setData(data, _settingsAction.getPointSizeAction().getValue(), _settingsAction.getPointOpacityAction().getValue());
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

    // Create an instance of our GlobalSettingsAction (derived from PluginGlobalSettingsGroupAction) and assign it to the factory
    setGlobalSettingsGroupAction(new GlobalSettingsAction(this, this));

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

    if (numberOfDatasets >= 1 && PluginFactory::areAllDatasetsOfTheSameType(datasets, PointType)) {
        auto pluginTriggerAction = new PluginTriggerAction(const_cast<LinePlotViewPluginFactory*>(this), this, "LinePlot", "LinePlot view data", icon(), [this, getPluginInstance, datasets](PluginTriggerAction& pluginTriggerAction) -> void {
            for (auto& dataset : datasets)
                getPluginInstance()->loadData(Datasets({ dataset }));
        });

        pluginTriggerActions << pluginTriggerAction;
    }

    return pluginTriggerActions;
}
void LinePlotViewPlugin::createData()
{
    QElapsedTimer methodTimer;
    methodTimer.start();
    auto datasets = mv::data().getAllDatasets();

    if (datasets.count() < 0)
    {
        qDebug() << "LinePlotViewPlugin::createData: No datasets available, creating example data.";
        return;
    }
    std::vector<int> dataset1Indices;
    std::vector<int> dataset2Indices;
    for (const auto& dataset : datasets)
    {
        if (dataset->getDataType() == ClusterType)
        {
            if (dataset->getGuiName() == "brain_section_label")
            {
                qDebug() << "LinePlotViewPlugin::createData: Found dataset with name 'brain_section_label'";
                Dataset<Clusters> clusterDatasetFull = dataset->getFullDataset<Clusters>();
                auto clusters = clusterDatasetFull->getClusters();
                for (const auto& cluster : clusters)
                {
                    QString clusterName = cluster.getName();
                    auto clusterIndices = cluster.getIndices();
                    if (clusterName == "CJ23.56.004.CX.42.16.06" || clusterName == "CJ23.56.004.CX.42.15.06")
                    {
                        for (const auto& index : clusterIndices)
                        {
                            dataset1Indices.push_back(index);
                        }
                    }
                    else
                    {
                        for (const auto& index : clusterIndices)
                        {
                            dataset2Indices.push_back(index);
                        }
                    }


                }


            }
        }
    }

    if (dataset1Indices.empty() || dataset2Indices.empty())
    {
        qDebug() << "LinePlotViewPlugin::createData: No indices found in the datasets, creating example data.";
        return;
    }


    for (const auto& dataset : datasets)
    {
        if (dataset->getDataType() == PointType)
        {
            if (dataset->getGuiName() == "Xenium-CJ23")
            {
                qDebug() << "LinePlotViewPlugin::createData: Found dataset with name 'Xenium-CJ23'";
                Dataset<Points> mainDatasetFull = dataset->getFullDataset<Points>();
                mainDatasetFull->setGroupIndex(666);
                auto childrenDatasets = dataset->getChildren();
                Dataset<Points>  pointDataset1 = mv::data().createDataset("Points", "Section1_Xenium-CJ23");
                pointDataset1->setGroupIndex(1);
                Dataset<Points>  pointDataset2 = mv::data().createDataset("Points", "Section2_Xenium-CJ23");
                pointDataset2->setGroupIndex(2);
                events().notifyDatasetAdded(pointDataset1);
                events().notifyDatasetAdded(pointDataset2);
                int numOfPointsDataset1 = dataset1Indices.size();
                int numOfPointsDataset2 = dataset2Indices.size();
                int numDimensions = mainDatasetFull->getNumDimensions();
                auto dimensionNames = mainDatasetFull->getDimensionNames();
                std::vector<int> dimensionIndices;
                for (int i = 0; i < numDimensions; ++i)
                {
                    dimensionIndices.push_back((i));
                }
                std::vector<float> datset1Data(numOfPointsDataset1 * numDimensions);
                std::vector<float> datset2Data(numOfPointsDataset2 * numDimensions);
                mainDatasetFull->populateDataForDimensions(datset1Data, dimensionIndices, dataset1Indices);
                mainDatasetFull->populateDataForDimensions(datset2Data, dimensionIndices, dataset2Indices);

                pointDataset1->setData(datset1Data.data(), numOfPointsDataset1, numDimensions);
                pointDataset1->setDimensionNames(dimensionNames);
                pointDataset2->setData(datset2Data.data(), numOfPointsDataset2, numDimensions);
                pointDataset2->setDimensionNames(dimensionNames);
                events().notifyDatasetDataChanged(pointDataset1);
              
                events().notifyDatasetDataChanged(pointDataset2);
                
                if (childrenDatasets.count() > 0)
                {
                    for (const auto& child : childrenDatasets)
                    {
                        if (child->getDataType() == PointType)
                        {
                            qDebug() << "LinePlotViewPlugin::createData: Found child dataset with name '" << child->getGuiName() << "'";
                            Dataset<Points> childDatasetFull = child->getFullDataset<Points>();
                            childDatasetFull->setGroupIndex(666);
                            Dataset<Points>  childPointDataset1 = mv::data().createDataset("Points", child->getGuiName() + "_" + pointDataset1->getGuiName(), pointDataset1);
                            childPointDataset1->setGroupIndex(1);
                            Dataset<Points>  childPointDataset2 = mv::data().createDataset("Points", child->getGuiName() + "_" + pointDataset2->getGuiName(), pointDataset2);
                            childPointDataset2->setGroupIndex(2);
                            events().notifyDatasetAdded(childPointDataset1);
                            events().notifyDatasetAdded(childPointDataset2);
                            int numDimensionsChild = childDatasetFull->getNumDimensions();
                            auto dimensionNamesChild = childDatasetFull->getDimensionNames();
                            std::vector<int> dimensionIndicesChild;
                            for (int i = 0; i < numDimensionsChild; ++i)
                            {
                                dimensionIndicesChild.push_back((i));
                            }

                            std::vector<float> childDatset1Data(numOfPointsDataset1 * numDimensionsChild);
                            std::vector<float> childDatset2Data(numOfPointsDataset2 * numDimensionsChild);
                            childDatasetFull->populateDataForDimensions(childDatset1Data, dimensionIndicesChild, dataset1Indices);
                            childDatasetFull->populateDataForDimensions(childDatset2Data, dimensionIndicesChild, dataset2Indices);
                            childPointDataset1->setData(childDatset1Data.data(), numOfPointsDataset1, numDimensionsChild);
                            childPointDataset1->setDimensionNames(dimensionNamesChild);
                            childPointDataset2->setData(childDatset2Data.data(), numOfPointsDataset2, numDimensionsChild);
                            childPointDataset2->setDimensionNames(dimensionNamesChild);
                            events().notifyDatasetDataChanged(childPointDataset1);
                         
                            events().notifyDatasetDataChanged(childPointDataset2);
                          
                        }
                        else if (child->getDataType() == ClusterType)
                        {
                            Dataset<Clusters> clusterDatasetFull = child->getFullDataset<Clusters>();
                            clusterDatasetFull->setGroupIndex(666);
                            Dataset<Clusters> childClusterDataset1 = mv::data().createDataset("Cluster", child->getGuiName() + "_" + pointDataset1->getGuiName(), pointDataset1);
                            childClusterDataset1->setGroupIndex(1);
                            Dataset<Clusters> childClusterDataset2 = mv::data().createDataset("Cluster", child->getGuiName() + "_" + pointDataset2->getGuiName(), pointDataset2);
                            childClusterDataset2->setGroupIndex(2);
                            events().notifyDatasetAdded(childClusterDataset1);
                            events().notifyDatasetAdded(childClusterDataset2);
                            auto clusters = clusterDatasetFull->getClusters();
                            for (const auto& cluster : clusters)
                            {
                                QString clusterName = cluster.getName();
                                auto clusterIndices = cluster.getIndices();
                                auto clusterColor = cluster.getColor();
                                std::vector<std::seed_seq::result_type> indicesDataset1;
                                std::vector<std::seed_seq::result_type> indicesDataset2;
                                for (const auto& index : clusterIndices)
                                {
                                    if (std::find(dataset1Indices.begin(), dataset1Indices.end(), index) != dataset1Indices.end())
                                    {
                                        indicesDataset1.push_back(index);
                                    }
                                    else if (std::find(dataset2Indices.begin(), dataset2Indices.end(), index) != dataset2Indices.end())
                                    {
                                        indicesDataset2.push_back(index);
                                    }
                                }

                                Cluster clusterValue1;
                                Cluster clusterValue2;

                                clusterValue1.setName(clusterName);
                                clusterValue1.setColor(clusterColor);
                                clusterValue1.setIndices(indicesDataset1);
                                childClusterDataset1->addCluster(clusterValue1);

                                clusterValue2.setName(clusterName);
                                clusterValue2.setColor(clusterColor);
                                clusterValue2.setIndices(indicesDataset2);
                                childClusterDataset2->addCluster(clusterValue2);

                            }
                            events().notifyDatasetDataChanged(childClusterDataset1);
                            events().notifyDatasetDataChanged(childClusterDataset2);
                        }
                        else
                        {
                            qDebug() << "LinePlotViewPlugin::createData: Found child dataset with name '" << child->getGuiName() << "' of unsupported type, skipping.";
                        }
                    }
                }




            }

        }
    }
    
    if (0)
    {
        for (const auto& dataset : datasets)
        {
            if (dataset->getGroupIndex() == 666)
            {
                qDebug() << "LinePlotViewPlugin::createData: Found dataset with group index 666, removing it.";
                mv::events().notifyDatasetAboutToBeRemoved(dataset);

                mv::data().removeDataset(dataset);

            }
        }
    }


    qDebug() << "LinePlotViewPlugin::createData: Method execution time:" << methodTimer.elapsed() << "ms";
}


void LinePlotViewPlugin::createDataOptimized()
{
    qDebug() << "createDataOptimized: ENTER";
    QElapsedTimer methodTimer;
    methodTimer.start();
    auto datasets = mv::data().getAllDatasets();
    qDebug() << "createDataOptimized: datasets.size() =" << datasets.size();

    if (datasets.empty())
    {
        qDebug() << "createDataOptimized: No datasets available, creating example data.";
        return;
    }

    // Step 1: Collect indices
    std::vector<int> dataset1Indices;
    std::vector<int> dataset2Indices;

    qDebug() << "createDataOptimized: Step 1 - Collect indices";
    for (const auto& dataset : datasets)
    {
        qDebug() << "createDataOptimized: Checking dataset" << dataset->getGuiName() << "type" << dataset->getDataType().getTypeString();
        if (dataset->getDataType() == ClusterType && dataset->getGuiName() == "brain_section_label")
        {
            qDebug() << "createDataOptimized: Found brain_section_label dataset";
            Dataset<Clusters> clusterDatasetFull = dataset->getFullDataset<Clusters>();
            auto clusters = clusterDatasetFull->getClusters();
            qDebug() << "createDataOptimized: clusters.size() =" << clusters.size();

            size_t count1 = 0, count2 = 0;
            for (const auto& cluster : clusters)
            {
                if (cluster.getName() == "CJ23.56.004.CX.42.16.06" ||
                    cluster.getName() == "CJ23.56.004.CX.42.15.06")
                {
                    count1 += cluster.getIndices().size();
                }
                else
                {
                    count2 += cluster.getIndices().size();
                }
            }
            qDebug() << "createDataOptimized: count1 =" << count1 << ", count2 =" << count2;

            dataset1Indices.reserve(count1);
            dataset2Indices.reserve(count2);

            for (const auto& cluster : clusters)
            {
                const auto& indices = cluster.getIndices();
                if (cluster.getName() == "CJ23.56.004.CX.42.16.06" ||
                    cluster.getName() == "CJ23.56.004.CX.42.15.06")
                {
                    dataset1Indices.insert(dataset1Indices.end(), indices.begin(), indices.end());
                }
                else
                {
                    dataset2Indices.insert(dataset2Indices.end(), indices.begin(), indices.end());
                }
            }
            qDebug() << "createDataOptimized: dataset1Indices.size() =" << dataset1Indices.size()
                     << ", dataset2Indices.size() =" << dataset2Indices.size();
            break;
        }
    }

    if (dataset1Indices.empty() || dataset2Indices.empty())
    {
        qDebug() << "createDataOptimized: No indices found in the datasets, creating example data.";
        return;
    }

    std::unordered_set<int> dataset1Lookup(dataset1Indices.begin(), dataset1Indices.end());
    std::unordered_set<int> dataset2Lookup(dataset2Indices.begin(), dataset2Indices.end());
    qDebug() << "createDataOptimized: Step 1 complete, lookup sets created";

    // Step 2: Process main Points dataset
    qDebug() << "createDataOptimized: Step 2 - Process main Points dataset";
    for (const auto& dataset : datasets)
    {
        qDebug() << "createDataOptimized: Checking dataset" << dataset->getGuiName() << "type" << dataset->getDataType().getTypeString();
        if (dataset->getDataType() == PointType && dataset->getGuiName() == "Xenium-CJ23")
        {
            qDebug() << "createDataOptimized: Found main Points dataset Xenium-CJ23";
            Dataset<Points> mainDatasetFull = dataset->getFullDataset<Points>();
            mainDatasetFull->setGroupIndex(666);

            const auto childrenDatasets = dataset->getChildren();
            qDebug() << "createDataOptimized: childrenDatasets.size() =" << childrenDatasets.size();
            const int numDimensions = mainDatasetFull->getNumDimensions();
            qDebug() << "createDataOptimized: numDimensions =" << numDimensions;
            const auto dimensionNames = mainDatasetFull->getDimensionNames();

            std::vector<int> dimensionIndices;
            dimensionIndices.reserve(numDimensions);
            for (int i = 0; i < numDimensions; ++i)
                dimensionIndices.push_back((i));
            qDebug() << "createDataOptimized: dimensionIndices.size() =" << dimensionIndices.size();

            // Create main datasets in main thread
            Dataset<Points> pointDataset1 = mv::data().createDataset("Points", "Section1_Xenium-CJ23");
            pointDataset1->setGroupIndex(1);
            Dataset<Points> pointDataset2 = mv::data().createDataset("Points", "Section2_Xenium-CJ23");
            pointDataset2->setGroupIndex(2);

            events().notifyDatasetAdded(pointDataset1);
            events().notifyDatasetAdded(pointDataset2);
            qDebug() << "createDataOptimized: Created and notified main point datasets";

            // Data containers for parallel processing
            struct ThreadData {
                std::vector<float> data;
                std::vector<int> indices;
                Dataset<Points> targetDataset;
                std::vector<QString> dimensionNames;
            };
            struct ThreadDataChild {
                std::vector<float> dataChild;
                std::vector<int> indices;
                Dataset<Points> targetDatasetChild;
                std::vector<QString> dimensionNamesChild;
            };
            // Prepare thread data
            ThreadData data1{
                std::vector<float>(dataset1Indices.size() * numDimensions),
                dataset1Indices,
                pointDataset1,
                dimensionNames
            };

            ThreadData data2{
                std::vector<float>(dataset2Indices.size() * numDimensions),
                dataset2Indices,
                pointDataset2,
                dimensionNames
            };

            // Lambda for processing data
            auto processData = [this, &mainDatasetFull, &dimensionIndices](ThreadData& td) {
                qDebug() << "createDataOptimized: processData: Populating data for targetDataset";
                mainDatasetFull->populateDataForDimensions(td.data, dimensionIndices, td.indices);

                QMetaObject::invokeMethod(this, [&]() {
                    qDebug() << "createDataOptimized: processData: Setting data and dimension names for targetDataset";
                    td.targetDataset->setData(td.data.data(), td.indices.size(), td.dimensionNames.size());
                    td.targetDataset->setDimensionNames(td.dimensionNames);
                    events().notifyDatasetDataChanged(td.targetDataset);
                    qDebug() << "createDataOptimized: processData: Notified data changed for targetDataset";
                    });
                };

            // Process in parallel
            qDebug() << "createDataOptimized: Launching QtConcurrent::run for main datasets";
            QFuture<void> future1 = QtConcurrent::run(processData, std::ref(data1));
            QFuture<void> future2 = QtConcurrent::run(processData, std::ref(data2));

            // Process children
            QVector<QFuture<void>> childFutures;
            int childIdx = 0;
            for (const auto& child : childrenDatasets)
            {
                qDebug() << "createDataOptimized: Processing child" << childIdx << "name:" << child->getGuiName() << "type:" << child->getDataType().getTypeString();
                if (child->getDataType() == PointType) {
                    Dataset<Points> childDatasetFull = child->getFullDataset<Points>();
                    childDatasetFull->setGroupIndex(666);

                    // Create child datasets in main thread
                    Dataset<Points> childPointDataset1 = mv::data().createDataset("Points",
                        child->getGuiName() + "_" + pointDataset1->getGuiName(), pointDataset1);
                    childPointDataset1->setGroupIndex(1);

                    Dataset<Points> childPointDataset2 = mv::data().createDataset("Points",
                        child->getGuiName() + "_" + pointDataset2->getGuiName(), pointDataset2);
                    childPointDataset2->setGroupIndex(2);

                    events().notifyDatasetAdded(childPointDataset1);
                    events().notifyDatasetAdded(childPointDataset2);
                    qDebug() << "createDataOptimized: Created and notified child point datasets for child" << childIdx;

                    int numDimensionsChild = childDatasetFull->getNumDimensions();
                    auto dimensionNamesChild = childDatasetFull->getDimensionNames();
                    std::vector<int> dimensionIndicesChild;
                    for (int i = 0; i < numDimensionsChild; ++i)
                    {
                        dimensionIndicesChild.push_back((i));
                    }
                    qDebug() << "createDataOptimized: numDimensionsChild =" << numDimensionsChild << ", dimensionIndicesChild.size() =" << dimensionIndicesChild.size();

                    // Prepare child thread data
                    ThreadDataChild childData1{
                        std::vector<float>(dataset1Indices.size() * numDimensionsChild),
                        dataset1Indices,
                        childPointDataset1,
                        dimensionNamesChild
                    };

                    ThreadDataChild childData2{
                        std::vector<float>(dataset2Indices.size() * numDimensionsChild),
                        dataset2Indices,
                        childPointDataset2,
                        dimensionNamesChild
                    };

                    // Process child data in parallel
                    qDebug() << "createDataOptimized: Launching QtConcurrent::run for child point datasets for child" << childIdx;
                    QFuture<void> childFuture1 = QtConcurrent::run([&, childIdx]() {
                        qDebug() << "createDataOptimized: childFuture1: Populating data for child" << childIdx;
                        childDatasetFull->populateDataForDimensions(childData1.dataChild, dimensionIndicesChild, childData1.indices);
                        QMetaObject::invokeMethod(this, [&]() {
                            qDebug() << "createDataOptimized: childFuture1: Setting data and dimension names for child" << childIdx;
                            childData1.targetDatasetChild->setData(childData1.dataChild.data(), childData1.indices.size(), childData1.dimensionNamesChild.size());
                            childData1.targetDatasetChild->setDimensionNames(childData1.dimensionNamesChild);
                            events().notifyDatasetDataChanged(childData1.targetDatasetChild);
                            qDebug() << "createDataOptimized: childFuture1: Notified data changed for child" << childIdx;
                            });
                        });

                    QFuture<void> childFuture2 = QtConcurrent::run([&, childIdx]() {
                        qDebug() << "createDataOptimized: childFuture2: Populating data for child" << childIdx;
                        childDatasetFull->populateDataForDimensions(childData2.dataChild, dimensionIndicesChild, childData2.indices);
                        QMetaObject::invokeMethod(this, [&]() {
                            qDebug() << "createDataOptimized: childFuture2: Setting data and dimension names for child" << childIdx;
                            childData2.targetDatasetChild->setData(childData2.dataChild.data(), childData2.indices.size(), childData2.dimensionNamesChild.size());
                            childData2.targetDatasetChild->setDimensionNames(childData2.dimensionNamesChild);
                            events().notifyDatasetDataChanged(childData2.targetDatasetChild);
                            qDebug() << "createDataOptimized: childFuture2: Notified data changed for child" << childIdx;
                            });
                        });

                    childFutures.append(childFuture1);
                    childFutures.append(childFuture2);
                }
                else if (child->getDataType() == ClusterType) {
                    qDebug() << "createDataOptimized: Processing cluster-type child" << childIdx;
                    // Process cluster-type children in main thread
                    Dataset<Clusters> clusterDatasetFull = child->getFullDataset<Clusters>();
                    clusterDatasetFull->setGroupIndex(666);

                    Dataset<Clusters> childClusterDataset1 = mv::data().createDataset("Cluster",
                        child->getGuiName() + "_" + pointDataset1->getGuiName(), pointDataset1);
                    childClusterDataset1->setGroupIndex(1);

                    Dataset<Clusters> childClusterDataset2 = mv::data().createDataset("Cluster",
                        child->getGuiName() + "_" + pointDataset2->getGuiName(), pointDataset2);
                    childClusterDataset2->setGroupIndex(2);

                    events().notifyDatasetAdded(childClusterDataset1);
                    events().notifyDatasetAdded(childClusterDataset2);

                    auto clusters = clusterDatasetFull->getClusters();
                    qDebug() << "createDataOptimized: clusterDatasetFull->getClusters().size() =" << clusters.size();
                    for (const auto& cluster : clusters)
                    {
                        std::vector<std::seed_seq::result_type> indices1, indices2;
                        const auto& clusterIndices = cluster.getIndices();

                        for (const auto& index : clusterIndices)
                        {
                            if (dataset1Lookup.count(index))
                                indices1.push_back(index);
                            else if (dataset2Lookup.count(index))
                                indices2.push_back(index);
                        }

                        Cluster cluster1, cluster2;
                        cluster1.setName(cluster.getName());
                        cluster1.setColor(cluster.getColor());
                        cluster1.setIndices(indices1);

                        cluster2.setName(cluster.getName());
                        cluster2.setColor(cluster.getColor());
                        cluster2.setIndices(indices2);

                        childClusterDataset1->addCluster(cluster1);
                        childClusterDataset2->addCluster(cluster2);
                    }

                    events().notifyDatasetDataChanged(childClusterDataset1);
                    events().notifyDatasetDataChanged(childClusterDataset2);
                    qDebug() << "createDataOptimized: Finished processing cluster-type child" << childIdx;
                }
                ++childIdx;
            }

            // Wait for all processing to complete
            qDebug() << "createDataOptimized: Waiting for main dataset futures";
            future1.waitForFinished();
            future2.waitForFinished();
            qDebug() << "createDataOptimized: Main dataset futures finished";

            // Wait for all child futures to complete
            qDebug() << "createDataOptimized: Waiting for child futures";
            for (auto& future : childFutures) {
                future.waitForFinished();
            }
            qDebug() << "createDataOptimized: All child futures finished";

            // Additional wait to ensure all events are processed
            QCoreApplication::processEvents();
            qDebug() << "createDataOptimized: QCoreApplication::processEvents() done";

            break;
        }
    }

    // Additional safety wait
    // Ensure all events are processed before deletion
    qDebug() << "createDataOptimized: Entering event processing safety loop";
    for (int i = 0; i < 20; ++i) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        QThread::msleep(10);
    }
    qDebug() << "createDataOptimized: Event processing safety loop done";



    qDebug() << "LinePlotViewPlugin::createDataOptimized: Method execution time:" << methodTimer.elapsed() << "ms";
    qDebug() << "createDataOptimized: EXIT";
}

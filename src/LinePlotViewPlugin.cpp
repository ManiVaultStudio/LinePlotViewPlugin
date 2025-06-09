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

    //createData();
    createDataOptimized();

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
    qDebug() << "LinePlotViewPlugin::createData: ENTER";
    QElapsedTimer methodTimer;
    methodTimer.start();
    auto datasets = mv::data().getAllDatasets();
    qDebug() << "LinePlotViewPlugin::createData: datasets.size() =" << datasets.size();

    if (datasets.isEmpty()) {
        qDebug() << "LinePlotViewPlugin::createData: No datasets available, creating example data.";
        return;
    }

    std::vector<int> dataset1Indices;
    std::vector<int> dataset2Indices;

    // === Identify cluster indices ===
    qDebug() << "LinePlotViewPlugin::createData: Identifying cluster indices...";
    for (const auto& dataset : datasets) {
        qDebug() << "LinePlotViewPlugin::createData: Checking dataset" << dataset->getGuiName() << "type" << dataset->getDataType().getTypeString();
        if (dataset->getDataType() != ClusterType) continue;
        if (dataset->getGuiName() != "brain_section_label") continue;

        qDebug() << "LinePlotViewPlugin::createData: Found dataset with name 'brain_section_label'";
        auto clusterDataset = dataset->getFullDataset<Clusters>();
        const auto& clusters = clusterDataset->getClusters();
        qDebug() << "LinePlotViewPlugin::createData: clusters.size() =" << clusters.size();

        for (const auto& cluster : clusters) {
            const auto& clusterName = cluster.getName();
            const auto& indices = cluster.getIndices();
            qDebug() << "LinePlotViewPlugin::createData: Cluster" << clusterName << "indices.size() =" << indices.size();

            if (clusterName == "CJ23.56.004.CX.42.16.06" || clusterName == "CJ23.56.004.CX.42.15.06") {
                dataset1Indices.insert(dataset1Indices.end(), indices.begin(), indices.end());
                qDebug() << "LinePlotViewPlugin::createData: Added" << indices.size() << "indices to dataset1Indices";
            }
            else {
                dataset2Indices.insert(dataset2Indices.end(), indices.begin(), indices.end());
                qDebug() << "LinePlotViewPlugin::createData: Added" << indices.size() << "indices to dataset2Indices";
            }
        }
    }
    qDebug() << "LinePlotViewPlugin::createData: dataset1Indices.size() =" << dataset1Indices.size()
             << ", dataset2Indices.size() =" << dataset2Indices.size();

    if (dataset1Indices.empty() || dataset2Indices.empty()) {
        qDebug() << "LinePlotViewPlugin::createData: No indices found in the datasets, creating example data.";
        return;
    }

    std::unordered_map<int, int> valueToIndex1, valueToIndex2;
    for (int i = 0; i < dataset1Indices.size(); ++i)
        valueToIndex1[dataset1Indices[i]] = i;
    for (int i = 0; i < dataset2Indices.size(); ++i)
        valueToIndex2[dataset2Indices[i]] = i;
    qDebug() << "LinePlotViewPlugin::createData: Created dataset1Set and dataset2Set";

    // === Process main dataset ===
    qDebug() << "LinePlotViewPlugin::createData: Processing main dataset...";
    for (const auto& dataset : datasets) {
        qDebug() << "LinePlotViewPlugin::createData: Checking dataset" << dataset->getGuiName() << "type" << dataset->getDataType().getTypeString();
        if (dataset->getDataType() != PointType) continue;
        if (dataset->getGuiName() != "Xenium-CJ23") continue;

        qDebug() << "LinePlotViewPlugin::createData: Found dataset with name 'Xenium-CJ23'";
        auto mainDataset = dataset->getFullDataset<Points>();
        mainDataset->setGroupIndex(666);

        qDebug() << "LinePlotViewPlugin::createData: Creating split datasets for Points";
        Dataset<Points> pointDataset1 = mv::data().createDataset("Points", "Section1_Xenium-CJ23");
        Dataset<Points> pointDataset2 = mv::data().createDataset("Points", "Section2_Xenium-CJ23");
        pointDataset1->setGroupIndex(1);
        pointDataset2->setGroupIndex(2);
        events().notifyDatasetAdded(pointDataset1);
        events().notifyDatasetAdded(pointDataset2);

        int numDimensions = mainDataset->getNumDimensions();
        auto dimensionNames = mainDataset->getDimensionNames();
        std::vector<int> dimIndices(numDimensions);
        std::iota(dimIndices.begin(), dimIndices.end(), 0);

        int size1 = static_cast<int>(dataset1Indices.size());
        int size2 = static_cast<int>(dataset2Indices.size());

        qDebug() << "LinePlotViewPlugin::createData: numDimensions =" << numDimensions
                 << ", size1 =" << size1 << ", size2 =" << size2;

        std::vector<float> data1(size1 * numDimensions);
        std::vector<float> data2(size2 * numDimensions);
        qDebug() << "LinePlotViewPlugin::createData: Populating data for Section1_Xenium-CJ23";
        mainDataset->populateDataForDimensions(data1, dimIndices, dataset1Indices);
        qDebug() << "LinePlotViewPlugin::createData: Populating data for Section2_Xenium-CJ23";
        mainDataset->populateDataForDimensions(data2, dimIndices, dataset2Indices);

        pointDataset1->setData(data1.data(), size1, numDimensions);
        pointDataset1->setDimensionNames(dimensionNames);
        pointDataset2->setData(data2.data(), size2, numDimensions);
        pointDataset2->setDimensionNames(dimensionNames);
        events().notifyDatasetDataChanged(pointDataset1);
        events().notifyDatasetDataChanged(pointDataset2);
        qDebug() << "LinePlotViewPlugin::createData: Finished setting data for split Points datasets";

        // === Process children ===
        qDebug() << "LinePlotViewPlugin::createData: Processing children datasets...";
        for (const auto& child : dataset->getChildren()) {
            auto childType = child->getDataType();
            auto childName = child->getGuiName();
            qDebug() << "LinePlotViewPlugin::createData: Child" << childName << "type" << childType.getTypeString();

            if (childType == PointType) {
                qDebug() << "LinePlotViewPlugin::createData: Found child dataset (PointType):" << childName;
                Dataset<Points> childDataset = child->getFullDataset<Points>();
                childDataset->setGroupIndex(666);

                Dataset<Points> child1 = mv::data().createDataset("Points", childName + "_" + pointDataset1->getGuiName(), pointDataset1);
                Dataset<Points> child2 = mv::data().createDataset("Points", childName + "_" + pointDataset2->getGuiName(), pointDataset2);
                child1->setGroupIndex(1);
                child2->setGroupIndex(2);
                events().notifyDatasetAdded(child1);
                events().notifyDatasetAdded(child2);

                int childDims = childDataset->getNumDimensions();
                auto childDimNames = childDataset->getDimensionNames();
                std::vector<int> childDimIndices(childDims);
                std::iota(childDimIndices.begin(), childDimIndices.end(), 0);

                qDebug() << "LinePlotViewPlugin::createData: childDims =" << childDims;

                std::vector<float> childData1(size1 * childDims);
                std::vector<float> childData2(size2 * childDims);
                qDebug() << "LinePlotViewPlugin::createData: Populating childData1 for" << childName;
                childDataset->populateDataForDimensions(childData1, childDimIndices, dataset1Indices);
                qDebug() << "LinePlotViewPlugin::createData: Populating childData2 for" << childName;
                childDataset->populateDataForDimensions(childData2, childDimIndices, dataset2Indices);

                child1->setData(childData1.data(), size1, childDims);
                child1->setDimensionNames(childDimNames);
                child2->setData(childData2.data(), size2, childDims);
                child2->setDimensionNames(childDimNames);
                events().notifyDatasetDataChanged(child1);
                events().notifyDatasetDataChanged(child2);
                qDebug() << "LinePlotViewPlugin::createData: Finished setting data for child Points datasets:" << childName;
            }
            else if (childType == ClusterType) {
                qDebug() << "LinePlotViewPlugin::createData: Found child dataset (ClusterType):" << childName;
                auto childCluster = child->getFullDataset<Clusters>();
                childCluster->setGroupIndex(666);

                Dataset<Clusters> childClus1 = mv::data().createDataset("Cluster", childName + "_" + pointDataset1->getGuiName(), pointDataset1);
                Dataset<Clusters> childClus2 = mv::data().createDataset("Cluster", childName + "_" + pointDataset2->getGuiName(), pointDataset2);
                childClus1->setGroupIndex(1);
                childClus2->setGroupIndex(2);
                events().notifyDatasetAdded(childClus1);
                events().notifyDatasetAdded(childClus2);

                int clusterIdx = 0;
                for (const auto& cluster : childCluster->getClusters()) {
                    const auto& name = cluster.getName();
                    const auto& color = cluster.getColor();
                    const auto& indices = cluster.getIndices();

                    std::vector<std::seed_seq::result_type> indices1;
                    std::vector<std::seed_seq::result_type> indices2;

                    for (int i : indices) {
                        if (auto it = valueToIndex1.find(i); it != valueToIndex1.end())
                            indices1.push_back(it->second);  // Index in dataset1Indices
                        else if (auto it = valueToIndex2.find(i); it != valueToIndex2.end())
                            indices2.push_back(it->second);  // Index in dataset2Indices
                    }

                    qDebug() << "LinePlotViewPlugin::createData: Cluster" << clusterIdx << "name:" << name
                             << "indices1.size() =" << indices1.size() << "indices2.size() =" << indices2.size();

                    Cluster clus1;
                    clus1.setName(name);
                    clus1.setColor(color);
                    clus1.setIndices(indices1);
                    childClus1->addCluster(clus1);

                    Cluster clus2;
                    clus2.setName(name);
                    clus2.setColor(color);
                    clus2.setIndices(indices2);
                    childClus2->addCluster(clus2);

                    ++clusterIdx;
                }

                events().notifyDatasetDataChanged(childClus1);
                events().notifyDatasetDataChanged(childClus2);
                qDebug() << "LinePlotViewPlugin::createData: Finished setting data for child Cluster datasets:" << childName;
            }
            else {
                qDebug() << "LinePlotViewPlugin::createData: Skipping unsupported child dataset type:" << childName;
            }
        }
        qDebug() << "LinePlotViewPlugin::createData: Finished processing children for main dataset";
    }

    // === Optional: Cleanup old datasets ===
    if (false) {
        qDebug() << "LinePlotViewPlugin::createData: Cleanup old datasets block entered";
        for (const auto& dataset : datasets) {
            if (dataset->getGroupIndex() == 666) {
                qDebug() << "LinePlotViewPlugin::createData: Removing dataset with group index 666:" << dataset->getGuiName();
                mv::events().notifyDatasetAboutToBeRemoved(dataset);
                mv::data().removeDataset(dataset);
            }
        }
    }

    qDebug() << "LinePlotViewPlugin::createData: Method execution time:" << methodTimer.elapsed() << "ms";
    qDebug() << "LinePlotViewPlugin::createData: EXIT";
}

void LinePlotViewPlugin::createDataOptimized()
{
    qDebug() << "createDataOptimized: ENTER";
    QElapsedTimer methodTimer;
    methodTimer.start();

    auto datasets = mv::data().getAllDatasets();
    qDebug() << "createDataOptimized: datasets.size() =" << datasets.size();
    if (datasets.empty()) {
        qDebug() << "createDataOptimized: No datasets available";
        return;
    }

    // Container to collect datasets for notifyDatasetDataChanged
    mv::Datasets datasetsToNotify;

    // Step 1: Collect indices
    std::vector<int> dataset1Indices, dataset2Indices;
    qDebug() << "createDataOptimized: Step 1 - Collect indices";

    for (const auto& ds : datasets) {
        qDebug() << "Checking dataset" << ds->getGuiName() << "type" << ds->getDataType().getTypeString();
        if (ds->getDataType() == ClusterType && ds->getGuiName() == "brain_section_label") {
            Dataset<Clusters> clusterDs = ds->getFullDataset<Clusters>();
            if (!clusterDs.isValid()) {
                qDebug() << "Invalid cluster dataset!";
                return;
            }
            auto clusters = clusterDs->getClusters();
            qDebug() << "clusters.size() =" << clusters.size();

            size_t count1 = 0, count2 = 0;
            for (const auto& cl : clusters) {
                if (cl.getName() == "CJ23.56.004.CX.42.16.06" ||
                    cl.getName() == "CJ23.56.004.CX.42.15.06")
                    count1 += cl.getIndices().size();
                else
                    count2 += cl.getIndices().size();
            }

            dataset1Indices.reserve(count1);
            dataset2Indices.reserve(count2);

            for (const auto& cl : clusters) {
                const auto& idx = cl.getIndices();
                if (cl.getName() == "CJ23.56.004.CX.42.16.06" ||
                    cl.getName() == "CJ23.56.004.CX.42.15.06")
                    dataset1Indices.insert(dataset1Indices.end(), idx.begin(), idx.end());
                else
                    dataset2Indices.insert(dataset2Indices.end(), idx.begin(), idx.end());
            }

            qDebug() << "dataset1Indices.size() =" << dataset1Indices.size()
                << ", dataset2Indices.size() =" << dataset2Indices.size();
            break;
        }
    }

    if (dataset1Indices.empty() || dataset2Indices.empty()) {
        qDebug() << "No indices found in the datasets";
        return;
    }

    std::unordered_map<int, int> valueToIndex1, valueToIndex2;
    for (int i = 0; i < dataset1Indices.size(); ++i)
        valueToIndex1[dataset1Indices[i]] = i;
    for (int i = 0; i < dataset2Indices.size(); ++i)
        valueToIndex2[dataset2Indices[i]] = i;
    qDebug() << "Step 1 complete, lookup sets created";

    // Step 2: Process main Points dataset
    qDebug() << "Step 2 - Process main Points dataset";
    for (const auto& ds : datasets) {
        if (ds->getDataType() == PointType && ds->getGuiName() == "Xenium-CJ23") {
            Dataset<Points> mainDs = ds->getFullDataset<Points>();
            if (!mainDs.isValid()) {
                qDebug() << "Invalid main Points dataset!";
                return;
            }
            mainDs->setGroupIndex(666);

            auto children = ds->getChildren();
            qDebug() << "childrenDatasets.size() =" << children.size();

            int numDim = mainDs->getNumDimensions();
            auto dimNames = mainDs->getDimensionNames();
            std::vector<int> allDims(numDim);
            std::iota(allDims.begin(), allDims.end(), 0);

            // Create split datasets
            auto createSplit = [&](const QString& suffix, int group)->Dataset<Points> {
                auto p = mv::data().createDataset("Points", suffix);
                p->setGroupIndex(group);
                events().notifyDatasetAdded(p);
                return p;
                };

            Dataset<Points> ds1 = createSplit("Section1_Xenium-CJ23", 1);
            Dataset<Points> ds2 = createSplit("Section2_Xenium-CJ23", 2);

            struct ThreadData {
                std::vector<float> data;
                std::vector<int> indices;
                Dataset<Points> target;
                std::vector<QString> dimNames;
            };

            ThreadData t1{ std::vector<float>(dataset1Indices.size() * numDim),
                           dataset1Indices, ds1, dimNames };
            ThreadData t2{ std::vector<float>(dataset2Indices.size() * numDim),
                           dataset2Indices, ds2, dimNames };

            // Instead of calling notifyDatasetDataChanged inside process, just collect the dataset
            auto process = [this, &mainDs, &allDims, &datasetsToNotify](ThreadData td) {
                mainDs->populateDataForDimensions(td.data, allDims, td.indices);
                QMetaObject::invokeMethod(this, [td = std::move(td), &datasetsToNotify]() mutable {
                    auto full = td.target->getFullDataset<Points>();
                    if (full.isValid()) {
                        full->setData(td.data.data(), td.indices.size(), td.dimNames.size());
                        full->setDimensionNames(td.dimNames);
                        datasetsToNotify.push_back(td.target); // collect for later notification
                    }
                    });
                };

            QFuture<void> f1 = QtConcurrent::run(process, std::move(t1));
            QFuture<void> f2 = QtConcurrent::run(process, std::move(t2));

            QVector<QFuture<void>> childFutures;
            int idx = 0;
            for (const auto& child : children) {
                qDebug() << "Processing child" << idx
                    << "name:" << child->getGuiName()
                    << "type:" << child->getDataType().getTypeString();

                if (child->getDataType() == PointType) {
                    Dataset<Points> childFull = child->getFullDataset<Points>();
                    if (!childFull.isValid()) {
                        qDebug() << "Invalid child Points dataset!";
                        continue;
                    }
                    childFull->setGroupIndex(666);

                    auto makeChildTargets = [&](Dataset<Points> base, Dataset<Points>& parentSplit) {
                        return mv::data().createDataset("Points",
                            base->getGuiName() + "_" + parentSplit->getGuiName(), parentSplit);
                        };

                    Dataset<Points> cp1 = makeChildTargets(child, ds1);
                    cp1->setGroupIndex(1);
                    events().notifyDatasetAdded(cp1);

                    Dataset<Points> cp2 = makeChildTargets(child, ds2);
                    cp2->setGroupIndex(2);
                    events().notifyDatasetAdded(cp2);

                    int nDim2 = childFull->getNumDimensions();
                    std::vector<QString> dn2 = childFull->getDimensionNames();
                    if (dn2.size() != nDim2) {
                        dn2.clear();
                        for (int d = 0; d < nDim2; ++d) dn2.push_back(QString("Dim_%1").arg(d + 1));
                    }
                    std::vector<int> dims2(nDim2);
                    std::iota(dims2.begin(), dims2.end(), 0);

                    struct CData {
                        std::vector<float> data;
                        std::vector<int> indices;
                        Dataset<Points> tgt;
                        std::vector<QString> dims;
                    };

                    CData cd1{ {}, dataset1Indices, cp1, dn2 };
                    cd1.data.resize(cd1.indices.size() * nDim2);
                    CData cd2{ {}, dataset2Indices, cp2, dn2 };
                    cd2.data.resize(cd2.indices.size() * nDim2);

                    auto spawnChild = [this, childFull, dims2, nDim2, &datasetsToNotify](CData cd) {
                        return [this, childFull, cd, dims2, nDim2, &datasetsToNotify]() mutable {
                            childFull->populateDataForDimensions(cd.data, dims2, cd.indices);
                            QMetaObject::invokeMethod(this, [cd = std::move(cd), nDim2, &datasetsToNotify]() mutable {
                                auto full = cd.tgt->getFullDataset<Points>();
                                if (full.isValid()) {
                                    full->setData(cd.data.data(), cd.indices.size(), nDim2);
                                    full->setDimensionNames(cd.dims);
                                    datasetsToNotify.push_back(cd.tgt); // collect for later notification
                                }
                                });
                            };
                        };

                    childFutures << QtConcurrent::run(spawnChild(cd1));
                    childFutures << QtConcurrent::run(spawnChild(cd2));
                }
                else if (child->getDataType() == ClusterType) {
                    qDebug() << "Processing cluster-type child" << idx;
                    Dataset<Clusters> cFull = child->getFullDataset<Clusters>();
                    if (!cFull.isValid()) {
                        qDebug() << "Invalid child Clusters dataset!";
                        continue;
                    }
                    cFull->setGroupIndex(666);

                    auto makeChildCluster = [&](Dataset<Points>& parentSplit) {
                        auto ds = mv::data().createDataset("Cluster",
                            child->getGuiName() + "_" + parentSplit->getGuiName(), parentSplit);
                        ds->setGroupIndex(parentSplit->getGroupIndex());
                        events().notifyDatasetAdded(ds);
                        return ds;
                        };

                    Dataset<Clusters> cl1 = makeChildCluster(ds1);
                    Dataset<Clusters> cl2 = makeChildCluster(ds2);

                    for (const auto& cl : cFull->getClusters()) {
                        std::vector<std::seed_seq::result_type> i1, i2;
                        const auto& indices = cl.getIndices();
                        for (int i : cl.getIndices()) {
                            if (auto it = valueToIndex1.find(i); it != valueToIndex1.end())
                                i1.push_back(it->second);  // Index in dataset1Indices
                            else if (auto it = valueToIndex2.find(i); it != valueToIndex2.end())
                                i2.push_back(it->second);  // Index in dataset2Indices
                        }
                        Cluster a = cl, b = cl;
                        a.setIndices(i1);
                        b.setIndices(i2);
                        cl1->addCluster(a);
                        cl2->addCluster(b);
                    }
                    datasetsToNotify.push_back(cl1);
                    datasetsToNotify.push_back(cl2);
                    qDebug() << "Finished processing cluster-type child" << idx;
                }
                ++idx;
            }

            f1.waitForFinished();
            f2.waitForFinished();
            for (auto& cf : childFutures) cf.waitForFinished();

            // Sequentially notify all datasets after all processing is finished
            for (auto& dsPtr : datasetsToNotify) {
                if (dsPtr.isValid())
                    events().notifyDatasetDataChanged(dsPtr);
            }

            QCoreApplication::processEvents();
            break;
        }
    }

    qDebug() << "createDataOptimized: execution time =" << methodTimer.elapsed() << "ms";
    qDebug() << "createDataOptimized: EXIT";
}

#include "GlobalSettingsAction.h"

#include <QHBoxLayout>

using namespace mv;
using namespace mv::gui;

GlobalSettingsAction::GlobalSettingsAction(QObject* parent, const plugin::PluginFactory* pluginFactory) :
    PluginGlobalSettingsGroupAction(parent, pluginFactory),
    _defaultPointSizeAction(this, "Default point Size", 1, 50, 10),
    _defaultPointOpacityAction(this, "Default point opacity", 0.f, 1.f, 0.5f)
{
    _defaultPointSizeAction.setToolTip("Default size of individual points");
    _defaultPointOpacityAction.setToolTip("Default opacity of individual points");

    // The add action automatically assigns a settings prefix to _pointSizeAction so there is no need to do this manually
    addAction(&_defaultPointSizeAction);
    addAction(&_defaultPointOpacityAction);
}

/*
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
*/
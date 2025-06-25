#include "SettingsAction.h"

#include <iostream>
#include <set>
#include "LinePlotViewPlugin.h"
#include <string>
#include <QFileDialog>
#include <QPageLayout>
#include <QWebEngineView>
#include <chrono>
#include <typeinfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStringList>
#include <QSet>
#include <QJsonValue>
using namespace mv;
using namespace mv::gui;

SettingsAction::SettingsAction(LinePlotViewPlugin& LinePlotViewPlugin) :
    WidgetAction(&LinePlotViewPlugin, "LinePlotViewPlugin"),
    _viewerPlugin(LinePlotViewPlugin),
    _datasetOptionsHolder(*this),
    _chartOptionsHolder(*this)
{
    setSerializationName("LinePlotViewPlugin:Settings");
    _datasetOptionsHolder.getPointDatasetAction().setSerializationName("LayerSurfer:PointDataset");
    _datasetOptionsHolder.getClusterDatasetAction().setSerializationName("LayerSurfer:ClusterDataset");

    _datasetOptionsHolder.getDataDimensionXSelectionAction().setSerializationName("LayerSurfer:DataDimensionXSelection");
    _datasetOptionsHolder.getDataDimensionYSelectionAction().setSerializationName("LayerSurfer:DataDimensionYSelection");

    _chartOptionsHolder.getSmoothingTypeAction().setSerializationName("LayerSurfer:SmoothingType");
    _chartOptionsHolder.getNormalizationTypeAction().setSerializationName("LayerSurfer:NormalizationType");
    _chartOptionsHolder.getSmoothingWindowAction().setSerializationName("LayerSurfer:SmoothingWindow");
    _chartOptionsHolder.getChartTitleAction().setSerializationName("LayerSurfer:ChartTitle");

    
    _datasetOptionsHolder.getPointDatasetAction().setToolTip("Point Dataset");
    _datasetOptionsHolder.getClusterDatasetAction().setToolTip("Cluster Dataset");

    _datasetOptionsHolder.getDataDimensionXSelectionAction().setToolTip("Data Dimension X Selection");
    _datasetOptionsHolder.getDataDimensionYSelectionAction().setToolTip("Data Dimension Y Selection");

    _chartOptionsHolder.getSmoothingTypeAction().setToolTip("Smoothing Type");
    _chartOptionsHolder.getNormalizationTypeAction().setToolTip("Normalization Type");
    _chartOptionsHolder.getSmoothingWindowAction().setToolTip("Smoothing Window");
    _chartOptionsHolder.getChartTitleAction().setToolTip("Chart Title");
   


    _datasetOptionsHolder.getPointDatasetAction().setFilterFunction([this](mv::Dataset<DatasetImpl> dataset) -> bool {
        return dataset->getDataType() == PointType;
    });

    _datasetOptionsHolder.getPointDatasetAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _datasetOptionsHolder.getClusterDatasetAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _datasetOptionsHolder.getDataDimensionXSelectionAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _datasetOptionsHolder.getDataDimensionYSelectionAction().setDefaultWidgetFlags(OptionAction::ComboBox);

    _chartOptionsHolder.getSmoothingTypeAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _chartOptionsHolder.getNormalizationTypeAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _chartOptionsHolder.getSmoothingWindowAction().setDefaultWidgetFlags(IntegralAction::SpinBox |IntegralAction::Slider);
    _chartOptionsHolder.getChartTitleAction().setDefaultWidgetFlags(OptionAction::LineEdit);
    _chartOptionsHolder.getChartTitleAction().setString("");


    _chartOptionsHolder.getSmoothingWindowAction().setMinimum(2);
    _chartOptionsHolder.getSmoothingWindowAction().setMaximum(1000);
    _chartOptionsHolder.getSmoothingWindowAction().setValue(5);


    _chartOptionsHolder.getSmoothingTypeAction().initialize(QStringList{
        "None",
        "Moving Average",
        "Savitzky-Golay",
        "Gaussian",
        "Exponential Moving Average",
        "Cubic Spline",
        "Linear Interpolation",
        "Min-Max Sampling",
        "Running Median"
        },"Moving Average");
    _chartOptionsHolder.getNormalizationTypeAction().initialize(QStringList{
        "None",
        "Z-Score",
        "Min-Max",
        "DecimalScaling"
        }, "None");





}

inline SettingsAction::DatasetOptionsHolder::DatasetOptionsHolder(SettingsAction& settingsAction) :
    HorizontalGroupAction(&settingsAction, "Dataset Options"),
    _settingsOptions(settingsAction),
    _pointDatasetAction(this, "Point dataset"),
    _dataDimensionXSelectionAction(this, "Data Dimension X Selection"),
    _dataDimensionYSelectionAction(this, "Data Dimension Y Selection"),
    _clusterDatasetAction(this, "Cluster dataset")
{
    setText("Dataset1 Options");
    setIcon(mv::util::StyledIcon("database"));
    setPopupSizeHint(QSize(300, 0));
    setConfigurationFlag(WidgetAction::ConfigurationFlag::Default);
    addAction(&_pointDatasetAction);
    addAction(&_dataDimensionXSelectionAction);
    addAction(&_dataDimensionYSelectionAction);
    addAction(&_clusterDatasetAction);
    //addAction(&_dataFromVariantAction);
    //addAction(&_lineDataVariant);
}

inline SettingsAction::ChartOptionsHolder::ChartOptionsHolder(SettingsAction& settingsAction) :
    VerticalGroupAction(&settingsAction, "Chart Options"),
    _settingsOptions(settingsAction),
    _smoothingTypeAction(this, "Snoothing Type"),
    _normalizationTypeAction(this, "Normalization Type"),
    _smoothingWindowAction(this, "Smoothing Window"),
    _chartTitleAction(this, "Chart Title")
{
    setText("Dataset1 Options");
    setIcon(mv::util::StyledIcon("database"));
    setPopupSizeHint(QSize(300, 0));
    setConfigurationFlag(WidgetAction::ConfigurationFlag::Default);
    addAction(&_smoothingTypeAction);
    addAction(&_smoothingWindowAction);
    addAction(&_normalizationTypeAction);
    addAction(&_chartTitleAction);
}

void SettingsAction::fromVariantMap(const QVariantMap& variantMap)
{
    WidgetAction::fromVariantMap(variantMap);
    _datasetOptionsHolder.getPointDatasetAction().fromParentVariantMap(variantMap);
    _datasetOptionsHolder.getClusterDatasetAction().fromParentVariantMap(variantMap);
    _datasetOptionsHolder.getDataDimensionXSelectionAction().fromParentVariantMap(variantMap);
    _datasetOptionsHolder.getDataDimensionYSelectionAction().fromParentVariantMap(variantMap);
    _chartOptionsHolder.getSmoothingTypeAction().fromParentVariantMap(variantMap);
    _chartOptionsHolder.getNormalizationTypeAction().fromParentVariantMap(variantMap);
    _chartOptionsHolder.getSmoothingWindowAction().fromParentVariantMap(variantMap);
    _chartOptionsHolder.getChartTitleAction().fromParentVariantMap(variantMap);


}

QVariantMap SettingsAction::toVariantMap() const
{
    QVariantMap variantMap = WidgetAction::toVariantMap();
    _datasetOptionsHolder.getPointDatasetAction().insertIntoVariantMap(variantMap);
    _datasetOptionsHolder.getClusterDatasetAction().insertIntoVariantMap(variantMap);
    _datasetOptionsHolder.getDataDimensionXSelectionAction().insertIntoVariantMap(variantMap);
    _datasetOptionsHolder.getDataDimensionYSelectionAction().insertIntoVariantMap(variantMap);
    _chartOptionsHolder.getSmoothingTypeAction().insertIntoVariantMap(variantMap);
    _chartOptionsHolder.getNormalizationTypeAction().insertIntoVariantMap(variantMap);
    _chartOptionsHolder.getSmoothingWindowAction().insertIntoVariantMap(variantMap);
    _chartOptionsHolder.getChartTitleAction().insertIntoVariantMap(variantMap);

    return variantMap;
}
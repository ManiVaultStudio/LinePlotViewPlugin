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
    _datasetOptionsHolder(*this)
{
    setSerializationName("LinePlotViewPlugin:Settings");
    _datasetOptionsHolder.getPointDatasetAction().setSerializationName("LayerSurfer:PointDataset");
    _datasetOptionsHolder.getLineDataVariantAction().setSerializationName("LayerSurfer:LineDataVariant");
    _datasetOptionsHolder.getDataDimensionSelectionAction().setSerializationName("LayerSurfer:DataDimensionSelection");
    
    _datasetOptionsHolder.getPointDatasetAction().setToolTip("Point Dataset");
    _datasetOptionsHolder.getLineDataVariantAction().setToolTip("Line Data Variant");
    _datasetOptionsHolder.getDataDimensionSelectionAction().setToolTip("Data Dimension Selection");

    _datasetOptionsHolder.getPointDatasetAction().setFilterFunction([this](mv::Dataset<DatasetImpl> dataset) -> bool {
        return dataset->getDataType() == PointType;
    });

    _datasetOptionsHolder.getPointDatasetAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _datasetOptionsHolder.getDataDimensionSelectionAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _datasetOptionsHolder.getLineDataVariantAction().setDefaultWidgetFlags(VariantAction::TextHeuristicRole);


}

inline SettingsAction::DatasetOptionsHolder::DatasetOptionsHolder(SettingsAction& settingsAction) :
    HorizontalGroupAction(&settingsAction, "Dataset Options"),
    _settingsOptions(settingsAction),
    _pointDatasetAction(this, "Point dataset"),
    _lineDataVariant(this, "Line Data Variant"),
    _dataDimensionSelectionAction(this, "Data Dimension Selection")
{
    setText("Dataset1 Options");
    setIcon(mv::util::StyledIcon("database"));
    setPopupSizeHint(QSize(300, 0));
    setConfigurationFlag(WidgetAction::ConfigurationFlag::Default);
    addAction(&_pointDatasetAction);
    addAction(&_dataDimensionSelectionAction);
    addAction(&_lineDataVariant);
}

void SettingsAction::fromVariantMap(const QVariantMap& variantMap)
{
    WidgetAction::fromVariantMap(variantMap);
    _datasetOptionsHolder.getPointDatasetAction().fromParentVariantMap(variantMap);
    _datasetOptionsHolder.getDataDimensionSelectionAction().fromParentVariantMap(variantMap);
    _datasetOptionsHolder.getLineDataVariantAction().fromParentVariantMap(variantMap);
}

QVariantMap SettingsAction::toVariantMap() const
{
    QVariantMap variantMap = WidgetAction::toVariantMap();
    _datasetOptionsHolder.getPointDatasetAction().insertIntoVariantMap(variantMap);
    _datasetOptionsHolder.getDataDimensionSelectionAction().insertIntoVariantMap(variantMap);
    _datasetOptionsHolder.getLineDataVariantAction().insertIntoVariantMap(variantMap);
    return variantMap;
}
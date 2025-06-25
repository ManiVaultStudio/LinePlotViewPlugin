#pragma once

#include <actions/WidgetAction.h>
#include <actions/IntegralAction.h>
#include <actions/OptionAction.h>
#include <actions/OptionsAction.h>
#include <actions/VariantAction.h>
#include "ClusterData/ClusterData.h"
#include "PointData/PointData.h"
#include <actions/ToggleAction.h>
#include "actions/DatasetPickerAction.h"
#include <PointData/DimensionPickerAction.h>
#include "event/EventListener.h"
#include "actions/Actions.h"
#include "Plugin.h"
#include "DataHierarchyItem.h"
#include "Set.h"
#include <AnalysisPlugin.h>
#include <memory>
#include <algorithm>    
#include <QDebug>
#include <QLabel>
#include <QComboBox>
#include <QGroupBox>
#include <QPushButton>
#include <QGridLayout>
#include <QFormLayout>
#include <QString>
#include <string>
#include <QRadioButton>
#include <event/Event.h>
#include <PointData/DimensionPickerAction.h>
#include <QDebug>
#include <QLabel>
#include <string>
#include <actions/ColorMap1DAction.h>
#include <set>
#include <actions/HorizontalToolbarAction.h>
#include <actions/VerticalToolbarAction.h>
#include "QStatusBar"

using namespace mv::gui;
class QMenu;
class LinePlotViewPlugin;

namespace mv
{
    class CoreInterface;
}

class SettingsAction : public WidgetAction
{
public:
    class DatasetOptionsHolder : public HorizontalGroupAction
    {
    public:
        DatasetOptionsHolder(SettingsAction& settingsAction);

        const DatasetPickerAction& getPointDatasetAction() const { return _pointDatasetAction; }
        DatasetPickerAction& getPointDatasetAction() { return _pointDatasetAction; }

        const DimensionPickerAction& getDataDimensionXSelectionAction() const { return _dataDimensionXSelectionAction; }
        DimensionPickerAction& getDataDimensionXSelectionAction() { return _dataDimensionXSelectionAction; }

        const DimensionPickerAction& getDataDimensionYSelectionAction() const { return _dataDimensionYSelectionAction; }
        DimensionPickerAction& getDataDimensionYSelectionAction() { return _dataDimensionYSelectionAction; }

        const DatasetPickerAction& getClusterDatasetAction() const { return _clusterDatasetAction; }
        DatasetPickerAction& getClusterDatasetAction() { return _clusterDatasetAction; }

    protected:
        SettingsAction& _settingsOptions;
        DatasetPickerAction _pointDatasetAction;
        DatasetPickerAction _clusterDatasetAction;
        DimensionPickerAction _dataDimensionXSelectionAction;
        DimensionPickerAction _dataDimensionYSelectionAction;
    };

    class ChartOptionsHolder : public VerticalGroupAction
    {
    public:
        ChartOptionsHolder(SettingsAction& settingsAction);



        const OptionAction& getSmoothingTypeAction() const { return _smoothingTypeAction; }
        OptionAction& getSmoothingTypeAction() { return _smoothingTypeAction; }
        const OptionAction& getNormalizationTypeAction() const { return _normalizationTypeAction; }
        OptionAction& getNormalizationTypeAction() { return _normalizationTypeAction; }
        const IntegralAction& getSmoothingWindowAction() const { return _smoothingWindowAction; }
        IntegralAction& getSmoothingWindowAction() { return _smoothingWindowAction; }

        const StringAction& getChartTitleAction() const { return _chartTitleAction; }
        StringAction& getChartTitleAction() { return _chartTitleAction; }

    protected:
        SettingsAction& _settingsOptions;

        OptionAction    _smoothingTypeAction;
        OptionAction    _normalizationTypeAction;
        IntegralAction    _smoothingWindowAction;
        StringAction    _chartTitleAction;
    };

public:
    SettingsAction(LinePlotViewPlugin& LinePlotViewPlugin);

    DatasetOptionsHolder& getDatasetOptionsHolder() { return _datasetOptionsHolder; }
    ChartOptionsHolder& getChartOptionsHolder() { return _chartOptionsHolder; }

public:
    void fromVariantMap(const QVariantMap& variantMap) override;
    QVariantMap toVariantMap() const override;



protected:
    LinePlotViewPlugin& _viewerPlugin;
    mv::CoreInterface* _core;
    DatasetOptionsHolder _datasetOptionsHolder;
    ChartOptionsHolder _chartOptionsHolder;

    friend class ChannelAction;
};
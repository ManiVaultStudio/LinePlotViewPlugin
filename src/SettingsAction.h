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

        const VariantAction& getLineDataVariantAction() const { return _lineDataVariant; }
        VariantAction& getLineDataVariantAction() { return _lineDataVariant; }

        const DimensionPickerAction& getDataDimensionSelectionAction() const { return _dataDimensionSelectionAction; }
        DimensionPickerAction& getDataDimensionSelectionAction() { return _dataDimensionSelectionAction; }

    protected:
        SettingsAction& _settingsOptions;
        DatasetPickerAction _pointDatasetAction;
        DimensionPickerAction _dataDimensionSelectionAction;
        VariantAction _lineDataVariant;
    };

public:
    SettingsAction(LinePlotViewPlugin& LinePlotViewPlugin);

    DatasetOptionsHolder& getDatasetOptionsHolder() { return _datasetOptionsHolder; }

public:
    void fromVariantMap(const QVariantMap& variantMap) override;
    QVariantMap toVariantMap() const override;



protected:
    LinePlotViewPlugin& _viewerPlugin;
    mv::CoreInterface* _core;
    DatasetOptionsHolder _datasetOptionsHolder;


    friend class ChannelAction;
};
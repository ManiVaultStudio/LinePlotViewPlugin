#pragma once

#include <ViewPlugin.h>
#include <Dataset.h>
#include <widgets/DropWidget.h>
#include <PointData/PointData.h>
#include <ClusterData/ClusterData.h>
#include <actions/PluginStatusBarAction.h>

#include "SettingsAction.h"
#include<unordered_set>
#include <QWidget>
#include "HighPerfLineChart.h"

/** All plugin related classes are in the ManiVault plugin namespace */
using namespace mv::plugin;

/** Drop widget used in this plugin is located in the ManiVault gui namespace */
using namespace mv::gui;

/** Dataset reference used in this plugin is located in the ManiVault util namespace */
using namespace mv::util;

class LinePlotViewWidget;

class LinePlotViewPlugin : public ViewPlugin
{
    Q_OBJECT

public:

    /**
     * Constructor
     * @param factory Pointer to the plugin factory
     */
    LinePlotViewPlugin(const PluginFactory* factory);

    /** Destructor */
    ~LinePlotViewPlugin() override = default;
    
    /** This function is called by the core after the view plugin has been created */
    void init() override;

    /** Store a private reference to the data set that should be displayed */
    void loadData(const mv::Datasets& datasets) override;

    /** Retrieves data to be shown and updates the OpenGL plot */
    void updatePlot();


private:

    QString getCurrentDataSetID() const;

protected:
    DropWidget*                 _dropWidget;            /** Widget for drag and drop behavior */
    LinePlotViewWidget*            _linePlotViewWidget;       /** The OpenGL widget */
    SettingsAction              _settingsAction;        /** Settings action */
    mv::Dataset<Points>         _currentDataSet;        /** Points smart pointer */
    std::vector<unsigned int>   _currentDimensions;     /** Stores which dimensions of the current data are shown */
    HighPerfLineChart* _lineChartWidget = nullptr;
};


class LinePlotViewPluginFactory : public ViewPluginFactory
{
    Q_INTERFACES(mv::plugin::ViewPluginFactory mv::plugin::PluginFactory)
    Q_OBJECT
    Q_PLUGIN_METADATA(IID   "studio.manivault.LinePlotViewPlugin"
                      FILE  "PluginInfo.json")

public:

    /** Default constructor */
    LinePlotViewPluginFactory();

    /** Perform post-construction initialization */
    void initialize() override;

    ViewPlugin* produce() override;

    mv::DataTypes supportedDataTypes() const override;

    PluginTriggerActions getPluginTriggerActions(const mv::Datasets& datasets) const override;

private:
    PluginStatusBarAction*  _statusBarAction;               /** For global action in a status bar */
    HorizontalGroupAction   _statusBarPopupGroupAction;     /** Popup group action for status bar action */
    StringAction            _statusBarPopupAction;          /** Popup action for the status bar */
    
};

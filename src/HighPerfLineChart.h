#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QVector>
#include <QColor>
#include <QString>
#include <QMap>
#include <map>
#include <unordered_map>
#include <QFont>
#include <QPen>

enum class LineDashStyle {
    Solid,
    Dash,
    Dot
};

namespace {
    enum OutCode {
        INSIDE = 0, // 0000
        LEFT = 1, // 0001
        RIGHT = 2, // 0010
        BOTTOM = 4, // 0100
        TOP = 8  // 1000
    };

    int computeOutCode(double x, double y, double xmin, double xmax, double ymin, double ymax) {
        int code = OutCode::INSIDE;
        if (x < xmin)       code |= OutCode::LEFT;
        else if (x > xmax)  code |= OutCode::RIGHT;
        if (y < ymin)       code |= OutCode::BOTTOM;
        else if (y > ymax)  code |= OutCode::TOP;
        return code;
    }

    // Returns true if a clipped segment exists, and modifies (x0,y0,x1,y1) to the clipped segment
    bool cohenSutherlandClip(double& x0, double& y0, double& x1, double& y1, double xmin, double xmax, double ymin, double ymax) {
        int outcode0 = computeOutCode(x0, y0, xmin, xmax, ymin, ymax);
        int outcode1 = computeOutCode(x1, y1, xmin, xmax, ymin, ymax);
        bool accept = false;

        while (true) {
            if (!(outcode0 | outcode1)) {
                accept = true;
                break;
            }
            else if (outcode0 & outcode1) {
                break;
            }
            else {
                double x, y;
                int outcodeOut = outcode0 ? outcode0 : outcode1;
                if (outcodeOut & OutCode::TOP) {
                    x = x0 + (x1 - x0) * (ymax - y0) / (y1 - y0);
                    y = ymax;
                }
                else if (outcodeOut & OutCode::BOTTOM) {
                    x = x0 + (x1 - x0) * (ymin - y0) / (y1 - y0);
                    y = ymin;
                }
                else if (outcodeOut & OutCode::RIGHT) {
                    y = y0 + (y1 - y0) * (xmax - x0) / (x1 - x0);
                    x = xmax;
                }
                else { // LEFT
                    y = y0 + (y1 - y0) * (xmin - x0) / (x1 - x0);
                    x = xmin;
                }
                if (outcodeOut == outcode0) {
                    x0 = x; y0 = y;
                    outcode0 = computeOutCode(x0, y0, xmin, xmax, ymin, ymax);
                }
                else {
                    x1 = x; y1 = y;
                    outcode1 = computeOutCode(x1, y1, xmin, xmax, ymin, ymax);
                }
            }
        }
        return accept;
    }
} // end anonymous namespace



struct LineData {
    QVector<QPointF> points;
    QColor color = Qt::black;
    float opacity = 1.0f;
    bool selected = false;
    float lineWidth = 0.0f;  
    float pointSize = 0.0f;  
    LineDashStyle dashStyle = LineDashStyle::Solid; 
};

enum class BinaryNormAxis { X, Y };

class HighPerfLineChart : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT
public:
    explicit HighPerfLineChart(QWidget* parent = nullptr);

    // Data and appearance
    void setLines(const QVector<LineData>& lines);
    void setXAxisName(const QString& name);
    void setYAxisName(const QString& name);
    void setXRange(double min, double max);
    void setYRange(double min, double max);
    void clear();
    void setEmptyMessage(const QString& msg);
    void setChartTitle(const QString& title); 

    // Selection/hover
    int selectedLine() const;
    int hoveredLine() const;


    // --- Customization API ---
    void setLineWidth(float w);
    void setPointSize(float s);
    void setAxisFont(const QFont& f);
    void setAxisColor(const QColor& c);
    void setNumXTicks(int n);
    void setNumYTicks(int n);
    void setGridEnabled(bool en);
    void setGridColor(const QColor& c);
    void setBackgroundColor(const QColor& c);
    void setShowPoints(bool en);
    bool showPoints() const;

    // NEW: show/hide lines connecting points
    void setShowLines(bool en);
    bool showLines() const;

    // New customization options
    void setAxisLineWidth(float w);
    void setXAxisLineColor(const QColor& c);
    void setYAxisLineColor(const QColor& c);
    void setTickLabelFont(const QFont& f);
    void setTickLabelColor(const QColor& c);
    void setChartMargins(int left, int right, int top, int bottom);
    void setLegendFont(const QFont& f);
    void setLegendColor(const QColor& c);
    void setLegendPosition(int x, int y);


    // New: show/hide all lines
    void setShowAllLines(bool show);
    bool showAllLines() const;



    // Legend show/hide
    void setShowLegend(bool show);
    bool showLegend() const;

    // Option to draw a line between the statistic (avg/median/mode) of first n and last n points
    enum class ConnectStatsType { None, Average, Median, Mode, Mean };

    void setConnectStatsLineEnabled(bool enabled);
    bool connectStatsLineEnabled() const;
    void setConnectStatsLineN(int n);
    int connectStatsLineN() const;
    void setConnectStatsLineType(ConnectStatsType type);
    ConnectStatsType connectStatsLineType() const;

    // Add setter/getter for the label visibility
    void setShowAvgNumPointsLabel(bool show);
    bool showAvgNumPointsLabel() const;

    // --- Moving average smoothing ---
    void setShowMovingAverageLine(bool show);
    bool showMovingAverageLine() const;
    void setMovingAverageWindow(int window);
    int movingAverageWindow() const;

signals:
    void lineSelected(int index);
    void lineHovered(int index);

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

    // For connecting stats line
    bool m_connectStatsLineEnabled = false;
    int m_connectStatsLineN = 5;
    ConnectStatsType m_connectStatsLineType = ConnectStatsType::None;

    // Add a flag to show/hide average number of points label
    bool m_showAvgNumPointsLabel = true;

private:
    QVector<LineData> m_lines;
    QString m_xAxisName, m_yAxisName;
    double m_xMin = 0, m_xMax = 1, m_yMin = 0, m_yMax = 1;
    int m_selectedLine = -1;
    int m_hoveredLine = -1;
    QString m_emptyMessage;
    QString m_chartTitle; 

    QColor slopeToColor(double slope) const;

    QPointF screenToChart(const QPoint& p) const;
    int findLineAt(const QPoint& pos) const;

    // --- Customization members ---
    float m_lineWidth;
    float m_pointSize;
    QFont m_axisFont;
    QColor m_axisColor;
    int m_numXTicks;
    int m_numYTicks;
    bool m_gridEnabled;
    QColor m_gridColor;
    QColor m_backgroundColor;
    bool m_showPoints = true;
    bool m_showLines = true; // NEW: show/hide lines connecting points

    // New customization members
    float m_axisLineWidth = 1.0f;
    QColor m_xAxisLineColor = Qt::black;
    QColor m_yAxisLineColor = Qt::black;
    QFont m_tickLabelFont;
    QColor m_tickLabelColor = Qt::black;
    int m_leftMargin = 50, m_rightMargin = 20, m_topMargin = 40, m_bottomMargin = 40;
    QFont m_legendFont;
    QColor m_legendColor = Qt::black;
    int m_legendX = -1, m_legendY = -1; // -1 means auto


    // --- Show/hide all lines member ---
    bool m_showAllLines = true;

    // --- Global line style members ---
    QColor m_globalLineColor = QColor();
    float m_globalLineWidth = 1.0f;
    float m_globalLineOpacity = 1.0f;

    // Legend show/hide
    bool m_showLegend = true; // Show/hide legend

    // --- Moving average smoothing ---
    bool m_showMovingAverageLine = false;
    int m_movingAverageWindow = 5;
    QVector<QPointF> movingAverage(const QVector<QPointF>& points, int window) const;
    QString m_movingAverageLineLabel;

    // --- New helper methods for refactored paintGL ---
    void drawChartWithOpenGL();
    void drawAxesAndLabels(QPainter& painter);
    void drawLegend(QPainter& painter);
    void drawAverageLabel(QPainter& painter);
    void updateAxisRange();
};
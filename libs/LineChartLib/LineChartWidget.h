#pragma once

#include <QWidget>
#include <QVector>
#include <QPair>
#include <QColor>
#include <QVariantMap>
#include <QString>
#include <QRectF>

class LineChartWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LineChartWidget(QWidget* parent = nullptr);

    void setData(const QVector<QPair<float, float>>& points,
        const QVector<QPair<QString, QColor>>& categories = {},
        const QVariantMap& statLine = QVariantMap(),
        const QString& title = QString(),
        const QColor& lineColor = QColor("#1f77b4"));
    void setData(const QVariantMap& root);
    void setShowEnvelope(bool show);
    void setShowStatLine(bool show);
    void setNoDataMessage(const QString& msg);
protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    QVector<QPair<float, float>> m_points;
    QVector<QPair<QString, QColor>> m_categories;
    QVariantMap m_statLine;
    QString m_title;
    QColor m_lineColor = QColor("#1f77b4");
    QString m_xAxisName = "X";
    QString m_yAxisName = "Y";
    QVector<QPair<float, float>> m_originalPoints;
    QRectF m_plotArea;
    double m_xMin = 0, m_xMax = 0, m_yMin = 0, m_yMax = 0;
    bool m_showEnvelope = true;
    bool m_showStatLine = false;
    int m_hoveredLineIdx = -1;
    int m_hoveredBarIdx = -1;
    QString m_noDataMessage = "No data available or insufficient data for chart.";
    void updatePlotArea();
    QPointF dataToScreen(float x, float y) const;
    float screenToDataX(int px) const;
    float screenToDataY(int py) const;
    int findNearestLineSegment(const QPoint& pos, double& minDist) const;
    int findCategoryBarAt(const QPoint& pos) const;
    void showTooltip(const QPoint& pos, const QString& text);
    void hideTooltip();
};
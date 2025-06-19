#pragma once

#include <QOpenGLWidget>
#include <QVector>
#include <QPair>
#include <QColor>
#include <QVariantMap>
#include <QString>
#include <QRectF>

class LineChartWidget : public QOpenGLWidget
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

protected:
    void paintGL() override;
    void resizeGL(int w, int h) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    QVector<QPair<float, float>> m_points;
    QVector<QPair<QString, QColor>> m_categories;
    QVariantMap m_statLine;
    QString m_title;
    QColor m_lineColor = QColor("#1f77b4");

    QRectF m_plotArea;
    double m_xMin = 0, m_xMax = 0, m_yMin = 0, m_yMax = 0;

    int m_hoveredLineIdx = -1;
    int m_hoveredBarIdx = -1;

    void updatePlotArea();
    QPointF dataToScreen(float x, float y) const;
    float screenToDataX(int px) const;
    float screenToDataY(int py) const;
    int findNearestLineSegment(const QPoint& pos, double& minDist) const;
    int findCategoryBarAt(const QPoint& pos) const;
    void showTooltip(const QPoint& pos, const QString& text);
    void hideTooltip();
};
#include "LineChartWidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QToolTip>
#include <algorithm>
#include <cmath>

LineChartWidget::LineChartWidget(QWidget* parent)
    : QWidget(parent)
{
    setMouseTracking(true);
}

void LineChartWidget::setData(const QVector<QPair<float, float>>& points,
    const QVector<QPair<QString, QColor>>& categories,
    const QVariantMap& statLine,
    const QString& title,
    const QColor& lineColor)
{
    m_points = points;
    m_categories = categories;
    m_statLine = statLine;
    m_title = title;
    m_lineColor = lineColor;
    update();
}

void LineChartWidget::setData(const QVariantMap& root)
{
    QVector<QPair<float, float>> points;
    QVector<QPair<QString, QColor>> categories;
    QColor lineColor = QColor("#1f77b4");
    QVariantList dataList = root.value("data").toList();
    for (const QVariant& v : dataList) {
        QVariantMap m = v.toMap();
        float x = m.value("x").toFloat();
        float y = m.value("y").toFloat();
        points.append({ x, y });
        // Parse category if present
        if (m.contains("category")) {
            QVariant catVar = m.value("category");
            if (catVar.canConvert<QVariantList>()) {
                QVariantList cat = catVar.toList();
                if (cat.size() == 2)
                    categories.append({ cat[1].toString(), QColor(cat[0].toString()) });
                else
                    categories.append({ "", QColor() });
            }
            else if (catVar.canConvert<QString>()) {
                categories.append({ catVar.toString(), QColor() });
            }
            else {
                categories.append({ "", QColor() });
            }
        }
        else {
            categories.append({ "", QColor() });
        }
    }
    QVariantMap statLine = root.value("statLine").toMap();
    QString title = root.value("title").toString();
    if (root.contains("lineColor")) {
        QVariant colorVar = root.value("lineColor");
        if (colorVar.canConvert<QColor>())
            lineColor = colorVar.value<QColor>();
        else
            lineColor = QColor(colorVar.toString());
    }
    setData(points, categories, statLine, title, lineColor);
}

void LineChartWidget::resizeEvent(QResizeEvent*)
{
    updatePlotArea();
}

void LineChartWidget::updatePlotArea()
{
    // Margins: left, right, top, bottom
    int l = 60, r = 30, t = 60, b = 40;
    m_plotArea = QRectF(l, t, width() - l - r, height() - t - b);

    // Compute data bounds
    if (m_points.size() < 2) {
        m_xMin = m_xMax = m_yMin = m_yMax = 0;
        return;
    }
    m_xMin = m_xMax = m_points[0].first;
    m_yMin = m_yMax = m_points[0].second;
    for (const auto& pt : m_points) {
        m_xMin = std::min(m_xMin, (double)pt.first);
        m_xMax = std::max(m_xMax, (double)pt.first);
        m_yMin = std::min(m_yMin, (double)pt.second);
        m_yMax = std::max(m_yMax, (double)pt.second);
    }
    // Expand bounds a bit for aesthetics
    double xPad = (m_xMax - m_xMin) * 0.05;
    double yPad = (m_yMax - m_yMin) * 0.1;
    if (xPad == 0) xPad = 1.0;
    if (yPad == 0) yPad = 1.0;
    m_xMin -= xPad; m_xMax += xPad;
    m_yMin -= yPad; m_yMax += yPad;
}

QPointF LineChartWidget::dataToScreen(float x, float y) const
{
    if (m_plotArea.width() <= 0 || m_plotArea.height() <= 0)
        return QPointF();
    double sx = m_plotArea.left() + (x - m_xMin) / (m_xMax - m_xMin) * m_plotArea.width();
    double sy = m_plotArea.bottom() - (y - m_yMin) / (m_yMax - m_yMin) * m_plotArea.height();
    return QPointF(sx, sy);
}

float LineChartWidget::screenToDataX(int px) const
{
    return m_xMin + (px - m_plotArea.left()) / m_plotArea.width() * (m_xMax - m_xMin);
}
float LineChartWidget::screenToDataY(int py) const
{
    return m_yMax - (py - m_plotArea.top()) / m_plotArea.height() * (m_yMax - m_yMin);
}

void LineChartWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Background
    p.fillRect(rect(), Qt::white);

    // No data
    if (m_points.size() < 2) {
        p.setPen(QColor("#888"));
        p.setFont(QFont("sans", 18, QFont::Bold));
        p.drawText(rect(), Qt::AlignCenter, "No data available or insufficient data for chart.");
        return;
    }

    updatePlotArea();

    // Draw title
    if (!m_title.isEmpty()) {
        p.setFont(QFont("sans", 16, QFont::Bold));
        p.setPen(QColor("#222"));
        p.drawText(QRectF(0, 0, width(), 40), Qt::AlignHCenter | Qt::AlignVCenter, m_title);
    }

    // Draw axes
    p.setPen(QPen(Qt::gray, 1));
    // X axis
    p.drawLine(QPointF(m_plotArea.left(), m_plotArea.bottom()), QPointF(m_plotArea.right(), m_plotArea.bottom()));
    // Y axis
    p.drawLine(QPointF(m_plotArea.left(), m_plotArea.top()), QPointF(m_plotArea.left(), m_plotArea.bottom()));

    // Draw axis ticks and labels
    p.setFont(QFont("sans", 10));
    int nTicks = 6;
    for (int i = 0; i < nTicks; ++i) {
        double tx = m_xMin + (m_xMax - m_xMin) * i / (nTicks - 1);
        QPointF pt = dataToScreen(tx, m_yMin);
        p.drawLine(QPointF(pt.x(), pt.y()), QPointF(pt.x(), pt.y() + 5));
        p.drawText(QRectF(pt.x() - 30, pt.y() + 8, 60, 16), Qt::AlignHCenter, QString::number(tx, 'g', 4));
    }
    for (int i = 0; i < nTicks; ++i) {
        double ty = m_yMin + (m_yMax - m_yMin) * i / (nTicks - 1);
        QPointF pt = dataToScreen(m_xMin, ty);
        p.drawLine(QPointF(pt.x() - 5, pt.y()), QPointF(pt.x(), pt.y()));
        p.drawText(QRectF(pt.x() - 55, pt.y() - 10, 50, 20), Qt::AlignRight | Qt::AlignVCenter, QString::number(ty, 'g', 4));
    }
    // Axis labels
    p.setFont(QFont("sans", 12, QFont::Bold));
    p.drawText(QRectF(m_plotArea.left(), m_plotArea.bottom() + 18, m_plotArea.width(), 20), Qt::AlignHCenter, "X");
    p.save();
    p.translate(m_plotArea.left() - 40, m_plotArea.top() + m_plotArea.height() / 2);
    p.rotate(-90);
    p.drawText(QRectF(-m_plotArea.height() / 2, -20, m_plotArea.height(), 20), Qt::AlignHCenter, "Y");
    p.restore();

    bool hasCategories = m_categories.size() == m_points.size() &&
        std::all_of(m_categories.begin(), m_categories.end(), [](const QPair<QString, QColor>& c) { return c.second.isValid(); });
    int barHeight = 12;
    int barY = static_cast<int>(m_plotArea.top()) - barHeight - 8;
    if (barY < 0) barY = 0;
    if (hasCategories) {
        for (int i = 0; i < m_points.size() - 1; ++i) {
            QColor color = m_categories[i].second;
            QPointF p0 = dataToScreen(m_points[i].first, m_yMax);
            QPointF p1 = dataToScreen(m_points[i + 1].first, m_yMax);
            QRectF barRect(p0.x(), barY, p1.x() - p0.x(), barHeight);
            p.setPen(Qt::NoPen);
            p.setBrush(color);
            p.drawRect(barRect);
            // Highlight on hover
            if (i == m_hoveredBarIdx) {
                p.setPen(QPen(QColor("#222"), 2));
                p.setBrush(Qt::NoBrush);
                p.drawRect(barRect.adjusted(1, 1, -1, -1));
            }
        }
    }

    // === MAIN LINE (category colored segments) ===
    for (int i = 0; i < m_points.size() - 1; ++i) {
        QPointF p0 = dataToScreen(m_points[i].first, m_points[i].second);
        QPointF p1 = dataToScreen(m_points[i + 1].first, m_points[i + 1].second);
        QColor color = (hasCategories && m_categories[i].second.isValid()) ? m_categories[i].second : m_lineColor;
        QPen pen(color, (i == m_hoveredLineIdx) ? 4 : 2);
        if (i == m_hoveredLineIdx)
            pen.setColor(QColor("#d62728"));
        p.setPen(pen);
        p.drawLine(p0, p1);
    }

    // === STAT LINE ===
    if (!m_statLine.isEmpty()) {
        double x1 = m_statLine.value("start_x").toDouble();
        double y1 = m_statLine.value("start_y").toDouble();
        double x2 = m_statLine.value("end_x").toDouble();
        double y2 = m_statLine.value("end_y").toDouble();
        QString label = m_statLine.value("label").toString();
        QColor color = QColor(m_statLine.value("color", "#d62728").toString());
        int pointSize = m_statLine.value("pointSize", 7).toInt();

        QPointF s0 = dataToScreen(x1, y1);
        QPointF s1 = dataToScreen(x2, y2);

        QPen statPen(color, 3, Qt::DashLine);
        p.setPen(statPen);
        p.drawLine(s0, s1);

        // Endpoints
        p.setBrush(color);
        p.setPen(QPen(Qt::white, 2));
        p.drawEllipse(s0, pointSize, pointSize);
        p.drawEllipse(s1, pointSize, pointSize);

        // Label
        if (!label.isEmpty()) {
            p.setFont(QFont("sans", 10, QFont::Bold));
            p.setPen(color);
            QPointF labelPos = (s0 + s1) / 2 + QPointF(8, -8);
            p.drawText(QRectF(labelPos, QSizeF(120, 20)), Qt::AlignLeft | Qt::AlignTop, label);
        }
    }

    // === LEGEND ===
    /*int legendW = 200, legendH = !m_statLine.isEmpty() ? 60 : 28;

    // Calculate available space on left and right of plot area at the bottom
    int margin = 20;
    int bottomY = height() - legendH - margin;
    int leftSpace = static_cast<int>(m_plotArea.left()) - margin;
    int rightSpace = width() - static_cast<int>(m_plotArea.right()) - margin;

    // Default to right, but use left if more space
    int legendX;
    if (leftSpace > rightSpace) {
        // Bottom left
        legendX = margin;
    }
    else {
        // Bottom right
        legendX = width() - legendW - margin;
    }
    int legendY = height() - legendH - margin;

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 255, 255, 240));
    p.drawRect(legendX, legendY, legendW, legendH);
    p.setPen(QPen(Qt::black, 1));
    p.setFont(QFont("sans", 11));
    int ly = legendY + 10;
    // Main line
    p.setPen(QPen(m_lineColor, 2));
    p.drawLine(legendX + 10, ly, legendX + 40, ly);
    p.setPen(Qt::black);
    p.drawText(legendX + 50, ly + 4, "Main Line");
    ly += 18;
    // Stat line
    if (!m_statLine.isEmpty()) {
        QColor color = QColor(m_statLine.value("color", "#d62728").toString());
        int pointSize = m_statLine.value("pointSize", 7).toInt();
        p.setPen(QPen(color, 3, Qt::DashLine));
        p.drawLine(legendX + 10, ly, legendX + 40, ly);
        p.setPen(Qt::black);
        p.drawText(legendX + 50, ly + 4, m_statLine.value("label", "Stat Line").toString());
        ly += 18;
        // Endpoints
        p.setPen(QPen(Qt::white, 2));
        p.setBrush(color);
        p.drawEllipse(QPointF(legendX + 25, ly), pointSize, pointSize);
        p.setPen(Qt::black);
        int nStart = m_statLine.value("n_start").toInt();
        int nEnd = m_statLine.value("n_end").toInt();
        p.drawText(legendX + 40, ly + 5, QString("Start: %1 points").arg(nStart));
        ly += 18;
        p.setPen(QPen(Qt::white, 2));
        p.setBrush(color);
        p.drawEllipse(QPointF(legendX + 25, ly), pointSize, pointSize);
        p.setPen(Qt::black);
        p.drawText(legendX + 40, ly + 5, QString("End: %1 points").arg(nEnd));
    }*/
}

void LineChartWidget::mouseMoveEvent(QMouseEvent* event)
{
    int oldLine = m_hoveredLineIdx;
    int oldBar = m_hoveredBarIdx;
    double minDist = 8.0;
    m_hoveredLineIdx = findNearestLineSegment(event->pos(), minDist);
    m_hoveredBarIdx = findCategoryBarAt(event->pos());

    if (m_hoveredBarIdx >= 0 && m_hoveredBarIdx < m_categories.size() && !m_categories[m_hoveredBarIdx].first.isEmpty()) {
        showTooltip(event->pos(), m_categories[m_hoveredBarIdx].first);
    }
    else if (m_hoveredLineIdx >= 0 && m_hoveredLineIdx < m_points.size() - 1) {
        QString tip = QString("x: %1\ny: %2").arg(m_points[m_hoveredLineIdx].first).arg(m_points[m_hoveredLineIdx].second);
        if (!m_categories.isEmpty() && !m_categories[m_hoveredLineIdx].first.isEmpty())
            tip += "\nCategory: " + m_categories[m_hoveredLineIdx].first;
        showTooltip(event->pos(), tip);
    }
    else {
        hideTooltip();
    }
    if (oldLine != m_hoveredLineIdx || oldBar != m_hoveredBarIdx)
        update();
}

void LineChartWidget::leaveEvent(QEvent*)
{
    m_hoveredLineIdx = -1;
    m_hoveredBarIdx = -1;
    hideTooltip();
    update();
}

int LineChartWidget::findNearestLineSegment(const QPoint& pos, double& minDist) const
{
    if (m_points.size() < 2) return -1;
    int bestIdx = -1;
    for (int i = 0; i < m_points.size() - 1; ++i) {
        QPointF p0 = dataToScreen(m_points[i].first, m_points[i].second);
        QPointF p1 = dataToScreen(m_points[i + 1].first, m_points[i + 1].second);
        // Distance from mouse to line segment
        double dx = p1.x() - p0.x();
        double dy = p1.y() - p0.y();
        double t = ((pos.x() - p0.x()) * dx + (pos.y() - p0.y()) * dy) / (dx * dx + dy * dy);
        t = std::max(0.0, std::min(1.0, t));
        double projX = p0.x() + t * dx;
        double projY = p0.y() + t * dy;
        double dist = std::hypot(pos.x() - projX, pos.y() - projY);
        if (dist < minDist) {
            minDist = dist;
            bestIdx = i;
        }
    }
    return bestIdx;
}

int LineChartWidget::findCategoryBarAt(const QPoint& pos) const
{
    if (m_categories.size() != m_points.size() || m_points.size() < 2)
        return -1;
    int barHeight = 12;
    int barY = m_plotArea.top() - barHeight - 8;
    if (pos.y() < barY || pos.y() > barY + barHeight)
        return -1;
    for (int i = 0; i < m_points.size() - 1; ++i) {
        QPointF p0 = dataToScreen(m_points[i].first, m_yMax);
        QPointF p1 = dataToScreen(m_points[i + 1].first, m_yMax);
        if (pos.x() >= std::min(p0.x(), p1.x()) && pos.x() <= std::max(p0.x(), p1.x()))
            return i;
    }
    return -1;
}

void LineChartWidget::showTooltip(const QPoint& pos, const QString& text)
{
    QToolTip::showText(mapToGlobal(pos), text, this);
}

void LineChartWidget::hideTooltip()
{
    QToolTip::hideText();
}
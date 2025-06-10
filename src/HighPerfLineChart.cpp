#include "HighPerfLineChart.h"
#include <QMouseEvent>
#include <QPainter>
#include <QtMath>
#include <QDebug>
#include <random>
#include <QFont>
#include <QPen>
#include <algorithm> 


HighPerfLineChart::HighPerfLineChart(QWidget* parent)
    : QOpenGLWidget(parent) {
    m_selectedLine = -1;
    m_hoveredLine = -1;
    m_xMin = 0; m_xMax = 1;
    m_yMin = 0; m_yMax = 1;
    m_lineWidth = 1.0f;
    m_pointSize = 6.0f;
    m_axisFont = QFont("Arial", 10);
    m_axisColor = Qt::black;
    m_numXTicks = 5;
    m_numYTicks = 5;
    m_gridEnabled = false;
    m_gridColor = QColor(220,220,220);
    m_backgroundColor = Qt::white;
    m_showPoints = true;
    m_showLines = true; 
    m_axisLineWidth = 1.0f;
    m_xAxisLineColor = Qt::black;
    m_yAxisLineColor = Qt::black;
    m_tickLabelFont = QFont("Arial", 8);
    m_tickLabelColor = Qt::black;
    m_leftMargin = 50;
    m_rightMargin = 20;
    m_topMargin = 40;
    m_bottomMargin = 40;
    m_legendFont = QFont("Arial", 9);
    m_legendColor = Qt::black;
    m_legendX = -1;
    m_legendY = -1;
    m_showAllLines = true;
    m_globalLineColor = QColor();
    m_globalLineWidth = 0.0f;
    m_globalLineOpacity = 1.0f;
    m_showLegend = true; 
    m_showAvgNumPointsLabel = false; 
    m_showMovingAverageLine = false;
    m_movingAverageWindow = 0;
}


// --- Customization setters/getters ---
void HighPerfLineChart::setLineWidth(float w) { m_lineWidth = w; updateAxisRange(); update(); }
void HighPerfLineChart::setPointSize(float s) { m_pointSize = s; updateAxisRange(); update(); }
void HighPerfLineChart::setAxisFont(const QFont& f) { m_axisFont = f; updateAxisRange(); update(); }
void HighPerfLineChart::setAxisColor(const QColor& c) { m_axisColor = c; updateAxisRange(); update(); }
void HighPerfLineChart::setNumXTicks(int n) { m_numXTicks = n; updateAxisRange(); update(); }
void HighPerfLineChart::setNumYTicks(int n) { m_numYTicks = n; updateAxisRange(); update(); }
void HighPerfLineChart::setGridEnabled(bool en) { m_gridEnabled = en; updateAxisRange(); update(); }
void HighPerfLineChart::setGridColor(const QColor& c) { m_gridColor = c; updateAxisRange(); update(); }
void HighPerfLineChart::setBackgroundColor(const QColor& c) { m_backgroundColor = c; updateAxisRange(); update(); }
void HighPerfLineChart::setShowPoints(bool en) {
    m_showPoints = en;
    updateAxisRange();
    update();
}
bool HighPerfLineChart::showPoints() const { return m_showPoints; }

// NEW: show/hide lines connecting points
void HighPerfLineChart::setShowLines(bool en) { m_showLines = en; updateAxisRange(); update(); }
bool HighPerfLineChart::showLines() const { return m_showLines; }

// New customization setters/getters
void HighPerfLineChart::setAxisLineWidth(float w) { m_axisLineWidth = w; updateAxisRange(); update(); }
void HighPerfLineChart::setXAxisLineColor(const QColor& c) { m_xAxisLineColor = c; updateAxisRange(); update(); }
void HighPerfLineChart::setYAxisLineColor(const QColor& c) { m_yAxisLineColor = c; updateAxisRange(); update(); }
void HighPerfLineChart::setTickLabelFont(const QFont& f) { m_tickLabelFont = f; updateAxisRange(); update(); }
void HighPerfLineChart::setTickLabelColor(const QColor& c) { m_tickLabelColor = c; updateAxisRange(); update(); }
void HighPerfLineChart::setChartMargins(int left, int right, int top, int bottom) {
    m_leftMargin = left; m_rightMargin = right; m_topMargin = top; m_bottomMargin = bottom; updateAxisRange(); update();
}
void HighPerfLineChart::setLegendFont(const QFont& f) { m_legendFont = f; updateAxisRange(); update(); }
void HighPerfLineChart::setLegendColor(const QColor& c) { m_legendColor = c; updateAxisRange(); update(); }
void HighPerfLineChart::setLegendPosition(int x, int y) { m_legendX = x; m_legendY = y; updateAxisRange(); update(); }

void HighPerfLineChart::setShowAllLines(bool show)
{
    m_showAllLines = show;
    updateAxisRange();
    update();
}

bool HighPerfLineChart::showAllLines() const
{
    return m_showAllLines;
}

void HighPerfLineChart::setShowLegend(bool show)
{
    m_showLegend = show;
    updateAxisRange();
    update();
}

bool HighPerfLineChart::showLegend() const
{
    return m_showLegend;
}

void HighPerfLineChart::setConnectStatsLineEnabled(bool enabled)
{
    m_connectStatsLineEnabled = enabled;
    updateAxisRange();
    update();
}

bool HighPerfLineChart::connectStatsLineEnabled() const
{
    return m_connectStatsLineEnabled;
}

void HighPerfLineChart::setConnectStatsLineN(int n)
{
    m_connectStatsLineN = n;
    updateAxisRange();
    update();
}

int HighPerfLineChart::connectStatsLineN() const
{
    return m_connectStatsLineN;
}

void HighPerfLineChart::setConnectStatsLineType(ConnectStatsType type)
{
    m_connectStatsLineType = type;
    updateAxisRange();
    update();
}

void HighPerfLineChart::setShowAvgNumPointsLabel(bool show)
{
    m_showAvgNumPointsLabel = show;
    updateAxisRange();
    update();
}

bool HighPerfLineChart::showAvgNumPointsLabel() const
{
    return m_showAvgNumPointsLabel;
}

void HighPerfLineChart::setShowMovingAverageLine(bool show)
{
    m_showMovingAverageLine = show;
    updateAxisRange();
    update();
}

bool HighPerfLineChart::showMovingAverageLine() const
{
    return m_showMovingAverageLine;
}

void HighPerfLineChart::setMovingAverageWindow(int window)
{
    m_movingAverageWindow = window;
    updateAxisRange();
    update();
}

int HighPerfLineChart::movingAverageWindow() const
{
    return m_movingAverageWindow;
}

void HighPerfLineChart::setLines(const QVector<LineData>& lines) {
    m_lines = lines;

    qDebug() << "[HighPerfLineChart] setLines called. Number of lines:" << m_lines.size();

    int totalPoints = 0;
    double xMin = std::numeric_limits<double>::max();
    double xMax = std::numeric_limits<double>::lowest();
    double yMin = std::numeric_limits<double>::max();
    double yMax = std::numeric_limits<double>::lowest();

    for (const auto& line : m_lines) {
        for (const QPointF& pt : line.points) {
            if (!qIsFinite(pt.x()) || !qIsFinite(pt.y()))
                continue;
            ++totalPoints;
            if (pt.x() < xMin) xMin = pt.x();
            if (pt.x() > xMax) xMax = pt.x();
            if (pt.y() < yMin) yMin = pt.y();
            if (pt.y() > yMax) yMax = pt.y();
        }
    }

    if (!m_lines.isEmpty()) {
        bool found = (totalPoints > 0) && qIsFinite(xMin) && qIsFinite(xMax) && qIsFinite(yMin) && qIsFinite(yMax);
        if (found) {
            if (xMin == xMax) { xMin -= 0.5; xMax += 0.5; }
            if (yMin == yMax) { yMin -= 0.5; yMax += 0.5; }
            // No margin: set axis exactly to min/max
            m_xMin = xMin;
            m_xMax = xMax;
            m_yMin = yMin;
            m_yMax = yMax;
            qDebug() << "[HighPerfLineChart] Auto-ranged x:" << m_xMin << m_xMax << "y:" << m_yMin << m_yMax;
        }
        else {
            m_xMin = 0; m_xMax = 1;
            m_yMin = 0; m_yMax = 1;
        }
    }
    else {
        m_xMin = 0; m_xMax = 1;
        m_yMin = 0; m_yMax = 1;
    }
    updateAxisRange();
    update();
}

void HighPerfLineChart::setXAxisName(const QString& name) { m_xAxisName = name; updateAxisRange(); update(); }
void HighPerfLineChart::setYAxisName(const QString& name) { m_yAxisName = name; updateAxisRange(); update(); }
void HighPerfLineChart::setXRange(double min, double max) { 
    if (!qIsFinite(min) || !qIsFinite(max) || min == max) {
        qWarning() << "[HighPerfLineChart] setXRange: Invalid range, ignoring.";
        return;
    }
    m_xMin = min; m_xMax = max; 
    qDebug() << "[HighPerfLineChart] setXRange:" << m_xMin << m_xMax;
    updateAxisRange();
    update(); 
}
void HighPerfLineChart::setYRange(double min, double max) { 
    if (!qIsFinite(min) || !qIsFinite(max) || min == max) {
        qWarning() << "[HighPerfLineChart] setYRange: Invalid range, ignoring.";
        return;
    }
    m_yMin = min; m_yMax = max; 
    qDebug() << "[HighPerfLineChart] setYRange:" << m_yMin << m_yMax;
    updateAxisRange();
    update(); 
}

void HighPerfLineChart::setChartTitle(const QString& title) {
    m_chartTitle = title;
    updateAxisRange();
    update();
}


int HighPerfLineChart::selectedLine() const { return m_selectedLine; }
int HighPerfLineChart::hoveredLine() const { return m_hoveredLine; }

void HighPerfLineChart::initializeGL() {
    initializeOpenGLFunctions();
    glClearColor(1, 1, 1, 1);
}

void HighPerfLineChart::paintGL() {
    if (!context() || !context()->isValid()) return;
    initializeOpenGLFunctions();

    glClearColor(m_backgroundColor.redF(), m_backgroundColor.greenF(), m_backgroundColor.blueF(), 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Guard: avoid division by zero if range is invalid or widget size is zero
    if (m_xMax == m_xMin || m_yMax == m_yMin || width() <= 0 || height() <= 0) {
        QPainter painter(this);
        painter.setPen(Qt::red);
        painter.setFont(QFont("Arial", 16, QFont::Bold));
        painter.drawText(rect(), Qt::AlignCenter, "Invalid axis range");
        qDebug() << "[HighPerfLineChart] Invalid axis range or widget size!";
        return;
    }

    if (m_lines.isEmpty()) {
        QPainter painter(this);
        painter.setPen(Qt::darkGray);
        painter.setFont(QFont("Arial", 16, QFont::Bold));
        painter.drawText(rect(), Qt::AlignCenter, m_emptyMessage.isEmpty() ? "No data to display" : m_emptyMessage);
        return;
    }

    bool anyPoints = false;
    int totalPoints = 0;
    for (int i = 0; i < m_lines.size(); ++i) {
        const auto& line = m_lines[i];
        if (line.points.isEmpty())
            continue;
        for (const QPointF& pt : line.points) {
            if (!qIsFinite(pt.x()) || !qIsFinite(pt.y()))
                continue;
            anyPoints = true;
            ++totalPoints;
        }
    }
    if (!anyPoints) {
        QPainter painter(this);
        painter.setPen(Qt::darkGray);
        painter.setFont(QFont("Arial", 16, QFont::Bold));
        painter.drawText(rect(), Qt::AlignCenter, m_emptyMessage.isEmpty() ? "No points in line data" : m_emptyMessage);
        qDebug() << "[HighPerfLineChart] No points in line data!";
        return;
    }

    drawChartWithOpenGL();

    // --- Ensure QPainter is used after OpenGL drawing ---
    QPainter painter(this);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    // --- Debug: confirm text drawing functions are called ---
    qDebug() << "[HighPerfLineChart] Calling drawAxesAndLabels";
    drawAxesAndLabels(painter);

    qDebug() << "[HighPerfLineChart] Calling drawLegend";
    drawLegend(painter);

    qDebug() << "[HighPerfLineChart] Calling drawAverageLabel";
    drawAverageLabel(painter);
}

void HighPerfLineChart::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}

QColor HighPerfLineChart::slopeToColor(double slope) const
{
    return QColor();
}

QPointF HighPerfLineChart::screenToChart(const QPoint& p) const {
    if (width() <= 0 || height() <= 0 || m_xMax == m_xMin || m_yMax == m_yMin)
        return QPointF(0, 0);
    float x = m_xMin + (float(p.x()) / width()) * (m_xMax - m_xMin);
    float y = m_yMax - (float(p.y()) / height()) * (m_yMax - m_yMin);
        return QPointF(x, y);
}

int HighPerfLineChart::findLineAt(const QPoint& pos) const {
    if (m_lines.isEmpty() || width() <= 0)
        return -1;
    QPointF chartPt = screenToChart(pos);
    double minDist = 5.0 * (m_xMax - m_xMin) / width(); // tolerance in chart units
    int found = -1;
    for (int i = 0; i < m_lines.size(); ++i) {
        const auto& line = m_lines[i];
        if (line.points.isEmpty())
            continue;
        for (const QPointF& pt : line.points) {
            if (!qIsFinite(pt.x()) || !qIsFinite(pt.y()))
                continue;
            if (qAbs(pt.x() - chartPt.x()) < minDist && qAbs(pt.y() - chartPt.y()) < minDist) {
                found = i;
                break;
            }
        }
        if (found != -1) break;
    }
    return found;
}

void HighPerfLineChart::clear() {
    m_lines.clear();
    m_selectedLine = -1;
    m_hoveredLine = -1;
    m_xMin = 0; m_xMax = 1;
    m_yMin = 0; m_yMax = 1;
    m_emptyMessage = "No data to display"; // Ensure the no data message is shown
    //updateAxisRange();
    update();
}

void HighPerfLineChart::setEmptyMessage(const QString& msg) {
    m_emptyMessage = msg;
    //updateAxisRange();
    update();
}

void HighPerfLineChart::mouseMoveEvent(QMouseEvent* event) {
    int idx = findLineAt(event->pos());
    if (idx != m_hoveredLine) {
        m_hoveredLine = idx;
        emit lineHovered(idx);
        updateAxisRange();
        update();
    }
}

void HighPerfLineChart::mousePressEvent(QMouseEvent* event) {
    int idx = findLineAt(event->pos());
    if (idx != m_selectedLine) {
        m_selectedLine = idx;
        emit lineSelected(idx);
        updateAxisRange();
        update();
    }
}

QVector<QPointF> HighPerfLineChart::movingAverage(const QVector<QPointF>& points, int window) const
{
    QVector<QPointF> result;
    if (window <= 1 || points.size() < window)
        return points;

    for (int i = 0; i <= points.size() - window; ++i) {
        double sumX = 0.0, sumY = 0.0;
        for (int j = 0; j < window; ++j) {
            sumX += points[i + j].x();
            sumY += points[i + j].y();
        }
        result.append(QPointF(sumX / window, sumY / window));
    }
    return result;
}

void HighPerfLineChart::drawChartWithOpenGL()
{
    if (!context() || !context()->isValid()) return;

    // --- Chart area margins and rect ---
    const int leftMargin = m_leftMargin;
    const int rightMargin = m_rightMargin;
    const int topMargin = m_topMargin;
    const int bottomMargin = m_bottomMargin;
    QRect chartRect(leftMargin, topMargin, width() - leftMargin - rightMargin, height() - topMargin - bottomMargin);

    // Ensure chartRect is valid
    if (chartRect.width() <= 0 || chartRect.height() <= 0) {
        qDebug() << "[HighPerfLineChart] drawChartWithOpenGL: chartRect is zero-sized!";
        return;
    }

    // --- Draw grid lines (if enabled) ---
    if (m_gridEnabled) {
        QPainter gridPainter(this);
        gridPainter.setPen(QPen(m_gridColor, 1, Qt::DashLine));
        // Vertical grid lines
        for (int i = 0; i <= m_numXTicks; ++i) {
            double t = double(i) / m_numXTicks;
            int x = chartRect.left() + int(t * chartRect.width());
            gridPainter.drawLine(QPoint(x, chartRect.top()), QPoint(x, chartRect.bottom()));
        }
        // Horizontal grid lines
        for (int i = 0; i <= m_numYTicks; ++i) {
            double t = double(i) / m_numYTicks;
            int y = chartRect.bottom() - int(t * chartRect.height());
            gridPainter.drawLine(QPoint(chartRect.left(), y), QPoint(chartRect.right(), y));
        }
    }

    // Helper lambda: map chart (data) coordinates to chartRect (screen) coordinates
    auto chartToScreen = [&](const QPointF& pt) -> QPointF {
        double xDenom = (m_xMax - m_xMin);
        double yDenom = (m_yMax - m_yMin);
        if (xDenom == 0.0) xDenom = 1e-12;
        if (yDenom == 0.0) yDenom = 1e-12;
        double xFrac = (pt.x() - m_xMin) / xDenom;
        double yFrac = (pt.y() - m_yMin) / yDenom;
        double x = chartRect.left() + xFrac * chartRect.width();
        double y = chartRect.bottom() - yFrac * chartRect.height();
        return QPointF(x, y);
    };

    // --- Draw lines and points (with line clipping to axes) ---
    if (m_showAllLines) {
        for (int i = 0; i < m_lines.size(); ++i) {
            const auto& line = m_lines[i];
            if (line.points.isEmpty())
                continue;
            const QVector<QPointF>& pts = line.points;

            QColor c = m_globalLineColor.isValid() ? m_globalLineColor
                : (line.color.isValid() ? line.color : Qt::black);
            float alpha = m_globalLineOpacity;
            float lw = (m_globalLineWidth > 0) ? m_globalLineWidth : ((line.lineWidth > 0) ? line.lineWidth : m_lineWidth);
            float ps = (line.pointSize > 0) ? line.pointSize : m_pointSize;
            if (i == m_selectedLine) {
                // Instead of forcing red, use a highlighted version of the existing color
                c = c.lighter(125);  // Make 25% brighter
                alpha = 1.0f;
                lw = std::max(lw, 3.0f);
            }
            else if (i == m_hoveredLine) {
                c = c.lighter(150);
                alpha = 1.0f;
            }

            // Set OpenGL line stipple for dash style (compatibility profile only)
            if (line.dashStyle == LineDashStyle::Dash) {
                glEnable(GL_LINE_STIPPLE);
                glLineStipple(1, 0x00FF);
            }
            else if (line.dashStyle == LineDashStyle::Dot) {
                glEnable(GL_LINE_STIPPLE);
                glLineStipple(1, 0x0101);
            }
            else {
                glDisable(GL_LINE_STIPPLE);
            }

            // --- DEBUG: Output number of points and lines being drawn ---
            //qDebug() << "[HighPerfLineChart] Drawing line" << i << "with" << pts.size() << "points, color:" << c << "alpha:" << alpha << "lw:" << lw << "ps:" << ps;

            // Draw points (all points inside axes)
            if (m_showPoints) {
                glPointSize(ps);
                glColor4f(c.redF(), c.greenF(), c.blueF(), alpha);
                glBegin(GL_POINTS);
                int pointsDrawn = 0;
                for (const QPointF& pt : pts) {
                    double xval = pt.x();
                    double yval = pt.y();
                    if (xval < m_xMin || xval > m_xMax || yval < m_yMin || yval > m_yMax)
                        continue;
                    QPointF screenPt = chartToScreen(pt);
                    float x = (screenPt.x() / width()) * 2.0f - 1.0f;
                    float y = 1.0f - (screenPt.y() / height()) * 2.0f;
                    glVertex2f(x, y);
                    ++pointsDrawn;
                }
                glEnd();
                //qDebug() << "[HighPerfLineChart]   Points drawn:" << pointsDrawn;
            }

            // Draw lines (clip each segment to axes)
            if (m_showLines && pts.size() > 1) {
                glLineWidth(lw);
                glColor4f(c.redF(), c.greenF(), c.blueF(), alpha);
                int linesDrawn = 0;
                for (int j = 1; j < pts.size(); ++j) {
                    QPointF p0 = pts[j - 1];
                    QPointF p1 = pts[j];
                    if (!qIsFinite(p0.x()) || !qIsFinite(p0.y()) || !qIsFinite(p1.x()) || !qIsFinite(p1.y()))
                        continue;
                    double x0 = p0.x();
                    double y0 = p0.y();
                    double x1 = p1.x();
                    double y1 = p1.y();
                    if (cohenSutherlandClip(x0, y0, x1, y1, m_xMin, m_xMax, m_yMin, m_yMax)) {
                        QPointF sp0 = chartToScreen(QPointF(x0, y0));
                        QPointF sp1 = chartToScreen(QPointF(x1, y1));
                        float sx0 = (sp0.x() / width()) * 2.0f - 1.0f;
                        float sy0 = 1.0f - (sp0.y() / height()) * 2.0f;
                        float sx1 = (sp1.x() / width()) * 2.0f - 1.0f;
                        float sy1 = 1.0f - (sp1.y() / height()) * 2.0f;
                        glBegin(GL_LINES);
                        glVertex2f(sx0, sy0);
                        glVertex2f(sx1, sy1);
                        glEnd();
                        ++linesDrawn;
                    }
                }
                //qDebug() << "[HighPerfLineChart]   Line segments drawn:" << linesDrawn;
            }
            glDisable(GL_LINE_STIPPLE); // Always disable after drawing
        }
    }

    // --- Draw moving average line if enabled ---
    if (m_showMovingAverageLine && m_movingAverageWindow > 1) {
        QColor offsetColor(255, 165, 0); // Orange as the constant offset color
        float avgAlpha = 1.0f;
        float avgWidth = 2.5f;

        for (int i = 0; i < m_lines.size(); ++i) {
            const auto& line = m_lines[i];
            if (line.points.size() < m_movingAverageWindow)
                continue;
            QVector<QPointF> avgPts = movingAverage(line.points, m_movingAverageWindow);
            if (avgPts.size() < 2) continue;

            // Blend the line color with the offset color
            QColor baseColor = m_globalLineColor.isValid() ? m_globalLineColor
                : (line.color.isValid() ? line.color : Qt::black);
            // Simple blend: 60% base, 40% offset
            QColor avgColor(
                (baseColor.red() * 3 + offsetColor.red() * 2) / 5,
                (baseColor.green() * 3 + offsetColor.green() * 2) / 5,
                (baseColor.blue() * 3 + offsetColor.blue() * 2) / 5
            );

            glLineWidth(avgWidth);
            glColor4f(avgColor.redF(), avgColor.greenF(), avgColor.blueF(), avgAlpha);

            for (int j = 1; j < avgPts.size(); ++j) {
                QPointF p0 = avgPts[j - 1];
                QPointF p1 = avgPts[j];
                if (!qIsFinite(p0.x()) || !qIsFinite(p0.y()) || !qIsFinite(p1.x()) || !qIsFinite(p1.y()))
                    continue;
                double x0 = p0.x();
                double y0 = p0.y();
                double x1 = p1.x();
                double y1 = p1.y();
                if (cohenSutherlandClip(x0, y0, x1, y1, m_xMin, m_xMax, m_yMin, m_yMax)) {
                    QPointF sp0 = chartToScreen(QPointF(x0, y0));
                    QPointF sp1 = chartToScreen(QPointF(x1, y1));
                    float sx0 = (sp0.x() / width()) * 2.0f - 1.0f;
                    float sy0 = 1.0f - (sp0.y() / height()) * 2.0f;
                    float sx1 = (sp1.x() / width()) * 2.0f - 1.0f;
                    float sy1 = 1.0f - (sp1.y() / height()) * 2.0f;
                    glBegin(GL_LINES);
                    glVertex2f(sx0, sy0);
                    glVertex2f(sx1, sy1);
                    glEnd();
                }
            }
        }
    }

    // --- Draw connect stats line if enabled ---
    if (m_connectStatsLineEnabled && m_connectStatsLineN > 0 && !m_lines.isEmpty()) {
        int n = m_connectStatsLineN;

        // Collect first n and last n points from all lines
        std::vector<double> firstXs, firstYs, lastXs, lastYs;
        for (const auto& line : m_lines) {
            const auto& pts = line.points;
            int count = pts.size();
            if (count == 0) continue;
            int nFirst = std::min(n, count);
            int nLast = std::min(n, count);

            // First n points
            for (int i = 0; i < nFirst; ++i) {
                if (qIsFinite(pts[i].x()) && qIsFinite(pts[i].y())) {
                    firstXs.push_back(pts[i].x());
                    firstYs.push_back(pts[i].y());
                }
            }
            // Last n points
            for (int i = count - nLast; i < count; ++i) {
                if (qIsFinite(pts[i].x()) && qIsFinite(pts[i].y())) {
                    lastXs.push_back(pts[i].x());
                    lastYs.push_back(pts[i].y());
                }
            }
        }

        // Helper lambdas for statistics
        auto mean = [](const std::vector<double>& v) -> double {
            if (v.empty()) return std::numeric_limits<double>::quiet_NaN();
            double sum = 0.0;
            for (double x : v) sum += x;
            return sum / v.size();
            };
        auto median = [](std::vector<double> v) -> double {
            if (v.empty()) return std::numeric_limits<double>::quiet_NaN();
            std::sort(v.begin(), v.end());
            size_t n = v.size();
            if (n % 2 == 0)
                return 0.5 * (v[n / 2 - 1] + v[n / 2]);
            else
                return v[n / 2];
            };
        auto mode = [](const std::vector<double>& v) -> double {
            if (v.empty()) return std::numeric_limits<double>::quiet_NaN();
            std::map<double, int> freq;
            for (double x : v) ++freq[x];
            int maxCount = 0;
            double modeVal = v[0];
            for (const auto& [val, count] : freq) {
                if (count > maxCount) {
                    maxCount = count;
                    modeVal = val;
                }
            }
            return modeVal;
            };

        // Compute stats for first and last bundles
        double firstX = 0, firstY = 0, lastX = 0, lastY = 0;
        switch (m_connectStatsLineType) {
        case ConnectStatsType::Mean:
            firstX = mean(firstXs); firstY = mean(firstYs);
            lastX = mean(lastXs);   lastY = mean(lastYs);
            break;
        case ConnectStatsType::Median:
            firstX = median(firstXs); firstY = median(firstYs);
            lastX = median(lastXs);   lastY = median(lastYs);
            break;
        case ConnectStatsType::Mode:
            firstX = mode(firstXs); firstY = mode(firstYs);
            lastX = mode(lastXs);   lastY = mode(lastYs);
            break;
        default:
            firstX = mean(firstXs); firstY = mean(firstYs);
            lastX = mean(lastXs);   lastY = mean(lastYs);
            break;
        }

        if (qIsFinite(firstX) && qIsFinite(firstY) && qIsFinite(lastX) && qIsFinite(lastY)) {
            // Draw ellipses at the endpoints
            QPainter painter(this);
            painter.setRenderHint(QPainter::Antialiasing, true);
            QColor ellipseColor = Qt::darkGreen;
            painter.setPen(QPen(ellipseColor, 2));
            painter.setBrush(QBrush(ellipseColor, Qt::SolidPattern));
            auto chartToScreen = [&](const QPointF& pt) -> QPointF {
                double xDenom = (m_xMax - m_xMin);
                double yDenom = (m_yMax - m_yMin);
                if (xDenom == 0.0) xDenom = 1e-12;
                if (yDenom == 0.0) yDenom = 1e-12;
                double xFrac = (pt.x() - m_xMin) / xDenom;
                double yFrac = (pt.y() - m_yMin) / yDenom;
                const int leftMargin = m_leftMargin;
                const int rightMargin = m_rightMargin;
                const int topMargin = m_topMargin;
                const int bottomMargin = m_bottomMargin;
                QRect chartRect(leftMargin, topMargin, width() - leftMargin - rightMargin, height() - topMargin - bottomMargin);
                double x = chartRect.left() + xFrac * chartRect.width();
                double y = chartRect.bottom() - yFrac * chartRect.height();
                return QPointF(x, y);
                };
            QPointF screenFirst = chartToScreen(QPointF(firstX, firstY));
            QPointF screenLast = chartToScreen(QPointF(lastX, lastY));

            // Calculate bundle sizes
            int numFirstPoints = int(firstXs.size());
            int numLastPoints = int(lastXs.size());
            int totalPoints = numFirstPoints + numLastPoints;

            // Avoid division by zero
            auto percent = [&](int n) -> double {
                return (totalPoints > 0) ? (double(n) / totalPoints) : 0.0;
                };

            // Scale factor for ellipse size (tweak as needed)
            int minEllipse = 1, maxEllipse = 100;
            int baseEllipse = 50; // This is the maximum ellipse size at 100%

            auto ellipseSize = [&](int n) {
                double p = percent(n);
                int size = int(baseEllipse * p);
                size = std::clamp(size, minEllipse, maxEllipse);
                return size;
                };

            auto drawPointBundle = [&](const QPointF& center, int numPoints, int ellipseW, int ellipseH) {
                // Compute a radius that shrinks as numPoints increases (tweak the formula as needed)
                double maxRadius = std::max(8.0, 40.0 / std::sqrt(std::max(1, numPoints))); // e.g. 8px min, 40px for 1 point
                double a = std::min(ellipseW / 2.0, maxRadius);
                double b = std::min(ellipseH / 2.0, maxRadius);

                std::mt19937 rng(static_cast<unsigned>(center.x() * 1000 + center.y()));
                std::uniform_real_distribution<double> distAngle(0, 2 * M_PI);
                std::uniform_real_distribution<double> distRadius(0, 1);

                for (int i = 0; i < numPoints; ++i) {
                    double theta = distAngle(rng);
                    double r = std::pow(distRadius(rng), 4.0); // strong bias towards center
                    double x = a * r * std::cos(theta);
                    double y = b * r * std::sin(theta);
                    painter.drawEllipse(QPointF(center.x() + x, center.y() + y), 2.0, 2.0); // 2.0 is the point radius
                }
                };

            drawPointBundle(screenFirst, numFirstPoints, ellipseSize(numFirstPoints), ellipseSize(numFirstPoints));
            drawPointBundle(screenLast, numLastPoints, ellipseSize(numLastPoints), ellipseSize(numLastPoints));
            painter.end();

            // Draw the straight line in OpenGL
            QColor statColor = Qt::darkGreen;
            float statAlpha = 1.0f;
            float statWidth = 3.0f;
            glLineWidth(statWidth);
            glColor4f(statColor.redF(), statColor.greenF(), statColor.blueF(), statAlpha);

            double x0 = firstX, y0 = firstY, x1 = lastX, y1 = lastY;
            if (cohenSutherlandClip(x0, y0, x1, y1, m_xMin, m_xMax, m_yMin, m_yMax)) {
                QPointF sp0 = chartToScreen(QPointF(x0, y0));
                QPointF sp1 = chartToScreen(QPointF(x1, y1));
                float sx0 = (sp0.x() / width()) * 2.0f - 1.0f;
                float sy0 = 1.0f - (sp0.y() / height()) * 2.0f;
                float sx1 = (sp1.x() / width()) * 2.0f - 1.0f;
                float sy1 = 1.0f - (sp1.y() / height()) * 2.0f;
                glBegin(GL_LINES);
                glVertex2f(sx0, sy0);
                glVertex2f(sx1, sy1);
                glEnd();
            }
        }
    }

    // --- Fallback: Draw a test line if no data is present, to confirm rendering works ---
   /* if (m_lines.isEmpty()) {
        glLineWidth(2.0f);
        glColor4f(0.2f, 0.2f, 0.8f, 1.0f);
        glBegin(GL_LINES);
        glVertex2f(-0.8f, -0.8f);
        glVertex2f(0.8f, 0.8f);
        glEnd();
        qDebug() << "[HighPerfLineChart] Fallback: drew test line (no data present)";
    }*/
}

void HighPerfLineChart::drawAxesAndLabels(QPainter& painter)
{
    // --- Draw axes and labels using QPainter for text ---

    // Use margins and chartRect as in drawChartWithOpenGL
    const int leftMargin = m_leftMargin;
    const int rightMargin = m_rightMargin;
    const int topMargin = m_topMargin;
    const int bottomMargin = m_bottomMargin;
    QRect chartRect(leftMargin, topMargin, width() - leftMargin - rightMargin, height() - topMargin - bottomMargin);

    painter.setRenderHint(QPainter::Antialiasing, true);

    // Draw X and Y axis lines with custom width and color
    QPen xPen(m_xAxisLineColor, m_axisLineWidth);
    QPen yPen(m_yAxisLineColor, m_axisLineWidth);

    // X axis
    painter.setPen(xPen);
    QPoint xAxisStart(chartRect.left(), chartRect.bottom());
    QPoint xAxisEnd(chartRect.right(), chartRect.bottom());
    painter.drawLine(xAxisStart, xAxisEnd);

    // Y axis
    painter.setPen(yPen);
    QPoint yAxisStart(chartRect.left(), chartRect.bottom());
    QPoint yAxisEnd(chartRect.left(), chartRect.top());
    painter.drawLine(yAxisStart, yAxisEnd);

    // Draw tick marks and values
    int numXTicks = m_numXTicks;
    int numYTicks = m_numYTicks;

    // Tick labels
    QFont tickFont = m_tickLabelFont.family().isEmpty() ? painter.font() : m_tickLabelFont;
    if (tickFont.pointSize() < 6) tickFont.setPointSize(8); // Ensure readable font size
    painter.setFont(tickFont);
    painter.setPen(m_tickLabelColor);
    QFontMetrics fmTick(tickFont);

    // X ticks and labels
    for (int i = 0; i <= numXTicks; ++i) {
        double t = double(i) / numXTicks;
        int x = chartRect.left() + t * chartRect.width();
        int y = chartRect.bottom();
        painter.drawLine(QPoint(x, y), QPoint(x, y + 6));
        double xValue = m_xMin + t * (m_xMax - m_xMin);
        QString xLabel;

            xLabel = QString::number(xValue, 'g', 4);
        int labelWidth = fmTick.horizontalAdvance(xLabel);
        painter.drawText(x - labelWidth / 2, y + 6 + fmTick.ascent(), xLabel);
    }

    // Y ticks and labels
    for (int i = 0; i <= numYTicks; ++i) {
        double t = double(i) / numYTicks;
        int y = chartRect.bottom() - t * chartRect.height();
        int x = chartRect.left();
        painter.drawLine(QPoint(x, y), QPoint(x - 6, y));
        double yValue = m_yMin + t * (m_yMax - m_yMin);
        QString yLabel;

            yLabel = QString::number(yValue, 'g', 4);
        int labelHeight = fmTick.height();
        painter.drawText(x - 8 - fmTick.horizontalAdvance(yLabel), y + labelHeight / 2 - 2, yLabel);
    }

    // Axis labels and title
    QFont axisFont = m_axisFont;
    if (axisFont.pointSize() < 8) axisFont.setPointSize(10);
    painter.setFont(axisFont);
    painter.setPen(m_axisColor);

    // Draw chart title at the top center
    if (!m_chartTitle.isEmpty()) {
        QFont titleFont = painter.font();
        titleFont.setBold(true);
        if (titleFont.pointSize() < 10) titleFont.setPointSize(14);
        painter.setFont(titleFont);
        painter.drawText(QRect(0, 0, width(), topMargin), Qt::AlignHCenter | Qt::AlignVCenter, m_chartTitle);
        painter.setFont(m_axisFont);
    }

    // Draw x-axis label
    QString xAxisLabel =  m_xAxisName;
    QString yAxisLabel =  m_yAxisName;
    if (!xAxisLabel.isEmpty())
        painter.drawText(QRect(chartRect.left(), chartRect.bottom() + 22, chartRect.width(), 20), Qt::AlignHCenter | Qt::AlignVCenter, xAxisLabel);

    // Draw y-axis label (rotated)
    if (!yAxisLabel.isEmpty()) {
        painter.save();
        painter.translate(leftMargin - 35, chartRect.top() + chartRect.height() / 2);
        painter.rotate(-90);
        painter.drawText(QRect(0, 0, chartRect.height(), 20), Qt::AlignHCenter | Qt::AlignVCenter, yAxisLabel);
        painter.restore();
    }
}

void HighPerfLineChart::drawLegend(QPainter& painter)
{
    if (!m_showLegend || m_lines.isEmpty())
        return;

    // Ensure legend text is visible
    QColor legendColor = m_legendColor;
    if (legendColor == m_backgroundColor) legendColor = Qt::black;

    const int legendMargin = 10;
    int x = m_legendX >= 0 ? m_legendX : width() - m_rightMargin - 120;
    int y = m_legendY >= 0 ? m_legendY : m_topMargin + legendMargin;

    QFont legendFont = m_legendFont;
    if (legendFont.pointSize() < 8) legendFont.setPointSize(10);
    painter.setFont(legendFont);
    painter.setPen(legendColor);

    int lineHeight = painter.fontMetrics().height() + 4;
    int i = 0;
    for (const auto& line : m_lines) {
        QColor c = m_globalLineColor.isValid() ? m_globalLineColor
            : (line.color.isValid() ? line.color : Qt::black);
        painter.setPen(QPen(c, 2));
        painter.drawLine(x, y + i * lineHeight + lineHeight / 2, x + 20, y + i * lineHeight + lineHeight / 2);
        painter.setPen(legendColor);
        painter.drawText(x + 28, y + (i + 1) * lineHeight - 4, QString("Line %1").arg(i + 1));
        ++i;
        if (i > 10) break; // Limit legend size
    }
    // Add moving average legend
    if (m_showMovingAverageLine && m_movingAverageWindow > 1) {
        QColor offsetColor(255, 165, 0); // Orange as the constant offset color
        for (int j = 0; j < m_lines.size(); ++j) {
            QColor baseColor = m_globalLineColor.isValid() ? m_globalLineColor
                : (m_lines[j].color.isValid() ? m_lines[j].color : Qt::black);
            // Simple blend: 60% base, 40% offset
            QColor avgColor(
                (baseColor.red() * 3 + offsetColor.red() * 2) / 5,
                (baseColor.green() * 3 + offsetColor.green() * 2) / 5,
                (baseColor.blue() * 3 + offsetColor.blue() * 2) / 5
            );
            painter.setPen(QPen(avgColor, 2, Qt::DashLine));
            painter.drawLine(x, y + i * lineHeight + lineHeight / 2, x + 20, y + i * lineHeight + lineHeight / 2);
            painter.setPen(legendColor);
            painter.drawText(x + 28, y + (i + 1) * lineHeight - 4,
                QString("Moving Average (window=%1) Line %2").arg(m_movingAverageWindow).arg(j + 1));
            ++i;
            if (i > 10) break; // Limit legend size
        }
    }
    // Add connect stats legend
    if (m_connectStatsLineEnabled && m_connectStatsLineN > 1) {
        QColor statColor = Qt::darkGreen;
        painter.setPen(QPen(statColor, 3));
        painter.drawLine(x, y + i * lineHeight + lineHeight / 2, x + 20, y + i * lineHeight + lineHeight / 2);
        painter.setPen(legendColor);
        QString statTypeStr = "Mean";
        if (m_connectStatsLineType == ConnectStatsType::Median) statTypeStr = "Median";
        painter.drawText(x + 28, y + (i + 1) * lineHeight - 4, QString("Connect Stats (%1)").arg(statTypeStr));
        ++i;
    }
}

void HighPerfLineChart::drawAverageLabel(QPainter& painter)
{
    // --- Chart area margins and rect ---
    const int leftMargin = m_leftMargin;
    const int rightMargin = m_rightMargin;
    const int topMargin = m_topMargin;
    const int bottomMargin = m_bottomMargin;
    QRect chartRect(leftMargin, topMargin, width() - leftMargin - rightMargin, height() - topMargin - bottomMargin);

    int avgLabelY = m_topMargin - 10;
    int avgLabelHeight = 0;

    QString avgLabel;
    if (m_showAvgNumPointsLabel && !m_lines.isEmpty()) {
        int totalPoints = 0;
        int numLines = 0;
        for (const auto& line : m_lines) {
            totalPoints += int(line.points.size());
            ++numLines;
        }
        if (numLines > 0) {
            double avgPoints = double(totalPoints) / numLines;
            avgLabel = QString("Average points per line: %1    Total lines: %2")
                .arg(avgPoints, 0, 'f', 2)
                .arg(numLines);
            QFont avgFont = painter.font();
            avgFont.setBold(true);
            if (avgFont.pointSize() < 8) avgFont.setPointSize(10);
            painter.setFont(avgFont);
            QColor avgColor(80, 80, 80);
            if (avgColor == m_backgroundColor) avgColor = Qt::black;
            painter.setPen(avgColor);
            if (!m_chartTitle.isEmpty()) {
                avgLabelY += 18; // leave space for title font
            }
            avgLabelHeight = painter.fontMetrics().height();
            painter.drawText(m_leftMargin, avgLabelY, avgLabel);
        }
    }
}

// Add this private helper to your class (or as a lambda in setLines if you prefer)
void HighPerfLineChart::updateAxisRange()
{
    // If nothing is visible, use default range
    if ((!m_showPoints && !m_showLines) &&
        !m_showMovingAverageLine && !m_connectStatsLineEnabled) {
        m_xMin = 0; m_xMax = 1;
        m_yMin = 0; m_yMax = 1;
        return;
    }

    double xMin = std::numeric_limits<double>::max();
    double xMax = std::numeric_limits<double>::lowest();
    double yMin = std::numeric_limits<double>::max();
    double yMax = std::numeric_limits<double>::lowest();
    int totalPoints = 0;

    // Use points if points or lines are shown
    if (m_showPoints || m_showLines) {
        for (const auto& line : m_lines) {
            for (const QPointF& pt : line.points) {
                if (!qIsFinite(pt.x()) || !qIsFinite(pt.y()))
                    continue;
                ++totalPoints;
                if (pt.x() < xMin) xMin = pt.x();
                if (pt.x() > xMax) xMax = pt.x();
                if (pt.y() < yMin) yMin = pt.y();
                if (pt.y() > yMax) yMax = pt.y();
            }
        }
    }

    // Use moving average if enabled and no points/lines are shown
    if (!m_showPoints && !m_showLines && m_showMovingAverageLine && m_movingAverageWindow > 1) {
        for (const auto& line : m_lines) {
            QVector<QPointF> avgPts = movingAverage(line.points, m_movingAverageWindow);
            for (const QPointF& pt : avgPts) {
                if (!qIsFinite(pt.x()) || !qIsFinite(pt.y()))
                    continue;
                ++totalPoints;
                if (pt.x() < xMin) xMin = pt.x();
                if (pt.x() > xMax) xMax = pt.x();
                if (pt.y() < yMin) yMin = pt.y();
                if (pt.y() > yMax) yMax = pt.y();
            }
        }
    }

    // Use connect stats line if enabled and nothing else is shown
    if (!m_showPoints && !m_showLines && !m_showMovingAverageLine && m_connectStatsLineEnabled && m_connectStatsLineN > 0) {
        // Compute stats endpoints as in drawChartWithOpenGL
        std::vector<double> firstXs, firstYs, lastXs, lastYs;
        for (const auto& line : m_lines) {
            const auto& pts = line.points;
            int count = pts.size();
            if (count == 0) continue;
            int nFirst = std::min(m_connectStatsLineN, count);
            int nLast = std::min(m_connectStatsLineN, count);
            for (int i = 0; i < nFirst; ++i) {
                if (qIsFinite(pts[i].x()) && qIsFinite(pts[i].y())) {
                    firstXs.push_back(pts[i].x());
                    firstYs.push_back(pts[i].y());
                }
            }
            for (int i = count - nLast; i < count; ++i) {
                if (qIsFinite(pts[i].x()) && qIsFinite(pts[i].y())) {
                    lastXs.push_back(pts[i].x());
                    lastYs.push_back(pts[i].y());
                }
            }
        }
        auto mean = [](const std::vector<double>& v) -> double {
            if (v.empty()) return std::numeric_limits<double>::quiet_NaN();
            double sum = 0.0;
            for (double x : v) sum += x;
            return sum / v.size();
            };
        double firstX = mean(firstXs), firstY = mean(firstYs);
        double lastX = mean(lastXs), lastY = mean(lastYs);
        if (qIsFinite(firstX) && qIsFinite(firstY)) {
            if (firstX < xMin) xMin = firstX;
            if (firstX > xMax) xMax = firstX;
            if (firstY < yMin) yMin = firstY;
            if (firstY > yMax) yMax = firstY;
        }
        if (qIsFinite(lastX) && qIsFinite(lastY)) {
            if (lastX < xMin) xMin = lastX;
            if (lastX > xMax) xMax = lastX;
            if (lastY < yMin) yMin = lastY;
            if (lastY > yMax) yMax = lastY;
        }
        totalPoints += int(firstXs.size() + lastXs.size());
    }

    if (totalPoints > 0 && qIsFinite(xMin) && qIsFinite(xMax) && qIsFinite(yMin) && qIsFinite(yMax)) {
        if (xMin == xMax) { xMin -= 0.5; xMax += 0.5; }
        if (yMin == yMax) { yMin -= 0.5; yMax += 0.5; }
        m_xMin = xMin;
        m_xMax = xMax;
        m_yMin = yMin;
        m_yMax = yMax;
    }
    else {
        m_xMin = 0; m_xMax = 1;
        m_yMin = 0; m_yMax = 1;
    }
}
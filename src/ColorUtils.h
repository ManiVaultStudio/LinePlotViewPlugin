#pragma once

#include <QColor>

/**
 * @brief Enum representing available colormap types.
 */
enum class ColormapTypeValue {
    BlackWhite,
    RdYlBu,
    Qualitative10,
    YlGn,
    RdYlGn,
    GnBu,
    YlGnBu,
    Spectral,
    BrBg,
    YlOrBr,
    RdBu,
    RdPu,
    Plasma,
    PuOr,
    BuPu,
    Reds,
    Viridis,
    Q_BiGrRd,
    Magma,
    PiYG,
    Constant
};

/**
 * @brief Get a QColor from a colormap based on a scalar value.
 *
 * @param t       Scalar input value.
 * @param type    Type of colormap.
 * @param minVal  Minimum scalar range.
 * @param maxVal  Maximum scalar range.
 * @return QColor Corresponding color.
 */
QColor getColorFromColormap(float t, ColormapTypeValue type, float minVal, float maxVal);

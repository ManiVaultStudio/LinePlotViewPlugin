#include <array>
#include <algorithm>
#include "ColorUtils.h"
#include <QHash>
// --- Colormap definitions ---

static const std::array<QColor, 10> kQualitative10 = {
    QColor("#8dd3c7"), QColor("#ffffb3"), QColor("#bebada"), QColor("#fb8072"),
    QColor("#80b1d3"), QColor("#fdb462"), QColor("#b3de69"), QColor("#fccde5"),
    QColor("#d9d9d9"), QColor("#bc80bd")
};

static const std::array<QColor, 9> kYlGn = {
    QColor("#ffffe5"), QColor("#f7fcb9"), QColor("#d9f0a3"), QColor("#addd8e"),
    QColor("#78c679"), QColor("#41ab5d"), QColor("#238443"), QColor("#006837"),
    QColor("#004529")
};

static const std::array<QColor, 11> kRdYlGn = {
    QColor("#a50026"), QColor("#d73027"), QColor("#f46d43"), QColor("#fdae61"),
    QColor("#fee08b"), QColor("#ffffbf"), QColor("#d9ef8b"), QColor("#a6d96a"),
    QColor("#66bd63"), QColor("#1a9850"), QColor("#006837")
};

static const std::array<QColor, 9> kGnBu = {
    QColor("#f7fcf0"), QColor("#e0f3db"), QColor("#ccebc5"), QColor("#a8ddb5"),
    QColor("#7bccc4"), QColor("#4eb3d3"), QColor("#2b8cbe"), QColor("#0868ac"),
    QColor("#084081")
};

static const std::array<QColor, 9> kYlGnBu = {
    QColor("#ffffd9"), QColor("#edf8b1"), QColor("#c7e9b4"), QColor("#7fcdbb"),
    QColor("#41b6c4"), QColor("#1d91c0"), QColor("#225ea8"), QColor("#253494"),
    QColor("#081d58")
};

static const std::array<QColor, 11> kSpectral = {
    QColor("#9e0142"), QColor("#d53e4f"), QColor("#f46d43"), QColor("#fdae61"),
    QColor("#fee08b"), QColor("#e6f598"), QColor("#abdda4"), QColor("#66c2a5"),
    QColor("#3288bd"), QColor("#5e4fa2"), QColor("#000000")
};

static const std::array<QColor, 11> kBrBg = {
    QColor("#543005"), QColor("#8c510a"), QColor("#bf812d"), QColor("#dfc27d"),
    QColor("#f6e8c3"), QColor("#f5f5f5"), QColor("#c7eae5"), QColor("#80cdc1"),
    QColor("#35978f"), QColor("#01665e"), QColor("#003c30")
};

static const std::array<QColor, 9> kYlOrBr = {
    QColor("#ffffe5"), QColor("#fff7bc"), QColor("#fee391"), QColor("#fec44f"),
    QColor("#fe9929"), QColor("#ec7014"), QColor("#cc4c02"), QColor("#993404"),
    QColor("#662506")
};

static const std::array<QColor, 11> kRdBu = {
    QColor("#67001f"), QColor("#b2182b"), QColor("#d6604d"), QColor("#f4a582"),
    QColor("#f7f7f7"), QColor("#92c5de"), QColor("#4393c3"), QColor("#2166ac"),
    QColor("#053061"), QColor("#ffffff"), QColor("#000000")
};

static const std::array<QColor, 9> kRdPu = {
    QColor("#fff7f3"), QColor("#fde0dd"), QColor("#fcc5c0"), QColor("#fa9fb5"),
    QColor("#f768a1"), QColor("#dd3497"), QColor("#ae017e"), QColor("#7a0177"),
    QColor("#49006a")
};

static const std::array<QColor, 9> kPlasma = {
    QColor("#0d0887"), QColor("#5b02a3"), QColor("#9a179b"), QColor("#cb4679"),
    QColor("#ed7953"), QColor("#fb9f3a"), QColor("#fdca26"), QColor("#f0f921"),
    QColor("#ffffff")
};

static const std::array<QColor, 11> kPuOr = {
    QColor("#7f3b08"), QColor("#b35806"), QColor("#e08214"), QColor("#fdb863"),
    QColor("#fee0b6"), QColor("#f7f7f7"), QColor("#d8daeb"), QColor("#b2abd2"),
    QColor("#8073ac"), QColor("#542788"), QColor("#2d004b")
};

static const std::array<QColor, 9> kBuPu = {
    QColor("#f7fcfd"), QColor("#e0ecf4"), QColor("#bfd3e6"), QColor("#9ebcda"),
    QColor("#8c96c6"), QColor("#8c6bb1"), QColor("#88419d"), QColor("#810f7c"),
    QColor("#4d004b")
};

static const std::array<QColor, 9> kReds = {
    QColor("#fff5f0"), QColor("#fee0d2"), QColor("#fcbba1"), QColor("#fc9272"),
    QColor("#fb6a4a"), QColor("#ef3b2c"), QColor("#cb181d"), QColor("#a50f15"),
    QColor("#67000d")
};

static const std::array<QColor, 9> kViridis = {
    QColor("#440154"), QColor("#482777"), QColor("#3e4989"), QColor("#31688e"),
    QColor("#26828e"), QColor("#1f9e89"), QColor("#35b779"), QColor("#6ece58"),
    QColor("#b5de2b")
};

static const std::array<QColor, 9> kQ_BiGrRd = {
    QColor("#40004b"), QColor("#762a83"), QColor("#9970ab"), QColor("#c2a5cf"),
    QColor("#e7d4e8"), QColor("#d9f0d3"), QColor("#a6dba0"), QColor("#5aae61"),
    QColor("#1b7837")
};

static const std::array<QColor, 9> kMagma = {
    QColor("#000004"), QColor("#1c1044"), QColor("#51127c"), QColor("#833790"),
    QColor("#b63679"), QColor("#ee605e"), QColor("#fb8761"), QColor("#f9c86a"),
    QColor("#fcfdbf")
};

static const std::array<QColor, 11> kPiYG = {
    QColor("#8e0152"), QColor("#c51b7d"), QColor("#de77ae"), QColor("#f1b6da"),
    QColor("#fde0ef"), QColor("#f7f7f7"), QColor("#e6f5d0"), QColor("#b8e186"),
    QColor("#7fbc41"), QColor("#4d9221"), QColor("#276419")
};

ColormapTypeValue getColorMapFromString(const QString& colormapselectedVal)
{
    static const QHash<QString, ColormapTypeValue> colormapMap = {
        { "Black to white", ColormapTypeValue::BlackWhite },
        { "RdYlBu",         ColormapTypeValue::RdYlBu },
        { "qualitative",    ColormapTypeValue::Qualitative10 },
        { "YlGn",           ColormapTypeValue::YlGn },
        { "RdYlGn",         ColormapTypeValue::RdYlGn },
        { "GnBu",           ColormapTypeValue::GnBu },
        { "YlGnBu",         ColormapTypeValue::YlGnBu },
        { "Spectral",       ColormapTypeValue::Spectral },
        { "BrBG",           ColormapTypeValue::BrBg },
        { "YlOrBr",         ColormapTypeValue::YlOrBr },
        { "RdBu",           ColormapTypeValue::RdBu },
        { "RdPu",           ColormapTypeValue::RdPu },
        { "Plasma",         ColormapTypeValue::Plasma },
        { "PuOr",           ColormapTypeValue::PuOr },
        { "BuPu",           ColormapTypeValue::BuPu },
        { "Reds",           ColormapTypeValue::Reds },
        { "Viridis",        ColormapTypeValue::Viridis },
        { "Q_BlGrRd",       ColormapTypeValue::Q_BiGrRd },
        { "Magma",          ColormapTypeValue::Magma },
        { "PiYG",           ColormapTypeValue::PiYG }
    };
    return colormapMap.value(colormapselectedVal, ColormapTypeValue::Constant);
}

// --- Colormap lookup function ---

QColor getColorFromColormap(float t, ColormapTypeValue type, float minVal, float maxVal) {
    float range = maxVal - minVal;
    t = (range != 0.0f) ? (t - minVal) / range : 0.0f;
    t = std::clamp(t, 0.0f, 1.0f);

    auto pick = [t](const auto& arr) {
        int idx = static_cast<int>(t * (arr.size() - 1));
        idx = std::clamp(idx, 0, static_cast<int>(arr.size() - 1));
        return arr[idx];
        };

    switch (type) {
    case ColormapTypeValue::BlackWhite:
        return QColor::fromRgbF(t, t, t);
    case ColormapTypeValue::Qualitative10: return pick(kQualitative10);
    case ColormapTypeValue::YlGn: return pick(kYlGn);
    case ColormapTypeValue::RdYlGn: return pick(kRdYlGn);
    case ColormapTypeValue::GnBu: return pick(kGnBu);
    case ColormapTypeValue::YlGnBu: return pick(kYlGnBu);
    case ColormapTypeValue::Spectral: return pick(kSpectral);
    case ColormapTypeValue::BrBg: return pick(kBrBg);
    case ColormapTypeValue::YlOrBr: return pick(kYlOrBr);
    case ColormapTypeValue::RdBu: return pick(kRdBu);
    case ColormapTypeValue::RdPu: return pick(kRdPu);
    case ColormapTypeValue::Plasma: return pick(kPlasma);
    case ColormapTypeValue::PuOr: return pick(kPuOr);
    case ColormapTypeValue::BuPu: return pick(kBuPu);
    case ColormapTypeValue::Reds: return pick(kReds);
    case ColormapTypeValue::Viridis: return pick(kViridis);
    case ColormapTypeValue::Q_BiGrRd: return pick(kQ_BiGrRd);
    case ColormapTypeValue::Magma: return pick(kMagma);
    case ColormapTypeValue::PiYG: return pick(kPiYG);
    case ColormapTypeValue::Constant:
        return QColor::fromHsl(240, 175, 159);
    case ColormapTypeValue::RdYlBu:
        // Special 3-color blend: red → yellow → blue
        if (t < 0.5f) {
            float s = t * 2.0f;
            return QColor::fromRgbF(1.0f, s, 0.0f); // red to yellow
        } else {
            float s = (t - 0.5f) * 2.0f;
            return QColor::fromRgbF(1.0f - s, 1.0f - s, s); // yellow to blue
        }
    default:
        return QColor::fromHsvF(t, 1.0, 1.0); // fallback rainbow
    }
}

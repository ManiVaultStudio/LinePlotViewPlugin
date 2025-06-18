// ManiVault invokes this function to set the plot data,
// when emitting qt_js_setDataInJS from the communication object
// The connection is established in qwebchannel.tools.js
function drawChart(d) {
    if (d != null && d.length < 1) {
        log("LineViewJS: line_chart.tools.js: data empty")
        return
    }

    log("LineViewJS: line_chart.tools.js: draw chart")

    // Defensive: Ensure LineChart is defined before using it
    if (typeof window.chart === "undefined") {
        if (typeof LineChart !== "undefined" && typeof LineChart.chart === "function") {
            window.chart = LineChart.chart();
        } else {
            console.error("Chart object is not defined.");
            return;
        }
    }

    // remove possible old chart 
    d3.select("div#container").select("svg").remove();

    // config chart
    window.chart.config({
        containerClass: 'line-chart',
        w: window.innerWidth,
        h: window.innerHeight,
    });

    // plot chart in auto-resizable box
    d3.select("div#container")
        .append("svg")
        .attr("preserveAspectRatio", "xMinYMin meet")
        .attr("viewBox", "0 0 " + window.innerWidth + " " + window.innerHeight)
        .classed("svg-content", true)
        .datum(d)
        .call(window.chart);

    // Pass selected item ID to ManiVault
    d3.select("div#container").select("svg").selectAll("polygon").on('click', function (event, dd) {
        if (!event) event = d3.event;
        if (event && event.stopPropagation) event.stopPropagation();
        passSelectionToQt([window.chart.config().tooltipFormatClass(dd.className)])
    });

    // De-select by clicking outside polygons
    d3.selectAll("div#container").select("svg").on('click', function (event) {
        if (!event) event = d3.event;
        if (event && event.stopPropagation) event.stopPropagation();
        passSelectionToQt([])
    });
}
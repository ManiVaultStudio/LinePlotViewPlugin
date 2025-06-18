// ManiVault invokes this function to set the plot data,
// when emitting qt_js_setDataInJS from the communication object
// The connection is established in qwebchannel.tools.js
function drawChart(d) {
    if (!d || (Array.isArray(d) && d.length < 1)) {
        log("LineViewJS: line_chart.tools.js: data empty")
        return
    }

    log("LineViewJS: line_chart.tools.js: draw chart")
    log(d);

    // Defensive: Ensure LineChart is defined before using it
    if (typeof window.chart === "undefined") {
        if (typeof LineChart !== "undefined" && typeof LineChart.chart === "function") {
            window.chart = LineChart.chart();
        } else {
            console.error("Chart object is not defined.");
            return;
        }
    }

    // Remove possible old chart 
    d3.select("div#container").select("svg").remove();

    // Support both old and new payloads
    let parsedData, statLine;
    if (d.data && Array.isArray(d.data)) {
        parsedData = d.data.map(function(row) {
            return {
                x: +row.x,
                y: +row.y,
                category: row.category
            };
        });
        statLine = d.statLine;
    } else {
        parsedData = d.map(function(row) {
            return {
                x: +row.x,
                y: +row.y,
                category: row.category
            };
        });
        statLine = undefined;
    }

    // Store last data for responsive redraw
    window._lastChartData = { data: parsedData, statLine: statLine };

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
        .datum({ data: parsedData, statLine: statLine })
        .call(window.chart);

    // No polygon/area selection for line chart, so skip click handlers
}

// Add a resize event listener to redraw the chart dynamically
window.addEventListener('resize', function() {
    if (window.chart && window._lastChartData) {
        // Remove possible old chart
        d3.select("div#container").select("svg").remove();

        // Update chart dimensions
        window.chart.config({
            containerClass: 'line-chart',
            w: window.innerWidth,
            h: window.innerHeight,
        });

        // Redraw chart with updated dimensions
        d3.select("div#container")
            .append("svg")
            .attr("preserveAspectRatio", "xMinYMin meet")
            .attr("viewBox", "0 0 " + window.innerWidth + " " + window.innerHeight)
            .classed("svg-content", true)
            .datum(window._lastChartData)
            .call(window.chart);
    }
});
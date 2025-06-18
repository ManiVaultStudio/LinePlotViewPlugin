// ManiVault invokes this function to set the plot data,
// when emitting qt_js_setDataInJS from the communication object
// The connection is established in qwebchannel.tools.js
function drawChart(d) {
    // Remove possible old chart and message
    d3.select("div#container").select("svg").remove();
    d3.select("div#container").selectAll(".no-data-message").remove();

    // Defensive: Ensure LineChart is defined before using it
    if (typeof window.chart === "undefined") {
        if (typeof LineChart !== "undefined" && typeof LineChart.chart === "function") {
            window.chart = LineChart.chart();
        } else {
            console.error("Chart object is not defined.");
            return;
        }
    }

    // Support both old and new payloads
    let parsedData, statLine, title;
    if (d && d.data && Array.isArray(d.data)) {
        parsedData = d.data
            .filter(row => row && row.x !== undefined && row.y !== undefined)
            .map(function(row) {
                return {
                    x: +row.x,
                    y: +row.y,
                    category: row.category // optional
                };
            });
        statLine = d.statLine; // optional
        title = d.title; // optional
    } else if (Array.isArray(d)) {
        parsedData = d
            .filter(row => row && row.x !== undefined && row.y !== undefined)
            .map(function(row) {
                return {
                    x: +row.x,
                    y: +row.y,
                    category: row.category // optional
                };
            });
        statLine = undefined;
        title = undefined;
    } else {
        parsedData = [];
        statLine = undefined;
        title = undefined;
    }

    // If no valid data, show message and return
    if (!parsedData || parsedData.length === 0) {
        d3.select("div#container")
            .append("div")
            .attr("class", "no-data-message")
            .text("No data available");
        window._lastChartData = null;
        return;
    }

    // Store last data for responsive redraw
    window._lastChartData = { data: parsedData, statLine: statLine, title: title };

    // config chart
    window.chart.config({
        containerClass: 'line-chart',
        w: window.innerWidth,
        h: window.innerHeight,
        title: title
    });

    // plot chart in auto-resizable box
    d3.select("div#container")
        .append("svg")
        .attr("preserveAspectRatio", "xMinYMin meet")
        .attr("viewBox", "0 0 " + window.innerWidth + " " + window.innerHeight)
        .classed("svg-content", true)
        .datum({ data: parsedData, statLine: statLine, title: title })
        .call(window.chart);

    // No polygon/area selection for line chart, so skip click handlers
}

// Add a resize event listener to redraw the chart dynamically
window.addEventListener('resize', function() {
    if (window.chart && window._lastChartData) {
        // Remove possible old chart and message
        d3.select("div#container").select("svg").remove();
        d3.select("div#container").selectAll(".no-data-message").remove();

        // Update chart dimensions
        window.chart.config({
            containerClass: 'line-chart',
            w: window.innerWidth,
            h: window.innerHeight,
            title: window._lastChartData.title
        });

        // Redraw chart with updated dimensions
        d3.select("div#container")
            .append("svg")
            .attr("preserveAspectRatio", "xMinYMin meet")
            .attr("viewBox", "0 0 " + window.innerWidth + " " + window.innerHeight)
            .classed("svg-content", true)
            .datum(window._lastChartData)
            .call(window.chart);
    } else {
        // If no valid data, show message
        d3.select("div#container")
            .append("div")
            .attr("class", "no-data-message")
            .text("No data available");
    }
});
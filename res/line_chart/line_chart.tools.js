function drawChart(d) {
    d3.select("div#container").select("svg").remove();
    d3.select("div#container").selectAll(".no-data-message").remove();

    if (typeof window.chart === "undefined") {
        if (typeof LineChart !== "undefined" && typeof LineChart.chart === "function") {
            window.chart = LineChart.chart();
        } else {
            console.error("Chart object is not defined.");
            return;
        }
    }

    let parsedData, statLine, title, lineColor;
    if (d && d.data && Array.isArray(d.data)) {
        parsedData = d.data
            .filter(row => row && row.x !== undefined && row.y !== undefined)
            .map(function(row) {
                return {
                    x: +row.x,
                    y: +row.y,
                    category: row.category
                };
            });
        statLine = d.statLine;
        title = d.title;
        lineColor = d.lineColor;
    } else if (Array.isArray(d)) {
        parsedData = d
            .filter(row => row && row.x !== undefined && row.y !== undefined)
            .map(function(row) {
                return {
                    x: +row.x,
                    y: +row.y,
                    category: row.category
                };
            });
        statLine = undefined;
        title = undefined;
        lineColor = undefined;
    } else {
        parsedData = [];
        statLine = undefined;
        title = undefined;
        lineColor = undefined;
    }

    if (!parsedData || parsedData.length < 2) {
        d3.select("div#container")
            .append("div")
            .attr("class", "no-data-message")
            .text("No data available or insufficient data for chart.");
        window._lastChartData = null;
        return;
    }

    window._lastChartData = { data: parsedData, statLine: statLine, title: title, lineColor: lineColor };

    window.chart.config({
        containerClass: 'line-chart',
        w: window.innerWidth,
        h: window.innerHeight,
        title: title
    });

    d3.select("div#container")
        .append("svg")
        .attr("preserveAspectRatio", "xMinYMin meet")
        .attr("viewBox", "0 0 " + window.innerWidth + " " + window.innerHeight)
        .classed("svg-content", true)
        .datum({ data: parsedData, statLine: statLine, title: title, lineColor: lineColor })
        .call(window.chart);
}

window.addEventListener('resize', function() {
    if (window.chart && window._lastChartData) {
        d3.select("div#container").select("svg").remove();
        d3.select("div#container").selectAll(".no-data-message").remove();

        window.chart.config({
            containerClass: 'line-chart',
            w: window.innerWidth,
            h: window.innerHeight,
            title: window._lastChartData.title
        });

        d3.select("div#container")
            .append("svg")
            .attr("preserveAspectRatio", "xMinYMin meet")
            .attr("viewBox", "0 0 " + window.innerWidth + " " + window.innerHeight)
            .classed("svg-content", true)
            .datum(window._lastChartData)
            .call(window.chart);
    } else {
        d3.select("div#container")
            .append("div")
            .attr("class", "no-data-message")
            .text("No data available");
    }
});
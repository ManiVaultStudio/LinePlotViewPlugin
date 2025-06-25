var isQtAvailable = false;

try {
    new QWebChannel(qt.webChannelTransport, function (channel) {
        QtBridge = channel.objects.QtBridge;

        QtBridge.qt_js_setDataAndPlotInJS.connect(function () {
            drawChart(arguments[0]);
        });

        isQtAvailable = true;
        notifyBridgeAvailable();
    });
} catch (error) {
    log("LineViewJSPlugin: qwebchannel: could not connect qt");
}

// The slot js_available is defined by ManiVault's WebWidget and will
// invoke the initWebPage function of our web widget (here, ChartWidget)
function notifyBridgeAvailable() {
    if (isQtAvailable) {
        QtBridge.js_available();
    } else {
        log("LineViewJSPlugin: qwebchannel: QtBridge is not available - something went wrong");
    }
}

function passSelectionToQt(dat) {
    if (isQtAvailable) {
        QtBridge.js_qt_passSelectionToQt(dat);
    }
}

window.onerror = function (msg, url, num) {
    log("LineViewJSPlugin: qwebchannel: Error: " + msg + "\nURL: " + url + "\nLine: " + num);
};

function log(logtext) {
    if (isQtAvailable) {
        QtBridge.js_debug(logtext.toString());
    } else {
        console.log(logtext);
    }
}
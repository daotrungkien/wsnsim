let $ = require('jquery')
require('popper.js')
require('bootstrap')
let d3 = require('d3')
const { ipcRenderer } = require('electron')
const { dialog, BrowserWindow } = require('electron').remote
const readline = require('readline')
const fs = require('fs')




function exportSvg(svg, hiddenAnchor) {
	// first create a clone of our svg node so we don't mess the original one
	var clone = svg[0].cloneNode(true);
	// parse the styles
	parseSvgStyles(clone);

	// create a doctype
	var svgDocType = document.implementation.createDocumentType('svg', "-//W3C//DTD SVG 1.1//EN", "http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd");
	// a fresh svg document
	var svgDoc = document.implementation.createDocument('http://www.w3.org/2000/svg', 'svg', svgDocType);
	// replace the documentElement with our clone 
	svgDoc.replaceChild(clone, svgDoc.documentElement);
	// get the data
	var svgData = (new XMLSerializer()).serializeToString(svgDoc);

	var a = hiddenAnchor[0];
	a.href = 'data:image/svg+xml; charset=utf8, ' + encodeURIComponent(svgData.replace(/></g, '>\n\r<'));
	a.download = 'wsnsim.svg';
	a.click();
};

function parseSvgStyles(svg) {
	var styleSheets = [];
	var i;
	// get the stylesheets of the document (ownerDocument in case svg is in <iframe> or <object>)
	var docStyles = svg.ownerDocument.styleSheets;

	// transform the live StyleSheetList to an array to avoid endless loop
	for (i = 0; i < docStyles.length; i++) {
		styleSheets.push(docStyles[i]);
	}

	if (!styleSheets.length) {
		return;
	}

	var defs = svg.querySelector('defs') || document.createElementNS('http://www.w3.org/2000/svg', 'defs');
	if (!defs.parentNode) {
		svg.insertBefore(defs, svg.firstElementChild);
	}
	svg.matches = svg.matches || svg.webkitMatchesSelector || svg.mozMatchesSelector || svg.msMatchesSelector || svg.oMatchesSelector;


	// iterate through all document's stylesheets
	for (i = 0; i < styleSheets.length; i++) {
		var currentStyle = styleSheets[i]

		var rules;
		try {
			rules = currentStyle.cssRules;
		} catch (e) {
			continue;
		}
		// create a new style element
		var style = document.createElement('style');
		// some stylesheets can't be accessed and will throw a security error
		var l = rules && rules.length;
		// iterate through each cssRules of this stylesheet
		for (var j = 0; j < l; j++) {
			// get the selector of this cssRules
			var selector = rules[j].selectorText;
			// probably an external stylesheet we can't access
			if (!selector) {
				continue;
			}

			// is it our svg node or one of its children ?
			if ((svg.matches && svg.matches(selector)) || svg.querySelector(selector)) {

				var cssText = rules[j].cssText;
				// append it to our <style> node
				style.innerHTML += cssText + '\n';
			}
		}
		// if we got some rules
		if (style.innerHTML) {
			// append the style node to the clone's defs
			defs.appendChild(style);
		}
	}
};



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

let canvas;
let _MINX = 0, _MINY = 0, _MAXX = 30, _MAXY = 30, _PADDING = 1;
let xTransf, yTransf;


function initCanvas(selector, minx, maxx, miny, maxy, padding) {
	_MINX = minx;
	_MINY = miny;
	_MAXX = maxx;
	_MAXY = maxy;
	_PADDING = padding;

	xTransf = x => x;
	yTransf = y => _MAXY - y;

	canvas = d3.select(selector);

	canvas.call(d3.zoom()
		.extent([[_MINX - _PADDING, _MINY - _PADDING], [_MAXX + _PADDING, _MAXY + _PADDING]])
		.scaleExtent([.1, 10])
		.on('zoom', () => {
			d3.select('.figure').attr('transform', d3.event.transform)
		})
	);
}

function linearScale(x1, x2, d) {
	let a = [];
	for (let x = x1; x <= x2; x += d)
		a.push(x);
	return a;
}

function haxis(y, xmin, xmax, ticks) {
	let n = d3.select('.axes').append('g');

	n.append('line')
		.classed('axis', true)
		.attr('x1', xTransf(xmin))
		.attr('x2', xTransf(xmax))
		.attr('y1', yTransf(y))
		.attr('y2', yTransf(y));

	n.selectAll('g').data(ticks)
		.enter().append('g')
		.call(tn => {
			tn.classed('tick', true);
			tn.append('line')
				.attr('x1', xTransf)
				.attr('x2', xTransf)
				.attr('y1', yTransf(y))
				.attr('y2', yTransf(y + 0.2));
			tn.append('text')
				.attr('x', xTransf)
				.attr('y', yTransf(y - .5))
				.attr('text-anchor', 'middle')
				.attr('alignment-baseline', 'hanging')
				.text(x => x);
		})
}

function vaxis(x, ymin, ymax, ticks) {
	let n = d3.select('.axes').append('g');

	n.append('line')
		.classed('axis', true)
		.attr('y1', yTransf(ymin))
		.attr('y2', yTransf(ymax))
		.attr('x1', xTransf(x))
		.attr('x2', xTransf(x));

	n.selectAll('g').data(ticks)
		.enter().append('g')
		.call(tn => {
			tn.classed('tick', true);
			tn.append('line')
				.attr('y1', yTransf)
				.attr('y2', yTransf)
				.attr('x1', xTransf(x))
				.attr('x2', xTransf(x + 0.2));
			tn.append('text')
				.attr('y', yTransf)
				.attr('x', xTransf(x - .2))
				.attr('text-anchor', 'end')
				.attr('alignment-baseline', 'middle')
				.text(y => y);
		})
}



let logProcessor = null;
let logContent = [];

function setLogProcessor(func) {
	logProcessor = func;

	ipcRenderer.on('log', (event, msg) => {
		func(msg);
		storeLogMessage(msg);
	});
}

function storeLogMessage(msg) {
	let i = msg.indexOf('\t');
	logContent.push({
		time: parseFloat(msg.slice(0, i)),
		msg
	});
}

function loadLogFile(finishedCallback) {
	dialog.showOpenDialog(BrowserWindow.getFocusedWindow(), {
		properties: ['openFile'],
		defaultPath: './',
		filters: [
			{ name: 'Log files', extensions: ['txt'] },
			{ name: 'All Files', extensions: ['*'] }
		],
	}, files => {
		if (!files || files.length != 1)  {
			if (finishedCallback) finishedCallback(false);
			return;
		}
		
		logContent = [];
		readline.createInterface({
			input: fs.createReadStream(files[0]),
			output: process.stdout,
			console: false
		}).on('line', storeLogMessage)
		.on('close', () => {
			if (finishedCallback) finishedCallback(true);
		});
	});
}



let logReplayTimer = null,
	logReplayPaused = false,
	logReplayIdx;

function isLogReplaying() {
	return logReplayTimer != null;
}

function isLogReplayPaused() {
	return logReplayPaused;
}

function pauseReplayLogFile(paused = true) {
	logReplayPaused = paused;
}

function nextReplayLogFile() {
	logProcessor(logContent[logReplayIdx].msg);
	if (++logReplayIdx == logContent.length) stopReplayLogFile();
}

function stopReplayLogFile() {
	clearInterval(logReplayTimer);
	logReplayTimer = null;
	onReplayLogFileFinished();
}

function onReplayLogFileFinished() {
	$('#btn-replay').text('Replay');
	$('#btn-replay-pause').text('Pause').attr('disabled', true);
	$('#btn-replay-next').attr('disabled', true);
}

function replayLogFile() {
	if (!logProcessor || logContent.length == 0 || isLogReplaying()) {
		onReplayLogFileFinished();
		return;
	}

	logReplayPaused = false;
	logReplayIdx = 0;

	logReplayTimer = setInterval(() => {
		if (isLogReplayPaused()) return;
		nextReplayLogFile();
	}, 100);
}

function setupButtons(toolbar, onClearCanvasFunc) {
	$(toolbar).html(`
		<div class="btn-group mr-1">
			<button class="btn btn-sm btn-danger" id="btn-clear">Clear</button>

			<button class="btn btn-sm btn-danger dropdown-toggle" data-toggle="dropdown"></button>
			<div class="dropdown-menu">
				<button class="dropdown-item" id="btn-clear-log">Clear log</button>
			</div>

			<button class="btn btn-sm btn-secondary" id="btn-load-file">Load file...</button>
		</div>

		<div class="btn-group mr-auto">
			<button class="btn btn-sm btn-secondary" id="btn-replay">Replay</button>
			<button class="btn btn-sm btn-secondary" id="btn-replay-pause" disabled>Pause</button>
			<button class="btn btn-sm btn-secondary" id="btn-replay-next" disabled>Next</button>
		</div>

		<div class="btn-group">
			<button class="btn btn-sm btn-secondary" id="btn-export">Export</button>
		</div>

		<a class="d-none" id="btn-download">Download</a>`);

	$('#btn-export').click(() => exportSvg($(".canvas"), $("#btn-download")));

	$('#btn-clear').click(() => {
		if (onClearCanvasFunc) onClearCanvasFunc();
	});

	$('#btn-clear-log').click(() => {
		logContent = [];
		if (onClearCanvasFunc) onClearCanvasFunc();
	});

	$('#btn-load-file').click(loadLogFile);

	$('#btn-replay').click(() => {
		if (isLogReplaying()) {	// stop
			stopReplayLogFile();

		} else {	// start
			if (onClearCanvasFunc) onClearCanvasFunc();

			$('#btn-replay').text('Stop');
			$('#btn-replay-pause').attr('disabled', null);
			replayLogFile();
		}
	});

	$('#btn-replay-pause').click(() => {
		if (!isLogReplaying()) return;

		if (isLogReplayPaused()) {	// resume
			$('#btn-replay-pause').text('Pause');
			$('#btn-replay-next').attr('disabled', true);
			pauseReplayLogFile(false);

		} else {	// pause
			$('#btn-replay-pause').text('Resume');
			$('#btn-replay-next').attr('disabled', null);
			pauseReplayLogFile();
		}
	});

	$('#btn-replay-next').click(nextReplayLogFile);
}

$(function () {
	$('body').tooltip({
		selector: '[data-toggle="tooltip"]'
	});
})
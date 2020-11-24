const { app, BrowserWindow, ipcMain } = require('electron')
const url = require('url')
const path = require('path')
const zmq = require("zeromq")

const config = require('./config')


let sock
let win

function initLog() {
	sock = zmq.socket('sub');
	sock.bindSync(`tcp://*:${config.logger_port}`);
	sock.subscribe('log');
	console.log(`Logger socket started on port ${config.logger_port}`)

	sock.on('message', (topic, msg) => {
		win.webContents.send('log', msg.toString())
	});
}


function initGui(html) {	
	app.on('ready', () => {
		win = new BrowserWindow({
			width: 750,
			height: 800
		})
		win.loadURL(url.format({
			pathname: path.join(__dirname, `../html/${html}.html`),
			protocol: 'file:',
			slashes: true
		}))

		ipcMain.on('on-top', (event, onTop) => {
			win.setAlwaysOnTop(onTop);
		});
	});

	app.on('window-all-closed', function () {
		app.quit()
	});
}



module.exports = {
	initLog,
	initGui
}
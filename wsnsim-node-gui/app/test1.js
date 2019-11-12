const common = require('./common')


common.initLog('log', (topic, msg) => {
	console.log(`${topic} ${msg}`)
});


common.initGui('test1');


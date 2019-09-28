mergeInto(LibraryManager.library, {
	frames: [],
	currentFrame: [],
	currentShape: {},
	printOut: function(ptr) {
		self.postMessage({code: 1, payload: Module.UTF8ToString(ptr)});
	},
	printDebug: function(ptr){
		self.postMessage({code: 2, payload: Module.UTF8ToString(ptr)});
	},
	printError: function(ptr){
		self.postMessage({code: 3, payload: Module.UTF8ToString(ptr)})
	},

	newShape: function(idPtr, tagPtr){
		_currentShape = {
			"id": idPtr,
			"tag": tagPtr,
			"attrs":{},
			"styles":{},
		}
	},

	addStyle: function(keyPtr, valPtr){
		let key = Module.UTF8ToString(keyPtr);
		let value = valPtr;
		_currentShape['styles'][key] = value;

	},
	addAttribute: function(keyPtr, valPtr){
		let key = Module.UTF8ToString(keyPtr);
		let value = valPtr;
		_currentShape['attrs'][key] = value;
	},

	paintShape: function(){
		console.log(_currentShape);
		self.postMessage({code: 4, payload: _currentShape});
	},

	paintFrame: function(){
		_frames.push(currentFrame);
		self.postMessage({code: 4, payload: _currentFrame});
		_currentFrame = {};
	},

	newShape__deps: [
		'currentShape'
	],
	paintShape__deps: [
		'currentShape'
	],
	addStyle__deps: [
        'currentShape'
	],
	addAttribute__deps: [
		'currentShape'
	],
	paintFrame__deps: [
        'frames',
        'currentFrame'
	],
});
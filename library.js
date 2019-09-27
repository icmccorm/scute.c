mergeInto(LibraryManager.library, {
	frames: {},
	currentFrame: {},
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
	addProperty: function(keyPtr, valPtr){
		let key = Module.UTF8ToString(keyPtr);
		let value = valPtr;
		_currentShape[key] = value;
	},
	addProperty__deps: [
        'currentShape'
	],
	paintShape: function(){
		_currentFrame.push(_currentShape);
		self.postMessage({code: 4, payload: _currentShape});
		_currentShape = {};
	},
	paintShape__deps: [
		'currentFrame',
		'currentShape'
	],
	paintFrame: function(){
		_frames.push(currentFrame);
		self.postMessage({code: 4, payload: _currentFrame});
		_currentFrame = {};
	},
	paintFrame__deps: [
        'frames',
        'currentFrame'
	],
});
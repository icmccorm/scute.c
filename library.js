mergeInto(LibraryManager.library, {
	frames: {},
	currentFrame: {},
	printOut: function(ptr) {
		self.postMessage({code: 1, payload: Module.UTF8ToString(ptr)});
	},
	printDebug: function(ptr){
		self.postMessage({code: 2, payload: Module.UTF8ToString(ptr)});
	},
	printError: function(ptr){
		self.postMessage({code: 3, payload: Module.UTF8ToString(ptr)})
	},
	addShape: function(){
		_currentFrame.push("hello");
	},
	addShape__deps: [
        'currentFrame'
	],
	nextFrame: function(){
		_frames.push(currentFrame);
		self.postMessage({code: 4, payload: _currentFrame});
		_currentFrame = {};
	},
	nextFrame__deps: [
        'frames',
        'currentFrame'
	],
});
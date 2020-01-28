mergeInto(LibraryManager.library, {
	currentShape: {},

	printOut: function(ptr) {
		self.postMessage({type: 3, payload: Module.UTF8ToString(ptr)});
	},
	printDebug: function(ptr){
		self.postMessage({type: 4, payload: Module.UTF8ToString(ptr)});
	},
	printError: function(ptr){
		self.postMessage({type: 5, payload: Module.UTF8ToString(ptr)})
	},
	
	newShape: function(idPtr, tagPtr){
		_currentShape = {
			"id": idPtr,
			"tag": tagPtr,
			"attrs":{},
			"style":{
				"values":{},
				"loc":{},
			},
		}
	},

	addStringAttribute: function(keyPtr, valuePtr, index, line){
		let key = Module.UTF8ToString(keyPtr);
		let value = Module.UTF8ToString(valuePtr);
		_currentShape['attrs'][key] = {
			value: value,
			index: index,
			line: line
		};
	},

	addAttribute: function(keyPtr, value, index, line){
		let key = Module.UTF8ToString(keyPtr);
		_currentShape['attrs'][key] = {
			value: value,
			index: index,
			line: line
		};
	},

	addStringStyle: function(keyPtr, valuePtr, index, line){
		let key = Module.UTF8ToString(keyPtr);
		let value = Module.UTF8ToString(valuePtr);
		_currentShape['style']['values'][key] = value;
		_currentShape['style']['loc'][key] = {
			index: index,
			line: line,
		}
	},

	addStyle: function(keyPtr, value, index, line){
		let key = Module.UTF8ToString(keyPtr);
		_currentShape['style']['values'][key] = value;
		_currentShape['style']['loc'][key] = {
			index: index,
			line: line,
		}
	},

	paintShape: function(){
		Module._currentFrame.push(_currentShape);
	},

	setMaxFrameIndex: function(num){
		Module._maxFrameIndex = num;
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

	addStringStyle__deps: [
        'currentShape'
	],

	addAttribute__deps: [
		'currentShape'
	],

	addStringAttribute__deps: [
		'currentShape'
	],
});
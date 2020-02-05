mergeInto(LibraryManager.library, {
	currentShape: {},
	values: [],

	em_addValue: function(inlineOffset, length){
		_values.push({
			inlineOffset: inlineOffset,
			length: length,
		})
	},

	em_endLine: function(newlineIndex){
		Module._lines.push({
			charIndex: newlineIndex,
			values: _values
		});
		_values = [];
	},

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

	addStringAttribute: function(keyPtr, valPtr, lineIndex, inlineIndex){
		let key = Module.UTF8ToString(keyPtr);
		let value = Module.UTF8ToString(valPtr);
		_currentShape['attrs'][key] = {
			value: value,
			lineIndex: lineIndex,
			inlineIndex: inlineIndex,
		};
	},

	addAttribute: function(keyPtr, value, lineIndex, inlineIndex){
		let key = Module.UTF8ToString(keyPtr);
		_currentShape['attrs'][key] = {
			value: value,
			lineIndex: lineIndex,
			inlineIndex: inlineIndex,
		};
	},

	addStringStyle: function(keyPtr, valuePtr, lineIndex, inlineIndex){
		let key = Module.UTF8ToString(keyPtr);
		let value = Module.UTF8ToString(valuePtr);
		_currentShape['style']['values'][key] = value;
		_currentShape['style']['loc'][key] = {
			lineIndex: lineIndex,
			inlineIndex: inlineIndex, 
		}
	},

	addStyle: function(keyPtr, value, lineIndex, inlineIndex){
		let key = Module.UTF8ToString(keyPtr);
		_currentShape['style']['values'][key] = value;
		_currentShape['style']['loc'][key] = {
			lineIndex: lineIndex,
			inlineIndex: inlineIndex, 
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

	em_addValue__deps: [
		'values',
	],

	em_endLine__deps: [
		'values',
	]
	
});
mergeInto(LibraryManager.library, {
	currentShape: {},
	currentLine: {
		charIndex: 0,
		values: [],
	},

	em_addValue: function(lineIndex, inlineOffset, length){
		_currentLine.values.push({
			lineIndex: lineIndex,
			inlineOffset: inlineOffset,
			length: length,
		})
	},

	em_endLine: function(newlineIndex){
		Module._lines.push(_currentLine);
		_currentLine = {
			charIndex: newlineIndex,
			values: [],
		}
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
		let value = Module.UTF8ToString(valuePtr);
		_currentShape['attrs'][key] = {
			value: value,
			lineIndex: index,
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
		_currentShape['style']['values'][key] = {
			value: value,
			lineIndex: lineIndex,
			inlineIndex: inlineIndex, 
		}
	},

	addStyle: function(keyPtr, value, lineIndex, inlineIndex){
		let key = Module.UTF8ToString(keyPtr);
		_currentShape['style']['values'][key] = {
			value: value,
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
		'currentLine',
	],

	em_endLine__deps: [
		'currentLine',
	]
	
});
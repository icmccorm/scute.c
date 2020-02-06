mergeInto(LibraryManager.library, {
	currentShape: {},
	values: [],
	valuePointerOffsets: {
		type: 0,
		union: 0,
		lineIndex: 0,
		inlineIndex: 0,
	},

	em_configureValuePointerOffsets(type, union, lineIndex, inlineIndex){
		console.log("Value conversion offsets: " + type + ", " + union + ", " + lineIndex + ", " + inlineIndex);
		_valuePointerOffsets.type = type;
		_valuePointerOffsets.union = union;
		_valuePointerOffsets.lineIndex = lineIndex;
		_valuePointerOffsets.inlinIndex = inlineIndex;
	},

	//getValue(ptr, type) and setValue(ptr, value, type)
	em_getValueMeta: function (valPtr){
		return {
			lineIndex: getValue(valPtr + _valuePointerOffsets.lineIndex, 'i32'),
			inlineIndex: getValue(valPtr + _valuePointerOffsets.inlineIndex, 'i32'),
		}
	},

	em_addValue: function(value, inlineOffset, length){
		_values.push({
			value: value,
			inlineOffset: inlineOffset,
			length: length,
		})
	},

	em_addStringValue(valuePtr, inlineOffset, length){
		let value = Module.UTF8ToString(valuePtr);
		_values.push({
			value: value,
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

	addStringAttribute: function(keyPtr, lineIndex, inlineIndex){
		let key = Module.UTF8ToString(keyPtr);
		let value = Module.UTF8ToString(valPtr);
		_currentShape['attrs'][key] = [lineIndex, inlineIndex];
	},

	addAttribute: function(keyPtr, lineIndex, inlineIndex){
		let key = Module.UTF8ToString(keyPtr);
		_currentShape['attrs'][key] = [lineIndex, inlineIndex];
	},

	addStringStyle: function(keyPtr, lineIndex, inlineIndex){
		let key = Module.UTF8ToString(keyPtr);
		_currentShape['style'][key] = [lineIndex, inlineIndex];
	},

	addStyle: function(keyPtr, lineIndex, inlineIndex){
		let key = Module.UTF8ToString(keyPtr);
		_currentShape['style'][key] = [lineIndex, inlineIndex];
	},

	paintShape: function(){
		Module._currentFrame.push(_currentShape);
	},

	setMaxFrameIndex: function(num){
		Module._maxFrameIndex = num;
	},

	setCanvasDimensions: function(widthLineIndex, heightLineIndex, widthInlineIndex, heightInlineIndex){
		Module._canvasDimensions = {
			width: [widthLineIndex, widthInlineIndex],
			height: [heightLineIndex, heightInlineIndex]
		}
	},

	setCanvasOrigin: function(xLineIndex, yLineIndex, xInlineIndex, yInlineIndex){
		Module._canvasOrigin = {
			originX: [xLineIndex, xInlineIndex],
			originY: [yLineIndex, yInlineIndex]
		}
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
	],

	em_getValueMeta__deps: [
		'valuePointerOffsets'
	],

	em_configureValuePointerOffsets__deps:[
		'valuePointerOffsets'
	]

	
});
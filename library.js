mergeInto(LibraryManager.library, {
	currentShape: {},
	values: [],
	valuePointerOffsets: {
		type: 0,
		union: 0,
		lineIndex: 0,
		inlineIndex: 0,
	},

	valueTypes: {
		0: "VL_NULL",
		1: "VL_BOOL",
		2: "VL_NUM",
		3: "VL_OBJ",
		4: "VL_CLR",
	},

	em_configureValuePointerOffsets: function (type, union, lineIndex, inlineIndex){
		_valuePointerOffsets.type = type;
		_valuePointerOffsets.union = union;
		_valuePointerOffsets.lineIndex = lineIndex;
		_valuePointerOffsets.inlineIndex = inlineIndex;
	},

	//getValue(ptr, type) and setValue(ptr, value, type)
	lib_getValueMeta: function (valPtr){
		return {
			type: getValue(valPtr + _valuePointerOffsets.type, 'i32'),
			lineIndex: getValue(valPtr + _valuePointerOffsets.lineIndex, 'i32'),
			inlineIndex: getValue(valPtr + _valuePointerOffsets.inlineIndex, 'i32'),
		}
	},

	lib_getValue: function(valPtr){
		let type = _valueTypes[getValue(valPtr + _valuePointerOffsets.type, 'i32')];
		let value = getValue(valPtr + _valuePointerOffsets.union, 'double');
		switch(type){
			case 'VL_BOOL':
				value = Boolean(value);
				break;
			case 'VL_NULL':
				value = null;
				break;
			case 'VL_NUM':
			default:
				break;
		}
		return value;
	},

	em_addValue: function(valuePtr, inlineOffset, length){
		let convertVal = _lib_getValue(valuePtr);
		_values.push({
			value: convertVal,
			inlineOffset: inlineOffset,
			length: length,
		})
	},

	em_addStringValue: function(charPtr, inlineOffset, length){
		_values.push({
			value: Module.UTF8ToString(charPtr),
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

	addAttribute: function(keyPtr, valuePtr){
		let key = Module.UTF8ToString(keyPtr);
		let meta = _lib_getValueMeta(valuePtr);
		_currentShape['attrs'][key] = meta;
	},

	addStringStyle: function(keyPtr, valuePtr){
		let key = Module.UTF8ToString(keyPtr);
		let meta = _lib_getValueMeta(valuePtr);
		_currentShape['style'][key] = meta;
	},

	addStyle: function(keyPtr, valuePtr){
		let key = Module.UTF8ToString(keyPtr);
		let meta = _lib_getValueMeta(valuePtr);
		_currentShape['style'][key] = meta;
	},

	paintShape: function(){
		Module._currentFrame.push(_currentShape);
	},

	setMaxFrameIndex: function(num){
		Module._maxFrameIndex = num;
	},

	setCanvas: function(widthPtr, heightPtr, xPtr, yPtr){

	},

	newShape__deps: [
		'currentShape'
	],
	
	paintShape__deps: [
		'currentShape'
	],

	addStyle__deps: [
		'currentShape',
		'lib_getValueMeta'
	],

	addStringStyle__deps: [
		'currentShape',
		'lib_getValueMeta'
	],

	addAttribute__deps: [
		'currentShape',
		'lib_getValueMeta',
	],

	addStringAttribute__deps: [
		'currentShape',
		'lib_getValueMeta'
	],

	em_addValue__deps: [
		'values',
		'lib_getValue'
	],

	em_endLine__deps: [
		'values',
	],

	em_getValueMeta__deps: [
		'valuePointerOffsets'
	],

	em_configureValuePointerOffsets__deps:[
		'valuePointerOffsets'
	],

	lib_getValue__deps: [
		'valueTypes'
	]

	
});
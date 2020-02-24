mergeInto(LibraryManager.library, {
	currentShape: {},
	values: [],
	currentTurtle: null,
	
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

	attrStatus: {
		CONST: 0,
		COMP: 1,
		OPEN: 2,
	},

	segmentTypes: {
		JUMP: 0,
		TURTLE: 1,
		VERTEX: 3,
	},

	em_configureValuePointerOffsets: function (type, union, lineIndex, inlineIndex){
		_valuePointerOffsets.type = type;
		_valuePointerOffsets.union = union;
		_valuePointerOffsets.lineIndex = lineIndex;
		_valuePointerOffsets.inlineIndex = inlineIndex;
	},

	//getValue(ptr, type) and setValue(ptr, value, type)
	lib_getValueMeta: function (valPtr){
		let lineIndex = getValue(valPtr + _valuePointerOffsets.lineIndex, 'i32');
		if(lineIndex > -1){
			let inlineIndex = getValue(valPtr + _valuePointerOffsets.inlineIndex, 'i32');
			return {
				type: getValue(valPtr + _valuePointerOffsets.type, 'i32'),
				status: _attrStatus.CONST,
				lineIndex: lineIndex,
				inlineIndex: inlineIndex,
			}
		}else{
			return {
				type: getValue(valPtr + _valuePointerOffsets.type, 'i32'),
				status: lineIndex > -1 ? _attrStatus.CONST : _attrStatus.COMP,
				value: _lib_getValue(valPtr),
			}
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
	
	em_newShape: function(idPtr, tagPtr){
		_currentShape = {
			"id": idPtr,
			"tag": tagPtr,
			"attrs":{},
			"styles":{},
			"segments": [],
		}
	},

	em_addAttribute: function(keyPtr, valuePtr){
		let key = Module.UTF8ToString(keyPtr);
		let meta = _lib_getValueMeta(valuePtr);
		_currentShape['attrs'][key] = meta;
	},

	em_addLine: function(valPtr){

	},

	em_addHorizontalLine: function(valPtr){

	},

	em_addVerticalLine: function(valPtr){

	},

	em_addStringStyle: function(keyPtr, valuePtr){
		let key = Module.UTF8ToString(keyPtr);
		let meta = _lib_getValueMeta(valuePtr);
		_currentShape['styles'][key] = meta;
	},

	em_addStyle: function(keyPtr, valuePtr){
		let key = Module.UTF8ToString(keyPtr);
		let meta = _lib_getValueMeta(valuePtr);
		_currentShape['styles'][key] = meta;
	},

	em_paintShape: function(){
		Module._currentFrame.push(_currentShape);
	},

	setMaxFrameIndex: function(num){
		Module._maxFrameIndex = num;
	},

	em_setCanvas: function(dimsPtr, originPtr){
		Module._canvas = {
			width: _lib_getValue(dimsPtr),
			height: _lib_getValue(dimsPtr + 1),
			originX: _lib_getValue(originPtr),
			originY: _lib_getValue(originPtr + 1),
		}
	},

	lib_intArrayToPoint: function(intPtr) {
		let x = getValue(intPtr, 'i32');
		let y = getValue(intPtr + 1, 'i32');
		return x + "," + y;
	},

	em_addJump: function(vecPtr){
		if(!_currentShape.segments) _currentShape.segments = [];
		_currentShape.segments.push({
			type: _segmentTypes.JUMP,
			x: _lib_getValueMeta(vecPtr),
			y: _lib_getValueMeta(vecPtr+1),
			point: [_lib_getValue(vecPtr), _lib_getValue(vecPtr+1)],
		});
	},

	em_addVertex: function(vecPtr){
		if(!_currentShape.segments) _currentShape.segments = [];
		_currentShape.segments.push({
			type: _segmentTypes.VERTEX,
			x: _lib_getValueMeta(vecPtr),
			y: _lib_getValueMeta(vecPtr+1),
			point: [_lib_getValue(vecPtr), _lib_getValue(vecPtr+1)],
		});ß
	},

	em_addMove: function(x, y, distancePtr){
		if(!_currentShape.segments) _currentShape.segments = [];
		if(!_currentTurtle) {
			_currentTurtle = {
				move: null,
				turn: null,
				point: null,
			}
		}
		_currentTurtle.move = _lib_getValueMeta(distancePtr);
		_currentShape.segments.push({
			type: _segmentTypes.TURTLE,
			move: _currentTurtle.move,
			turn: _currentTurtle.turn,
			point: [x, y],
		});
	},


	em_addTurn: function(degreesPtr){
		if(!_currentShape.segments) _currentShape.segments = [];
		if(!_currentTurtle) {
			_currentTurtle = {
				move: null,
				turns: null,
				point: null,
			}
		}
		_currentTurtle.turn = _lib_getValueMeta(degreesPtr);
	},

	em_newShape__deps: [
		'currentShape'
	],
	
	em_paintShape__deps: [
		'currentShape'
	],

	em_addStyle__deps: [
		'currentShape',
		'lib_getValueMeta'
	],

	em_addStringStyle__deps: [
		'currentShape',
		'lib_getValueMeta'
	],

	em_addAttribute__deps: [
		'currentShape',
		'lib_getValueMeta',
	],

	em_addStringAttribute__deps: [
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
	],

	em_addJump__deps: [
		'currentShape',
		'segmentTypes'
	],

	em_addTurn__deps: [
		'currentShape',
		'segmentTypes',
		'currentTurtle'

	],

	em_addMove__deps: [
		'currentShape',
		'segmentTypes',
		'currentTurtle',
		'lib_intArrayToPoint'
	],

	lib_getValueMeta__deps: [
		'attrStatus'
	]
});
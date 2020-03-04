mergeInto(LibraryManager.library, {
	currentShape: {},
	values: [],
	currentTurtle: null,
	
	valuePointerOffsets: {
		type: 0,
		union: 0,
		lineIndex: 0,
		inlineIndex: 0,
		totalLength: 0,
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
		VERTEX: 2,
		CBEZIER: 3,
		QBEZIER: 4,
		ARC: 5,
	},

	em_configureValuePointerOffsets: function (type, union, lineIndex, inlineIndex){
		_valuePointerOffsets.type = type;
		_valuePointerOffsets.union = union;
		_valuePointerOffsets.lineIndex = lineIndex;
		_valuePointerOffsets.inlineIndex = inlineIndex;
		_valuePointerOffsets.totalLength = 24;
	},

	//getValue(ptr, type) and setValue(ptr, value, type)
	lib_getValueMeta: function (valPtr){
		let lineIndex = getValue(valPtr + _valuePointerOffsets.lineIndex, 'i32');
		let inlineIndex = getValue(valPtr + _valuePointerOffsets.inlineIndex, 'i32');
		return {
			value: _lib_getValue(valPtr),
			lineIndex: lineIndex, 
			inlineIndex: inlineIndex,
		}
	},

	lib_getValue: function(valPtr){
		return getValue(valPtr + _valuePointerOffsets.union, 'double');
	},

	em_addValue: function(value, role, inlineOffset, length){
		_values.push({
			mouseDelta: 0,
			targetValue: value,
			role: role,
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

	em_addColorStyle: function(keyPtr, length, valPtr){
		let key = Module.UTF8ToString(keyPtr);
		let rgba = [];
		for(let i = 0; i<length*_valuePointerOffsets.totalLength; i += _valuePointerOffsets.totalLength){
			rgba.push(_lib_getValueMeta(valPtr + i));
		}
		_currentShape['styles'][key] = rgba;
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
			height: _lib_getValue(dimsPtr + _valuePointerOffsets.totalLength),
			originX: _lib_getValue(originPtr),
			originY: _lib_getValue(originPtr + _valuePointerOffsets.totalLength),
		}
	},

	em_addJump: function(vecPtr){
		if(!_currentShape.segments) _currentShape.segments = [];
		_currentShape.segments.push({
			type: _segmentTypes.JUMP,
			x: _lib_getValueMeta(vecPtr),
			y: _lib_getValueMeta(vecPtr + _valuePointerOffsets.totalLength),
		});
	},

	em_addVertex: function(vecPtr){
		if(!_currentShape.segments) _currentShape.segments = [];
		_currentShape.segments.push({
			type: _segmentTypes.VERTEX,
			x: _lib_getValueMeta(vecPtr),
			y: _lib_getValueMeta(vecPtr+_valuePointerOffsets.totalLength),
		});
	},

	em_addMove: function(x, y, distancePtr){
		if(!_currentShape.segments) _currentShape.segments = [];
		if(!_currentTurtle) {
			_currentTurtle = {
				move: null,
				turn: null,
				x: null,
				y: null,
			}
		}
		_currentTurtle.move = _lib_getValueMeta(distancePtr);
		_currentShape.segments.push({
			type: _segmentTypes.TURTLE,
			move: _currentTurtle.move,
			turn: _currentTurtle.turn,
			x: x,
			y: y,
		});
	},


	em_addTurn: function(degreesPtr){
		if(!_currentShape.segments) _currentShape.segments = [];
		if(!_currentTurtle) {
			_currentTurtle = {
				move: null,
				turns: null,
				x: null,
				y: null,
			}
		}
		_currentTurtle.turn = _lib_getValueMeta(degreesPtr);
	},

	lib_getVector: function(vecPtr){
		return [_lib_getValueMeta(vecPtr), _lib_getValueMeta(vecPtr + _valuePointerOffsets.totalLength)];
	},

	em_addQuadBezier: function(control, end){
		if(!_currentShape.segments) _currentShape.segments = [];
		_currentShape.segments.push({
			type: _segmentTypes.QBEZIER,
			control: _lib_getVector(control),
			end: _lib_getVector(end),
		});
	},

	em_addCubicBezier: function(control1, control2, end){
		if(!_currentShape.segments) _currentShape.segments = [];
		if(!_currentShape.segments) _currentShape.segments = [];
		_currentShape.segments.push({
			type: _segmentTypes.CBEZIER,
			control1: _lib_getVector(control1),
			control2: _lib_getVector(control2),
			end: _lib_getVector(end),
		});
	},

	em_addArc: function(center, degrees){
		if(!_currentShape.segments) _currentShape.segments = [];
		_currentShape.segments.push({
			type: _segmentTypes.ARC,
			center: _lib_getVector(center),
			degrees: _lib_getValueMeta(degrees),
		});
	},

	lib_getVector__deps: [
		'lib_getValueMeta',
		'valuePointerOffsets',
	],

	em_addArc__deps: [
		'currentShape',
		'segmentTypes',
		'lib_getValue',
		'lib_getVector'
	],
	em_addCubicBezier__deps: [
		'currentShape',
		'segmentTypes',
		'lib_getVector'
	],
	em_addQuadBezier__deps: [
		'currentShape',
		'segmentTypes',
		'lib_getVector'
	],
	
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
	],

	lib_getValueMeta__deps: [
		'attrStatus'
	],

	em_addColorStyle__deps: [
		'lib_getValue',
		'currentShape'
	] 

});
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



	em_configureValuePointerOffsets: function (typePtr, unionPtr, lineIndexPtr, inlineIndexPtr){
		_valuePointerOffsets.type = getValue(typePtr, 'i32');
		_valuePointerOffsets.union = getValue(unionPtr, 'i32');
		_valuePointerOffsets.lineIndex = getValue(lineIndexPtr, 'i32');
		_valuePointerOffsets.inlineIndex = getValue(inlineIndexPtr, 'i32');
		_valuePointerOffsets.totalLength = 24;
	},

	//getValue(ptr, type) and setValue(ptr, value, type)
	lib_getValueMeta: function (valPtr){
		var lineIndex = getValue(valPtr + _valuePointerOffsets.lineIndex, 'i32');
		var inlineIndex = getValue(valPtr + _valuePointerOffsets.inlineIndex, 'i32');
		return {
			value: _lib_getValue(valPtr),
			lineIndex: lineIndex, 
			inlineIndex: inlineIndex,
		}
	},

	lib_getValue: function(valPtr){
		return getValue(valPtr + _valuePointerOffsets.union, 'double');
	},

	em_addValue: function(inlineOffsetPtr, lengthPtr, opPtr, valPtr){
		var inlineOffset = getValue(inlineOffsetPtr, 'i32');
		var length = getValue(lengthPtr, 'i32');
		var operator = getValue(opPtr, 'i32');
		_values.push({
			delta: 0,
			origin: _lib_getValue(valPtr),
			op: operator,
			inlineOffset: inlineOffset,
			length: length,
		});
	},

	em_addUnlinkedValue: function(inlineOffsetPtr, valuePtr){
		var inlineOffset = getValue(inlineOffsetPtr, 'i32');
		var length = 0;
		var operator = 0;
		_values.push({
			delta: 0,
			origin: _lib_getValue(valuePtr),
			op: operator,
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

	em_endLine: function(newlineIndexPtr){
		var newlineIndex = getValue(newlineIndexPtr, 'i32');
		Module._lines.push({
			charIndex: newlineIndex,
			values: _values
		});
		_values = [];
	},

	em_printOut: function(ptr) {
		Module._printFunction({type: 3, payload: Module.UTF8ToString(ptr)});
	},
	em_printDebug: function(ptr){
		Module._printFunction({type: 4, payload: Module.UTF8ToString(ptr)});
	},
	em_printError: function(ptr){
		Module._printFunction({type: 4, payload: Module.UTF8ToString(ptr)});
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
		var key = Module.UTF8ToString(keyPtr);
		var meta = _lib_getValueMeta(valuePtr);
		_currentShape['attrs'][key] = meta;
	},

	em_addVectorAttr: function(keyPtr, vecPtr){
		var key = Module.UTF8ToString(keyPtr);
		_currentShape['attrs'][key] = _lib_getVector(vecPtr);
	},

	em_addLine: function(valPtr){

	},

	em_addColorStyle: function(keyPtr, length, valPtr){
		var key = Module.UTF8ToString(keyPtr);
		var rgba = [];
		for(var i = 0; i<length*_valuePointerOffsets.totalLength; i += _valuePointerOffsets.totalLength){
			rgba.push(_lib_getValueMeta(valPtr + i));
		}
		_currentShape['styles'][key] = rgba;
	},

	em_addStringStyle: function(keyPtr, valuePtr){
		var key = Module.UTF8ToString(keyPtr);
		var meta = _lib_getValueMeta(valuePtr);
		_currentShape['styles'][key] = meta;
	},

	em_addStyle: function(keyPtr, valuePtr){
		var key = Module.UTF8ToString(keyPtr);
		var meta = _lib_getValueMeta(valuePtr);
		_currentShape['styles'][key] = meta;
	},

	em_paintShape: function(){
		Module._currentFrame.push(_currentShape);
	},

	setMaxFrameIndex: function(num){
		Module._maxFrameIndex = num;
	},

	em_setCanvas: function(originPtr, dimsPtr){
		Module._canvas = {
			size: _lib_getVector(dimsPtr),
			origin: _lib_getVector(originPtr),
		}
	},

	em_addJump: function(vecPtr){
		if(!_currentShape.segments) _currentShape.segments = [];
		_currentShape.segments.push({
			type: _segmentTypes.JUMP,
			point: _lib_getVector(vecPtr),
		});
	},

	em_addVertex: function(vecPtr){
		if(!_currentShape.segments) _currentShape.segments = [];
		_currentShape.segments.push({
			type: _segmentTypes.VERTEX,
			point: _lib_getVector(vecPtr),
		});
	},

	em_addMove: function(distancePtr){
		if(!_currentShape.segments) _currentShape.segments = [];
		if(!_currentTurtle) {
			_currentTurtle = {
				type: _segmentTypes.TURTLE,
				move: null,
				turn: null,
			}
		}
		_currentTurtle.move = _lib_getValueMeta(distancePtr);
		_currentShape.segments.push(_currentTurtle);
		_currentTurtle = null;
	},


	em_addTurn: function(degreesPtr){
		if(!_currentShape.segments) _currentShape.segments = [];
		if(!_currentTurtle) {
			_currentTurtle = {
				type: _segmentTypes.TURTLE,
				move: null,
				turn: null,
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

	em_printOut__deps: [

	],

	em_printDebug__deps: [

	],

	em_printError__deps: [

	],

	lib_getVector__deps: [
		'lib_getValueMeta',
		'valuePointerOffsets',
	],

	em_addVertex__deps: [
		'currentShape',
		'lib_getVector'
	],

	em_addVectorAttr__deps: [
		'currentShape',
		'lib_getVector'
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

	em_addValueInsert__deps: [
		'values',
		'lib_getValue'
	],

	em_endLine__deps: [
		'values',
	],

	em_addStage__deps: [
		'lib_getValueMeta'
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
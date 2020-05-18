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

	//OP_ADD 		0
	//OP_SUBTRACT,	1
	//OP_MULTIPLY,	2
	//OP_DIVIDE,	3
	//OP_MODULO,	4

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
		var lineIndex = getValue(valPtr + _valuePointerOffsets.lineIndex, 'i32');
		var inlineIndex = getValue(valPtr + _valuePointerOffsets.inlineIndex, 'i32');
		return {
			value: _lib_getValue(valPtr),
			lineIndex: lineIndex, 
			inlineIndex: inlineIndex,
		}
	},

	lib_getValue: function(valPtr){
		return Math.floor(getValue(valPtr + _valuePointerOffsets.union, 'double') * 1000) / 1000;
	},

	em_addValue: function(inlineOffset, length){
		_values.push({
			delta: 0,
			inlineOffset: inlineOffset,
			length: length,
			stages: [],
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

	em_addStage: function(valPtr, opPtr){
		var role = getValue(opPtr, 'i8');
		var valueMeta = _lib_getValueMeta(valPtr);
		var line = Module._lines[valueMeta.lineIndex];
		var value = line.values[valueMeta.inlineIndex];
		var stages = value["stages"];
		stages.push({
			value: valueMeta.value,
			role: role,
		});
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
mergeInto(LibraryManager.library, {
	currentShape: {},
	values: [],
	currentTurtle: null,
	currentLower: 0,
	currentUpper: 999,
	animations: {},
	currentAnimation: null,

	em_initAnimationChunk: function(animObjectPtr){
		_currentAnimation = {
			id: animObjectPtr,
		}
	},
	
	em_finalizeAnimationChunk: function(){
		if(_currentAnimation != null){
			Module._animations[_currentAnimation.id] = _currentAnimation;
		}
	},

	em_animateValue: function(propertyString, valuePtr){
		var key = UTF8ToString(propertyString);
		var value = _lib_getValueMeta(valuePtr);
		_currentAnimation[key] = value;
	},

	valuePointerOffsets: {
		type: 0,
		union: 0,
		lineIndex: 0,
		inlineIndex: 0,
		totalLength: 0,
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
			prevDelta: 0,
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
		Module["_printFunction"]({type: 3, payload: Module.UTF8ToString(ptr)});
	},
	em_printDebug: function(ptr){
		Module["_printFunction"]({type: 4, payload: Module.UTF8ToString(ptr)});
	},
	em_printError: function(ptr){
		Module["_printFunction"]({type: 4, payload: Module.UTF8ToString(ptr)});
	},

	em_assignAnimation: function(ptr){
		_currentShape.animationObject = ptr;
	},
	
	em_newRect: function(idPtr){
		_lib_newShape(idPtr, Shapes.RECT);
	},

	em_newCirc: function(idPtr){
		_lib_newShape(idPtr, Shapes.CIRC);
	},

	em_newEllip: function(idPtr){
		_lib_newShape(idPtr, Shapes.ELLIP);
	},

	em_newLine: function(idPtr){
		_lib_newShape(idPtr, Shapes.LINE);
	},

	em_newPolygon: function(idPtr){
		_lib_newShape(idPtr, Shapes.POLYG);
	},

	em_newPolyline: function(idPtr){
		_lib_newShape(idPtr, Shapes.POLYL);
	},

	em_newPath: function(idPtr){
		_lib_newShape(idPtr, Shapes.PATH);
	},

	em_newUngon: function(idPtr){
		_lib_newShape(idPtr, Shapes.UNGON);
	},

	lib_newShape: function(idPtr, tag){
		_currentShape = {
			"id": idPtr,
			"tag": tag,
			"attrs":{},
			"styles":{},
			"segments": [],
		};
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

	em_paintShape: function(uniqueId){
		Module["_currentFrame"].shapes[uniqueId] = _currentShape;
		_currentTurtle = {
			type: Segments.TURTLE,
			move: null,
			turn: null,
			horizontal: 0,
		};
	},

	em_setMaxFrameIndex: function(num){
		Module._maxFrameIndex = num;
	},

	em_setCanvas: function(originPtr, dimsPtr){
		Module["_canvas"] = {
			size: _lib_getVector(dimsPtr),
			origin: _lib_getVector(originPtr),
		}
	},

	em_addJump: function(anim, vecPtr){
		_currentShape.segments.push(anim)
		Module["_currentFrame"].segments[anim] = {
			type: Segments.JUMP,
			point: _lib_getVector(vecPtr),
		};
	},

	em_addVertex: function(anim, vecPtr){	
		_currentShape.segments.push(anim)	
		Module["_currentFrame"].segments[anim] = {
			type: Segments.VERTEX,
			point: _lib_getVector(vecPtr),
		};
	},

	em_addMove: function(anim, distancePtr, horizontalPtr){
		_currentShape.segments.push(anim)	

		if(!_currentTurtle) {
			_currentTurtle = {
				type: Segments.TURTLE,
				move: null,
				turn: null,
				horizontal: 0,
			}
		}
		_currentTurtle.move = _lib_getValueMeta(distancePtr);
		_currentTurtle.horizontal = getValue(horizontalPtr, "i32");
		
		Module["_currentFrame"].segments[anim] = _currentTurtle;

		_currentTurtle = {
				type: Segments.TURTLE,
				move: null,
				turn: null,
				horizontal: 0,
			};
	},


	em_addTurn: function(degreesPtr){
		if(!_currentTurtle) {
			_currentTurtle = {
				type: Segments.TURTLE,
				move: null,
				turn: null,
				horizontal: 0,
			}
		}
		_currentTurtle.turn = _lib_getValueMeta(degreesPtr);
	},

	lib_getVector: function(vecPtr){
		return [_lib_getValueMeta(vecPtr), _lib_getValueMeta(vecPtr + _valuePointerOffsets.totalLength)];
	},

	em_addQuadBezier: function(anim, control, end){
		_currentShape.segments.push(anim)	
		Module["_currentFrame"].segments[anim] = {
			type: Segments.QBEZIER,
			control: _lib_getVector(control),
			end: _lib_getVector(end),
		};
	},

	em_addMirror: function(anim, originPtr, x, y){
		_currentShape.segments.push(anim)	
		Module["_currentFrame"].segments[anim] = {
			type: Segments.MIRROR,
			axis: (x && y ? Axes.XY : (x ? Axes.X : Axes.Y)),
			origin: _lib_getVector(originPtr)
		};
	},

	em_addCubicBezier: function(anim, control1, control2, end){
		_currentShape.segments.push(anim)	
		Module["_currentFrame"].segments[anim] = {
			type: Segments.CBEZIER,
			control1: _lib_getVector(control1),
			control2: _lib_getVector(control2),
			end: _lib_getVector(end),
		};
	},

	em_addArc: function(anim, center, degrees, radius){
		_currentShape.segments.push(anim)	
		Module["_currentFrame"].segments[anim] = {
			type: Segments.ARC,
			center: _lib_getVector(center),
			degrees: _lib_getValueMeta(degrees),
		};
	},

	em_addMirror: function(anim, originPtr, axisPtr){
		_currentShape.segments.push(anim)	
		Module["_currentFrame"].segments[anim] = {
			type: Segments.MIRROR,
			origin: _lib_getVector(originPtr),
			axis: _lib_getValue(axisPtr),
		};
	},

	em_printOut__deps: [

	],

	em_printDebug__deps: [

	],

	em_printError__deps: [

	],

	em_newCirc__deps: [
		"lib_newShape",
	],
	
	em_newRect__deps: [
		"lib_newShape",
	],

	em_newEllip__deps: [
		"lib_newShape",
	],

	em_newLine__deps: [
		"lib_newShape",	 
	],

	em_newPolygon__deps: [
		"lib_newShape",
	],

	em_newPolyline__deps: [
		"lib_newShape",
	],

	em_newPath__deps: [
		"lib_newShape",
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
		'lib_getValue',
		'lib_getVector'
	],
	em_addCubicBezier__deps: [
		'currentShape',
		'lib_getVector'
	],
	em_addQuadBezier__deps: [
		'currentShape',
		'lib_getVector'
	],
	
	lib_newShape__deps: [
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

	em_addValue__deps: [
		'values',
		'lib_getValue'
	],

	em_endLine__deps: [
		'values',
	],

	em_configureValuePointerOffsets__deps:[
		'valuePointerOffsets'
	],

	em_addJump__deps: [
		'currentShape',
	],

	em_addTurn__deps: [
		'currentShape',
		'currentTurtle'

	],

	em_addMove__deps: [
		'currentShape',
		'currentTurtle',
	],

	em_addColorStyle__deps: [
		'lib_getValue',
		'currentShape'
	],

	em_animateValue__deps: [
		'currentAnimation',
		'lib_getValueMeta',
	],
	
	em_initAnimationChunk__deps: [
		'currentAnimation',
	],

	em_finalizeAnimationChunk__deps: [
		'currentAnimation'
	],
	
});
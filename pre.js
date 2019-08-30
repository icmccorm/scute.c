window.InterpreterModule = {
	onRuntimeInitialized : function() {
		console.log("Emscripten Ready!");
		document.dispatchEvent(new Event('emready'));
	}
}
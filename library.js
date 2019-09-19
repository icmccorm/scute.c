mergeInto(LibraryManager.library, {
	print: function(ptr) {
		self.postMessage({cmd: 1, payload: Module.UTF8ToString(ptr)});
	},
	printDebug: function(ptr){
		self.postMessage({cmd: 2, payload: Module.UTF8ToString(ptr)});
	},
	printError: function(ptr){
		self.postMessage({cmd: 3, payload: Module.UTF8ToString(ptr)})
	}
  });
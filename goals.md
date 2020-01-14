# new behavior:
	* shape styles and attributes are read at drawtime,
	  and default to NULL (0) if unset

	* string objects are no longer deallocated during runtime

	* new scope attribute syntax
		
		dims = (50, 50)
		// is equivalent to
		x = 50
		y = 50
		// within a shape statement

	* fix/implement/debug assignment expressions to autoresolve to local, global

	* when a shape-scope is created, subscope members are autodefined (without attributes)
		* for example, stroke
		
		stroke 		//initialized as a hashmap at runtime (scope allocation)
		stroke.width 	//initialized as a member at runtime

		* this means that stroke can be dereferenced without initialization
	          
		stroke.width = 5

		//instead of having to predeclare stroke

		stroke = (/*color*/, /*width*/)
		stroke.width = 5
		
	* explore behavior for direct manipulation resolving to local variables/inserting addition statements
		* when should each behavior occur? 
		* should this be a used-configured mode? 

	* create a syntax for points

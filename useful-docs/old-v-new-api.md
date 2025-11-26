Key changes for v10.1.1:


=====================================================================================

Old API					|	New API

=====================================================================================

result.d = value			|	WofValue::make\_double(value)

value.d (direct access)			|	value.as\_numeric()

init\_plugin(OpTable\*)			|	register\_plugin(WoflangInterpreter\&)

(\*op\_table)\["cmd"] = ...;		|	interp.register\_op("cmd", ...);

Stack: std::stack<WofValue>		|	Stack: std::vector<WofValue>

Lambda: \[](std::stack\&)	Lambda: 	|	\[](WoflangInterpreter\&)
=====================================================================================


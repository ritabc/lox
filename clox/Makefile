test: interpret_test.c
	cc -Wno-deprecated-non-prototype -o interpret_test interpret_test.c chunk.c memory.c debug.c value.c vm.c compiler.c scanner.c object.c table.c \
		&& ./interpret_test
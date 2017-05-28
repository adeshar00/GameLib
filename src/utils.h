

// Function Prototypes
//{{{

void lnxLogError(char s[], ...);

//}}}

//TODO put lnxLog in here? rename this utililities? Get filename via preprocessor if possible, 
// and prepend lnxLog output with filename if debug enabled??

// TODO move checkAlloc from graphics.c into here??

// Alloc Macros
//{{{

// Wrappers for malloc and realloc: checks if alloc failed, prints an error and exits if so
// Note how malloc and realloc take type and count like calloc
#define EMALLOC(VAR,TYPE,COUNT) \
{\
	(VAR) = (TYPE*)malloc(sizeof(TYPE)*(COUNT));\
	if((VAR)==0)\
	{\
		lnxLogError("Fatal Error: memory allocation failed in file \"%s\" on line %d.  The number of bytes requested was %lu.\n",__FILE__,__LINE__,(sizeof(TYPE)*(COUNT)));\
		exit(1);\
	}\
}
#define ECALLOC(VAR,TYPE,COUNT) \
{\
	(VAR) = (TYPE*)calloc(sizeof(TYPE),(COUNT));\
	if((VAR)==0)\
	{\
		lnxLogError("Fatal Error: memory allocation failed in file \"%s\" on line %d.  The number of bytes requested was %lu.\n",__FILE__,__LINE__,(sizeof(TYPE)*(COUNT)));\
		exit(1);\
	}\
}
#define EREALLOC(VAR,TYPE,COUNT) \
{\
	(VAR) = (TYPE*)realloc((VAR),sizeof(TYPE)*(COUNT));\
	if((VAR)==0)\
	{\
		lnxLogError("Fatal Error: memory allocation failed in file \"%s\" on line %d.  The number of bytes requested was %lu.\n",__FILE__,__LINE__,(sizeof(TYPE)*(COUNT)));\
		exit(1);\
	}\
}

//}}}


// TODO make this based compiletime option
#define LNX_DEBUG

#ifdef LNX_DEBUG
// TODO maybe replace lnxLogError with lnxDebugError and lnxDebugWarning: make it so only do anything if debug enabled.
// Maybe have a two kinds of warnings: those that happen occasionally (like on object creation/deletion) and those that happen often (like on a draw call or program/model sets)

	// If non-zero, this will make it so the 'printError' function calls glGetError
	//  and print a warning if the result is positive (if zero, the printError and
	//  clearError functions are just null macros).
	#define LNX_D_WARN_GLERROR (1)
	
	// When a lnxSetUniform_v function is called, it will check if the passed count is
	//  higher/lower the array size of that uniform, and print warnings if so.
	#define LNX_D_WARN_UNILOWCOUNT (0)
	

#endif



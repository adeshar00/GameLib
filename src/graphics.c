
// Includes
//{{{

#include <GLES2/gl2.h>
#include <stdlib.h>
#include <string.h>

#include "matrix.h"
#include "utils.h"

//}}}

// Definitions
//{{{

 // Max number of chars allowed for attribute names
#define MAX_ATTRIB_NAME_SIZE (32)

 // Max number of chars allowed for uniform names
#define MAX_UNIFORM_NAME_SIZE (32+1)

 // Minimum number of textures specified by API
#define MAX_TEXTURES (8)

 // Null texture (used to signal that a texture unit value is not a texture)
#define NO_TEXTURE (GL_TEXTURE0-1)

 // Uniform function enumerations (used for glGraphicsSetUniform functions)
enum
{
	// 0 means the uniform can't be set
	UF_1I = 1,
	UF_2I,
	UF_3I,
	UF_4I,
	UF_1F,
	UF_2F,
	UF_3F,
	UF_4F,
	UF_2M,
	UF_3M,
	UF_4M,
	UF_SAMPLER
};

//}}}

// Structs & Typedefs
//{{{

struct uniform
{
	// Program uniform metadata
	int function;     // Enum of the function that should be called for the uniform
	GLsizei count;    // "count" argument to be passed to glUniform function
	GLint location;   // "location" argument to be passed to glUniform function
	int texture;      // If uniform is a texture, this is the unit it's bound to (otherwise
	                  //  this is NO_TEXTURE)
	char name[MAX_UNIFORM_NAME_SIZE+1];
};

struct lnxProgramRaw
{
	GLuint program;               // Reference to program object
	int uniformCount;               // Number of uniforms required
	struct uniform* uniforms;       // Array of uniform data for program
	int attributeCount;               // Number of attributes required
	GLint* attributeProgramLocations; // Program's attribute index (for glVertexAttribPointer)
	int* attributeArrayIndices;       // Index of attribute in "lvarAttributeArray"
};
typedef struct lnxProgramRaw* lnxProgram;

struct lnxModelRaw
{
	int attributeCount;       // Number of attributes had by model
	GLuint ibo;                 // Index buffer object for model
	GLsizei iboSize;            // Size of index buffer
	GLuint* attributeBufferArray; // Buffer object "names" for each model attribute
	int* attributeArrayIndices;   // Index of attributes in "lvarAttributeArray" for 
	                              //  attribute metadata (see the attribute struct
	                              //  below for more info)
};
typedef struct lnxModelRaw* lnxModel;

struct attribute
{
	// Metadata for attributes for programs and models
	char name[MAX_ATTRIB_NAME_SIZE+1]; // Attribute name
	GLint size;                        // Number of components per attribute (between 1 and 4)
	GLenum type;                       // Attribute type. Note: this should always be GL_FLOAT;
	                                   //  I put this in place before realizing attributes
	                                   //  can only be floats in OGLES2, but figure I'd
	                                   //  leave it in case I decide to use this code with
	                                   //  a different version of OGL..
};

struct lnxTextureRaw
{
	GLuint textureName;  // The "texture name" of the texture (passed to glBindTexture)
	GLsizei width;       // Texture width
	GLsizei height;      // Texture height
};
typedef struct lnxTextureRaw* lnxTexture;

//}}}

// Variables
//{{{

 // Attribute array stores metadata about each attribute, and serves as a way to coordinate
 //  attributes between programs and models (see documentation in graphics.h for more information)
static struct attribute* lvarAttributeArray = 0;
static int lvarAttributeArrayLength = 0;

 // Pointers to the program and model with data currently bound to OpenGL
static lnxProgram lvarCurrentProgram = 0;
static lnxModel lvarCurrentModel = 0;

 // Pointers to the current paired program and model ("pairing" means that the
 //  attributes required by the program have been enabled and are bound to the
 //  attribute buffers of the model)
static lnxProgram lvarLastPairedProgram = 0;
static lnxModel lvarLastPairedModel = 0;

//}}}


// Internal Functions
//{{{

 // Print & Clear GL Error
//{{{
#if(LNX_D_WARN_GLERROR)

static void clearGlError()
{
	if(glGetError()!=GL_NO_ERROR)
	{
		// TODO put some predefine here
		lnxLogError("Uncaught GL error.\n");
	}
}
static void printGlError(char error[])
{
	if(glGetError()!=GL_NO_ERROR)
	{
		lnxLogError(error);
	}
}

#else

#define clearGlError() {}
#define printGlError(error) {}

#endif
//}}}

 // Uniform Type to Enum
//{{{
int typeToFunction(GLenum type)
{
	// Takes an enum representing a GLSL uniform variable type, and returns an enumeration
	//  for the glUniform function which sets that uniform
	// Returns 0 if the type has no match

	switch(type)
	{
		case GL_INT:
		case GL_BOOL:
			return UF_1I;
		case  GL_INT_VEC2:
		case GL_BOOL_VEC2:
			return UF_2I;
		case  GL_INT_VEC3:
		case GL_BOOL_VEC3:
			return UF_3I;
		case  GL_INT_VEC4:
		case GL_BOOL_VEC4:
			return UF_4I;

		case GL_FLOAT:
			return UF_1F;
		case GL_FLOAT_VEC2:
			return UF_2F;
		case GL_FLOAT_VEC3:
			return UF_3F;
		case GL_FLOAT_VEC4:
			return UF_4F;

		case GL_FLOAT_MAT2:
			return UF_2M;
		case GL_FLOAT_MAT3:
			return UF_3M;
		case GL_FLOAT_MAT4:
			return UF_4M;

		case GL_SAMPLER_2D:
		case GL_SAMPLER_CUBE:
			return UF_SAMPLER;

	}

	return 0;
}
//}}}

 // Fetch Attribute Index
//{{{
static int fetchAttributeIndex(const char* name, GLint size, GLenum type)
{
	// Returns index (0+) if found/created, returns -1 if error


	// Check if attribute already exists: return index if match
	{
		int a;
		for(a=0;a<lvarAttributeArrayLength;a++)
		{
			if(!strncmp(name, lvarAttributeArray[a].name, MAX_ATTRIB_NAME_SIZE))
			{
				if(size!=lvarAttributeArray[a].size)
				{
					lnxLogError("Attribute \"%s\" size inconsistency detected.\n", name);
					return -1;
				}

				if(type!=lvarAttributeArray[a].type)
				{
					lnxLogError("Attribute \"%s\" type inconsistency detected.\n", name);
					return -1;
				}

				return  a;
			}
		}
	}

	// Attribute not in array: create an entry for it
	{
		int index = lvarAttributeArrayLength;
		lvarAttributeArrayLength++;
		EREALLOC(lvarAttributeArray, struct attribute, lvarAttributeArrayLength)
		strcpy(lvarAttributeArray[index].name, name);
		lvarAttributeArray[index].type = type;
		lvarAttributeArray[index].size = size;
		return index;
	}

}
//}}}

 // Get GL_TYPE Size
//{{{
static GLint getGlTypeSize(GLenum type)
{
	switch(type)
	{
		case GL_BYTE:
			return sizeof(GLbyte);

		case GL_UNSIGNED_BYTE:
			return sizeof(GLubyte);

		case GL_SHORT:
			return sizeof(GLshort);

		case GL_UNSIGNED_SHORT:
			return sizeof(GLushort);

		case GL_FIXED:
			return sizeof(GLfixed);

		case GL_FLOAT:
			return sizeof(GLfloat);

		default:
			lnxLogError("Invalid type %d (for an attribute) passed to getGLTypeSize.\n",type);
			return 0;
	}
}
//}}}

 // Create Shader
//{{{
static GLuint createShader(const GLchar* source[], GLenum shaderType)
{
	GLuint shader;
	GLint status;

	// Compile shader source
	shader = glCreateShader(shaderType);
	glShaderSource(shader, 1, source, 0);
	glCompileShader(shader);

	// Check compilation
	status = GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if(status==GL_FALSE)
	{
		switch(shaderType)
		{
			case GL_VERTEX_SHADER:
				lnxLogError("Vertex Shader compilation failure: ");
				break;
			case GL_FRAGMENT_SHADER:
				lnxLogError("Fragment Shader compilation failure: ");
				break;
			default:
				lnxLogError("Shader compilation failure: ");
				break;
		}

		if(!glIsShader(shader))
		{
			lnxLogError("Shader creation failed.\n");
			return 0;
		}

		{
			int logLength;
			glGetShaderiv(shader,GL_INFO_LOG_LENGTH,&logLength);
			GLchar infoLog[logLength];
			glGetShaderInfoLog(shader, logLength, 0, infoLog);
			lnxLogError(infoLog);
			glDeleteShader(shader);
		}

		return 0;
	}

	return shader;
}
//}}}

 // Unpair
//{{{
static void unpair()
{
	clearGlError();
	
	// Disable attribute arrays
	if(lvarLastPairedProgram!=0)
	{
		int a;
		GLint* pls = lvarLastPairedProgram->attributeProgramLocations;
		for(a=0;a<lvarLastPairedProgram->attributeCount;a++)
		{
			glDisableVertexAttribArray(pls[a]);
		}
	}

	// Unbind index buffer object
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	printGlError("Unpairing error\n");

	// Reset last paired variables
	lvarLastPairedProgram = 0;
	lvarLastPairedModel = 0;

}
//}}}

//}}}

// External Functions
//{{{

 // Clear
//{{{
void lnxGraphicsClear(float red, float green, float blue, float alpha)
{
	// TODO split this up?
	glClearColor( red, green, blue, alpha);
	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);
}
//}}}

 // Enable/Disable Depth Test
//{{{
// TODO document somewhere about gl state: how scissor test is enabled at window creation and is only disabled by framebuffer; how depth is set here in graphics; how cullface enabled at window creation and not changed (at least not atm), etc
void lnxGraphicsEnableDepthTest()
{
	glEnable(GL_DEPTH_TEST);
}
void lnxGraphicsDisableDepthTest()
{
	glDisable(GL_DEPTH_TEST);
}
//}}}

 // Set Scissored Viewport
//{{{
void lnxGraphicsSetSViewport(int x, int y, int width, int height)
{
	glViewport(x,y,width,height);
	glScissor(x,y,width,height);
}
//}}}


 // Destroy Model
//{{{
void lnxGraphicsDestroyModel(lnxModel model)
{
	if(model==0)
	{
		return;
	}

	if(lvarCurrentModel==model)
	{
		unpair();
	}

	// Free index buffer array 
	glDeleteBuffers(1,&(model->ibo));

	// Free attribute buffer arrays
	{
		int a;
		int ac = model->attributeCount;
		for(a=0;a<ac;a++)
		{
			glDeleteBuffers(1, &(model->attributeBufferArray[a]));
		}
	}

	// Free model data
	free(model->attributeBufferArray);
	free(model->attributeArrayIndices);
	free(model);
}
//}}}

 // Destroy Program
//{{{
void lnxGraphicsDestroyProgram(lnxProgram program)
{
	if(lvarCurrentProgram==program)
	{
		unpair();
	}
	glDeleteProgram(program->program);
	free(program->attributeArrayIndices);
	free(program->attributeProgramLocations);
	free(program->uniforms);
	free(program);
}
//}}}

 // Destroy Texture
//{{{
void lnxGraphicsDestroyTexture(lnxTexture texture)
{
	glDeleteTextures(1,&(texture->textureName));
	free(texture);
}
//}}}


 // Create Model
//{{{
lnxModel lnxGraphicsCreateModel(
	int indisize,
	unsigned int indices[],
	int vertCount,
	int attributeCount,
	const char* attributeNames[],
	int attributeSizes[],
	float* attributePointers[])
{

	// Allocate memory for model
	lnxModel model;
	EMALLOC(model, struct lnxModelRaw, 1)
	model->iboSize = indisize;
	model->attributeCount = 0;
	EMALLOC(model->attributeBufferArray, GLuint, attributeCount)
	EMALLOC(model->attributeArrayIndices, int, attributeCount)

	// Bind index buffer object
	glGenBuffers(1, &(model->ibo));
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indisize*sizeof(GLuint), indices, GL_STATIC_DRAW);

	// Bind attributes
	{
		// TODO check max attributes?

		int a;
		for(a=0;a<attributeCount;a++)
		{
			// Check string length
			if(strlen(attributeNames[a])>MAX_ATTRIB_NAME_SIZE)
			{
				lnxLogError("Model Creation warning: the length of attribute name \"%s\" exceeds the maximum tracked by the library (%d).  If two or more attributes with different names share the first %d characters of their names, it will result in errors.\n", attributeNames[a],MAX_ATTRIB_NAME_SIZE,MAX_ATTRIB_NAME_SIZE);
			}

			// Fetch attribArray index of attribute
			int ai = fetchAttributeIndex(attributeNames[a], attributeSizes[a], GL_FLOAT);
			if(ai==-1)
			{
				lnxLogError("Model Creation failed due to attribute discrepancy.\n");
				void lnxGraphicsDestroyModel(lnxModel sm);
				lnxGraphicsDestroyModel(model);
				return 0;
			}
			model->attributeArrayIndices[a] = ai;

			// Generate & bind buffer for attribute data
			clearGlError();
			glGenBuffers(1, &(model->attributeBufferArray[a]));
			glBindBuffer(GL_ARRAY_BUFFER, model->attributeBufferArray[a]);
			glBufferData(
					GL_ARRAY_BUFFER,
					vertCount*attributeSizes[a]*getGlTypeSize(GL_FLOAT),
					attributePointers[a],
					GL_STATIC_DRAW);
			printGlError("Model Creation Error when binding attribute");

			model->attributeCount++;
		}
	}

	return model;
}
//}}}

 // Create Program
//{{{
lnxProgram lnxGraphicsCreateProgram(
		const char* vertexShaderSource[],
		const char* fragmentShaderSource[],
		int uniformCount,
		const char* uniformNameMap[])
{

	GLuint program;

	// Create shaders and link to program
	{
		GLuint vertexShader;
		GLuint fragmentShader;
		GLint status;

		// Create shaders
		vertexShader = createShader(vertexShaderSource, GL_VERTEX_SHADER);
		if(vertexShader==0)
		{
			lnxLogError("Program Creation Failue: Vertex shader creation failed.\n");
			return 0;
		}
		fragmentShader = createShader(fragmentShaderSource, GL_FRAGMENT_SHADER);
		if(fragmentShader==0)
		{
			lnxLogError("Program Creation Failue: Fragment shader creation failed.\n");
			glDeleteShader(vertexShader);
			return 0;
		}

		// Attach shaders and link program
		program = glCreateProgram();
		glAttachShader(program, vertexShader);
		glAttachShader(program, fragmentShader);
		glLinkProgram(program);
		status = GL_FALSE;
		glGetProgramiv(program, GL_LINK_STATUS, &status);
		if(status==GL_FALSE)
		{
			lnxLogError("Program Link failure: ");

			if(!glIsProgram(program))
			{
				lnxLogError("Program creation failed.\n");
			}
			else
			{
				int logLength;
				glGetProgramiv(program,GL_INFO_LOG_LENGTH,&logLength);
				GLchar infoLog[logLength];
				glGetProgramInfoLog(program, logLength, 0, infoLog);
				lnxLogError(infoLog);
			}

			glDeleteShader(vertexShader);
			glDeleteShader(fragmentShader);
			glDeleteProgram(program);

			return 0;
		}

		// Flag shaders for deletion when program is deleted
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);
	}

	// TODO generate warnings if attrib or uniform counts exceed minimum required
	//  as a maximum by the API?
	// Min max-attribs appears to be 8: (see page 71)
	// https://www.khronos.org/registry/gles/specs/2.0/es_cm_spec_2.0.24.pdf#section.6.2
	// Check for attribs, vertex uniforms, frag uniforms, varying

	// Create lnxProgram
	lnxProgram progdata;
	ECALLOC(progdata, struct lnxProgramRaw, 1);

	progdata->program = program;

	// Set Attributes
	{
		// Get name buffer size
		GLsizei bufSize;
		glGetProgramiv(program, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &bufSize);

		// Allocate data for attributes
		GLint count;
		glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &count);
		progdata->attributeCount = count;
		EMALLOC(progdata->attributeArrayIndices,int,count)
		EMALLOC(progdata->attributeProgramLocations,GLint,count)

		// Process each attribute
		int a;
		for(a=0;a<count;a++)
		{
			GLenum type;
			GLchar name[bufSize];
			GLsizei length;

			// Get attribute info
			glGetActiveAttrib(program, (GLuint)a, bufSize, &length, 0, &type, name);

			// Issue warning if length exceeds MAX_ATTRIB_NAME_SIZE
			if(length>MAX_ATTRIB_NAME_SIZE)
			{
				lnxLogError("Program Creation Warning: the length of the attribute name \"%s\" exceeds the maximum size stored by the library (%d).  If another attribute exists with the same %d letters at the beginning of it's name, there may be undefined behavior.\n",name,MAX_ATTRIB_NAME_SIZE,MAX_ATTRIB_NAME_SIZE);
				name[MAX_ATTRIB_NAME_SIZE] = '\0';
			}

			// Set size
			GLint size;
			switch(type)
			{
				case GL_FLOAT:
					size = 1;
					break;
				case GL_FLOAT_VEC2:
					size = 2;
					break;
				case GL_FLOAT_VEC3:
					size = 3;
					break;
				case GL_FLOAT_VEC4:
					size = 4;
					break;
				default:
					lnxLogError("Program creation error: matrix attribute types are currently not supported by this library (sorry! If it's an issue, contact me.  In the meantime, if you rewrite the sahder to take 2-4 vector attributes in place of the matrix, it should work).\n");
					lnxGraphicsDestroyProgram(progdata);
					return 0;
				
			}

			progdata->attributeArrayIndices[a] = fetchAttributeIndex(name, size, GL_FLOAT);
			if(progdata->attributeArrayIndices[a]==-1)
			{
				lnxLogError("Program creation failure: attribute \"%s\" is inconsistent with existing programs or models.\n",
						name);
				lnxGraphicsDestroyProgram(progdata);
				return 0;
			}
			progdata->attributeProgramLocations[a] = glGetAttribLocation(program, name);
			if(progdata->attributeProgramLocations[a]==-1)
			{
				lnxLogError("Program creation failure: attribute \"%s\" is not active in program.\n",
						name);
				lnxGraphicsDestroyProgram(progdata);
				return 0;
			}
		}
	}

	// Set uniform data
	{
		// Initialize uniform data
		{
			EMALLOC(progdata->uniforms, struct uniform, uniformCount)
			progdata->uniformCount = uniformCount;
			int u;
			for(u=0;u<uniformCount;u++)
			{
				progdata->uniforms[u].function = 0;
				progdata->uniforms[u].count = 0;
				progdata->uniforms[u].location = -1;
				progdata->uniforms[u].texture = NO_TEXTURE;
				strncpy(progdata->uniforms[u].name,uniformNameMap[u],MAX_UNIFORM_NAME_SIZE);
				progdata->uniforms[u].name[MAX_UNIFORM_NAME_SIZE] = '\0';
			}
		}

		// Get active uniform count from program, check if it matches argument uniformCount
		GLint activeUniCount;
		glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &activeUniCount);
		if(activeUniCount>uniformCount)
		{
			lnxLogError("Program Creation Warning: uniformCount is lower than then number of uniforms active in program.\n");
		}
		if(activeUniCount<uniformCount)
		{
			lnxLogError("Program Creation Warning: uniformCount is higher than then number of uniforms active in program.  Some uniforms may have been deemed unnecessary and discarded during shader compilation.\n");
		}

		// Get buffer size to store uniform names
		GLint bufSize = 0;
		glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &bufSize);

		// Iterate over active uniforms and match their names to uniformNameMap
		int textureCount = 0;
		int au;
		for(au=0;au<activeUniCount;au++)
		{
			// Initiliaze uniform info
			GLsizei length = 0;
			GLint size = 0;
			GLenum type = 0;
			GLchar name[bufSize];

			// Get uniform info
			glGetActiveUniform(program,au,bufSize,&length,&size,&type,name);

			// Check that name length doesn't exceed MAX_UNIFORM_NAME_SIZE
			if(length>MAX_UNIFORM_NAME_SIZE)
			{
				if(length>MAX_UNIFORM_NAME_SIZE+3 || name[length-3]!='[')
				{
					lnxLogError("Program Creation Warning: the length of the uniform name \"%s\" exceeds the maximum size stored by the library (%d).  The mapping between this uniform name and the names passed via uniformNameMap will only consider the first %d characters.  If another uniform exists with the same %d letters at the beginning of it's name, there will be mapping errors.\n",name,MAX_UNIFORM_NAME_SIZE,MAX_UNIFORM_NAME_SIZE,MAX_UNIFORM_NAME_SIZE);
				}
			}

			// Find the uniform map name which matches this uniform
			int u;
			for(u=0;u<uniformCount;u++)
			{
				if(progdata->uniforms[u].count!=0)
					continue;

				int match = 1;
				int c;
				for(c=0;c<MAX_UNIFORM_NAME_SIZE;c++)
				{
					char uc = uniformNameMap[u][c];
					char auc = name[c];

					if(uc=='\0')
					{
						// Check if active uniform name has array suffix
						int isArray = (auc=='[' && name[c+1]=='0' && name[c+2]==']');

						// Check if uniform map name is of a struct
						int isStruct = (auc=='.' || (isArray && name[c+3]=='.'));

						// Print error if uniform map name is of a struct
						if(isStruct)
						{
							lnxLogError("\nProgram Creation Error: the name given in the uniformNameMap (\"%s\") corresponds to a struct, and not an individual field within the struct.  Though the program creation hasn't halted, the mapping has failed.  Here is an excerpt from the OpenGL ES 2.0 reference page for glGetActiveUniform, explaining how struct uniform names are handled:\n\n\
\"Uniform variables that are declared as structures or arrays of structures will not be returned directly by this function. Instead, each of these uniform variables will be reduced to its fundamental components containing the \".\" and \"[]\" operators such that each of the names is valid as an argument to glGetUniformLocation. Each of these reduced uniform variables is counted as one active uniform variable and is assigned an index. A valid name cannot be a structure, an array of structures, or a subcomponent of a vector or matrix.\"\n\n\
For more information, see the documentation of the glGetActiveUniform and glGetUniformLocation functions ( https://www.khronos.org/opengles/sdk/docs/man/ ) \n\n",uniformNameMap[u]);
							match = 0;
							break; // Exit string comparison
						}

						// Check if active uniform name matches map name
						if(auc=='\0' || (isArray && !isStruct))
							break; // Exit string comparison
					}

					// Break if names differ
					if(uc!=auc)
					{
						match = 0;
						break; // Exit string comparison
					}
				}
				if(match)
				{
					// Set uniform texture field
					if(type==GL_SAMPLER_2D || type==GL_SAMPLER_CUBE)
					{
						if(type==GL_SAMPLER_CUBE)
						{
							lnxLogError("Program Creation Warning: samplerCube uniforms aren't supported by the library at the moment (Sorry about this!  If it's an issue, contact me and let me know)\n");
							break; // Exit match search for active uniform
						}

						if(textureCount+size > MAX_TEXTURES)
						{
							lnxLogError("Program Creation Warning: library only supports up to %d textures per program; program creation hasn't failed, but sampler %s was not bound (Sorry about this!  If it's an issue, contact me and let me know)\n",MAX_TEXTURES,name);
							break; // Exit match search for active uniform
						}

						progdata->uniforms[u].texture = GL_TEXTURE0 + textureCount;
						textureCount+= size;
					}
					else
					{
						progdata->uniforms[u].texture = NO_TEXTURE;
					}

					// Set uniform data
					progdata->uniforms[u].function = typeToFunction(type);
					progdata->uniforms[u].count = size;
					progdata->uniforms[u].location = glGetUniformLocation(program, name);

					break; // Exit match search for active uniform
				}
			}
		}
	}

	return progdata;

}
//}}}

 // Create Texture
//{{{
lnxTexture lnxGraphicsCreateTexture(int width, int height, unsigned char* pixels, int linear)
{
	// Clear errors
	clearGlError(); // Give warning for uncaught errors
	glGetError(); // In case not debugging, since errors effect logic

	// Get currently bound texture name (for rebinding after creation)
	GLint boundTexture;
	const GLenum textureUnit = GL_TEXTURE0;
	glActiveTexture(textureUnit);
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &boundTexture);

	// Create new lnxTexture
	lnxTexture texture;
	EMALLOC(texture, struct lnxTextureRaw, 1);
	glGenTextures(1,&(texture->textureName));
	texture->width = width;
	texture->height = height;

	// Send pixels to GL and bind to lnxTexture->texture
	glBindTexture(GL_TEXTURE_2D, texture->textureName);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

	// Check for errors
	if(glGetError()!=GL_NO_ERROR)
	{
		GLint max;
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max);
		if(width>max || height>max)
		{
			lnxLogError("Texture Creation Error: width or height exceeds maximum texture size for this device (%d).\n",max);
		}
		else
		{
			lnxLogError("Texture Creation GL Error.\n");
		}
		lnxGraphicsDestroyTexture(texture);
		return 0;
	}

	// Set parameters, and generate mipmap if applicable
	if(linear)
	{
		glGenerateMipmap(GL_TEXTURE_2D);
		if(glGetError()!=GL_NO_ERROR)
		{
			printGlError("Texture Creation GL Error: mipmapped texture creation failed; may be due to non-power of two dimensions.\n");
			lnxGraphicsDestroyTexture(texture);
			return 0;
		}
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	// Set textureUnit back to it's previous binding
	glBindTexture(GL_TEXTURE_2D, boundTexture);

#ifdef LNX_DEBUG
	// Give warning if either dimension exceeds 2048
	if(width>2048 || height>2048)
	{
		lnxLogError("Texture Creation Warning: width or height exceeds 2048; this may cause errors on some devices.\n");
	}
#endif

	return texture;

}
//}}}


 // Set Framebuffer Texture
//{{{
int lnxGraphicsSetFramebufferTexture(lnxTexture texture, int useDepth)
{
	clearGlError();

	// If texture non-zero, set texture to be render target
	if(texture && glIsTexture(texture->textureName)==GL_TRUE)
	{
		GLuint framebuffer;
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&framebuffer);

		// Free current framebuffer in non-zero
		if(framebuffer!=0)
		{
			lnxGraphicsSetFramebufferTexture(0,0);
		}

		// Create and bind framebuffer
		glGenFramebuffers(1,&framebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

		// Attach texture to framebuffer
		glFramebufferTexture2D(
				GL_FRAMEBUFFER,
				GL_COLOR_ATTACHMENT0,
				GL_TEXTURE_2D,
				texture->textureName,
				0);

		// Depth buffer
		if(useDepth)
		{
			GLuint renderbuffer;
			glGenRenderbuffers(1, &renderbuffer);
			glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
			glRenderbufferStorage(
					GL_RENDERBUFFER,
					GL_DEPTH_COMPONENT16,
					texture->width,
					texture->height);
			glFramebufferRenderbuffer(
					GL_FRAMEBUFFER,
					GL_DEPTH_ATTACHMENT,
					GL_RENDERBUFFER,
					renderbuffer);
		}
		/* TODO Stencil buffer; if not using, check that stencil is disabled (give warning if not?)
		GLuint rbos;
		glGenRenderbuffers(1, &rbos);
		glBindRenderbuffer(GL_RENDERBUFFER, rbos);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, strlen*6,9);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbos);
		*/

		// Check status
		GLenum status  = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if(status==GL_FRAMEBUFFER_COMPLETE)
		{
			glDisable(GL_SCISSOR_TEST);
			printGlError("Set framebuffer texture gl error(status OK).\n");
			return 1;
		}
		else
		{
			lnxLogError("Framebuffer texture bind error.\n");
			lnxGraphicsSetFramebufferTexture(0,0);
			printGlError("Set framebuffer texture gl error(status fail).\n");
			return 0;
		}
	}
	else // Delete framebuffer and set to default (window framebuffer)
	{
		glEnable(GL_SCISSOR_TEST);
		GLuint framebuffer;
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&framebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDeleteFramebuffers(1, &framebuffer);
		printGlError("Set framebuffer null gl error.\n");
		return 1;
	}

}
//}}}

 // Set Program
//{{{
void lnxGraphicsSetProgram(lnxProgram prog)
{

	if(prog!=lvarLastPairedProgram)
	{
		unpair();
	}

#ifdef LNX_DEBUG // TODO make a specific definition for this
	{
		int t;
		for(t=0;t<MAX_TEXTURES;t++)
		{
			glActiveTexture(GL_TEXTURE0+t);
			glBindTexture(GL_TEXTURE_2D, 0);// make a debug texture??
		}
	}
#endif

	if(prog)
	{
		lvarCurrentProgram = prog;
		glUseProgram(prog->program);
	}
	else
	{
		lvarCurrentProgram = 0;
		glUseProgram(0);
	}

}
//}}}

 // Set Uniform Functions
//{{{

static inline int uniformCheck(int uniformNumber, int count, int type)
{
	if(lvarCurrentProgram==0)
	{
		lnxLogError("Set Uniform failure: no program is currently active.\n");
		return 1;
	}

	if(uniformNumber>=lvarCurrentProgram->uniformCount || uniformNumber<0)
	{
		lnxLogError("Set Uniform failure: invalid uniform number (%d).\n",uniformNumber);
		return 1;
	}

	struct uniform* up = &(lvarCurrentProgram->uniforms[uniformNumber]);

	if(up->function==UF_SAMPLER)
	{
		if(up->texture==NO_TEXTURE)
		{
			lnxLogError("Set Uniform Texture failure: uniform %d:\"%s\" is not a sampler.\n",uniformNumber, up->name);
			return 1;
		}
	}
	else
	{
		if(up->texture!=NO_TEXTURE)
		{
			lnxLogError("Set Uniform failure: uniform %d:\"%s\" is a sampler; use lnxGraphicsSetUniformTexture to set it.\n",uniformNumber, up->name);
			return 1;
		}
	}

	if(up->function!=type)
	{
		if(up->function==0)
			lnxLogError("Set Uniform Failure: uniform %d:\"%s\" isn't present in the current program (it was possibly deemed unnecessary and discarded during shader compilation.\n",uniformNumber,up->name);
		else
			lnxLogError("Set Uniform Failure: set uniform function doesn't match uniform type for uniform %d:\"%s\".\n",uniformNumber,up->name);

		return 1;
	}

	GLsizei ucount = lvarCurrentProgram->uniforms[uniformNumber].count;
	if(count>ucount)
	{
		lnxLogError("Set Uniform Warning: count for uniform %d:\"%s\" is more than expected (expected %d, recieved %d).\n",uniformNumber, up->name, ucount, count);
	}
#if(LNX_D_WARN_UNILOWCOUNT) 
	if(count<ucount)
	{
		lnxLogError("Set Uniform Warning: count for uniform %d:\"%s\" is less than expected (expected %d, recieved %d).\n",uniformNumber, up->name, ucount, count);
	}
#endif

	return 0;
}

// TODO test ALL of these: don't add until graphics.h until tested (make sure they're all added!)

void lnxGraphicsSetUniform1i(int uniformNumber, int value)
{
	clearGlError();

	if(uniformCheck(uniformNumber, 1, UF_1I)) return;

	GLint location = lvarCurrentProgram->uniforms[uniformNumber].location;
	glUniform1i(location, value);

	printGlError("Uniform Error\n");
}

void lnxGraphicsSetUniform2i(int uniformNumber, int val0, int val1)
{
	clearGlError();

	if(uniformCheck(uniformNumber, 2, UF_2I)) return;

	GLint location = lvarCurrentProgram->uniforms[uniformNumber].location;
	glUniform2i(location, val0, val1);

	printGlError("Uniform Error\n");
}

void lnxGraphicsSetUniform3i(int uniformNumber, int val0, int val1, int val2)
{
	clearGlError();

	if(uniformCheck(uniformNumber, 3, UF_3I)) return;

	GLint location = lvarCurrentProgram->uniforms[uniformNumber].location;
	glUniform3i(location, val0, val1, val2);

	printGlError("Uniform Error\n");
}

void lnxGraphicsSetUniform4i(int uniformNumber, int val0, int val1, int val2, int val3)
{
	clearGlError();

	if(uniformCheck(uniformNumber, 4, UF_4I)) return;

	GLint location = lvarCurrentProgram->uniforms[uniformNumber].location;
	glUniform4i(location, val0, val1, val2, val3);

	printGlError("Uniform Error\n");
}

void lnxGraphicsSetUniform1iv(int uniformNumber, int count, int* value)
{
	clearGlError();

	if(uniformCheck(uniformNumber, count, UF_1I)) return;

	GLint location = lvarCurrentProgram->uniforms[uniformNumber].location;
	glUniform1iv(location, count, value);

	printGlError("Uniform Error\n");
}

void lnxGraphicsSetUniform2iv(int uniformNumber, int count, int* value)
{
	clearGlError();

	if(uniformCheck(uniformNumber, count, UF_2I)) return;

	GLint location = lvarCurrentProgram->uniforms[uniformNumber].location;
	glUniform2iv(location, count, value);

	printGlError("Uniform Error\n");
}

void lnxGraphicsSetUniform3iv(int uniformNumber, int count, int* value)
{
	clearGlError();

	if(uniformCheck(uniformNumber, count, UF_3I)) return;

	GLint location = lvarCurrentProgram->uniforms[uniformNumber].location;
	glUniform3iv(location, count, value);

	printGlError("Uniform Error\n");
}

void lnxGraphicsSetUniform4iv(int uniformNumber, int count, int* value)
{
	clearGlError();

	if(uniformCheck(uniformNumber, count, UF_4I)) return;

	GLint location = lvarCurrentProgram->uniforms[uniformNumber].location;
	glUniform4iv(location, count, value);

	printGlError("Uniform Error\n");
}


void lnxGraphicsSetUniform1f(int uniformNumber, float value)
{
	clearGlError();

	if(uniformCheck(uniformNumber, 1, UF_1F)) return;

	GLint location = lvarCurrentProgram->uniforms[uniformNumber].location;
	glUniform1f(location, value);

	printGlError("Uniform Error\n");
}

void lnxGraphicsSetUniform2f(int uniformNumber, float val0, float val1)
{
	clearGlError();

	if(uniformCheck(uniformNumber, 2, UF_2F)) return;

	GLint location = lvarCurrentProgram->uniforms[uniformNumber].location;
	glUniform2f(location, val0, val1);

	printGlError("Uniform Error\n");
}

void lnxGraphicsSetUniform3f(int uniformNumber, float val0, float val1, float val2)
{
	clearGlError();

	if(uniformCheck(uniformNumber, 3, UF_3F)) return;

	GLint location = lvarCurrentProgram->uniforms[uniformNumber].location;
	glUniform3f(location, val0, val1, val2);

	printGlError("Uniform Error\n");
}

void lnxGraphicsSetUniform4f(int uniformNumber, float val0, float val1, float val2, float val3)
{
	clearGlError();

	if(uniformCheck(uniformNumber, 4, UF_4F)) return;

	GLint location = lvarCurrentProgram->uniforms[uniformNumber].location;
	glUniform4f(location, val0, val1, val2, val3);

	printGlError("Uniform Error\n");
}

void lnxGraphicsSetUniform1fv(int uniformNumber, int count, float* value)
{
	clearGlError();

	if(uniformCheck(uniformNumber, count, UF_1F)) return;

	GLint location = lvarCurrentProgram->uniforms[uniformNumber].location;
	glUniform1fv(location, count, value);

	printGlError("Uniform Error\n");
}

void lnxGraphicsSetUniform2fv(int uniformNumber, int count, float* value)
{
	clearGlError();

	if(uniformCheck(uniformNumber, count, UF_2F)) return;

	GLint location = lvarCurrentProgram->uniforms[uniformNumber].location;
	glUniform2fv(location, count, value);

	printGlError("Uniform Error\n");
}

void lnxGraphicsSetUniform3fv(int uniformNumber, int count, float* value)
{
	clearGlError();

	if(uniformCheck(uniformNumber, count, UF_3F)) return;

	GLint location = lvarCurrentProgram->uniforms[uniformNumber].location;
	glUniform3fv(location, count, value);

	printGlError("Uniform Error\n");
}

void lnxGraphicsSetUniform4fv(int uniformNumber, int count, float* value)
{
	clearGlError();

	if(uniformCheck(uniformNumber, count, UF_4F)) return;

	GLint location = lvarCurrentProgram->uniforms[uniformNumber].location;
	glUniform4fv(location, count, value);

	printGlError("Uniform Error\n");
}


void lnxGraphicsSetUniformMatrix2fv(int uniformNumber, int count, float* value)
{
	clearGlError();

	if(uniformCheck(uniformNumber, count, UF_2M)) return;

	GLint location = lvarCurrentProgram->uniforms[uniformNumber].location;
	glUniformMatrix2fv(location, count, GL_FALSE, value);

	printGlError("Uniform Error\n");
}

void lnxGraphicsSetUniformMatrix3fv(int uniformNumber, int count, float* value)
{
	clearGlError();

	if(uniformCheck(uniformNumber, count, UF_3M)) return;

	GLint location = lvarCurrentProgram->uniforms[uniformNumber].location;
	glUniformMatrix3fv(location, count, GL_FALSE, value);

	printGlError("Uniform Error\n");
}

void lnxGraphicsSetUniformMatrix4fv(int uniformNumber, int count, float* value)
{
	clearGlError();

	if(uniformCheck(uniformNumber, count, UF_4M)) return;

	GLint location = lvarCurrentProgram->uniforms[uniformNumber].location;
	glUniformMatrix4fv(location, count, GL_FALSE, value);

	printGlError("Uniform Error\n");
}


void lnxGraphicsSetUniformTexture(int uniformNumber, lnxTexture texture)
{
	clearGlError();

	if(uniformCheck(uniformNumber, 1, UF_SAMPLER)) return;

	if(texture==0)
	{
		lnxLogError("Set Uniform Texture Failure: null texture object passed.\n");
		return;
	}

	GLenum tunit = lvarCurrentProgram->uniforms[uniformNumber].texture;
	GLint location = lvarCurrentProgram->uniforms[uniformNumber].location;

	glActiveTexture(tunit);
	glBindTexture(GL_TEXTURE_2D, texture->textureName);
	glUniform1i(location, tunit-GL_TEXTURE0);

	printGlError("Uniform Error\n");
}

void lnxGraphicsSetUniformTextures(int uniformNumber, int count, lnxTexture texture[count])
{
	// TODO add "textureCount" parameter to programs, and check whether # of textures
	//  pass exceeds it?
	clearGlError();

	if(uniformCheck(uniformNumber, count, UF_SAMPLER)) return;

	GLenum tunitbase = lvarCurrentProgram->uniforms[uniformNumber].texture;
	GLint values[count];
	int t;
	for(t=0;t<count;t++)
	{
		if(texture[t]==0)
		{
			lnxLogError("Set Uniform Textures Failure: null texture object passed.\n");
			return;
		}

		GLenum tunit = tunitbase+t;
		glActiveTexture(tunit);
		glBindTexture(GL_TEXTURE_2D, texture[t]->textureName);
		values[t] = tunit-GL_TEXTURE0;
	}

	GLint location = lvarCurrentProgram->uniforms[uniformNumber].location;
	glUniform1iv(location, count, values);

	printGlError("Uniform Error\n");
}

//}}}

 // Set Model
//{{{
void lnxGraphicsSetModel(lnxModel model)
{
	if(model!=lvarLastPairedModel)
	{
		unpair();
	}

	lvarCurrentModel = model;

	if(model)
	{
		// Bind Index Buffer Object
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->ibo);
	}

}
//}}}


 // Draw
//{{{
void lnxGraphicsDraw()
{
	if(lvarCurrentProgram==0)
	{
		lnxLogError("Draw error: no program is currently bound.\n");
		return;
	}
	if(lvarCurrentModel==0)
	{
		lnxLogError("Draw error: no model is currently bound.\n");
		return;
	}
	if(lvarLastPairedModel!=lvarCurrentModel || lvarLastPairedProgram!=lvarCurrentProgram)
	{

		clearGlError();

		int a;
		for(a=0;a<lvarCurrentProgram->attributeCount;a++)
		{
			int ai = lvarCurrentProgram->attributeArrayIndices[a];
			GLint pl = lvarCurrentProgram->attributeProgramLocations[a];
			GLenum type = lvarAttributeArray[ai].type;
			GLint size = lvarAttributeArray[ai].size;
			GLuint vbo = 0;
			{
				int mac = lvarCurrentModel->attributeCount;
				int ma;
				for(ma=0;ma<mac;ma++)
				{
					if(lvarCurrentModel->attributeArrayIndices[ma] == ai)
					{
						vbo = lvarCurrentModel->attributeBufferArray[ma];
					}
				}
				if(vbo==0)
				{
					lnxLogError("Draw failure: unable to pair current model and program; model missing attribute required by program.\n");
					lvarLastPairedModel = lvarCurrentModel;
					lvarLastPairedProgram = lvarCurrentProgram;
					unpair();
					return;
				}
			}

			glEnableVertexAttribArray(pl);
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glVertexAttribPointer(pl, size, type, GL_FALSE, 0, 0);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}

		lvarLastPairedModel = lvarCurrentModel;
		lvarLastPairedProgram = lvarCurrentProgram;

		printGlError("Pairing error\n");
	}
	// TODO do validity checks on textures?

	clearGlError();

	glDrawElements(GL_TRIANGLES, lvarCurrentModel->iboSize, GL_UNSIGNED_INT, 0);

	printGlError("Draw Error\n");
}
//}}}

//}}}



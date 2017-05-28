
// Documentation
/*{{{

  TODO move this to graphics.h??

OpenGL ES 2.0 reference page:
https://www.khronos.org/opengles/sdk/docs/man/
GLSL reference page
https://www.khronos.org/opengles/sdk/docs/man31/index.php
ES2 reference card (both api and glsl)
https://www.khronos.org/files/opengles20-reference-card.pdf
Full OpenGL ES 2.0 spec
https://www.khronos.org/registry/gles/specs/2.0/es_full_spec_2.0.25.pdf
Full GLSL 100 spec
https://www.khronos.org/files/opengles_shading_language.pdf
...Not sure what "common profile spec" means, but it has data on state and stuff?
https://www.khronos.org/registry/gles/specs/2.0/es_cm_spec_2.0.24.pdf

  TODO MAKE BIG NOTES ABOUT EXCEPTIONS:
   No matrix attributes
   No samplerCube uniforms

Create programs, models, textures.  Set programs and models, set uniforms for current program. draw
Draw with them! Hurray! Set and draw!  It's fun!
Write about how pairing works: when draw is called, it checks to see if current program and model have been drawn together before: if not, it binds the VBOs of the model that the current program requires
Note how uniformNameMap works: during program creation, must pass an array of uniform names which serve as a map: to set a uniform you pass the array of the index with that var name.
Example: if uniformNameMaps = {"matrix","color","position"} for the current program, then to set the "color" uniform you would call SetUniform with an index of '1', and to set "matrix" you would pass the SetUniform function an index of 0, like this:
 lnxGraphicsSetUniform3f(1, r,g,b);
 lnxGraphicsSetUniformMatrix4(0, matrix);
If desiring to use uniform structs, must be named as specified in docs for glGetUniformLocation: 
 https://www.khronos.org/opengles/sdk/docs/man/xhtml/glGetUniformLocation.xml

Textures and Programs:
When a program is set, texture binding is handled via the lnxGraphicsSetTextureUniform function.
This function automatically binds the texture object's texture name to a texture unit, and
 sets the sampler uniform to the bound texture unit.
Only eight textures can be bound at a time (since any OpenGL ES 2.0 compliant graphics card
 is guaranteed to have at least 8 texture units).


lvarAttributeArray has no way to be purged at the moment, so if models or programs are created
and then destroyed, then there's technically a memory leak in lvarAttributeArray (though in reality
nobody will likely have so many programs and models with novel attribute names that this should
ever be a problem)


How it works(roll this into above explanation when writing):

A global array for attributes is maintained, for all of the attributes used by currently bound programs or models created.  Each attribute has a name string, and a type.

Program structures contain a pointer to a gl program object, along with a list of attribute indices(the indices corresponding to attributes in the attribute lists) to signify the attributes required to use the gl program object.

Model structures contain a pointer to a gl buffer with the indices of a 3D model, and an array of "model attribute" structures: each model attribute containing a pointer to a gl buffer representing the attribute, and an index into the attribute array signifying it's name and type.

When a program or model structure is created, attribute/uniform indices are obtained with the "fetchAttributeId" function, which takes a name and a type, and returns an index.  It creates a new entry in the array if the attribute doesn't exist yet, or finds the existing entry (and makes sure that the types match: if there's a type mis-match it returns an error)

}}}*/

// Typedefs
//{{{

typedef struct lnxProgramRaw* lnxProgram;
typedef struct lnxModelRaw* lnxModel;
typedef struct lnxTextureRaw* lnxTexture;

//}}}

// Function Prototypes
//{{{

void lnxGraphicsClear(float red, float green, float blue, float alpha);
void lnxGraphicsEnableDepthTest();
void lnxGraphicsDisableDepthTest();

lnxProgram lnxGraphicsCreateProgram(
		const char* vertexShaderSource[],
		const char* fragmentShaderSource[],
		int uniformCount,
		const char* uniformNameMap[uniformCount]);
void lnxGraphicsDestroyProgram(lnxProgram p);

lnxModel lnxGraphicsCreateModel(
	int indisize,           // Size of "indices" array
	unsigned int indices[], // Indices of triangle elements
	int vertCount,            // Number of vertices in the model
	int attributeCount,           // Number of attributes the model has
	const char* attributeNames[], // The names of the attributes
	int attributeSizes[],         // Size of each attribute per vertex (must be 1, 2, 3, or 4)
	float* attributePointers[]);  // Pointers to the vertex data of each attribute
void lnxGraphicsDestroyModel(lnxModel model);

// If linear non-zero, generates mipmap and uses linear filter; otherwise uses nearest pixel
lnxTexture lnxGraphicsCreateTexture(int width, int height, unsigned char* pixels, int linear);
//TODO Make a note of how webgl textures must not repeat if
// not a power of two, and how mipmapping requires power of two.  Maybe even check?
// Maybe just one arg: mipmap? if true, mipmap+linear, otherwise nearest?
// https://developer.mozilla.org/en-US/docs/Web/API/WebGL_API/Tutorial/Using_textures_in_WebGL
void lnxGraphicsDestroyTexture(lnxTexture texture);

// Sets both scissor and viewport
void lnxGraphicsSetSViewport(int x, int y, int width, int height);

// Binds a texture to the framebuffer, so that all draw calls render to the texture
//  instead of the window.
// Pass '0' as the texture to bind the window buffer and resume normal rendering.
// Argument 'useDepth' determines if a depth buffer is created as well.
// Note: if the bound texture is used in a draw call, it will result in undefined behavior.
// Returns 1 if successful, 0 if there's an error
int lnxGraphicsSetFramebufferTexture(lnxTexture texture, int useDepth);

void lnxGraphicsSetProgram(lnxProgram prog);
void lnxGraphicsSetModel(lnxModel model);
void lnxGraphicsDraw();

void lnxGraphicsSetUniform1i(int uniformNumber, int value);
void lnxGraphicsSetUniform1iv(int uniformNumber, int count, int* value);
void lnxGraphicsSetUniformMatrix4fv(int uniformNumber, int count, float* value);
void lnxGraphicsSetUniformTexture(int uniformNumber, lnxTexture texture);
void lnxGraphicsSetUniformTextures(int uniformNumber, int count, lnxTexture texture[count]);

//}}}



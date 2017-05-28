
// Function Prototypes
//{{{

void lnxMatrixIdentity(float matrix[]);
void lnxMatrixScale(float matrix[], float x, float y, float z);
void lnxMatrixTranslate(float matrix[], float x, float y, float z);
void lnxMatrixRotateX(float mat[], float cos, float sin);
void lnxMatrixRotateY(float matrix[], float cos, float sin);
void lnxMatrixRotateZ(float matrix[], float cos, float sin);
void lnxMatrixMultiply(float result[], float left[], float right[]);

// Make sure near and far are negative!!
// Converts argument matrix into a projection matrix
// Arguments:
//  near:      Z value to be mapped to -1  (MUST BE NEGATIVE!)
//  far:       Z value to be mapped to 1 (MUST BE NEGATIVE!)
//  widthTan:  (width of window/2)/(distance from eye to window)
//  heightTan: (height of window/2)/(distance from eye to window)
void lnxMatrixProjection(float matrix[], float near, float far, float widthTan, float heightTan);

//}}}



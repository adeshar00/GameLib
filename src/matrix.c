
// External Functions
//{{{

 // Identity
//{{{
void lnxMatrixIdentity(float matrix[])
{
	// Converts argument matrix into an identity matrix

	matrix[0] = 1.0f;
	matrix[1] = 0.0f;
	matrix[2] = 0.0f;
	matrix[3] = 0.0f;
	matrix[4] = 0.0f;
	matrix[5] = 1.0f;
	matrix[6] = 0.0f;
	matrix[7] = 0.0f;
	matrix[8] = 0.0f;
	matrix[9] = 0.0f;
	matrix[10]= 1.0f;
	matrix[11]= 0.0f;
	matrix[12]= 0.0f;
	matrix[13]= 0.0f;
	matrix[14]= 0.0f;
	matrix[15]= 1.0f;
}
//}}}

 // Scale
//{{{
void lnxMatrixScale(float matrix[], float x, float y, float z)
{
	// matrix = matrix*ScaleMatrix(x,y,z)

	matrix[0] *= x;
	matrix[1] *= x;
	matrix[2] *= x;
	matrix[3] *= x;
	matrix[4] *= y;
	matrix[5] *= y;
	matrix[6] *= y;
	matrix[7] *= y;
	matrix[8] *= z;
	matrix[9] *= z;
	matrix[10]*= z;
	matrix[11]*= z;
}
//}}}

 // Translate
//{{{
void lnxMatrixTranslate(float matrix[], float x, float y, float z)
{
	// matrix = matrix*TranslateMatrix(x,y,z)

	matrix[12] = x*matrix[0] + y*matrix[4] + z*matrix[ 8] + matrix[12];
	matrix[13] = x*matrix[1] + y*matrix[5] + z*matrix[ 9] + matrix[13];
	matrix[14] = x*matrix[2] + y*matrix[6] + z*matrix[10] + matrix[14];
	matrix[15] = x*matrix[3] + y*matrix[7] + z*matrix[11] + matrix[15];
}
//}}}

 // Rotate X
//{{{
void lnxMatrixRotateX(float matrix[], float cos, float sin)
{
	// matrix = matrix*RotationMatrixX(angle)

	float t1, t2;

	t1 = cos*matrix[4] + sin*matrix[8];
	t2 = cos*matrix[8] - sin*matrix[4];
	matrix[4] = t1;
	matrix[8] = t2;
	t1 = cos*matrix[5] + sin*matrix[9];
	t2 = cos*matrix[9] - sin*matrix[5];
	matrix[5] = t1;
	matrix[9] = t2;
	t1 = cos*matrix[6] + sin*matrix[10];
	t2 = cos*matrix[10] - sin*matrix[6];
	matrix[6] = t1;
	matrix[10] = t2;
	t1 = cos*matrix[7] + sin*matrix[11];
	t2 = cos*matrix[11] - sin*matrix[7];
	matrix[7] = t1;
	matrix[11] = t2;
}
//}}}

 // Rotate Y
//{{{
void lnxMatrixRotateY(float matrix[], float cos, float sin)
{
	// matrix = matrix*RotationMatrixY(angle)

	float t1, t2;

	t1 = cos*matrix[8] + sin*matrix[0];
	t2 = cos*matrix[0] - sin*matrix[8];
	matrix[8] = t1;
	matrix[0] = t2;
	t1 = cos*matrix[9] + sin*matrix[1];
	t2 = cos*matrix[1] - sin*matrix[9];
	matrix[9] = t1;
	matrix[1] = t2;
	t1 = cos*matrix[10] + sin*matrix[2];
	t2 = cos*matrix[2] - sin*matrix[10];
	matrix[10] = t1;
	matrix[2] = t2;
	t1 = cos*matrix[11] + sin*matrix[3];
	t2 = cos*matrix[3] - sin*matrix[11];
	matrix[11] = t1;
	matrix[3] = t2;
}
//}}}

 // Rotate Z
//{{{
void lnxMatrixRotateZ(float matrix[], float cos, float sin)
{
	// matrix = matrix*RotationMatrixZ(angle)

	float t1, t2;

	t1 = cos*matrix[0] + sin*matrix[4];
	t2 = cos*matrix[4] - sin*matrix[0];
	matrix[0] = t1;
	matrix[4] = t2;
	t1 = cos*matrix[1] + sin*matrix[5];
	t2 = cos*matrix[5] - sin*matrix[1];
	matrix[1] = t1;
	matrix[5] = t2;
	t1 = cos*matrix[2] + sin*matrix[6];
	t2 = cos*matrix[6] - sin*matrix[2];
	matrix[2] = t1;
	matrix[6] = t2;
	t1 = cos*matrix[3] + sin*matrix[7];
	t2 = cos*matrix[7] - sin*matrix[3];
	matrix[3] = t1;
	matrix[7] = t2;
}
//}}}

 // Multiply
//{{{
void lnxMatrixMultiply(float result[], float left[], float right[])
{
	// result = left*right

	float cl00 = left[0];
	float cl01 = left[1];
	float cl02 = left[2];
	float cl03 = left[3];
	float cl04 = left[4];
	float cl05 = left[5];
	float cl06 = left[6];
	float cl07 = left[7];
	float cl08 = left[8];
	float cl09 = left[9];
	float cl10 = left[10];
	float cl11 = left[11];
	float cl12 = left[12];
	float cl13 = left[13];
	float cl14 = left[14];
	float cl15 = left[15];
	float cr00 = right[0];
	float cr01 = right[1];
	float cr02 = right[2];
	float cr03 = right[3];
	float cr04 = right[4];
	float cr05 = right[5];
	float cr06 = right[6];
	float cr07 = right[7];
	float cr08 = right[8];
	float cr09 = right[9];
	float cr10 = right[10];
	float cr11 = right[11];
	float cr12 = right[12];
	float cr13 = right[13];
	float cr14 = right[14];
	float cr15 = right[15];

	result[0]  = cl00*cr00 + cl04*cr01 + cl08*cr02 + cl12*cr03;
	result[1]  = cl01*cr00 + cl05*cr01 + cl09*cr02 + cl13*cr03;
	result[2]  = cl02*cr00 + cl06*cr01 + cl10*cr02 + cl14*cr03;
	result[3]  = cl03*cr00 + cl07*cr01 + cl11*cr02 + cl15*cr03;
	result[4]  = cl00*cr04 + cl04*cr05 + cl08*cr06 + cl12*cr07;
	result[5]  = cl01*cr04 + cl05*cr05 + cl09*cr06 + cl13*cr07;
	result[6]  = cl02*cr04 + cl06*cr05 + cl10*cr06 + cl14*cr07;
	result[7]  = cl03*cr04 + cl07*cr05 + cl11*cr06 + cl15*cr07;
	result[8]  = cl00*cr08 + cl04*cr09 + cl08*cr10 + cl12*cr11;
	result[9]  = cl01*cr08 + cl05*cr09 + cl09*cr10 + cl13*cr11;
	result[10] = cl02*cr08 + cl06*cr09 + cl10*cr10 + cl14*cr11;
	result[11] = cl03*cr08 + cl07*cr09 + cl11*cr10 + cl15*cr11;
	result[12] = cl00*cr12 + cl04*cr13 + cl08*cr14 + cl12*cr15;
	result[13] = cl01*cr12 + cl05*cr13 + cl09*cr14 + cl13*cr15;
	result[14] = cl02*cr12 + cl06*cr13 + cl10*cr14 + cl14*cr15;
	result[15] = cl03*cr12 + cl07*cr13 + cl11*cr14 + cl15*cr15;

}
//}}}

 // Projection
//{{{
void lnxMatrixProjection(float matrix[], float near, float far, float widthTan, float heightTan)
{

	matrix[0]  = 1.0f/widthTan;
	matrix[1]  = 0.0f;
	matrix[2]  = 0.0f;
	matrix[3]  = 0.0f;
	matrix[4]  = 0.0f;
	matrix[5]  = 1.0f/heightTan;
	matrix[6]  = 0.0f;
	matrix[7]  = 0.0f;
	matrix[8]  = 0.0f;
	matrix[9]  = 0.0f;
	matrix[10] = (far+near)/(near-far);
	matrix[11] = -1.0f;
	matrix[12] = 0.0f;
	matrix[13] = 0.0f;
	matrix[14] = 2.0*far*near/(far-near);
	matrix[15] = 0.0f;
}
//}}}

//}}}



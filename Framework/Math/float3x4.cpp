/** @file
    @author Jukka Jyl�nki

    This work is copyrighted material and may NOT be used for any kind of commercial or 
    personal advantage and may NOT be copied or redistributed without prior consent
    of the author(s). 
*/
#include "StableHeaders.h"

#include "Math/float3.h"
#include "Math/float4.h"
#include "Math/float3x3.h"
#include "Math/float3x4.h"
#include "Math/float4x4.h"
#include "Math/Matrix.inl"
#include "Math/Quat.h"
#include "Math/MathFunc.h"
#include "Math/TransformOps.h"

float3x4::float3x4(float _00, float _01, float _02, float _03,
         float _10, float _11, float _12, float _13,
         float _20, float _21, float _22, float _23)
{
    Set(_00, _01, _02, _03,
        _10, _11, _12, _13,
        _20, _21, _22, _23);
}

float3x4::float3x4(const float3x3 &other)
{
    SetRotatePart(other);
    SetTranslatePart(0, 0, 0);
}

float3x4::float3x4(const float3 &col0, const float3 &col1, const float3 &col2, const float3 &col3)
{
    SetCol(0, col0);
    SetCol(1, col1);
    SetCol(2, col2);
    SetCol(3, col3);
}

float3x4::float3x4(const Quat &orientation)
{
    SetRotatePart(orientation);
    SetTranslatePart(0, 0, 0);
}

TranslateOp float3x4::Translate(float tx, float ty, float tz)
{
    return TranslateOp(tx, ty, tz);
}

TranslateOp float3x4::Translate(const float3 &offset)
{
    return TranslateOp(offset);
}

float3x4 float3x4::RotateX(float angle)
{
    float3x4 r;
    r.SetRotatePartX(angle);
    r.SetTranslatePart(0, 0, 0);
    return r;
}

float3x4 float3x4::RotateY(float angle)
{
    float3x4 r;
    r.SetRotatePartY(angle);
    r.SetTranslatePart(0, 0, 0);
    return r;
}

float3x4 float3x4::RotateZ(float angle)
{
    float3x4 r;
    r.SetRotatePartZ(angle);
    r.SetTranslatePart(0, 0, 0);
    return r;
}

float3x4 float3x4::RotateAxisAngle(const float3 &axisDirection, float angleRadians)
{
    float3x4 r;
    r.SetRotatePart(Quat::RotateAxisAngle(axisDirection, angleRadians));
    r.SetTranslatePart(0, 0, 0);
    return r;
}

float3x4 float3x4::RotateFromTo(const float3 &sourceDirection, const float3 &targetDirection)
{
    float3x4 r;
    r.SetRotatePart(Quat::RotateFromTo(sourceDirection, targetDirection));
    r.SetTranslatePart(0, 0, 0);
    return r;
}

float3x4 float3x4::FromQuat(const Quat &orientation)
{
    float3x4 r;
    r.SetRotatePart(orientation);
    r.SetTranslatePart(0, 0, 0);
    return r;
}

float3x4 float3x4::FromTRS(const float3 &translate, const Quat &rotate, const float3 &scale)
{
    return float3x4::Translate(translate) * float3x4(rotate) * float3x4::Scale(scale);
}

float3x4 float3x4::FromTRS(const float3 &translate, const float3x3 &rotate, const float3 &scale)
{
    return float3x4::Translate(translate) * float3x4(rotate) * float3x4::Scale(scale);
}

float3x4 float3x4::FromTRS(const float3 &translate, const float3x4 &rotate, const float3 &scale)
{
    return float3x4::Translate(translate) * float3x4(rotate) * float3x4::Scale(scale);
}

/*
float3x4 float3x4::FromEulerXYX(float x2, float y, float x)
{
    return float3x4::RotateX(x2) * float3x4::RotateY(y) * float3x4::RotateX(x);
}

float3x4 float3x4::FromEulerXZX(float x2, float z, float x)
{
    return float3x4::RotateX(x2) * float3x4::RotateZ(z) * float3x4::RotateX(x);
}

float3x4 float3x4::FromEulerYXY(float y2, float x, float y)
{
    return float3x4::RotateY(y2) * float3x4::RotateX(x) * float3x4::RotateY(y);
}

float3x4 float3x4::FromEulerYZY(float y2, float z, float y)
{
    return float3x4::RotateY(y2) * float3x4::RotateZ(z) * float3x4::RotateY(y);
}

float3x4 float3x4::FromEulerZXZ(float z2, float x, float z)
{
    return float3x4::RotateZ(z2) * float3x4::RotateX(x) * float3x4::RotateZ(z);
}

float3x4 float3x4::FromEulerZYZ(float ez, float ey, float ez2)
{
    return float3x4::RotateZ(ez) * float3x4::RotateY(ey) * float3x4::RotateZ(ez2);
}

float3x4 float3x4::FromEulerXYZ(float x, float y, float z)
{
    return float3x4::RotateX(x) * float3x4::RotateY(y) * float3x4::RotateX(z);
}

float3x4 float3x4::FromEulerXZY(float x, float z, float y)
{
    return float3x4::RotateX(x) * float3x4::RotateZ(z) * float3x4::RotateY(y);
}

float3x4 float3x4::FromEulerYXZ(float y, float x, float z)
{
    return float3x4::RotateY(y) * float3x4::RotateX(x) * float3x4::RotateZ(z);
}

float3x4 float3x4::FromEulerYZX(float y, float z, float x)
{
    return float3x4::RotateY(y) * float3x4::RotateZ(z) * float3x4::RotateX(x);
}

float3x4 float3x4::FromEulerZXY(float z, float x, float y)
{
    return float3x4::RotateZ(z) * float3x4::RotateX(x) * float3x4::RotateY(y);
}

float3x4 float3x4::FromEulerZYX(float z, float y, float x)
{
    return float3x4::RotateZ(z) * float3x4::RotateY(y) * float3x4::RotateX(x);
}
*/

float3x4 float3x4::FromEulerXYX(float x2, float y, float x)
{
    float3x4 r;
    r.SetTranslatePart(0,0,0);
    Set3x3PartRotateEulerXYX(r, x2, y, x);
    assert(r.Equals(float3x4::RotateX(x2) * float3x4::RotateY(y) * float3x4::RotateX(x)));
    return r;
}

float3x4 float3x4::FromEulerXZX(float x2, float z, float x)
{
    float3x4 r;
    r.SetTranslatePart(0,0,0);
    Set3x3PartRotateEulerXZX(r, x2, z, x);
    assert(r.Equals(float3x4::RotateX(x2) * float3x4::RotateZ(z) * float3x4::RotateX(x)));
    return r;
}

float3x4 float3x4::FromEulerYXY(float y2, float x, float y)
{
    float3x4 r;
    r.SetTranslatePart(0,0,0);
    Set3x3PartRotateEulerYXY(r, y2, x, y);
    assert(r.Equals(float3x4::RotateY(y2) * float3x4::RotateX(x) * float3x4::RotateY(y)));
    return r;
}

float3x4 float3x4::FromEulerYZY(float y2, float z, float y)
{
    float3x4 r;
    r.SetTranslatePart(0,0,0);
    Set3x3PartRotateEulerYZY(r, y2, z, y);
    assert(r.Equals(float3x4::RotateY(y2) * float3x4::RotateZ(z) * float3x4::RotateY(y)));
    return r;
}

float3x4 float3x4::FromEulerZXZ(float z2, float x, float z)
{
    float3x4 r;
    r.SetTranslatePart(0,0,0);
    Set3x3PartRotateEulerZXZ(r, z2, x, z);
    assert(r.Equals(float3x4::RotateZ(z2) * float3x4::RotateX(x) * float3x4::RotateZ(z)));
    return r;
}

float3x4 float3x4::FromEulerZYZ(float z2, float y, float z)
{
    float3x4 r;
    r.SetTranslatePart(0,0,0);
    Set3x3PartRotateEulerZYZ(r, z2, y, z);
    assert(r.Equals(float3x4::RotateZ(z2) * float3x4::RotateY(y) * float3x4::RotateZ(z)));
    return r;
}

float3x4 float3x4::FromEulerXYZ(float x, float y, float z)
{
    float3x4 r;
    r.SetTranslatePart(0,0,0);
    Set3x3PartRotateEulerXYZ(r, x, y, z);
    assert(r.Equals(float3x4::RotateX(x) * float3x4::RotateY(y) * float3x4::RotateX(z)));
    return r;
}

float3x4 float3x4::FromEulerXZY(float x, float z, float y)
{
    float3x4 r;
    r.SetTranslatePart(0,0,0);
    Set3x3PartRotateEulerXZY(r, x, z, y);
    assert(r.Equals(float3x4::RotateX(x) * float3x4::RotateZ(z) * float3x4::RotateY(y)));
    return r;
}

float3x4 float3x4::FromEulerYXZ(float y, float x, float z)
{
    float3x4 r;
    r.SetTranslatePart(0,0,0);
    Set3x3PartRotateEulerYXZ(r, y, x, z);
    assert(r.Equals(float3x4::RotateY(y) * float3x4::RotateX(x) * float3x4::RotateZ(z)));
    return r;
}

float3x4 float3x4::FromEulerYZX(float y, float z, float x)
{
    float3x4 r;
    r.SetTranslatePart(0,0,0);
    Set3x3PartRotateEulerYZX(r, y, z, x);
    assert(r.Equals(float3x4::RotateY(y) * float3x4::RotateZ(z) * float3x4::RotateX(x)));
    return r;
}

float3x4 float3x4::FromEulerZXY(float z, float x, float y)
{
    float3x4 r;
    r.SetTranslatePart(0,0,0);
    Set3x3PartRotateEulerZXY(r, z, x, y);
    assert(r.Equals(float3x4::RotateZ(z) * float3x4::RotateX(x) * float3x4::RotateY(y)));
    return r;
}

float3x4 float3x4::FromEulerZYX(float z, float y, float x)
{
    float3x4 r;
    r.SetTranslatePart(0,0,0);
    Set3x3PartRotateEulerZYX(r, z, y, x);
    assert(r.Equals(float3x4::RotateZ(z) * float3x4::RotateY(y) * float3x4::RotateX(x)));
    return r;
}

ScaleOp float3x4::Scale(float sx, float sy, float sz)
{
    return ScaleOp(sx, sy, sz);
}

ScaleOp float3x4::Scale(const float3 &scale)
{
    return ScaleOp(scale);
}

float3x4 float3x4::ScaleAlongAxis(const float3 &axis, float scalingFactor)
{
    return Scale(axis * scalingFactor);
}

ScaleOp float3x4::UniformScale(float uniformScale)
{
    return ScaleOp(uniformScale, uniformScale, uniformScale);
}

float3x4 float3x4::ShearX(float yFactor, float zFactor)
{
    return float3x4(1.f, yFactor, zFactor, 0.f,
                    0.f, 1.f, 0.f, 0.f,
                    0.f, 0.f, 1.f, 0.f);
}

float3x4 float3x4::ShearY(float xFactor, float zFactor)
{
    return float3x4(1.f, 0.f, 0.f,  0.f,
                    xFactor, 1.f, zFactor, 0.f,
                    0.f, 0.f, 1.f, 0.f);
}

float3x4 float3x4::ShearZ(float xFactor, float yFactor)
{
    return float3x4(1.f, 0.f, 0.f, 0.f,
                    0.f, 1.f, 0.f, 0.f,
                    xFactor, yFactor, 1.f, 0.f);
}

float3x4 float3x4::Reflect(const Plane &p)
{
    assume(false && "Not implemented!");
    return float3x3(); ///\todo
}

float3x4 float3x4::MakeOrthographicProjection(float nearPlaneDistance, float farPlaneDistance, float horizontalViewportSize, float verticalViewportSize)
{
    assume(false && "Not implemented!");
    return float3x3(); ///\todo
}

float3x4 float3x4::MakeOrthographicProjection(const Plane &target)
{
    assume(false && "Not implemented!");
    return float3x3(); ///\todo
}

float3x4 float3x4::MakeOrthographicProjectionYZ()
{
    assume(false && "Not implemented!");
    return float3x3(); ///\todo
}

float3x4 float3x4::MakeOrthographicProjectionXZ()
{
    assume(false && "Not implemented!");
    return float3x3(); ///\todo
}

float3x4 float3x4::MakeOrthographicProjectionXY()
{
    assume(false && "Not implemented!");
    return float3x3(); ///\todo
}

MatrixProxy<float3x4::Cols> &float3x4::operator[](int row)
{
    assert(row >= 0);
    assert(row < Rows);

    return *(reinterpret_cast<MatrixProxy<Cols>*>(v[row]));
}

const MatrixProxy<float3x4::Cols> &float3x4::operator[](int row) const
{
    assert(row >= 0);
    assert(row < Rows);

    return *(reinterpret_cast<const MatrixProxy<Cols>*>(v[row]));
}

float &float3x4::At(int row, int col)
{
    return v[row][col];
}

const float float3x4::At(int row, int col) const
{
    return v[row][col];
}

float4 &float3x4::Row(int row)
{
    return reinterpret_cast<float4 &>(v[row]);
}

const float4 &float3x4::Row(int row) const
{
    return reinterpret_cast<const float4 &>(v[row]);
}

float3 &float3x4::Row3(int row)
{
    return reinterpret_cast<float3 &>(v[row]);
}

const float3 &float3x4::Row3(int row) const
{
    return reinterpret_cast<const float3 &>(v[row]);
}

const float3 float3x4::Col(int col) const
{
    return float3(v[0][col], v[1][col], v[2][col]);
}

const float3 float3x4::Diagonal() const
{
    return float3(v[0][0], v[1][1], v[2][2]);
}

void float3x4::ScaleRow3(int row, float scalar)
{
    Row3(row) *= scalar;
}

void float3x4::ScaleRow(int row, float scalar)
{
    Row(row) *= scalar;
}

void float3x4::ScaleCol(int col, float scalar)
{
    v[0][col] *= scalar;
    v[1][col] *= scalar;
    v[2][col] *= scalar;
    v[3][col] *= scalar;
}

float3x3 float3x4::Float3x3Part() const
{
    return float3x3(v[0][0], v[0][1], v[0][2],
                    v[1][0], v[1][1], v[1][2],
                    v[2][0], v[2][1], v[2][2]);
}

float3 float3x4::TranslatePart() const
{
    return Col(3);
}

float3x3 float3x4::RotatePart() const
{
    return Float3x3Part();
}

float3 float3x4::WorldX() const
{
    return Col(0);
}

float3 float3x4::WorldY() const
{
    return Col(1);
}

float3 float3x4::WorldZ() const
{
    return Col(2);
}

float *float3x4::ptr()
{
    return &v[0][0];
}

const float *float3x4::ptr() const
{
    return &v[0][0];
}

void float3x4::SetRow(int row, float x, float y, float z, float w)
{
    v[row][0] = x;
    v[row][1] = y;
    v[row][2] = z;
    v[row][3] = w;
}

void float3x4::SetRow(int row, const float4 &rowVector)
{
    v[row][0] = rowVector.x;
    v[row][1] = rowVector.y;
    v[row][2] = rowVector.z;
    v[row][3] = rowVector.w;
}

void float3x4::SetRow(int row, const float *data)
{
    v[row][0] = data[0];
    v[row][1] = data[1];
    v[row][2] = data[2];
    v[row][3] = data[3];
}

void float3x4::SetCol(int column, float x, float y, float z)
{
    v[0][column] = x;
    v[1][column] = y;
    v[2][column] = z;
}

void float3x4::SetCol(int column, const float3 &columnVector)
{
    v[0][column] = columnVector.x;
    v[1][column] = columnVector.y;
    v[2][column] = columnVector.z;
}

void float3x4::SetCol(int column, const float *data)
{
    v[0][column] = data[0];
    v[1][column] = data[1];
    v[2][column] = data[2];
}

void float3x4::Set(float _00, float _01, float _02, float _03,
                   float _10, float _11, float _12, float _13,
                   float _20, float _21, float _22, float _23)
{
    v[0][0] = _00; v[0][1] = _01; v[0][2] = _02; v[0][3] = _03;
    v[1][0] = _10; v[1][1] = _11; v[1][2] = _12; v[1][3] = _13;
    v[2][0] = _20; v[2][1] = _21; v[2][2] = _22; v[2][3] = _23;
}

void float3x4::SetIdentity()
{
    Set(1,0,0,0,
        0,1,0,0,
        0,0,1,0);
}

void float3x4::Set(const float *values)
{
    memcpy(ptr(), values, sizeof(float) * Rows * Cols);
}

void float3x4::Set3x3Part(const float3x3 &r)
{
    v[0][0] = r[0][0]; v[0][1] = r[0][1]; v[0][2] = r[0][2];
    v[1][0] = r[1][0]; v[1][1] = r[1][1]; v[1][2] = r[1][2];
    v[2][0] = r[2][0]; v[2][1] = r[2][1]; v[2][2] = r[2][2];
}
void float3x4::SwapColumns(int col1, int col2)
{
    std::swap(v[0][col1], v[0][col2]);
    std::swap(v[1][col1], v[1][col2]);
    std::swap(v[2][col1], v[2][col2]);
}

void float3x4::SwapRows(int row1, int row2)
{
    std::swap(v[row1][0], v[row2][0]);
    std::swap(v[row1][1], v[row2][1]);
    std::swap(v[row1][2], v[row2][2]);
    std::swap(v[row1][3], v[row2][3]);
}

void float3x4::SetRotatePartX(float angle)
{
    Set3x3PartRotateX(*this, angle);
}

void float3x4::SetRotatePartY(float angle)
{
    Set3x3PartRotateY(*this, angle);
}

void float3x4::SetRotatePartZ(float angle)
{
    Set3x3PartRotateZ(*this, angle);
}

void float3x4::SetRotatePart(const float3 &axisDirection, float angle)
{
    SetRotatePart(Quat(axisDirection, angle));
}
void float3x4::SetRotatePart(const Quat &q)
{
    // See e.g. http://www.geometrictools.com/Documentation/LinearAlgebraicQuaternions.pdf .

    assume(q.IsNormalized());
    const float x = q.x; const float y = q.y; const float z = q.z; const float w = q.w;
    v[0][0] = 1 - 2*(y*y + z*z); v[0][1] =     2*(x*y - z*w); v[0][2] =     2*(x*z + y*w);
    v[1][0] =     2*(x*y + z*w); v[1][1] = 1 - 2*(x*x + z*z); v[1][2] =     2*(y*z - x*w);
    v[2][0] =     2*(x*z - y*w); v[2][1] =     2*(y*z + x*w); v[2][2] = 1 - 2*(x*x + y*y);
}

float float3x4::Determinant() const
{
    const float a = v[0][0];
    const float b = v[0][1];
    const float c = v[0][2];
    const float d = v[1][0];
    const float e = v[1][1];
    const float f = v[1][2];
    const float g = v[2][0];
    const float h = v[2][1];
    const float i = v[2][2];

    return a*e*i + b*f*g + c*d*h - a*f*h - b*d*i - c*e*g;
}

bool float3x4::Inverse()
{
    return false; ///\todo
}

float3x4 float3x4::Inverted() const
{
    float3x4 copy = *this;
    copy.Inverse();
    return copy;
}

bool float3x4::InverseAffine()
{
    std::swap(v[0][1], v[1][0]);
    std::swap(v[0][2], v[2][0]);
    std::swap(v[1][2], v[2][1]);
    float scale1 = sqrtf(1.f / float3(v[0][0], v[0][1], v[0][2]).LengthSq());
    float scale2 = sqrtf(1.f / float3(v[1][0], v[1][1], v[1][2]).LengthSq());
    float scale3 = sqrtf(1.f / float3(v[2][0], v[2][1], v[2][2]).LengthSq());

    v[0][0] *= scale1; v[0][1] *= scale2; v[0][2] *= scale3;
    v[1][0] *= scale1; v[1][1] *= scale2; v[1][2] *= scale3;
    v[2][0] *= scale1; v[2][1] *= scale2; v[2][2] *= scale3;

    SetTranslatePart(TransformDir(-v[0][3], -v[1][3], -v[2][3]));

    return true;
}

bool float3x4::InverseAffineUniformScale()
{
    std::swap(v[0][1], v[1][0]);
    std::swap(v[0][2], v[2][0]);
    std::swap(v[1][2], v[2][1]);
    const float scale = sqrtf(1.f / float3(v[0][0], v[0][1], v[0][2]).LengthSq());

    v[0][0] *= scale; v[0][1] *= scale; v[0][2] *= scale;
    v[1][0] *= scale; v[1][1] *= scale; v[1][2] *= scale;
    v[2][0] *= scale; v[2][1] *= scale; v[2][2] *= scale;

    SetTranslatePart(TransformDir(-v[0][3], -v[1][3], -v[2][3]));

    return true;
}

void float3x4::InverseAffineNoScale()
{
    std::swap(v[0][1], v[1][0]);
    std::swap(v[0][2], v[2][0]);
    std::swap(v[1][2], v[2][1]);
    SetTranslatePart(TransformDir(-v[0][3], -v[1][3], -v[2][3]));
}

void float3x4::Transpose()
{
    std::swap(v[0][1], v[1][0]);
    std::swap(v[0][2], v[2][0]);
    std::swap(v[1][2], v[2][1]);
}

float3x4 float3x4::Transposed() const
{
    float3x4 copy = *this;
    copy.Transpose();
    return copy;
}

bool float3x4::InverseTranspose()
{
    bool success = Inverse();
    Transpose();
    return success;
}

float3x4 float3x4::InverseTransposed() const
{
    float3x4 copy = *this;
    copy.Transpose();
    copy.Inverse();
    return copy;
}

float float3x4::Trace() const
{
    return v[0][0] + v[1][1] + v[2][2];
}

void float3x4::Orthonormalize(int c0, int c1, int c2)
{
    assume(c0 != c1 && c0 != c2 && c1 != c2);
    if (c0 == c1 || c0 == c2 || c1 == c2)
        return;

    ///\todo Optimize away copies.
    float3 v0 = Col(c0);
    float3 v1 = Col(c1);
    float3 v2 = Col(c2);
    float3::Orthonormalize(v0, v1, v2);
    SetCol(c0, v0);
    SetCol(c1, v1);
    SetCol(c2, v2);
}

void float3x4::RemoveScale()
{
    float x = Row3(0).Normalize();
    float y = Row3(1).Normalize();
    float z = Row3(2).Normalize();
    assume(x != 0 && y != 0 && z != 0 && "float3x4::RemoveScale failed!");
}

float3 float3x4::TransformPoint(const float3 &pointVector) const
{
    return float3(DOT3(v[0], pointVector) + v[0][3],
                  DOT3(v[1], pointVector) + v[1][3],
                  DOT3(v[2], pointVector) + v[2][3]);
}

float3 float3x4::TransformDir(const float3 &directionVector) const
{
    return float3(DOT3(v[0], directionVector),
                  DOT3(v[1], directionVector),
                  DOT3(v[2], directionVector));
}

float3 float3x4::TransformDir(float x, float y, float z) const
{
    return TransformDir(float3(x,y,z));
}

float4 float3x4::Transform(const float4 &vector) const
{
    return float4(DOT4(v[0], vector),
                  DOT4(v[1], vector),
                  DOT4(v[2], vector),
                  vector.w);
}

void float3x4::BatchTransformPoint(float3 *pointArray, int numPoints) const
{
    assume(false && "Not implemented!"); ///\todo
}

void float3x4::BatchTransformPoint(float3 *pointArray, int numPoints, int stride) const
{
    assume(false && "Not implemented!"); ///\todo
}

void float3x4::BatchTransformDir(float3 *dirArray, int numVectors) const
{
    assume(false && "Not implemented!"); ///\todo
}

void float3x4::BatchTransformDir(float3 *dirArray, int numVectors, int stride) const
{
    assume(false && "Not implemented!"); ///\todo
}

void float3x4::BatchTransform(float4 *vectorArray, int numVectors) const
{
    assume(false && "Not implemented!"); ///\todo
}

void float3x4::BatchTransform(float4 *vectorArray, int numVectors, int stride) const
{
    assume(false && "Not implemented!"); ///\todo
}

float3x4 float3x4::operator *(const float3x3 &rhs) const
{
    float3x4 r;
    const float *c0 = rhs.ptr();
    const float *c1 = rhs.ptr() + 1;
    const float *c2 = rhs.ptr() + 2;
    r[0][0] = DOT3STRIDED(v[0], c0, 3);
    r[0][1] = DOT3STRIDED(v[0], c1, 3);
    r[0][2] = DOT3STRIDED(v[0], c2, 3);
    r[0][3] = v[0][3];

    r[1][0] = DOT3STRIDED(v[1], c0, 3);
    r[1][1] = DOT3STRIDED(v[1], c1, 3);
    r[1][2] = DOT3STRIDED(v[1], c2, 3);
    r[1][3] = v[1][3];

    r[2][0] = DOT3STRIDED(v[2], c0, 3);
    r[2][1] = DOT3STRIDED(v[2], c1, 3);
    r[2][2] = DOT3STRIDED(v[2], c2, 3);
    r[2][3] = v[2][3];

    return r;
}

float3x4 float3x4::operator *(const float3x4 &rhs) const
{
    float3x4 r;
    const float *c0 = rhs.ptr();
    const float *c1 = rhs.ptr() + 1;
    const float *c2 = rhs.ptr() + 2;
    const float *c3 = rhs.ptr() + 3;
    r[0][0] = DOT3STRIDED(v[0], c0, 4);
    r[0][1] = DOT3STRIDED(v[0], c1, 4);
    r[0][2] = DOT3STRIDED(v[0], c2, 4);
    r[0][3] = DOT3STRIDED(v[0], c3, 4) + v[0][3];

    r[1][0] = DOT3STRIDED(v[1], c0, 4);
    r[1][1] = DOT3STRIDED(v[1], c1, 4);
    r[1][2] = DOT3STRIDED(v[1], c2, 4);
    r[1][3] = DOT3STRIDED(v[1], c3, 4) + v[1][3];

    r[2][0] = DOT3STRIDED(v[2], c0, 4);
    r[2][1] = DOT3STRIDED(v[2], c1, 4);
    r[2][2] = DOT3STRIDED(v[2], c2, 4);
    r[2][3] = DOT3STRIDED(v[2], c3, 4) + v[2][3];

    return r;
}

float3x4 float3x4::operator *(const Quat &rhs) const
{
    float3x3 rot(rhs);
    return *this * rot;
}

float4 float3x4::operator *(const float4 &rhs) const
{
    return Transform(rhs);
}

bool float3x4::IsFinite() const
{
    for(int y = 0; y < Rows; ++y)
        for(int x = 0; x < Cols; ++x)
            if (!isfinite(v[y][x]))
                return false;
    return true;
}

bool float3x4::IsIdentity(float epsilon) const
{
    for(int y = 0; y < Rows; ++y)
        for(int x = 0; x < Cols; ++x)
            if (!EqualAbs(v[y][x], (x == y) ? 1.f : 0.f, epsilon))
                return false;

    return true;
}

bool float3x4::IsLowerTriangular(float epsilon) const
{
    return EqualAbs(v[0][1], 0.f, epsilon)
        && EqualAbs(v[0][2], 0.f, epsilon)
        && EqualAbs(v[0][3], 0.f, epsilon)
        && EqualAbs(v[1][2], 0.f, epsilon)
        && EqualAbs(v[1][3], 0.f, epsilon)
        && EqualAbs(v[2][3], 0.f, epsilon);
}

bool float3x4::IsUpperTriangular(float epsilon) const
{
    return EqualAbs(v[1][0], 0.f, epsilon)
        && EqualAbs(v[2][0], 0.f, epsilon)
        && EqualAbs(v[3][0], 0.f, epsilon)
        && EqualAbs(v[2][1], 0.f, epsilon)
        && EqualAbs(v[3][1], 0.f, epsilon)
        && EqualAbs(v[3][2], 0.f, epsilon);
}

bool float3x4::IsInvertible(float epsilon) const
{
    ///\todo Optimize.
    float3x4 copy = *this;
    return copy.Inverse();
}

bool float3x4::IsSymmetric(float epsilon) const
{
    for(int y = 0; y < Rows; ++y)
        for(int x = y+1; x < Cols; ++x)
            if (!EqualAbs(v[y][x], v[x][y], epsilon))
                return false;
    return true;
}

bool float3x4::IsSkewSymmetric(float epsilon) const
{
    for(int y = 0; y < Rows; ++y)
        for(int x = y; x < Cols; ++x)
            if (!EqualAbs(v[y][x], -v[x][y], epsilon))
                return false;
    return true;
}

bool float3x4::HasUnitaryScale(float epsilon) const
{
    float3 scale = ExtractScale();
    return scale.Equals(1.f, 1.f, 1.f, epsilon);
}

bool float3x4::HasNegativeScale() const
{
    return Determinant() < 0.f;
}

bool float3x4::HasUniformScale(float epsilon) const
{
    float3 scale = ExtractScale();
    return EqualAbs(scale.x, scale.y, epsilon) && EqualAbs(scale.x, scale.z, epsilon);
}

bool float3x4::IsOrthogonal(float epsilon) const
{
    return Row(0).IsPerpendicular3(Row(1), epsilon)
        && Row(0).IsPerpendicular3(Row(2), epsilon)
        && Row(1).IsPerpendicular3(Row(2), epsilon);
}

bool float3x4::Equals(const float3x4 &other, float epsilon) const
{
    for(int y = 0; y < Rows; ++y)
        for(int x = 0; x < Cols; ++x)
            if (!EqualAbs(v[y][x], other[y][x], epsilon))
                return false;
    return true;
}


std::string float3x4::ToString() const
{
    char str[256];
    sprintf(str, "(%.2f, %.2f, %.2f, %.2f) (%.2f, %.2f, %.2f, %.2f) (%.2f, %.2f, %.2f, %.2f)", 
        v[0][0], v[0][1], v[0][2], v[0][3],
        v[1][0], v[1][1], v[1][2], v[1][3],
        v[2][0], v[2][1], v[2][2], v[2][3]);

    return std::string(str);
}

float3 float3x4::ToEulerXYX() const { float3 f; ExtractEulerXYX(*this, f[0], f[1], f[2]); return f; }
float3 float3x4::ToEulerXZX() const { float3 f; ExtractEulerXZX(*this, f[0], f[1], f[2]); return f; }
float3 float3x4::ToEulerYXY() const { float3 f; ExtractEulerYXY(*this, f[0], f[1], f[2]); return f; }
float3 float3x4::ToEulerYZY() const { float3 f; ExtractEulerYZY(*this, f[0], f[1], f[2]); return f; }
float3 float3x4::ToEulerZXZ() const { float3 f; ExtractEulerZXZ(*this, f[0], f[1], f[2]); return f; }
float3 float3x4::ToEulerZYZ() const { float3 f; ExtractEulerZYZ(*this, f[0], f[1], f[2]); return f; }
float3 float3x4::ToEulerXYZ() const { float3 f; ExtractEulerXYZ(*this, f[0], f[1], f[2]); return f; }
float3 float3x4::ToEulerXZY() const { float3 f; ExtractEulerXZY(*this, f[0], f[1], f[2]); return f; }
float3 float3x4::ToEulerYXZ() const { float3 f; ExtractEulerYXZ(*this, f[0], f[1], f[2]); return f; }
float3 float3x4::ToEulerYZX() const { float3 f; ExtractEulerYZX(*this, f[0], f[1], f[2]); return f; }
float3 float3x4::ToEulerZXY() const { float3 f; ExtractEulerZXY(*this, f[0], f[1], f[2]); return f; }
float3 float3x4::ToEulerZYX() const { float3 f; ExtractEulerZYX(*this, f[0], f[1], f[2]); return f; }

float3 float3x4::ExtractScale() const
{
    return float3(Col(0).Length(), Col(1).Length(), Col(2).Length());
}

void float3x4::Decompose(float3 &translate, Quat &rotate, float3 &scale) const
{
    float3x3 r;
    Decompose(translate, r, scale);
    rotate = Quat(r);
}

void float3x4::Decompose(float3 &translate, float3x3 &rotate, float3 &scale) const
{
    assume(this->IsOrthogonal());

    translate = Col(3);
    rotate = RotatePart();
    scale.x = rotate.Col(0).Length();
    scale.y = rotate.Col(1).Length();
    scale.z = rotate.Col(2).Length();
    assume(!EqualAbs(scale.x, 0));
    assume(!EqualAbs(scale.y, 0));
    assume(!EqualAbs(scale.z, 0));
    rotate.ScaleCol(0, 1.f / scale.x);
    rotate.ScaleCol(1, 1.f / scale.y);
    rotate.ScaleCol(2, 1.f / scale.z);
}

void float3x4::Decompose(float3 &translate, float3x4 &rotate, float3 &scale) const
{
    float3x3 r;
    Decompose(translate, r, scale);
    rotate.SetRotatePart(r);
    rotate.SetTranslatePart(0,0,0);
}

std::ostream &operator <<(std::ostream &out, const float3x4 &rhs)
{
    out << rhs.ToString();
    return out;
}

float3x4 operator *(const Quat &lhs, const float3x4 &rhs)
{
    return float3x4(lhs) * rhs;
}

float3x4 operator *(const float3x3 &lhs, const float3x4 &rhs)
{
    return float3x4(lhs) * rhs;
}

float4 operator *(const float4 &lhs, const float3x4 &rhs)
{
    return float4(DOT3STRIDED(lhs, rhs.ptr(), 4),
                  DOT3STRIDED(lhs, rhs.ptr()+1, 4),
                  DOT3STRIDED(lhs, rhs.ptr()+2, 4),
                  DOT3STRIDED(lhs, rhs.ptr()+3, 4) + lhs.w);
}

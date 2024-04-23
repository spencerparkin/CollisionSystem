#include "Matrix3x3.h"
#include "Vector3.h"
#include "Quaternion.h"

using namespace Collision;

Matrix3x3::Matrix3x3()
{
	// The compiler will likely be able to unroll all loops like this.
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++)
			this->ele[i][j] = 0.0;
}

Matrix3x3::Matrix3x3(const Matrix3x3& matrix)
{
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++)
			this->ele[i][j] = matrix.ele[i][j];
}

/*virtual*/ Matrix3x3::~Matrix3x3()
{
}

void Matrix3x3::operator=(const Matrix3x3& matrix)
{
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++)
			this->ele[i][j] = matrix.ele[i][j];
}

void Matrix3x3::operator+=(const Matrix3x3& matrix)
{
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++)
			this->ele[i][j] += matrix.ele[i][j];
}

void Matrix3x3::operator-=(const Matrix3x3& matrix)
{
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++)
			this->ele[i][j] -= matrix.ele[i][j];
}

void Matrix3x3::operator*=(double scalar)
{
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++)
			this->ele[i][j] *= scalar;
}

bool Matrix3x3::IsValid() const
{
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			if (::isnan(this->ele[i][j]))
				return false;

			if (::isinf(this->ele[i][j]))
				return false;
		}
	}

	return true;
}

void Matrix3x3::SetIdentity()
{
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++)
			this->ele[i][j] == (i == j) ? 1.0 : 0.0;
}

void Matrix3x3::GetRowVectors(Vector3& xAxis, Vector3& yAxis, Vector3& zAxis) const
{
	xAxis.x = this->ele[0][0];
	xAxis.y = this->ele[0][1];
	xAxis.z = this->ele[0][2];

	yAxis.x = this->ele[1][0];
	yAxis.y = this->ele[1][1];
	yAxis.z = this->ele[1][2];

	zAxis.x = this->ele[2][0];
	zAxis.y = this->ele[2][1];
	zAxis.z = this->ele[2][2];
}

void Matrix3x3::SetRowVectors(const Vector3& xAxis, const Vector3& yAxis, const Vector3& zAxis)
{
	this->ele[0][0] = xAxis.x;
	this->ele[0][1] = xAxis.y;
	this->ele[0][2] = xAxis.z;

	this->ele[1][0] = yAxis.x;
	this->ele[1][1] = yAxis.y;
	this->ele[1][2] = yAxis.z;

	this->ele[2][0] = zAxis.x;
	this->ele[2][1] = zAxis.y;
	this->ele[2][2] = zAxis.z;
}

void Matrix3x3::GetColumnVectors(Vector3& xAxis, Vector3& yAxis, Vector3& zAxis) const
{
	xAxis.x = this->ele[0][0];
	xAxis.y = this->ele[1][0];
	xAxis.z = this->ele[2][0];

	yAxis.x = this->ele[0][1];
	yAxis.y = this->ele[1][1];
	yAxis.z = this->ele[2][1];

	zAxis.x = this->ele[0][2];
	zAxis.y = this->ele[1][2];
	zAxis.z = this->ele[2][2];
}

void Matrix3x3::SetColumnVectors(const Vector3& xAxis, const Vector3& yAxis, const Vector3& zAxis)
{
	this->ele[0][0] = xAxis.x;
	this->ele[1][0] = xAxis.y;
	this->ele[2][0] = xAxis.z;

	this->ele[0][1] = yAxis.x;
	this->ele[1][1] = yAxis.y;
	this->ele[2][1] = yAxis.z;

	this->ele[0][2] = zAxis.x;
	this->ele[1][2] = zAxis.y;
	this->ele[2][2] = zAxis.z;
}

void Matrix3x3::SetFromAxisAngle(const Vector3& unitAxis, double angle)
{
	Vector3 xAxis(1.0, 0.0, 0.0);
	Vector3 yAxis(0.0, 1.0, 0.0);
	Vector3 zAxis(0.0, 0.0, 1.0);

	// This is not the most efficient way, but it works.
	xAxis = xAxis.Rotated(unitAxis, angle);
	yAxis = yAxis.Rotated(unitAxis, angle);
	zAxis = zAxis.Rotated(unitAxis, angle);

	this->SetColumnVectors(xAxis, yAxis, zAxis);
}

void Matrix3x3::GetToAxisAngle(Vector3& unitAxis, double& angle) const
{
	Quaternion unitQuat;
	this->GetToQuat(unitQuat);
	unitQuat.GetToAxisAngle(unitAxis, angle);
}

void Matrix3x3::SetFromQuat(const Quaternion& unitQuat)
{
	Vector3 xAxis(1.0, 0.0, 0.0);
	Vector3 yAxis(0.0, 1.0, 0.0);
	Vector3 zAxis(0.0, 0.0, 1.0);

	xAxis = unitQuat.Rotate(xAxis);
	yAxis = unitQuat.Rotate(yAxis);
	zAxis = unitQuat.Rotate(zAxis);

	this->SetColumnVectors(xAxis, yAxis, zAxis);
}

void Matrix3x3::GetToQuat(Quaternion& unitQuat) const
{
	// This is Cayley's method taken from "A Survey on the Computation of Quaternions from Rotation Matrices" by Sarabandi & Thomas.

	double r11 = this->ele[0][0];
	double r21 = this->ele[1][0];
	double r31 = this->ele[2][0];

	double r12 = this->ele[0][1];
	double r22 = this->ele[1][1];
	double r32 = this->ele[2][1];

	double r13 = this->ele[0][2];
	double r23 = this->ele[1][2];
	double r33 = this->ele[2][2];

	unitQuat.w = 0.25 * ::sqrt(COLL_SYS_SQUARED(r11 + r22 + r33 + 1.0) + COLL_SYS_SQUARED(r32 - r23) + COLL_SYS_SQUARED(r13 - r31) + COLL_SYS_SQUARED(r21 - r12));
	unitQuat.x = 0.25 * ::sqrt(COLL_SYS_SQUARED(r32 - r23) + COLL_SYS_SQUARED(r11 - r22 - r33 + 1.0) + COLL_SYS_SQUARED(r21 + r12) + COLL_SYS_SQUARED(r31 + r13)) * COLL_SYS_SIGN(r32 - r23);
	unitQuat.y = 0.25 * ::sqrt(COLL_SYS_SQUARED(r13 - r31) + COLL_SYS_SQUARED(r21 + r12) + COLL_SYS_SQUARED(r22 - r11 - r33 + 1.0) + COLL_SYS_SQUARED(r32 + r23)) * COLL_SYS_SIGN(r13 - r31);
	unitQuat.z = 0.25 * ::sqrt(COLL_SYS_SQUARED(r21 - r12) + COLL_SYS_SQUARED(r31 + r13) + COLL_SYS_SQUARED(r32 + r23) + COLL_SYS_SQUARED(r33 - r11 - r22 + 1.0)) * COLL_SYS_SIGN(r21 - r12);
}

void Matrix3x3::SetOuterProduct(const Vector3& vectorA, const Vector3& vectorB)
{
	this->ele[0][0] = vectorA.x * vectorB.x;
	this->ele[0][1] = vectorA.y * vectorB.x;
	this->ele[0][2] = vectorA.z * vectorB.x;

	this->ele[1][0] = vectorA.x * vectorB.y;
	this->ele[1][1] = vectorA.y * vectorB.y;
	this->ele[1][2] = vectorA.z * vectorB.y;

	this->ele[2][0] = vectorA.x * vectorB.z;
	this->ele[2][1] = vectorA.y * vectorB.z;
	this->ele[2][2] = vectorA.z * vectorB.z;
}

void Matrix3x3::SetForCrossProduct(const Vector3& vector)
{
	this->ele[0][0] = 0.0;
	this->ele[1][0] = vector.z;
	this->ele[2][0] = -vector.y;

	this->ele[0][1] = -vector.z;
	this->ele[1][1] = 0.0;
	this->ele[2][1] = vector.x;

	this->ele[0][2] = vector.y;
	this->ele[1][2] = -vector.x;
	this->ele[2][2] = 0.0;
}

Matrix3x3 Matrix3x3::Orthonormalized() const
{
	Vector3 xAxis, yAxis, zAxis;
	this->GetColumnVectors(xAxis, yAxis, zAxis);

	xAxis.Normalize();
	yAxis = yAxis.RejectedFrom(xAxis).Normalized();
	zAxis = zAxis.RejectedFrom(xAxis).RejectedFrom(yAxis).Normalized();

	Matrix3x3 result;
	result.SetColumnVectors(xAxis, yAxis, zAxis);
	return result;
}

Matrix3x3 Matrix3x3::Inverted() const
{
	Matrix3x3 inverse;
	inverse.Invert(*this);
	return inverse;
}

bool Matrix3x3::Invert(const Matrix3x3& matrix)
{
	double det = matrix.Determinant();
	if (det == 0.0)
		return false;

	double scale = 1.0 / det;
	if (::isnan(scale) || ::isinf(scale))
		return false;

	this->ele[0][0] = matrix.ele[1][1] * matrix.ele[2][2] - matrix.ele[2][1] * matrix.ele[1][2];
	this->ele[0][1] = matrix.ele[0][2] * matrix.ele[2][1] - matrix.ele[2][2] * matrix.ele[0][1];
	this->ele[0][2] = matrix.ele[0][1] * matrix.ele[1][2] - matrix.ele[1][1] * matrix.ele[0][2];

	this->ele[1][0] = matrix.ele[1][2] * matrix.ele[2][0] - matrix.ele[2][2] * matrix.ele[1][0];
	this->ele[1][1] = matrix.ele[0][0] * matrix.ele[2][2] - matrix.ele[2][0] * matrix.ele[0][2];
	this->ele[1][2] = matrix.ele[0][2] * matrix.ele[1][0] - matrix.ele[1][2] * matrix.ele[0][0];

	this->ele[2][0] = matrix.ele[1][0] * matrix.ele[2][1] - matrix.ele[2][0] * matrix.ele[1][1];
	this->ele[2][1] = matrix.ele[0][1] * matrix.ele[2][0] - matrix.ele[2][1] * matrix.ele[0][0];
	this->ele[2][2] = matrix.ele[0][0] * matrix.ele[1][1] - matrix.ele[1][0] * matrix.ele[0][1];

	*this *= scale;

	return true;
}

Matrix3x3 Matrix3x3::Transposed() const
{
	Matrix3x3 transpose;

	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++)
			transpose.ele[i][j] = this->ele[j][i];

	return transpose;
}

void Matrix3x3::Transpose(const Matrix3x3& matrix)
{
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++)
			this->ele[i][j] = matrix.ele[j][i];
}

double Matrix3x3::Determinant() const
{
	return
		+ this->ele[0][0] * (this->ele[1][1] * this->ele[2][2] - this->ele[2][1] * this->ele[1][2])
		- this->ele[0][1] * (this->ele[1][0] * this->ele[2][2] - this->ele[2][0] * this->ele[1][2])
		+ this->ele[0][2] * (this->ele[1][0] * this->ele[2][1] - this->ele[2][0] * this->ele[1][1]);
}

namespace Collision
{
	Matrix3x3 operator+(const Matrix3x3& matrixA, const Matrix3x3& matrixB)
	{
		Matrix3x3 sum;

		for (int i = 0; i < 3; i++)
			for (int j = 0; j < 3; j++)
				sum.ele[i][j] = matrixA.ele[i][j] + matrixB.ele[i][j];

		return sum;
	}

	Matrix3x3 operator-(const Matrix3x3& matrixA, const Matrix3x3& matrixB)
	{
		Matrix3x3 diff;

		for (int i = 0; i < 3; i++)
			for (int j = 0; j < 3; j++)
				diff.ele[i][j] = matrixA.ele[i][j] - matrixB.ele[i][j];

		return diff;
	}

	Matrix3x3 operator*(const Matrix3x3& matrixA, const Matrix3x3& matrixB)
	{
		Matrix3x3 product;

		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				product.ele[i][j] =
					matrixA.ele[i][0] * matrixB.ele[0][j] +
					matrixA.ele[i][1] * matrixB.ele[1][j] +
					matrixA.ele[i][2] * matrixB.ele[2][j];
			}
		}

		return product;
	}

	Matrix3x3 operator/(const Matrix3x3& matrixA, const Matrix3x3& matrixB)
	{
		return matrixA * matrixB.Inverted();
	}

	Matrix3x3 operator*(const Matrix3x3& matrix, double scalar)
	{
		Matrix3x3 product;

		for (int i = 0; i < 3; i++)
			for (int j = 0; j < 3; j++)
				product.ele[i][j] = matrix.ele[i][j] * scalar;

		return product;
	}

	Matrix3x3 operator*(double scalar, const Matrix3x3& matrix)
	{
		Matrix3x3 product;

		for (int i = 0; i < 3; i++)
			for (int j = 0; j < 3; j++)
				product.ele[i][j] = matrix.ele[i][j] * scalar;

		return product;
	}

	Vector3 operator*(const Matrix3x3& matrix, const Vector3& vector)
	{
		Vector3 product;

		product.x =
			matrix.ele[0][0] * vector.x +
			matrix.ele[0][1] * vector.y +
			matrix.ele[0][2] * vector.z;

		product.y =
			matrix.ele[1][0] * vector.x +
			matrix.ele[1][1] * vector.y +
			matrix.ele[1][2] * vector.z;

		product.z =
			matrix.ele[2][0] * vector.x +
			matrix.ele[2][1] * vector.y +
			matrix.ele[2][2] * vector.z;

		return product;
	}

	Vector3 operator*(const Vector3& vector, const Matrix3x3& matrix)
	{
		Vector3 product;

		product.x =
			matrix.ele[0][0] * vector.x +
			matrix.ele[1][0] * vector.y +
			matrix.ele[2][0] * vector.z;

		product.y =
			matrix.ele[0][1] * vector.x +
			matrix.ele[1][1] * vector.y +
			matrix.ele[2][1] * vector.z;

		product.z =
			matrix.ele[0][2] * vector.x +
			matrix.ele[1][2] * vector.y +
			matrix.ele[2][2] * vector.z;

		return product;
	}
}
// Math.h
// Contains mathematical scalar & vector representations and operations used on the FP Master Server.

#pragma once

// TEMPLATED VECTOR TYPES

template<typename ScalarType>
struct Vec2
{
	ScalarType X, Y;

	template<typename OtherScalarType>
	Vec2 operator+(const Vec2<OtherScalarType>& B)
	{
		return { X + B.X, Y + B.Y };
	}

	template<typename OtherScalarType>
	Vec2 operator-(const Vec2<OtherScalarType>& B)
	{
		return { X - B.X, Y - B.Y };
	}

	Vec2<ScalarType> operator *(const float& Mul)
	{
		return { X * Mul, Y * Mul };
	}

	Vec2<ScalarType> operator /(const float& Mul)
	{
		return { X / Mul, Y / Mul };
	}
};

// Returns the Squared Distance on the first 2 dimensions between two vectors of at least 2 dimensions.
template<typename ScalarType, typename OtherScalarType>
float SquaredDist2D(const Vec2<ScalarType>& A, const Vec2<OtherScalarType>& B)
{
	return (A.X - B.X) * (A.X - B.X) + (A.Y - B.Y) * (A.Y - B.Y);
}
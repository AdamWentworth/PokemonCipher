#pragma once

#include <cmath>

class Vector2D {
public:
    float x = 0.0f;
    float y = 0.0f;

    Vector2D() = default;
    Vector2D(float xValue, float yValue) : x(xValue), y(yValue) {}

    Vector2D& operator+=(const Vector2D& other);
    Vector2D& operator-=(const Vector2D& other);
    Vector2D& operator*=(float scalar);
    Vector2D& operator/=(float scalar);

    Vector2D operator+(const Vector2D& other) const;
    Vector2D operator-(const Vector2D& other) const;
    Vector2D operator*(float scalar) const;
    Vector2D operator/(float scalar) const;

    friend Vector2D operator*(float scalar, const Vector2D& vec);

    bool operator==(const Vector2D& other) const;
    bool operator!=(const Vector2D& other) const;
    Vector2D operator-() const;

    Vector2D& normalize();
};

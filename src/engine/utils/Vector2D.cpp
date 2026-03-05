#include "engine/utils/Vector2D.h"

Vector2D& Vector2D::operator+=(const Vector2D& other) {
    x += other.x;
    y += other.y;
    return *this;
}

Vector2D& Vector2D::operator-=(const Vector2D& other) {
    x -= other.x;
    y -= other.y;
    return *this;
}

Vector2D& Vector2D::operator*=(const float scalar) {
    x *= scalar;
    y *= scalar;
    return *this;
}

Vector2D& Vector2D::operator/=(const float scalar) {
    if (scalar != 0.0f) {
        x /= scalar;
        y /= scalar;
    }
    return *this;
}

Vector2D Vector2D::operator+(const Vector2D& other) const {
    return Vector2D(x + other.x, y + other.y);
}

Vector2D Vector2D::operator-(const Vector2D& other) const {
    return Vector2D(x - other.x, y - other.y);
}

Vector2D Vector2D::operator*(const float scalar) const {
    return Vector2D(x * scalar, y * scalar);
}

Vector2D Vector2D::operator/(const float scalar) const {
    if (scalar == 0.0f) {
        return *this;
    }
    return Vector2D(x / scalar, y / scalar);
}

Vector2D operator*(const float scalar, const Vector2D& vec) {
    return Vector2D(vec.x * scalar, vec.y * scalar);
}

bool Vector2D::operator==(const Vector2D& other) const {
    return x == other.x && y == other.y;
}

bool Vector2D::operator!=(const Vector2D& other) const {
    return !(*this == other);
}

Vector2D Vector2D::operator-() const {
    return Vector2D(-x, -y);
}

Vector2D& Vector2D::normalize() {
    const float length = std::sqrt((x * x) + (y * y));
    if (length > 0.0f) {
        x /= length;
        y /= length;
    }

    return *this;
}

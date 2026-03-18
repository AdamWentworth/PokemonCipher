#pragma once

#include <cmath>
#include <iostream>
#include "utils/Vector2D.h"

// member operator function so we can use the Vector2D that we want to change on the left hand side
Vector2D& Vector2D::operator+=(const Vector2D &vec) {
    this->x += vec.x;
    this->y += vec.y;
    return *this;
}

// member operator function so we can use the vector 2D on the left hand side
Vector2D Vector2D::operator*(float scaler) const {
    return Vector2D (x * scaler, y * scaler);
};

// non-member operator function so we make use of putting the Vector2D on the right hand side
Vector2D operator*(float scaler, Vector2D& vec) {
    return Vector2D(vec.x * scaler, vec.y * scaler);
}

Vector2D &Vector2D::normalize() {
    // pythagorean theorem
    float length = std::sqrt(x * x + y * y);

    // if the length was 4.4
    // x = x / 4.4
    if (length > 0) {
        this-> x /= length;
        this-> y /= length;
    }

    return *this;
}

// return a new vector that is the difference
Vector2D Vector2D::operator-(const Vector2D& vec) const {
    return Vector2D(this->x - vec.x, this->y - vec.y);
}

// check if two vectors are equal
bool Vector2D::operator==(const Vector2D& vec) const {
    return this->x == vec.x && this->y == vec.y;
}

// return the negation of a vector (flips its direction)
Vector2D Vector2D::operator-() const {
    return Vector2D(-this->x, -this->y);
}

void Vector2D_DemoOperators() {
    Vector2D a(3, 4);
    Vector2D b(1, 2);

    std::cout << "a = (" << a.x << ", " << a.y << ")\n";
    std::cout << "b = (" << b.x << ", " << b.y << ")\n\n";

    a += b;  // a is now (4, 6)
    std::cout << "After a += b:\n";
    std::cout << "a = (" << a.x << ", " << a.y << ")  expected (4, 6)\n\n";

    Vector2D c = a - b;  // c is (3, 4)
    std::cout << "Vector2D c = a - b:\n";
    std::cout << "c = (" << c.x << ", " << c.y << ")  expected (3, 4)\n\n";

    Vector2D d = a * 2;  // d is (8, 12)
    std::cout << "Vector2D d = a * 2:\n";
    std::cout << "d = (" << d.x << ", " << d.y << ")  expected (8, 12)\n\n";

    Vector2D e = 2 * a;  // e is also (8, 12)
    std::cout << "Vector2D e = 2 * a:\n";
    std::cout << "e = (" << e.x << ", " << e.y << ")  expected (8, 12)\n\n";

    Vector2D f = -a;     // f is (-4, -6)
    std::cout << "Vector2D f = -a:\n";
    std::cout << "f = (" << f.x << ", " << f.y << ")  expected (-4, -6)\n\n";

    bool same = (a == b); // false
    std::cout << "bool same = (a == b):\n";
    std::cout << "same = " << (same ? "true" : "false") << "  expected false\n\n";
}

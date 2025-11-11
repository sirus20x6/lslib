#pragma once

#include <cmath>
#include <cstdint>

namespace gr2 {

// ============================================================================
// Vector Types
// ============================================================================

struct Vector2 {
    float x, y;

    Vector2() : x(0), y(0) {}
    Vector2(float x, float y) : x(x), y(y) {}

    float Length() const {
        return std::sqrt(x * x + y * y);
    }

    Vector2 operator+(const Vector2& other) const {
        return Vector2(x + other.x, y + other.y);
    }

    Vector2 operator-(const Vector2& other) const {
        return Vector2(x - other.x, y - other.y);
    }

    bool operator==(const Vector2& other) const {
        return x == other.x && y == other.y;
    }
};

struct Vector3 {
    float x, y, z;

    Vector3() : x(0), y(0), z(0) {}
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

    static Vector3 Zero() { return Vector3(0, 0, 0); }
    static Vector3 One() { return Vector3(1, 1, 1); }

    float Length() const {
        return std::sqrt(x * x + y * y + z * z);
    }

    Vector3 operator+(const Vector3& other) const {
        return Vector3(x + other.x, y + other.y, z + other.z);
    }

    Vector3 operator-(const Vector3& other) const {
        return Vector3(x - other.x, y - other.y, z - other.z);
    }

    Vector3 operator*(float scalar) const {
        return Vector3(x * scalar, y * scalar, z * scalar);
    }

    bool operator==(const Vector3& other) const {
        return x == other.x && y == other.y && z == other.z;
    }

    float& operator[](int index) {
        switch (index) {
            case 0: return x;
            case 1: return y;
            case 2: return z;
            default: throw std::out_of_range("Vector3 index out of range");
        }
    }

    const float& operator[](int index) const {
        switch (index) {
            case 0: return x;
            case 1: return y;
            case 2: return z;
            default: throw std::out_of_range("Vector3 index out of range");
        }
    }
};

struct Vector4 {
    float x, y, z, w;

    Vector4() : x(0), y(0), z(0), w(0) {}
    Vector4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

    bool operator==(const Vector4& other) const {
        return x == other.x && y == other.y && z == other.z && w == other.w;
    }
};

// ============================================================================
// Quaternion
// ============================================================================

struct Quaternion {
    float x, y, z, w;

    Quaternion() : x(0), y(0), z(0), w(1) {}
    Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

    static Quaternion Identity() { return Quaternion(0, 0, 0, 1); }

    float Length() const {
        return std::sqrt(x * x + y * y + z * z + w * w);
    }

    Quaternion Normalize() const {
        float len = Length();
        if (len < 0.0001f) return Identity();
        return Quaternion(x / len, y / len, z / len, w / len);
    }
};

// ============================================================================
// Matrix Types
// ============================================================================

struct Matrix3 {
    float m[3][3];

    Matrix3() {
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                m[i][j] = (i == j) ? 1.0f : 0.0f;
    }

    static Matrix3 Identity() {
        return Matrix3();
    }

    float* operator[](int row) {
        return m[row];
    }

    const float* operator[](int row) const {
        return m[row];
    }

    Vector3 GetDiagonal() const {
        return Vector3(m[0][0], m[1][1], m[2][2]);
    }
};

struct Matrix4 {
    float m[4][4];

    Matrix4() {
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                m[i][j] = (i == j) ? 1.0f : 0.0f;
    }

    static Matrix4 Identity() {
        return Matrix4();
    }

    float* operator[](int row) {
        return m[row];
    }

    const float* operator[](int row) const {
        return m[row];
    }

    static Matrix4 CreateFromQuaternion(const Quaternion& q) {
        Matrix4 result;
        float xx = q.x * q.x;
        float yy = q.y * q.y;
        float zz = q.z * q.z;
        float xy = q.x * q.y;
        float xz = q.x * q.z;
        float yz = q.y * q.z;
        float wx = q.w * q.x;
        float wy = q.w * q.y;
        float wz = q.w * q.z;

        result[0][0] = 1.0f - 2.0f * (yy + zz);
        result[0][1] = 2.0f * (xy + wz);
        result[0][2] = 2.0f * (xz - wy);
        result[0][3] = 0.0f;

        result[1][0] = 2.0f * (xy - wz);
        result[1][1] = 1.0f - 2.0f * (xx + zz);
        result[1][2] = 2.0f * (yz + wx);
        result[1][3] = 0.0f;

        result[2][0] = 2.0f * (xz + wy);
        result[2][1] = 2.0f * (yz - wx);
        result[2][2] = 1.0f - 2.0f * (xx + yy);
        result[2][3] = 0.0f;

        result[3][0] = 0.0f;
        result[3][1] = 0.0f;
        result[3][2] = 0.0f;
        result[3][3] = 1.0f;

        return result;
    }

    static Matrix4 CreateTranslation(const Vector3& translation) {
        Matrix4 result;
        result[3][0] = translation.x;
        result[3][1] = translation.y;
        result[3][2] = translation.z;
        return result;
    }

    Matrix4 operator*(const Matrix4& other) const {
        Matrix4 result;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                result[i][j] = 0;
                for (int k = 0; k < 4; k++) {
                    result[i][j] += m[i][k] * other.m[k][j];
                }
            }
        }
        return result;
    }
};

} // namespace gr2

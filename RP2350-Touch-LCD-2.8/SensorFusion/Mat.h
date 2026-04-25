#pragma once
#include <cmath>
#include <utility>

namespace sf {

template <int R, int C>
struct Mat {
    float data[R][C] = {};

    Mat() = default;

    float& operator()(int r, int c) { return data[r][c]; }
    const float& operator()(int r, int c) const { return data[r][c]; }

    static Mat zeros() { return Mat{}; }

    static Mat identity() {
        static_assert(R == C, "identity requires square matrix");
        Mat m;
        for (int i = 0; i < R; i++) m.data[i][i] = 1.0f;
        return m;
    }

    // 3x3 skew-symmetric matrix from vector components
    static Mat<3, 3> skew(float x, float y, float z) {
        Mat<3, 3> m;
        m.data[0][1] = -z; m.data[0][2] =  y;
        m.data[1][0] =  z; m.data[1][2] = -x;
        m.data[2][0] = -y; m.data[2][1] =  x;
        return m;
    }

    Mat<C, R> transpose() const {
        Mat<C, R> t;
        for (int i = 0; i < R; i++)
            for (int j = 0; j < C; j++)
                t.data[j][i] = data[i][j];
        return t;
    }

    Mat operator+(const Mat& b) const {
        Mat r;
        for (int i = 0; i < R; i++)
            for (int j = 0; j < C; j++)
                r.data[i][j] = data[i][j] + b.data[i][j];
        return r;
    }

    Mat operator-(const Mat& b) const {
        Mat r;
        for (int i = 0; i < R; i++)
            for (int j = 0; j < C; j++)
                r.data[i][j] = data[i][j] - b.data[i][j];
        return r;
    }

    Mat operator*(float s) const {
        Mat r;
        for (int i = 0; i < R; i++)
            for (int j = 0; j < C; j++)
                r.data[i][j] = data[i][j] * s;
        return r;
    }

    template <int C2>
    Mat<R, C2> operator*(const Mat<C, C2>& b) const {
        Mat<R, C2> r;
        for (int i = 0; i < R; i++)
            for (int j = 0; j < C2; j++) {
                float sum = 0.0f;
                for (int k = 0; k < C; k++)
                    sum += data[i][k] * b.data[k][j];
                r.data[i][j] = sum;
            }
        return r;
    }

    // Gauss-Jordan inverse with partial pivoting (square matrices only)
    Mat inverse() const {
        static_assert(R == C, "inverse requires square matrix");
        Mat aug;
        Mat work = *this;
        for (int i = 0; i < R; i++) aug.data[i][i] = 1.0f;

        for (int col = 0; col < R; col++) {
            int maxRow = col;
            float maxVal = std::fabs(work.data[col][col]);
            for (int row = col + 1; row < R; row++) {
                float v = std::fabs(work.data[row][col]);
                if (v > maxVal) { maxVal = v; maxRow = row; }
            }
            if (maxRow != col) {
                for (int j = 0; j < R; j++) {
                    std::swap(work.data[col][j], work.data[maxRow][j]);
                    std::swap(aug.data[col][j], aug.data[maxRow][j]);
                }
            }
            float pivot = work.data[col][col];
            if (std::fabs(pivot) < 1e-12f) continue;
            float inv = 1.0f / pivot;
            for (int j = 0; j < R; j++) {
                work.data[col][j] *= inv;
                aug.data[col][j] *= inv;
            }
            for (int row = 0; row < R; row++) {
                if (row == col) continue;
                float factor = work.data[row][col];
                for (int j = 0; j < R; j++) {
                    work.data[row][j] -= factor * work.data[col][j];
                    aug.data[row][j] -= factor * aug.data[col][j];
                }
            }
        }
        return aug;
    }

    template <int BR, int BC>
    void setBlock(int startR, int startC, const Mat<BR, BC>& block) {
        for (int i = 0; i < BR; i++)
            for (int j = 0; j < BC; j++)
                data[startR + i][startC + j] = block.data[i][j];
    }

    template <int BR, int BC>
    Mat<BR, BC> block(int startR, int startC) const {
        Mat<BR, BC> b;
        for (int i = 0; i < BR; i++)
            for (int j = 0; j < BC; j++)
                b.data[i][j] = data[startR + i][startC + j];
        return b;
    }

    void symmetrize() {
        static_assert(R == C, "symmetrize requires square matrix");
        for (int i = 0; i < R; i++)
            for (int j = i + 1; j < C; j++)
                data[i][j] = data[j][i] = 0.5f * (data[i][j] + data[j][i]);
    }
};

template <int R, int C>
Mat<R, C> operator*(float s, const Mat<R, C>& m) { return m * s; }

using Mat3  = Mat<3, 3>;
using Mat6  = Mat<6, 6>;
using Mat15 = Mat<15, 15>;

} // namespace sf

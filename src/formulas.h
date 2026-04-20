#pragma once
#include <iostream>
#include <vector>
#include <cmath>
using uchar = unsigned char;

float findDistance(float x0, float y0, float x1, float y1)
{
    return sqrt(pow(x0 - x1, 2) + pow(y0 - y1, 2));
}

void inverseMatrix3x3(float* H)
{
    float det = H[0] * (H[4] * H[8] - H[5] * H[7]) -
        H[1] * (H[3] * H[8] - H[5] * H[6]) +
        H[2] * (H[3] * H[7] - H[4] * H[6]);

    float invDet = 1.0f / det;

    std::vector<float> H_inv(9, 0);

    // Row 1
    H_inv[0] = (H[4] * H[8] - H[5] * H[7]) * invDet;
    H_inv[1] = (H[2] * H[7] - H[1] * H[8]) * invDet;
    H_inv[2] = (H[1] * H[5] - H[2] * H[4]) * invDet;

    // Row 2
    H_inv[3] = (H[5] * H[6] - H[3] * H[8]) * invDet;
    H_inv[4] = (H[0] * H[8] - H[2] * H[6]) * invDet;
    H_inv[5] = (H[2] * H[3] - H[0] * H[5]) * invDet;

    // Row 3
    H_inv[6] = (H[3] * H[7] - H[4] * H[6]) * invDet;
    H_inv[7] = (H[1] * H[6] - H[0] * H[7]) * invDet;
    H_inv[8] = (H[0] * H[4] - H[1] * H[3]) * invDet;

    float epsilon = 1e-4;
    for (int i = 0; i < 9; i++)
    {
        if (abs(H_inv[i]) < epsilon) {
            H_inv[i] = 0.0f;
        }
        if (abs(H_inv[i] - round(H_inv[i])) < epsilon) {
            H_inv[i] = round(H_inv[i]);
        }

        H[i] = H_inv[i];
    }

}

float calculateWilkinson(float* A, int start, int end) {
    //Formül tam olmamasýna raðmen çalýþýyor, neden bilmiyorum
    // We look at the bottom 2x2 of the active block
    // [ d[n-1]   e[n-1] ]
    // [   0      d[n]   ]

    float d1 = A[(end - 1) * 9 + (end - 1)]; // Bottom-left of 2x2
    float e1 = A[(end - 1) * 9 + end];       // Top-right of 2x2
    float d2 = A[end * 9 + end];             // Bottom-right of 2x2

    // This is the standard formula for the Wilkinson Shift
    float d = (d1 * d1 - d2 * d2) / 2.0f;
    float sign_d = (d >= 0) ? 1.0f : -1.0f;
    float mu = d2 * d2 - (e1 * e1) / (d + sign_d * sqrt(d * d + e1 * e1));

    return mu;
}

void calculateGivens(float& r, float& c, float& s, float first, float second)//QR
{
    if (second == 0) {
        c = 1; s = 0; r = first;
        return;
    }

    r = sqrt(first * first + second * second);
    c = first / r;
    s = -second / r;
}

void QR(float* A, int column, int row, float* VT)
{
    //calculate wilkinson shift
    //chase the bulge till end(bottom right corner)
    //calculate the shift again and it goes like this
    //if you have a diagonal zero, you must first zero its super diagonal(right to the diagonal)
    //it creates separate blocks that needs to be calculated separately
    //find out how to calculate them separately

    float r = 0, c = 0, s = 0;

    for (int i = 0; i < row; i++)// diagonal zero control
    {
        if (A[i * column + i] == 0)// if the diagonal is zero
        {
            for (int j = i + 1; j < row; j++)//since superdiagonal is right of diagonal it's i + 1
            {
                calculateGivens(r, c, s, A[j * column + j], A[i * column + j]);
                for (int k = i; k < column; k++)
                {
                    float oldA = A[i * column + k];//because of the order we calculate, we only need oldA
                    A[i * column + k] = c * oldA + s * A[j * column + k];//row which had zero
                    A[j * column + k] = -s * oldA + c * A[j * column + k];//row which the bulge is in now
                }
            }
        }
    }

    int end = 8;
    while (end > 0)
    {
        while (end > 0 && abs(A[(end - 1) * column + end]) < 1e-9) {
            end--;
        }
        if (end == 0) break;

        // Find the start of the current non-zero block
        int start = end - 1;
        while (start > 0 && abs(A[(start - 1) * column + start]) > 1e-9) {
            start--;
        }

        float shift = calculateWilkinson(A, start, end);

        float y = A[start * column + start] * A[start * column + start] - shift;
        float z = A[start * column + start] * A[start * column + start + 1];

        calculateGivens(r, c, s, y, z);
        for (int k = 0; k < 9; k++) { // Full length for A columns and VT rows
            float a1 = A[k * column + start];
            float a2 = A[k * column + start + 1];
            A[k * column + start] = c * a1 - s * a2;
            A[k * column + start + 1] = s * a1 + c * a2;

            float v1 = VT[start * 9 + k];
            float v2 = VT[(start + 1) * 9 + k];
            VT[start * 9 + k] = c * v1 - s * v2;
            VT[(start + 1) * 9 + k] = s * v1 + c * v2;
        }

        for (int j = start; j < end; j++)//bulge chasing
        {
            //Row
            calculateGivens(r, c, s, A[j * column + j], A[(j + 1) * column + j]);
            for (int k = j; k < column; k++) {
                float r1 = A[j * column + k];
                float r2 = A[(j + 1) * column + k];
                A[j * column + k] = c * r1 - s * r2;
                A[(j + 1) * column + k] = s * r1 + c * r2;
            }

            //Column
            // Only if we haven't reached the end of the block
            if (j < end - 1) {
                calculateGivens(r, c, s, A[j * column + j + 1], A[j * column + j + 2]);

                for (int k = 0; k < 9; k++) {
                    // Update Matrix A Columns
                    float c1 = A[k * column + j + 1];
                    float c2 = A[k * column + j + 2];
                    A[k * column + j + 1] = c * c1 - s * c2;
                    A[k * column + j + 2] = s * c1 + c * c2;

                    // Update Matrix VT Rows
                    float v1 = VT[(j + 1) * 9 + k];
                    float v2 = VT[(j + 2) * 9 + k];
                    VT[(j + 1) * 9 + k] = c * v1 - s * v2;
                    VT[(j + 2) * 9 + k] = s * v1 + c * v2;
                }
            }
        }
    }
}

void triangulate(float* A, int column, int row, float* matrix, int offset, int i)
{
    for (i; i < row - offset; i++)//sütun sayýsý kadar
    {
        int vecStart = i + offset;

        std::vector<float> v(row - vecStart, 0);//Her sütunla birlikte 1 aþaðý indiðimiz için v row - vecStart boyutunda

        float x = 0;
        for (int j = vecStart; j < row; j++)
        {
            v[j - vecStart] = A[j * column + i];
            x += A[j * column + i] * A[j * column + i];
        }

        if (A[vecStart * column + i] > 0) x = -sqrt(x);
        else x = sqrt(x);
        v[0] -= x;

        float vNorm = 0;
        for (int j = 0; j < row - vecStart; j++)
            vNorm += v[j] * v[j];

        if (vNorm == 0) vNorm = 1; //when it's 0 it makes P undefined

        //A =  A - v(beta * v^T * A)
        float beta = 2.0f / vNorm;

        for (int j = i; j < column; j++)//A yý güncelleme
        {
            float sum = 0.0f;
            for (int k = vecStart; k < row; k++)
                sum += v[k - vecStart] * A[k * column + j];

            sum *= beta;

            for (int k = vecStart; k < row; k++)
                A[k * column + j] -= sum * v[k - vecStart];
        }

        for (int j = 0; j < row; j++)//VT ve U nun boyutu row a baðlý bu yüzden hepsi row
        {
            float sum = 0.0f;
            for (int k = vecStart; k < row; k++)
                sum += v[k - vecStart] * matrix[k * row + j];

            sum *= beta;

            for (int k = vecStart; k < row; k++)
                matrix[k * row + j] -= sum * v[k - vecStart];
        }
    }
}

void computeSVD(float* A, float* A9x9, float* u, float* v)
{
    //Bidiagonalization
    float newA[72] = { 0 };

    for (int i = 0; i < 9; i++)
        v[10 * i] = 1;

    for (int adim = 0; adim < 8; adim++)//HOUSEHOLD BIDIAGONALIZATION
    {
        triangulate(A, 9, 8, u, 0, adim);

        std::fill(std::begin(newA), std::end(newA), 0.0f);
        for (int i = 0; i < 8; i++)//Transpose A to use the same triangulate function
        {
            for (int j = 0; j < 9; j++)
                newA[j * 8 + i] = A[i * 9 + j];
        }

        //Right zeroing
        triangulate(newA, 8, 9, v, 1, adim);

        for (int i = 0; i < 8; i++)//Transpose again
        {
            for (int j = 0; j < 9; j++)
                A[i * 9 + j] = newA[j * 8 + i];
        }
    }

    for (int i = 0; i < 9; i++)//Removing float errors
    {
        for (int j = 0; j < 8; j++)
        {
            if (j == i || j == i + 1) continue;

            if (std::abs(newA[i * 8 + j]) < 0.01f)
                newA[i * 8 + j] = 0;
        }
    }

    for (int i = 0; i < 72; i++)
        A9x9[i] = A[i];

    QR(A9x9, 9, 9, v);
}

void computeHomography(float* H, std::vector<float> src, std::vector<float> dst)
{
    float A[9 * 8] = { 0 };//Decide if we really need A
    float A9x9[81] = { 0 };
    float u[8 * 8] = { 0 };//It should be 8x8 but since i dont need it, im just doing it like this to not break my code
    float v[9 * 9] = { 0 };

    for (int i = 0; i < 4; i++)//noktalarý ver
    {
        A[i * 18 + 0] = -src[i * 2 + 0];
        A[i * 18 + 1] = -src[i * 2 + 1];
        A[i * 18 + 2] = -1;
        A[i * 18 + 3] = 0;
        A[i * 18 + 4] = 0;
        A[i * 18 + 5] = 0;
        A[i * 18 + 6] = src[i * 2 + 0] * dst[i * 2 + 0];
        A[i * 18 + 7] = src[i * 2 + 1] * dst[i * 2 + 0];
        A[i * 18 + 8] = dst[i * 2 + 0];
        A[i * 18 + 9] = 0;
        A[i * 18 + 10] = 0;
        A[i * 18 + 11] = 0;
        A[i * 18 + 12] = -src[i * 2 + 0];
        A[i * 18 + 13] = -src[i * 2 + 1];
        A[i * 18 + 14] = -1;
        A[i * 18 + 15] = src[i * 2 + 0] * dst[i * 2 + 1];
        A[i * 18 + 16] = src[i * 2 + 1] * dst[i * 2 + 1];
        A[i * 18 + 17] = dst[i * 2 + 1];
    }

    computeSVD(A, A9x9, u, v);

    int smallestIndex = 0;
    for (int i = 0; i < 9; i++)
    {
        if ((abs(A9x9[i * 10]) < abs(A9x9[smallestIndex * 10])))
            smallestIndex = i;
    }
    std::cout << smallestIndex << std::endl;

    for (int i = 0; i < 9; i++)
        H[i] = v[smallestIndex * 9 + i];

}

void getPixelColor(const uchar* inputImage, int x, int y, int width, uchar color[3])
{
    int index = (y * width + x) * 3;
    for (int i = 0; i < 3; i++)
        color[i] = inputImage[index + i];
}

void mapImage(const uchar* inputImage, std::vector<uchar>& outputImage, int inputWidth, int inputHeight, int destWidth, int destHeight, float* H)//H = H^-1
{
    uchar tl[3], tr[3], bl[3], br[3];
    float topMix, bottomMix, finalColor;

    for (int y_dest = 0; y_dest < destHeight; y_dest++)
    {
        //we precalculate these for optimization
        float h_u_y = H[1] * y_dest + H[2];
        float h_v_y = H[4] * y_dest + H[5];
        float h_w_y = H[7] * y_dest + H[8];

        for (int x_dest = 0; x_dest < destWidth; x_dest++)
        {
            float u = H[0] * x_dest + h_u_y;
            float v = H[3] * x_dest + h_v_y;
            float w = H[6] * x_dest + h_w_y;

            float x_src = u / w;
            float y_src = v / w;

            int dest_idx = (y_dest * destWidth + x_dest) * 3;

            if (x_src < 0 || x_src >= inputWidth - 1 || y_src < 0 || y_src >= inputHeight - 1)
            {
                for (int k = 0; k < 3; k++)
                    outputImage[dest_idx + k] = 0;
                continue;
            }

            int x_floor = (int)x_src;      // Integer part (left)
            int y_floor = (int)y_src;      // Integer part (top)
            int x_ceil = x_floor + 1;      // Right neighbor
            int y_ceil = y_floor + 1;      // Bottom neighbor

            float x_weight = x_src - x_floor;
            float y_weight = y_src - y_floor;

            //Get the colors of the 4 neighbors
            //i can change these to normal array, idk if it makes any difference tho
            //it definitely did
            getPixelColor(inputImage, x_floor, y_floor, inputWidth, tl); 
            getPixelColor(inputImage, x_ceil, y_floor, inputWidth, tr); 
            getPixelColor(inputImage, x_floor, y_ceil, inputWidth, bl); 
            getPixelColor(inputImage, x_ceil, y_ceil, inputWidth, br); 

            // Interpolate Top pair and Bottom pair and get the final color
            for (int k = 0; k < 3; k++)
            {
                topMix = tl[k] * (1.0f - x_weight) + tr[k] * x_weight;
                bottomMix = bl[k] * (1.0f - x_weight) + br[k] * x_weight;

                finalColor = topMix * (1.0f - y_weight) + bottomMix * y_weight;

                outputImage[dest_idx + k] = finalColor;
            }
        }
    }
}
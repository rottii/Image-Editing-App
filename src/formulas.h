#pragma once
#include <iostream>
#include <vector>
#include <cmath>
#include <thread>
#include <utility> // Required for std::pair
using uchar = uint8_t;
const double PI = 3.14159265358979323846;

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

    H_inv[0] = (H[4] * H[8] - H[5] * H[7]) * invDet;
    H_inv[1] = (H[2] * H[7] - H[1] * H[8]) * invDet;
    H_inv[2] = (H[1] * H[5] - H[2] * H[4]) * invDet;

    H_inv[3] = (H[5] * H[6] - H[3] * H[8]) * invDet;
    H_inv[4] = (H[0] * H[8] - H[2] * H[6]) * invDet;
    H_inv[5] = (H[2] * H[3] - H[0] * H[5]) * invDet;

    H_inv[6] = (H[3] * H[7] - H[4] * H[6]) * invDet;
    H_inv[7] = (H[1] * H[6] - H[0] * H[7]) * invDet;
    H_inv[8] = (H[0] * H[4] - H[1] * H[3]) * invDet;

    float epsilon = 1e-4;
    for (int i = 0; i < 9; i++)
    {
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

void computeHomography(float* H, const float* src, const float* dst)
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

    for (int i = 0; i < 9; i++)
        H[i] = v[smallestIndex * 9 + i];

}

void mapImage(const uchar* inputImage, std::vector<uchar>& outputImage, int inputWidth, int inputHeight, int destWidth, int destHeight, float* H)//H = H^-1
{
    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 4;

    int rowsPerThread = destHeight / numThreads;
    std::vector<std::thread> threads;

    auto worker = [&](int startY, int endY)
        {
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

                    int dest_idx = (y_dest * destWidth + x_dest) * 4;

                    if (x_src < -1.0f || x_src >= inputWidth || y_src < -1.0f || y_src >= inputHeight)
                    {
                        for (int k = 0; k < 4; k++)
                            outputImage[dest_idx + k] = 0;
                        continue;
                    }

                    x_src = std::max(0.0f, std::min(x_src, (float)inputWidth - 1.001f));
                    y_src = std::max(0.0f, std::min(y_src, (float)inputHeight - 1.001f));

                    int x_floor = (int)x_src;
                    int y_floor = (int)y_src;
                    int x_ceil = x_floor + 1;
                    int y_ceil = y_floor + 1;

                    float x_weight = x_src - x_floor;
                    float y_weight = y_src - y_floor;

                    int idx_tl = (y_floor * inputWidth + x_floor) * 4;
                    int idx_tr = (y_floor * inputWidth + x_ceil) * 4;
                    int idx_bl = (y_ceil * inputWidth + x_floor) * 4;
                    int idx_br = (y_ceil * inputWidth + x_ceil) * 4;

                    // Interpolate Top pair and Bottom pair and get the final color
                    for (int k = 0; k < 3; k++)
                    {
                        topMix = inputImage[idx_tl + k] * (1.0f - x_weight) + inputImage[idx_tr + k] * x_weight;
                        bottomMix = inputImage[idx_bl + k] * (1.0f - x_weight) + inputImage[idx_br + k] * x_weight;

                        finalColor = topMix * (1.0f - y_weight) + bottomMix * y_weight;

                        outputImage[dest_idx + k] = (uchar)finalColor;
                    }
                    outputImage[dest_idx + 3] = 255;//channel A
                }
            }
        };

    for (unsigned int i = 0; i < numThreads; i++)
    {
        int startY = i * rowsPerThread;

        int endY = (i == numThreads - 1) ? destHeight : startY + rowsPerThread;
        threads.push_back(std::thread(worker, startY, endY));
    }

    for (auto& t : threads)
    {
        t.join();
    }
}

void warpImage(const uchar* inputImage, std::vector<uchar>& outputImage, float* inputPoints, int inputWidth, int inputHeight, int& outWidth, int& outHeight)
{
    outWidth = (int)round(std::max(sqrt(pow(inputPoints[0] - inputPoints[2], 2) + pow(inputPoints[1] - inputPoints[3], 2)),
        sqrt(pow(inputPoints[4] - inputPoints[6], 2) + pow(inputPoints[5] - inputPoints[7], 2))));
    outHeight = (int)round(std::max(sqrt(pow(inputPoints[0] - inputPoints[6], 2) + pow(inputPoints[1] - inputPoints[7], 2)),
        sqrt(pow(inputPoints[2] - inputPoints[4], 2) + pow(inputPoints[3] - inputPoints[5], 2))));

    float outputPoints[8] = { 0, 0, (float)outWidth, 0, (float)outWidth, (float)outHeight, 0, (float)outHeight };

    // ---- Hartley Normalizasyonu ----

    float cxSrc = 0, cySrc = 0;
    for (int i = 0; i < 4; i++) { cxSrc += inputPoints[i * 2]; cySrc += inputPoints[i * 2 + 1]; }
    cxSrc /= 4; cySrc /= 4;

    // 2. Hedef noktalarýn aðýrlýk merkezini bul
    float cxDst = 0, cyDst = 0;
    for (int i = 0; i < 4; i++) { cxDst += outputPoints[i * 2]; cyDst += outputPoints[i * 2 + 1]; }
    cxDst /= 4; cyDst /= 4;

    // 3. Kaynak noktalarýn ortalama uzaklýðýný bul
    float dSrc = 0;
    for (int i = 0; i < 4; i++)
    {
        float dx = inputPoints[i * 2] - cxSrc;
        float dy = inputPoints[i * 2 + 1] - cySrc;
        dSrc += sqrt(dx * dx + dy * dy);
    }
    dSrc /= 4;
    float sSrc = sqrt(2.0f) / dSrc;

    // 4. Hedef noktalarýn ortalama uzaklýðýný bul
    float dDst = 0;
    for (int i = 0; i < 4; i++)
    {
        float dx = outputPoints[i * 2] - cxDst;
        float dy = outputPoints[i * 2 + 1] - cyDst;
        dDst += sqrt(dx * dx + dy * dy);
    }
    dDst /= 4;
    float sDst = sqrt(2.0f) / dDst;

    // 5. Noktalarý normalize et
    float localInput[8], localOutput[8];
    for (int i = 0; i < 4; i++)
    {
        localInput[i * 2] = sSrc * (inputPoints[i * 2] - cxSrc);
        localInput[i * 2 + 1] = sSrc * (inputPoints[i * 2 + 1] - cySrc);
        localOutput[i * 2] = sDst * (outputPoints[i * 2] - cxDst);
        localOutput[i * 2 + 1] = sDst * (outputPoints[i * 2 + 1] - cyDst);
    }

    // 6. Normalize koordinatlarla H'yi hesapla
    float H[9] = { 0 };
    computeHomography(H, localInput, localOutput);

    // ---- H = T_dst^-1 * H' * T_src ----
    // T_src = | sSrc   0    -sSrc*cxSrc |
    //         |  0    sSrc  -sSrc*cySrc |
    //         |  0     0         1      |
    //
    // T_dst^-1 = | 1/sDst    0      cxDst |
    //            |   0    1/sDst    cyDst  |
    //            |   0       0        1   |

    float TSrc[9] = {sSrc,  0, -sSrc * cxSrc, 0, sSrc, -sSrc * cySrc, 0, 0, 1};
    float TDstInv[9] = {1/sDst, 0, cxDst, 0, 1/sDst, cyDst, 0, 0, 1};

    // H = TDstInv * H * TSrc
    // Önce tmp = H * TSrc
    float tmp[9] = { 0 };
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            for (int k = 0; k < 3; k++)
                tmp[i * 3 + j] += H[i * 3 + k] * TSrc[k * 3 + j];

    // Sonra H = TDstInv * tmp
    float Hfinal[9] = { 0 };
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            for (int k = 0; k < 3; k++)
                Hfinal[i * 3 + j] += TDstInv[i * 3 + k] * tmp[k * 3 + j];

    // H[8]'e göre normalize et
    for (int i = 0; i < 9; i++)
        Hfinal[i] /= Hfinal[8];

    inverseMatrix3x3(Hfinal);
    outputImage.resize(outWidth * outHeight * 4);
    mapImage(inputImage, outputImage, inputWidth, inputHeight, outWidth, outHeight, Hfinal);
}

void findCorners(const uchar* inputImage, std::vector<uchar>& outputImage, int inputWidth, int inputHeight)
{
    //kenarlara boþluk ekle ki görüntü küçülmesin
    //bunu ekle
    /*double window[9] = {
     1.0 / 16.0, 2.0 / 16.0, 1.0 / 16.0 ,
     2.0 / 16.0, 4.0 / 16.0, 2.0 / 16.0 ,
     1.0 / 16.0, 2.0 / 16.0, 1.0 / 16.0  };*/

    int sobelX[9] = { -1, 0, 1, -2, 0, 2, -1, 0, 1 };
    int sobelY[9] = { -1, -2, -1, 0, 0, 0, 1, 2, 1 };

    std::vector<int> Ix((inputHeight - 2) * (inputWidth - 2), 0);
    std::vector<int> Iy((inputHeight - 2) * (inputWidth - 2), 0);

    int gx, gy;

    for (int i = 0; i < inputHeight - 2; i++)
    {
        for (int j = 0; j < inputWidth - 2; j++)
        {
            gx = 0;
            gy = 0;
            for (int k = 0; k < 3; k++)
            {
                for (int l = 0; l < 3; l++)
                {
                    int idx = ((k + i) * inputWidth + l + j) * 4;

                    int gray = (inputImage[idx] + inputImage[idx + 1] + inputImage[idx + 2]) / 3;

                    gx += gray * sobelY[k * 3 + l];
                    gy += gray * sobelX[k * 3 + l];
                }
            }

            Ix[i * (inputWidth - 2) + j] = gx;
            Iy[i * (inputWidth - 2) + j] = gy;
        }
    }

    outputImage.resize((inputHeight - 2) * (inputWidth - 2) * 4);

    double maxR = 0;
    std::vector<double> R((inputHeight - 2) * (inputWidth - 2), 0.0);

    for (int i = 1; i < inputHeight - 3; i++)
    {
        for (int j = 1; j < inputWidth - 3; j++)
        {
            double m[3] = { 0 };

            for (int k = -1; k <= 1; k++)
            {
                for (int l = -1; l <= 1; l++)
                {
                    int idx = (k + i) * (inputWidth - 2) + l + j;

                    m[0] +=  Ix[idx] * Ix[idx];
                    m[1] += Ix[idx] * Iy[idx]; //sað üst ve sol alt
                    m[2] += Iy[idx] * Iy[idx];
                }
            }

            double r = ((m[0] + m[2]) - std::sqrt((m[0] - m[2]) * (m[0] - m[2]) + 4.0 * m[1] * m[1])) / 2.0;

            R[i * (inputWidth - 2) + j] = r;
            if (r > maxR) maxR = r;
        }
    }

    //Non maxima suppression yapýp köþeleri gösterme
    for (int i = 1; i < inputHeight - 3; i++)
    {
        for (int j = 1; j < inputWidth - 3; j++)
        {
            int idx = i * (inputWidth - 2) + j;
            double r = R[i * (inputWidth - 2) + j];

            if (R[idx] <= 0 || R[idx] < (maxR * 0.05))
            {
                outputImage[idx * 4] = 0;
                outputImage[idx * 4 + 1] = 0;
                outputImage[idx * 4 + 2] = 0;
                outputImage[idx * 4 + 3] = 255;
                continue;
            }

            bool isLocalMax = true;

            for (int k = -1; k <= 1; k++)
            {
                for (int l = -1; l <= 1; l++)
                {
                    if (k == 0 && l == 0) continue;

                    int neighborIdx = (i + k) * (inputWidth - 2) + (j + l);

                    if (R[idx] <= R[neighborIdx])
                    {
                        isLocalMax = false;
                        break; 
                    }
                }
                if (!isLocalMax) break;
            }

            if (isLocalMax)
            {
                uchar color = (uchar)((R[idx] / maxR) * 255.0);
                outputImage[idx * 4] = color;
                outputImage[idx * 4 + 1] = color;
                outputImage[idx * 4 + 2] = color;
                outputImage[idx * 4 + 3] = 255;
            }
            else
            {
                outputImage[idx * 4] = 0;
                outputImage[idx * 4 + 1] = 0;
                outputImage[idx * 4 + 2] = 0;
                outputImage[idx * 4 + 3] = 255;
            }
        }
    }
}

void findEdges(const uchar* inputImage, std::vector<uchar>& outputImage, int inputWidth, int inputHeight)
{
    int outW = inputWidth;
    int outH = inputHeight;

    // 1. Allocate the full original size (no need to pre-fill with black anymore)
    std::vector<uchar> blurredImage(outH * outW * 4, 0);

    double gaussianKernel[25] = {
            1.0 / 256.0,  4.0 / 256.0,  6.0 / 256.0,  4.0 / 256.0, 1.0 / 256.0,
            4.0 / 256.0, 16.0 / 256.0, 24.0 / 256.0, 16.0 / 256.0, 4.0 / 256.0,
            6.0 / 256.0, 24.0 / 256.0, 36.0 / 256.0, 24.0 / 256.0, 6.0 / 256.0,
            4.0 / 256.0, 16.0 / 256.0, 24.0 / 256.0, 16.0 / 256.0, 4.0 / 256.0,
            1.0 / 256.0,  4.0 / 256.0,  6.0 / 256.0,  4.0 / 256.0, 1.0 / 256.0
    };

    // 2. Loop over the ENTIRE image
    for (int i = 0; i < outH; i++)
    {
        for (int j = 0; j < outW; j++)
        {
            double sum = 0;

            // 3. Loop from -2 to 2 to cover the 5x5 grid
            for (int k = -2; k <= 2; k++)
            {
                for (int l = -2; l <= 2; l++)
                {
                    // CLAMP TO EDGE
                    int clampY = std::max(0, std::min(outH - 1, i + k));
                    int clampX = std::max(0, std::min(outW - 1, j + l));

                    int idx = (clampY * inputWidth + clampX) * 4;

                    int gray = (inputImage[idx] + inputImage[idx + 1] + inputImage[idx + 2]) / 3;

                    // 4. Map the -2 to 2 range to the 0 to 24 kernel array index
                    int kernelIdx = (k + 2) * 5 + (l + 2);

                    sum += gray * gaussianKernel[kernelIdx];
                }
            }

            if (sum > 255) sum = 255;
            if (sum < 0) sum = 0;

            uchar sumInt = static_cast<uchar>(sum);

            // Write safely to the output
            int outIdx = (i * outW + j) * 4;
            blurredImage[outIdx] = sumInt;
            blurredImage[outIdx + 1] = sumInt;
            blurredImage[outIdx + 2] = sumInt;
            blurredImage[outIdx + 3] = 255;
        }
    }
    

    int sobelX[9] = { -1, 0, 1, -2, 0, 2, -1, 0, 1 };
    int sobelY[9] = { -1, -2, -1, 0, 0, 0, 1, 2, 1 };

    std::vector<int> magMap(outH * outW, 0);
    std::vector<uchar> angleImage(outH * outW, 0);
    outputImage.assign(outH * outW * 4, 0);

    for (int i = 0; i < outH; i++)
    {
        for (int j = 0; j < outW; j++)
        {
            int gx = 0;
            int gy = 0;

            // 2. Loop from -1 to 1 to intuitively center the kernel on the current pixel (i, j)
            for (int k = -1; k <= 1; k++)
            {
                for (int l = -1; l <= 1; l++)
                {
                    // 3. Clamp coordinates to prevent out-of-bounds crashes at the extreme edges
                    int clampY = std::max(0, std::min(outH - 1, i + k));
                    int clampX = std::max(0, std::min(outW - 1, j + l));

                    int idx = (clampY * outW + clampX) * 4;

                    // Read from your blurred image buffer! 
                    // Since the blur output is already grayscale, we just read the red channel.
                    int gray = blurredImage[idx];

                    // Map the -1 to 1 coordinates safely to your 0 to 8 array indices
                    int kernelIdx = (k + 1) * 3 + (l + 1);

                    // Fixed: gx maps to sobelX, gy maps to sobelY
                    gx += gray * sobelX[kernelIdx];
                    gy += gray * sobelY[kernelIdx];
                }
            }

            int g = std::sqrt(gx * gx + gy * gy);
            double angle = atan2(gy, gx) * 180.0 / PI;

            if ((angle >= -22.5 && angle < 22.5) || (angle > 157.5 || angle <= -157.5))
                angle = 0;
            else if ((angle >= 22.5 && angle < 67.5) || (angle > -157.5 && angle <= -112.5))
                angle = 45;
            else if ((angle >= 67.5 && angle < 112.5) || (angle > -112.5 && angle <= -67.5))
                angle = 90;
            else angle = 135;

            if (g < 0) g = 0;
            if (g > 255) g = 255;

            // 4. Save to our full-sized reference maps for the NMS phase
            int flatIdx = i * outW + j;
            magMap[flatIdx] = g;
            angleImage[flatIdx] = angle;

            // 5. Write to outputImage (Optional, useful if you want to see the image before NMS)
            int outIdx = flatIdx * 4;
            outputImage[outIdx] = angle;
            outputImage[outIdx + 1] = angle;
            outputImage[outIdx + 2] = angle;
            outputImage[outIdx + 3] = 255;
        }
    }

    //Non Maxima Suppression
    for (int i = 0; i < outH; i++) {
        for (int j = 0; j < outW; j++) {

            int flatIdx = i * outW + j;
            int currentMag = magMap[flatIdx];
            int currentAngle = angleImage[flatIdx];

            int neighbor1 = 0;
            int neighbor2 = 0;

            // Clean boundary checks: Only read a neighbor if it actually exists inside the image bounds.
            // If it's outside the bounds, it stays 0.
            if (currentAngle == 0) {
                // Horizontal gradient (Vertical edge) -> Check Left & Right
                if (j > 0) neighbor1 = magMap[i * outW + (j - 1)];
                if (j < outW - 1) neighbor2 = magMap[i * outW + (j + 1)];
            }
            else if (currentAngle == 90) {
                // Vertical gradient (Horizontal edge) -> Check Up & Down
                if (i > 0) neighbor1 = magMap[(i - 1) * outW + j];
                if (i < outH - 1) neighbor2 = magMap[(i + 1) * outW + j];
            }
            else if (currentAngle == 45) {
                // Diagonal \ -> Check Top-Left & Bottom-Right
                if (i > 0 && j > 0) neighbor1 = magMap[(i - 1) * outW + (j - 1)];
                if (i < outH - 1 && j < outW - 1) neighbor2 = magMap[(i + 1) * outW + (j + 1)];
            }
            else if (currentAngle == 135) {
                // Diagonal / -> Check Top-Right & Bottom-Left
                if (i > 0 && j < outW - 1) neighbor1 = magMap[(i - 1) * outW + (j + 1)];
                if (i < outH - 1 && j > 0) neighbor2 = magMap[(i + 1) * outW + (j - 1)];
            }

            // Write the result directly to your RGBA outputImage
            int outIdx = flatIdx * 4;

            // If it is the local maximum, keep the magnitude. Otherwise, suppress it to 0.
            if (currentMag >= neighbor1 && currentMag > neighbor2) {
                outputImage[outIdx] = currentMag; // R
                outputImage[outIdx + 1] = currentMag; // G
                outputImage[outIdx + 2] = currentMag; // B
                outputImage[outIdx + 3] = 255;        // A
            }
            else {
                outputImage[outIdx] = 0;
                outputImage[outIdx + 1] = 0;
                outputImage[outIdx + 2] = 0;
                outputImage[outIdx + 3] = 255;
            }
        }
    }

    int highThreshold = 70;  // Adjust based on your image lighting
    int lowThreshold = 30;

    // Category values
    const uchar STRONG = 255;
    const uchar WEAK = 75;
    const uchar NON_EDGE = 0;

    // Stack to hold the coordinates of confirmed strong pixels
    std::vector<std::pair<int, int>> strongPixels;

    // Step 1: Double Thresholding
    for (int i = 0; i < outH; i++) {
        for (int j = 0; j < outW; j++) {
            int outIdx = (i * outW + j) * 4;

            // Read the magnitude left behind by the NMS loop (Red channel)
            int mag = outputImage[outIdx];

            if (mag >= highThreshold) {
                outputImage[outIdx] = STRONG;
                outputImage[outIdx + 1] = STRONG;
                outputImage[outIdx + 2] = STRONG;

                // Save coordinates of strong edges to track their connections later
                strongPixels.push_back({ i, j });
            }
            else if (mag >= lowThreshold && mag > 0) {
                // mag > 0 ensures we don't accidentally revive pixels you correctly killed in NMS
                outputImage[outIdx] = WEAK;
                outputImage[outIdx + 1] = WEAK;
                outputImage[outIdx + 2] = WEAK;
            }
            else {
                outputImage[outIdx] = NON_EDGE;
                outputImage[outIdx + 1] = NON_EDGE;
                outputImage[outIdx + 2] = NON_EDGE;
            }
        }
    }

    // Step 2: Edge Tracking by Hysteresis (Depth-First Search)
    // Check the 8-way neighbors (horizontal, vertical, diagonal)
    int dx[] = { -1, -1, -1,  0, 0,  1, 1, 1 };
    int dy[] = { -1,  0,  1, -1, 1, -1, 0, 1 };

    while (!strongPixels.empty()) {
        std::pair<int, int> p = strongPixels.back();
        strongPixels.pop_back();

        int y = p.first;
        int x = p.second;

        // Check all 8 neighboring pixels
        for (int k = 0; k < 8; k++) {
            int ny = y + dy[k];
            int nx = x + dx[k];

            // Safely prevent out-of-bounds crashes at the image borders
            if (ny >= 0 && ny < outH && nx >= 0 && nx < outW) {
                int neighborIdx = (ny * outW + nx) * 4;

                // If the neighbor is WEAK, it's connected to a STRONG edge!
                if (outputImage[neighborIdx] == WEAK) {
                    // Promote it to STRONG
                    outputImage[neighborIdx] = STRONG;
                    outputImage[neighborIdx + 1] = STRONG;
                    outputImage[neighborIdx + 2] = STRONG;

                    // Add this newly promoted pixel to the stack to check ITS neighbors
                    strongPixels.push_back({ ny, nx });
                }
            }
        }
    }

    // Step 3: Final Cleanup
    // Any pixels that are still WEAK weren't connected to a STRONG edge. 
    // They are just noise, so we drop them to 0 (Black).
    for (int i = 0; i < outH; i++) {
        for (int j = 0; j < outW; j++) {
            int outIdx = (i * outW + j) * 4;

            if (outputImage[outIdx] == WEAK) {
                outputImage[outIdx] = NON_EDGE;
                outputImage[outIdx + 1] = NON_EDGE;
                outputImage[outIdx + 2] = NON_EDGE;
                // Alpha (+3) is already 255 from NMS, so we leave it alone.
            }
        }
    }
}

void closeEdgeGaps(std::vector<uchar>& image, int width, int height, int gapSize = 1) {
    std::vector<uchar> tempImage = image;

    // ---------------------------------------------------------
    // STEP 1: DILATION (Expand edges to bridge the gaps)
    // ---------------------------------------------------------
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int idx = (y * width + x) * 4;
            bool isEdgeNeighbor = false;

            // Check the surrounding pixels within the gapSize radius
            for (int ky = -gapSize; ky <= gapSize; ++ky) {
                for (int kx = -gapSize; kx <= gapSize; ++kx) {
                    int ny = y + ky;
                    int nx = x + kx;

                    // Boundary check
                    if (ny >= 0 && ny < height && nx >= 0 && nx < width) {
                        // If any neighbor is a STRONG edge (255)
                        if (image[(ny * width + nx) * 4] == 255) {
                            isEdgeNeighbor = true;
                            break;
                        }
                    }
                }
                if (isEdgeNeighbor) break;
            }

            // If an edge is nearby, become an edge (Dilate)
            if (isEdgeNeighbor) {
                tempImage[idx] = 255;       // R
                tempImage[idx + 1] = 255;   // G
                tempImage[idx + 2] = 255;   // B
                tempImage[idx + 3] = 255;   // A
            }
        }
    }

    // Save the dilated state back to the main image so erosion can work on it
    image = tempImage;

    // ---------------------------------------------------------
    // STEP 2: EROSION (Shrink edges back to original thickness)
    // ---------------------------------------------------------
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int idx = (y * width + x) * 4;
            bool isBackgroundNeighbor = false;

            // Only erode if the current pixel is an edge
            if (image[idx] == 255) {
                // Check the surrounding pixels within the gapSize radius
                for (int ky = -gapSize; ky <= gapSize; ++ky) {
                    for (int kx = -gapSize; kx <= gapSize; ++kx) {
                        int ny = y + ky;
                        int nx = x + kx;

                        // Boundary check
                        if (ny >= 0 && ny < height && nx >= 0 && nx < width) {
                            // If any neighbor is background (0)
                            if (image[(ny * width + nx) * 4] == 0) {
                                isBackgroundNeighbor = true;
                                break;
                            }
                        }
                    }
                    if (isBackgroundNeighbor) break;
                }

                // If background is nearby, revert back to background (Erode)
                if (isBackgroundNeighbor) {
                    tempImage[idx] = 0;
                    tempImage[idx + 1] = 0;
                    tempImage[idx + 2] = 0;
                    tempImage[idx + 3] = 255; // Leave alpha solid
                }
            }
        }
    }

    // Finalize the closed image
    image = tempImage;
}

const int dx8[8] = { 1,  1,  0, -1, -1, -1,  0,  1 };
const int dy8[8] = { 0,  1,  1,  1,  0, -1, -1, -1 };

int getNextNeighbor(const std::vector<int>& img, int width, int height, int cx, int cy, int startDir) {
    for (int i = 0; i < 8; i++) {
        int dir = (startDir + i) % 8;
        int nx = cx + dx8[dir];
        int ny = cy + dy8[dir];

        // Ensure we are inside image bounds
        if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
            // Treat any non-zero pixel as part of a border
            if (img[ny * width + nx] != 0) {
                return dir;
            }
        }
    }
    return -1; // Isolated pixel
}

std::vector<std::vector<std::pair<int, int>>> suzukiFindContours(const std::vector<uchar>& edgeImageRGBA, int width, int height) {

    // 1. Convert the RGBA Canny edge output to a 1-channel integer working grid.
    // Background = 0, Unvisited Edge = 1.
    std::vector<int> workingGrid(width * height, 0);
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            // Read the Red channel (stride of 4). Since STRONG edges are 255, we map them to 1.
            if (edgeImageRGBA[(i * width + j) * 4] == 255) {
                workingGrid[i * width + j] = 1;
            }
        }
    }

    std::vector<std::vector<std::pair<int, int>>> contours;
    int nbd = 1; // Number of Border Discovered

    // 2. Raster scan the image to find border starting points
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            int idx = i * width + j;
            int pixel = workingGrid[idx];

            // If it's background or already part of a fully processed border, skip.
            if (pixel == 0 || pixel > 1) continue;

            // Found a potential outer border start (Foreground pixel with a background pixel to its left)
            bool isOuterBorder = (pixel == 1 && (j == 0 || workingGrid[i * width + (j - 1)] == 0));

            if (isOuterBorder) {
                nbd++; // Increment Border ID
                std::vector<std::pair<int, int>> currentContour;

                int startX = j;
                int startY = i;

                // For an outer border, the background pixel is to the West (direction 4)
                int startDir = 4;

                // Find the first edge pixel by looking clockwise
                int dir1 = getNextNeighbor(workingGrid, width, height, startX, startY, startDir);

                if (dir1 == -1) {
                    // It's an isolated single-pixel point
                    workingGrid[idx] = nbd;
                    currentContour.push_back({ startX, startY });
                    contours.push_back(currentContour);
                    continue;
                }

                int currX = startX + dx8[dir1];
                int currY = startY + dy8[dir1];

                // Track the second point to satisfy Suzuki's stopping criterion
                int startNextX = currX;
                int startNextY = currY;

                // The next search begins clockwise from the pixel we just came from
                // Since we moved in `dir1`, we came from `(dir1 + 4) % 8`.
                // +1 to start the search *after* the pixel we came from.
                int nextSearchDir = (dir1 + 5) % 8;

                currentContour.push_back({ startX, startY });

                // 3. Trace the border
                while (true) {
                    workingGrid[currY * width + currX] = nbd; // Mark as visited with current NBD
                    currentContour.push_back({ currX, currY });

                    int dir2 = getNextNeighbor(workingGrid, width, height, currX, currY, nextSearchDir);

                    if (dir2 == -1) break; // Fallback safety

                    int nextX = currX + dx8[dir2];
                    int nextY = currY + dy8[dir2];

                    // Suzuki's stopping criterion: 
                    // We returned to the start pixel AND the next pixel to visit is the exact same second pixel.
                    if (currX == startX && currY == startY && nextX == startNextX && nextY == startNextY) {
                        break;
                    }

                    currX = nextX;
                    currY = nextY;
                    nextSearchDir = (dir2 + 5) % 8;
                }

                contours.push_back(currentContour);
            }
        }
    }

    return contours;
}

double calculateShoelaceArea(const std::vector<std::pair<int, int>>& contour) {
    int n = contour.size();

    // A contour with fewer than 3 points cannot form a polygon (it's a point or line)
    if (n < 3) return 0.0;

    double area = 0.0;

    for (int i = 0; i < n; i++) {
        // (i + 1) % n ensures the last point connects back to the first point (x_0, y_0)
        int j = (i + 1) % n;

        // Shoelace cross-multiplication
        area += (double)contour[i].first * contour[j].second;
        area -= (double)contour[j].first * contour[i].second;
    }

    return std::abs(area) / 2.0;
}

void processBiggestContour(
    const std::vector<std::vector<std::pair<int, int>>>& contours,
    std::vector<uchar>& finalOutputImage,
    int width, int height)
{
    int biggestContourIndex = -1;
    double maxArea = -1.0;

    // A. Find the largest contour
    for (size_t i = 0; i < contours.size(); i++) {
        double currentArea = calculateShoelaceArea(contours[i]);

        if (currentArea > maxArea) {
            maxArea = currentArea;
            biggestContourIndex = (int)i;
        }
    }

    // Initialize the final output image to completely black/transparent
    finalOutputImage.assign(width * height * 4, 0);

    // B. Draw ONLY the biggest contour
    if (biggestContourIndex != -1) {
        std::cout << "Found biggest contour (Index: " << biggestContourIndex
            << ") with an area of " << maxArea << " square pixels.\n";

        const auto& biggestContour = contours[biggestContourIndex];

        for (const auto& point : biggestContour) {
            int x = point.first;
            int y = point.second;

            // Safe boundary check
            if (x >= 0 && x < width && y >= 0 && y < height) {
                int idx = (y * width + x) * 4;

                // Let's draw the biggest contour in bright Green
                finalOutputImage[idx] = 0;     // R
                finalOutputImage[idx + 1] = 255;   // G
                finalOutputImage[idx + 2] = 0;     // B
                finalOutputImage[idx + 3] = 255;   // Alpha
            }
        }
    }
    else {
        std::cout << "No valid contours found to draw.\n";
    }
}

// 1. Helper: Calculate perpendicular distance from point 'pt' to line segment 'lineStart'-'lineEnd'
double perpendicularDistance(std::pair<int, int> pt, std::pair<int, int> lineStart, std::pair<int, int> lineEnd) {
    double dx = lineEnd.first - lineStart.first;
    double dy = lineEnd.second - lineStart.second;

    // If the line is practically a single point
    if (dx == 0 && dy == 0) {
        dx = pt.first - lineStart.first;
        dy = pt.second - lineStart.second;
        return std::sqrt(dx * dx + dy * dy);
    }

    double num = std::abs(dy * pt.first - dx * pt.second + lineEnd.first * lineStart.second - lineEnd.second * lineStart.first);
    double den = std::sqrt(dx * dx + dy * dy);
    return num / den;
}

// 2. The core Ramer-Douglas-Peucker recursive algorithm
void ramerDouglasPeucker(const std::vector<std::pair<int, int>>& pointList, double epsilon, std::vector<std::pair<int, int>>& out) {
    if (pointList.size() < 2) {
        out = pointList;
        return;
    }

    // Find the point with the maximum distance from the line connecting the start and end points
    double dmax = 0.0;
    int index = 0;
    int end = pointList.size() - 1;

    for (int i = 1; i < end; i++) {
        double d = perpendicularDistance(pointList[i], pointList[0], pointList[end]);
        if (d > dmax) {
            index = i;
            dmax = d;
        }
    }

    // If max distance is greater than epsilon, recursively simplify
    if (dmax > epsilon) {
        std::vector<std::pair<int, int>> recResults1;
        std::vector<std::pair<int, int>> recResults2;

        std::vector<std::pair<int, int>> firstLine(pointList.begin(), pointList.begin() + index + 1);
        std::vector<std::pair<int, int>> lastLine(pointList.begin() + index, pointList.end());

        ramerDouglasPeucker(firstLine, epsilon, recResults1);
        ramerDouglasPeucker(lastLine, epsilon, recResults2);

        // Build the result list (excluding the duplicated middle point)
        out.assign(recResults1.begin(), recResults1.end() - 1);
        out.insert(out.end(), recResults2.begin(), recResults2.end());
    }
    else {
        // Base case: just return the start and end points
        out.push_back(pointList[0]);
        out.push_back(pointList[end]);
    }
}

// 3. Wrapper for closed contours (Suzuki outputs closed loops)
std::vector<std::pair<int, int>> approximateClosedPolygon(const std::vector<std::pair<int, int>>& contour, double epsilon) {
    if (contour.size() < 3) return contour;

    // To process a closed loop with RDP, we must split it into two open lines.
    // We find the point furthest from the start point to act as the halfway split.
    double maxDistSq = 0;
    int splitIdx = 0;
    for (size_t i = 1; i < contour.size(); i++) {
        double dx = contour[i].first - contour[0].first;
        double dy = contour[i].second - contour[0].second;
        double distSq = dx * dx + dy * dy;
        if (distSq > maxDistSq) {
            maxDistSq = distSq;
            splitIdx = i;
        }
    }

    std::vector<std::pair<int, int>> half1(contour.begin(), contour.begin() + splitIdx + 1);
    std::vector<std::pair<int, int>> half2(contour.begin() + splitIdx, contour.end());
    half2.push_back(contour[0]); // Close the loop physically for the second half

    std::vector<std::pair<int, int>> approx1, approx2;
    ramerDouglasPeucker(half1, epsilon, approx1);
    ramerDouglasPeucker(half2, epsilon, approx2);

    // Stitch the two simplified halves back together
    std::vector<std::pair<int, int>> finalApprox = approx1;
    finalApprox.pop_back(); // Remove duplicate at the split index
    finalApprox.insert(finalApprox.end(), approx2.begin(), approx2.end() - 1); // Remove duplicate at the start/end connection

    return finalApprox;
}

// 4. Binary Search to find EXACTLY 4 corners
std::vector<std::pair<int, int>> getFourCorners(const std::vector<std::pair<int, int>>& contour) {
    double minEpsilon = 0.0;
    // The maximum possible epsilon won't exceed the length of the contour itself
    double maxEpsilon = static_cast<double>(contour.size());
    std::vector<std::pair<int, int>> bestApprox;

    // Binary search loop (Limit to 50 iterations to prevent infinite loops on weird shapes)
    for (int iter = 0; iter < 50; iter++) {
        double epsilon = (minEpsilon + maxEpsilon) / 2.0;
        std::vector<std::pair<int, int>> approx = approximateClosedPolygon(contour, epsilon);

        if (approx.size() == 4) {
            return approx; // Success! We found exactly a quad.
        }
        else if (approx.size() > 4) {
            minEpsilon = epsilon; // Too many points -> We need a stronger simplification -> Increase Epsilon
        }
        else {
            maxEpsilon = epsilon; // Too few points (e.g., a triangle) -> We simplified too much -> Decrease Epsilon
        }

        // Save the closest result as a fallback just in case the shape makes exactly 4 impossible
        bestApprox = approx;
    }

    std::cout << "Warning: Could not find exactly 4 corners. Returning " << bestApprox.size() << " corners.\n";
    return bestApprox;
}

void findSomething(const uchar* inputImage, std::vector<uchar>& outputImage, int inputWidth, int inputHeight, std::vector<sf::Vector2f>& cornerPoints)
{
    findEdges(inputImage, outputImage, inputWidth, inputHeight);

    closeEdgeGaps(outputImage, inputWidth, inputHeight, 2);

    std::vector<std::vector<std::pair<int, int>>> contours = suzukiFindContours(outputImage, inputWidth, inputHeight);

    processBiggestContour(contours, outputImage, inputWidth, inputHeight);

    int biggestContourIndex = -1;
    double maxArea = -1.0;

    for (size_t i = 0; i < contours.size(); i++) {
        double currentArea = calculateShoelaceArea(contours[i]);
        if (currentArea > maxArea) {
            maxArea = currentArea;
            biggestContourIndex = (int)i;
        }
    }

    // Prepare a clean black image for our final visualization output
    //outputImage.assign(inputWidth * inputHeight * 4, 0);

    // If we didn't find any valid object, stop here
    if (biggestContourIndex == -1) {
        std::cout << "No object found in image.\n";
        return;
    }

    // -------------------------------------------------------------------------
    // STEP 5: Run Polygon Approximation (RDP) to simplify down to 4 corners
    // -------------------------------------------------------------------------
    std::vector<std::pair<int, int>> fourCorners = getFourCorners(contours[biggestContourIndex]);

    // -------------------------------------------------------------------------
    // STEP 6: PLACE THE CORNER WHITENING CODE HERE
    // -------------------------------------------------------------------------
    if (fourCorners.size() == 4) {
        for (int i = 0; i < 4; i++) {
            int x = fourCorners[i].first;
            int y = fourCorners[i].second;
            cornerPoints[i].x = x;
            cornerPoints[i].y = y;


            // Safe boundary check to prevent crashes near edges
            if (x >= 0 && x < inputWidth && y >= 0 && y < inputHeight) {

                // Option A: Just turn exactly that single pixel pure white
                int idx = (y * inputWidth + x) * 4;
                outputImage[idx] = 255; // R
                outputImage[idx + 1] = 255; // G
                outputImage[idx + 2] = 255; // B
                outputImage[idx + 3] = 255; // Alpha

                
                // Option B: If 1 pixel is too small to see, uncomment this 3x3 block instead
                for(int dy = -1; dy <= 1; dy++) {
                    for(int dx = -1; dx <= 1; dx++) {
                        int nx = x + dx;
                        int ny = y + dy;
                        if (nx >= 0 && nx < inputWidth && ny >= 0 && ny < inputHeight) {
                            int bIdx = (ny * inputWidth + nx) * 4;
                            outputImage[bIdx] = 255; outputImage[bIdx+1] = 255;
                            outputImage[bIdx+2] = 255; outputImage[bIdx+3] = 255;
                        }
                    }
                }
                
            }
        }
    }
}

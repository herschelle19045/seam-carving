#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>

using namespace cv;
using namespace std;

set<int> Set;

double getEnergy(Mat mat, int i, int j, int size) {
    if (j < 0) {
        return mat.at<double>(i, 0);
    }
    if (j >= size) {
        return mat.at<double>(i, size - 1);
    }
    return mat.at<double>(i, j);
}

Mat generateEnergyMap(Mat& img) {

    // Gaussian Blue to reduce noise
    Mat blurImage;
    GaussianBlur(img, blurImage, Size(3, 3), 0, 0, BORDER_DEFAULT);

    // Grayscale
    Mat grayImage;
    cvtColor(blurImage, grayImage, COLOR_BGR2GRAY);


    // Sobel operator to compute the gradient of the image
    Mat xGradient, yGradient;
    Sobel(grayImage, xGradient, CV_16S, 1, 0, 3, 1, 0, BORDER_DEFAULT);
    Sobel(grayImage, yGradient, CV_16S, 0, 1, 3, 1, 0, BORDER_DEFAULT);

    // Convert gradients to absolute
    Mat xGradAbs, yGradAbs;
    convertScaleAbs(xGradient, xGradAbs);
    convertScaleAbs(yGradient, yGradAbs);

    // Summation of gradiants
    Mat gradient;
    addWeighted(xGradAbs, 0.5, yGradAbs, 0.5, 0, gradient);

    // Convert to double
    Mat energyImage;
    gradient.convertTo(energyImage, CV_64F, 1.0 / 255.0);

    namedWindow("Energy Map", WINDOW_AUTOSIZE);
    imshow("Energy Map", energyImage);

    return energyImage;
}

Mat generateCumulativeMap(Mat& eImg, Size_<int> size) {
    Mat cMap = Mat(size, CV_64F);

    // Base Condition
    for (int i = 0; i < size.width; ++i) {
        cMap.at<double>(0, i) = eImg.at<double>(0, i);
    }

    // Dynamic Programming
    for (int i = 1; i < eImg.rows; i++) {
        for (int j = 0; j < eImg.cols; j++) {
            cMap.at<double>(i, j) = eImg.at<double>(i, j) +
                min({
                    getEnergy(cMap, i - 1, j - 1, size.width),
                    getEnergy(cMap, i - 1, j, size.width),
                    getEnergy(cMap, i - 1, j + 1, size.width)
                    });
        }
    }

    return cMap;
}

vector<int> findSeam(Mat& cMap, Size_<int> size) {
    vector<int> optPath;
    double Min = 1e9;
    int j = -1; // Min Index ie index at which Min val occurs

    for (int i = 1; i < cMap.row(0).cols; ++i) {
        if (Set.find(i) != Set.end()) {
            continue;
        }
        double val = cMap.at<double>(0, i);
        if (val < Min) {
            Min = val;
            j = i;
        }
    }
    Set.insert(j);
    optPath.push_back(j);

    for (int i = 1; i < size.height; i++) {
        double a = getEnergy(cMap, i, j - 1, size.width),
            b = getEnergy(cMap, i, j, size.width),
            c = getEnergy(cMap, i, j + 1, size.width);

        if (min({ a, b, c }) == c) {
            j++;
        }
        else if (min({ a, b, c }) == a) {
            j--;
        }
        if (j < 0) j = 0;
        else if (j >= size.width) j = size.width - 1;
        optPath.push_back(j);
    }

    return optPath;
}

void generateSeams(Mat& cMap, Mat& eMap, Size_<int> size) {
    for (int j = 0; j < 50; ++j) {
        vector<int> path = findSeam(cMap, size);
        for (int i = 0; i < eMap.rows; i++) {
            eMap.at<double>(i, path[i]) = 1;
        }
        namedWindow("Seam on Energy Map", WINDOW_AUTOSIZE);
        imshow("Seam on Energy Map", eMap);

        waitKey(0);
    }
}

int main() {
    string filename = "seam.jpg";

    Mat image = imread(filename);
    if (image.empty()) {
        cout << "Image not found\n";
        return 0;
    }

    Size_<int> imgSize = Size(image.cols, image.rows);

    namedWindow("Original Image", WINDOW_AUTOSIZE);
    imshow("Original Image", image);


    Mat energyMap = generateEnergyMap(image);
    Mat cumulativeMap = generateCumulativeMap(energyMap, imgSize);
    generateSeams(cumulativeMap, energyMap, imgSize);


    return 0;
}

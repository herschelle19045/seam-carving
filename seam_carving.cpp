#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>

using namespace cv;
using namespace std;

// Function to get energy values at pixel i,j
double getEnergy(Mat mat, int i, int j, int size) {
    if (j < 0) {
        return mat.at<double>(i, 0);
    }
    if (j >= size) {
        return mat.at<double>(i, size - 1);
    }
    return mat.at<double>(i, j);
}

// https://docs.opencv.org/3.4/d2/d2c/tutorial_sobel_derivatives.html
Mat generateEnergyMap(Mat& img) {
    // Gaussian Blur to reduceWidth noise
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

    imshow("Energy Map", energyImage);

    return energyImage;
}

Mat generateCumulativeMap(Mat& energyImage, Size_<int> size) {
    Mat cumulativeEnergyMap = Mat(size, CV_64F);

    // Base Condition (Copying first row of image to cumulative map)
    for (int i = 0; i < size.width; ++i) {
        cumulativeEnergyMap.at<double>(0, i) = energyImage.at<double>(0, i);
    }

    // Dynamic Programming
    for (int i = 1; i < energyImage.rows; i++) {
        for (int j = 0; j < energyImage.cols; j++) {
            cumulativeEnergyMap.at<double>(i, j) = energyImage.at<double>(i, j) +
                                                   min({
                                                getEnergy(cumulativeEnergyMap, i - 1, j - 1, size.width),
                                                getEnergy(cumulativeEnergyMap, i - 1, j, size.width),
                                                getEnergy(cumulativeEnergyMap, i - 1, j + 1, size.width)
                                        });
        }
    }

    return cumulativeEnergyMap;
}

vector<int> findSeam(Mat& cumulativeEnergyMap, Size_<int> size) {
    vector<int> optPath(size.height);
    double Min = 1e9;
    int j = -1; // Min Index ie index at which Min val occurs

    // Finding index of min energy at the last row of image
    for (int i = 0; i < size.width; ++i) {
        double val = cumulativeEnergyMap.at<double>(size.height - 1, i);
        if (val < Min) {
            Min = val;
            j = i;
        }
    }
    optPath[size.height - 1] = j;

    // Generating seam (path) starting from the bottom of the image
    for (int i = size.height - 2; i >= 0; i--) {
        double a = getEnergy(cumulativeEnergyMap, i, j - 1, size.width),
                b = getEnergy(cumulativeEnergyMap, i, j, size.width),
                c = getEnergy(cumulativeEnergyMap, i, j + 1, size.width);

        // From i_th row to i-1_th row we need to select from min energy(i, j-1), energy(i, j), energy(i, j+1)
        if (min({ a, b, c }) == c) {
            j++;
        }
        else if (min({ a, b, c }) == a) {
            j--;
        }
        if (j < 0) j = 0;
        else if (j >= size.width) j = size.width - 1;

        optPath[i] = j;
    }

    return optPath;
}

void reduceWidth(Mat& image, vector<int> path, Size_<int> size) {
    // Initially copying image(i, j) = image(i, j)
    for (int i = 0; i < size.height; i++) {
        int k = 0;
        for (int j = 0; j < size.width; ++j) {
            if (j == path[i]) continue;
            // Shifting all pixels by one pixel in a row
            // Once pixel to remove has been crossed then copy image(i, j-1) = image(i, j)
            image.at<Vec3b>(i, k++) = image.at<Vec3b>(i, j); 
        }
    }
    image = image.colRange(0, size.width - 1);

    imshow("Reduced Image", image);
}

int main() {
    string filename = "original.jpg";

    Mat image = imread(filename);
    if (image.empty()) {
        cout << "Image not found\n";
        return 0;
    }

    Size_<int> imgSize = Size(image.cols, image.rows);

    imshow("Original Image", image);

    for (int i = 0; i < 150; i++) {
        Mat energyMap = generateEnergyMap(image);
        Mat cumulativeEnergyMap = generateCumulativeMap(energyMap, imgSize);
        vector<int> path = findSeam(cumulativeEnergyMap, imgSize);

        for (int j = 0; j < energyMap.rows; j++) {
            energyMap.at<double>(j, path[j]) = 1;
        }
        imshow("Seam Path", energyMap);

        waitKey(10);
        reduceWidth(image, path, imgSize);
        imgSize.width--;
    }
    waitKey(0);
    return 0;
}

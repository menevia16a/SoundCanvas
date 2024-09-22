#include <iostream>
#include <string>
#include <filesystem>
#include <opencv2/opencv.hpp>
#include <sndfile.h>
#include <cmath>

// Function prototypes
cv::Mat processImage(const std::string& filePath);
void generateWavFile(const std::string& outputFilePath, const cv::Mat& image);

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <image_file>" << std::endl;
        return 1;
    }

    std::string imageFilePath = argv[1];
    cv::Mat processedImage = processImage(imageFilePath);

    // Generate output WAV file path
    std::filesystem::path imagePath(imageFilePath);
    std::string outputWavFilePath = imagePath.stem().string() + ".wav";

    generateWavFile(outputWavFilePath, processedImage);

    std::cout << "WAV file generated: " << outputWavFilePath << std::endl;
    return 0;
}

cv::Mat processImage(const std::string& filePath) {
    // Read the image using OpenCV
    cv::Mat image = cv::imread(filePath, cv::IMREAD_UNCHANGED);
    if (image.empty()) {
        std::cerr << "Error: Could not open or find the image." << std::endl;
        return cv::Mat();
    }

    std::cout << "Processing image.." << std::endl;

    // Process the image converting it to grayscale
    cv::Mat grayImage;
    cv::cvtColor(image, grayImage, cv::COLOR_BGR2GRAY);

    // Flip the image vertically
    cv::Mat flippedImage;
    cv::flip(grayImage, flippedImage, 0);

    // Rotate the image 90 degrees counterclockwise
    cv::Mat rotatedImage;
    cv::rotate(flippedImage, rotatedImage, cv::ROTATE_90_COUNTERCLOCKWISE);

    std::cout << "Image processed successfully." << std::endl;
    return rotatedImage;
}

void generateWavFile(const std::string& outputFilePath, const cv::Mat& image) {
    if (image.empty()) {
        std::cerr << "Error: No image data to convert to WAV." << std::endl;
        return;
    }

    // Define WAV file parameters
    SF_INFO sfInfo;
    sfInfo.channels = 1;
    sfInfo.samplerate = 44100;
    sfInfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

    // Open the WAV file for writing
    SNDFILE* outFile = sf_open(outputFilePath.c_str(), SFM_WRITE, &sfInfo);
    if (!outFile) {
        std::cerr << "Error: Could not open output WAV file." << std::endl;
        return;
    }

    std::cout << "Generating WAV file..." << std::endl;

    // Convert image data to audio data
    std::vector<short> audioData;
    int sampleRate = sfInfo.samplerate;
    int duration = 5; // Duration in seconds
    int totalSamples = sampleRate * duration;

    // Calculate the number of samples per row
    int samplesPerRow = totalSamples / image.rows;

    // Adjust the frequency range to better fit the image dimensions
    double minFrequency = 200.0;
    double maxFrequency = 8000.0;
    double frequencyRange = maxFrequency - minFrequency;

    for (int row = 0; row < image.rows; ++row) {
        for (int i = 0; i < samplesPerRow; ++i) {
            double t = static_cast<double>(i + row * samplesPerRow) / sampleRate;
            double sampleValue = 0.0;

            for (int col = 0; col < image.cols; ++col) {
                double frequency = minFrequency + (frequencyRange * col / (image.cols - 1)); // Map column to frequency
                double amplitude = static_cast<double>(image.at<uchar>(row, col)) / 255.0; // Normalize pixel value
                sampleValue += amplitude * sin(2.0 * CV_PI * frequency * t);
            }

            // Normalize the sample value to the range of short
            sampleValue = std::clamp(sampleValue, -1.0, 1.0);
            audioData.push_back(static_cast<short>(sampleValue * 32767));
        }
    }

    // Write audio data to the WAV file
    sf_write_short(outFile, audioData.data(), audioData.size());

    // Close the WAV file
    sf_close(outFile);

    std::cout << "WAV file generated successfully." << std::endl;
}

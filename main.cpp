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
    else if (std::string(argv[1]).find(".png") == std::string::npos) { // Check for png extension
        std::cerr << "Error: " << argv[1] << " is not a PNG image." << std::endl;
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
    cv::Mat image = cv::imread(filePath, cv::IMREAD_UNCHANGED); // Ensure the alpha channel is preserved
    if (image.empty()) {
        std::cerr << "Error: Could not open or find the image." << std::endl;
        return cv::Mat();
    }

    std::cout << "Processing image.." << std::endl;

    // Reduce the image size by 50%
    cv::Mat resizedImage;
    cv::resize(image, resizedImage, cv::Size(), 0.5, 0.5, cv::INTER_LINEAR);

    // Process the image converting it to grayscale
    cv::Mat grayImage;
    cv::cvtColor(resizedImage, grayImage, cv::COLOR_BGR2GRAY);

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

    // Calculate duration based on image dimensions
    int duration = static_cast<int>(std::sqrt(image.rows * image.cols) / 10); // Heuristic
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
                double intensity = static_cast<double>(image.at<cv::Vec4b>(row, col)[0]) / 255.0; // Grayscale intensity
                double alpha = static_cast<double>(image.at<cv::Vec4b>(row, col)[3]) / 255.0; // Alpha channel
                double amplitude = intensity * alpha; // Combine intensity and alpha
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

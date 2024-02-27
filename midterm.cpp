#include "stdafx.h"
#include "midterm.h"
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <omp.h>
#include <chrono>
#include <thread>

using namespace cv;
using namespace std;
using namespace chrono;

midterm::midterm(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    connect(ui.selectImageButton, &QPushButton::clicked, this, &midterm::selectImage);
    connect(ui.blurSlider, &QSlider::valueChanged, this, &midterm::blurImage);
    connect(ui.applyBlurButton, &QPushButton::clicked, this, &midterm::applyBlur);
    connect(ui.prevImageButton, &QPushButton::clicked, this, &midterm::showPreviousImage);
    connect(ui.nextImageButton, &QPushButton::clicked, this, &midterm::showNextImage);

    ui.prevImageButton->setEnabled(false);
    ui.nextImageButton->setEnabled(false);

    parallelStatusLabel = new QLabel(this);
    ui.statusBar->addWidget(parallelStatusLabel);

    sequentialStatusLabel = new QLabel(this);
    ui.statusBar->addWidget(sequentialStatusLabel);
}

midterm::~midterm()
{
    selectedImage = QImage();
}

void midterm::selectImage() {
    QStringList newImagePaths = QFileDialog::getOpenFileNames(this, tr("Open Images"), "", tr("Image Files (*.png *.jpg *.bmp)"));
    if (!newImagePaths.isEmpty()) {
        // Check if adding the new images will exceed the maximum limit
        if (imagePaths.size() + newImagePaths.size() > 10) {
            QMessageBox::warning(this, "Warning", "You can select up to 10 images.");
            return;
        }

        // Resize Image max 612x612
        for (int i = 0; i < newImagePaths.size(); i++) {
			Mat image = imread(newImagePaths[i].toStdString());
            if (image.empty()) {
				updateStatus(QString("Failed to load the image: %1").arg(newImagePaths[i]));
				continue;
			}

            if (image.cols > 612 || image.rows > 408) {
				cv::resize(image, image, Size(612, 408));
				imwrite(newImagePaths[i].toStdString(), image);
			}
		}

        // Add the new images to the list
        imagePaths.append(newImagePaths);

        // Display the first image
        currentIndex = imagePaths.size() - newImagePaths.size();
        displayImage(imagePaths[currentIndex]);

        // Update the status
        updateStatus(QString("Selected %1 images").arg(imagePaths.size()));

        // Update the next and previous buttons
        ui.prevImageButton->setEnabled(true);
        ui.nextImageButton->setEnabled(true);

        // Update the current index
        currentIndex = imagePaths.size() - 1;
    }
    else {
		updateStatus("No images selected");
	}
}

void midterm::blurImage(int value) {
	// Update the label to display the current value of the slider
    ui.blurValueLabel->setText(tr("Blur Value: %1").arg(value));
}

void midterm::applyBlur() {
    int blurValue = ui.blurSlider->value();

    if (imagePaths.isEmpty()) {
        QMessageBox::warning(this, "Warning", "Please select an image first");
        return;
    }

    omp_set_num_threads(std::thread::hardware_concurrency());

    try {
        startParallel = std::chrono::steady_clock::now();
        // Loop through all selected images and apply Gaussian blur to each of them in parallel
        #pragma omp parallel for
        for (int i = 0; i < imagePaths.size(); i++) {
            Mat image = imread(imagePaths[i].toStdString());
            if (image.empty()) {
                updateStatus(QString("Failed to load the image: %1").arg(imagePaths[i]));
                continue;
            }

            // Check if the blur value is valid
            if (blurValue <= 0) {
                updateStatus("Invalid blur value");
                continue;
            }

            // Check if the kernel size is odd
            int kernelSize = blurValue * 2 + 1;
            if (kernelSize % 2 == 0) {
                updateStatus("Kernel size must be odd");
                continue;
            }

            Mat blurredImage;
            try {
                GaussianBlur(image, blurredImage, Size(kernelSize, kernelSize), 0);
            }
            catch (const cv::Exception& e) {
                updateStatus(QString("Failed to apply blur: %1").arg(e.what()));
                continue;
            }

            imwrite(imagePaths[i].toStdString(), blurredImage);
            }

        ExecutionTimeParallel();

        // Sequential execution
        startSequential = std::chrono::steady_clock::now();
        for (int i = 0; i < imagePaths.size(); i++) {
			Mat image = imread(imagePaths[i].toStdString());
            if (image.empty()) {
				updateStatus(QString("Failed to load the image: %1").arg(imagePaths[i]));
				continue;
			}

			// Check if the blur value is valid
            if (blurValue <= 0) {
				updateStatus("Invalid blur value");
				continue;
			}

			// Check if the kernel size is odd
			int kernelSize = blurValue * 2 + 1;
            if (kernelSize % 2 == 0) {
				updateStatus("Kernel size must be odd");
				continue;
			}

			Mat blurredImage;
            try {
				GaussianBlur(image, blurredImage, Size(kernelSize, kernelSize), 0);
			}
            catch (const cv::Exception& e) {
				updateStatus(QString("Failed to apply blur: %1").arg(e.what()));
				continue;
			}

		}
        ExecutionTimeSequential();
    }
    catch (const cv::Exception& ex) {
        // Handle OpenCV exceptions
        QMessageBox::critical(this, "Error", QString("OpenCV Exception: %1").arg(ex.what()));
    }
    catch (const std::exception& ex) {
        // Handle other exceptions
        QMessageBox::critical(this, "Error", QString("Exception: %1").arg(ex.what()));
    }
    catch (...) {
        // Handle unknown exceptions
        QMessageBox::critical(this, "Error", "An unknown error occurred during image processing.");
    }
}

void midterm::showPreviousImage() {
    if (currentIndex > 0) {
        currentIndex--;
        displayImage(imagePaths[currentIndex]);
    }
    else {
        updateStatus("Already at the first image");
    }
}

void midterm::showNextImage() {
    if (currentIndex < imagePaths.size() - 1) {
        currentIndex++;
        displayImage(imagePaths[currentIndex]);
    }
    else {
        updateStatus("Already at the last image");
    }
}

void midterm::displayImage(const QString& imagePath) {
    QImage image(imagePath);
    if (image.isNull()) {
        QMessageBox::warning(this, "Warning", "Failed to load the image");
        return;
    }

    ui.imageLabel->setPixmap(QPixmap::fromImage(image));
    updateStatus(QString("Image displayed: %1").arg(imagePath));
}

void midterm::updateStatus(const QString& status) {
    ui.statusLabel->setText(status);
}

void midterm::ExecutionTimeSequential() {
    auto endSequential = std::chrono::steady_clock::now();
    auto durationSequential = std::chrono::duration_cast<std::chrono::milliseconds>(endSequential - startSequential);
    sequentialStatusLabel->setText(QString("Sequential execution time: %1 ms").arg(durationSequential.count()));
}

void midterm::ExecutionTimeParallel() {
    auto endParallel = std::chrono::steady_clock::now();
    auto durationParallel = std::chrono::duration_cast<std::chrono::milliseconds>(endParallel - startParallel);
    parallelStatusLabel->setText(QString("Parallel execution time: %1 ms").arg(durationParallel.count()));
}
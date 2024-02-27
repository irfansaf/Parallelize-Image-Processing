#pragma once

#include <QMainWindow>
#include <QImage>
#include <QFileDialog>
#include <QMessageBox>
#include <QMutex>
#include <chrono>
#include "ui_midterm.h"
#include <opencv2/core.hpp>

class midterm : public QMainWindow
{
    Q_OBJECT

public:
    midterm(QWidget* parent = nullptr);
    ~midterm();

private slots:
    void selectImage();
    void blurImage(int value);
    void applyBlur();
    void showPreviousImage();
    void showNextImage();

private:
    Ui::midtermClass ui;
    QImage selectedImage;
    QMutex mutex;
    QStringList imagePaths;
    std::chrono::time_point<std::chrono::steady_clock> startTime;
    int currentIndex;

    void displayImage(const QString& imagePath);
    void updateStatus(const QString& status);
    void updateExecutionTime();
};
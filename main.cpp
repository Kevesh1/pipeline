#include <iostream>
#include <mutex>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <opencv2/opencv.hpp>

extern "C" {
#include "apriltag/apriltag.h"
#include "apriltag/tag36h11.h"
}

#define buff_max 25
#define mod %



cv::Mat buffer[buff_max];

std::atomic<int> free_index(0);
std::atomic<int> full_index(0);
std::mutex mtx;

cv::VideoCapture initCamera(){
    cv::VideoCapture cap(2);
    if (!cap.isOpened()){
        std::cout << "No video stream detected" << std::endl;
        std::system("Pause");
    }
    return cap;
}

cv::Mat getImage(cv::VideoCapture cap){
    cv::Mat image;
    cap >> image;
    return image;
}



void producer() {
   
    cv::Mat image;
    cv::VideoCapture cap = initCamera();
    while(true){
        //Uncomment for time counting
        //auto start_time = std::chrono::high_resolution_clock::now();
        image = getImage(cap);
        cv::Mat converted_img;
        cv::cvtColor(image, converted_img, cv::COLOR_RGB2BGR);
        if (image.empty()){
            break;
        }
        //Uncomment 3 lines under for time counting
        //auto end_time = std::chrono::high_resolution_clock::now();
        //auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time-start_time);
        //std::cout << "Color conversion time: " << duration.count() << " milliseconds" << std::endl;
        
        //std::this_thread::sleep_for(std::chrono::milliseconds(100));
        while(((free_index + 1) mod buff_max) == full_index) {
            //buffer full
            //std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        //cv::imshow("Video Player", converted_img);
        mtx.lock();

        buffer[free_index] = converted_img;
        free_index = (free_index + 1) mod buff_max;
        std::cout << "sending image" << std::endl;
        std::cout << free_index << std::endl;
        mtx.unlock();
        char c = (char)cv::waitKey(1);
        if (c==27){
            break;
        }
    }
    std::cout << "bb" << std::endl;

    cap.release();
}

void consumer() {
    cv::Mat consumed_image;
    //Init detector
    apriltag_detector_t *detector = apriltag_detector_create();
    apriltag_family_t *apriltag_family = tag36h11_create();
    apriltag_detector_add_family(detector, apriltag_family);
    while(true){
        while(free_index == full_index){
            //std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        mtx.lock();
        consumed_image = buffer[full_index];
        cv::Mat gray;
        cv::cvtColor(consumed_image, gray, cv::COLOR_BGR2GRAY);
        full_index = (full_index + 1) mod buff_max;
        mtx.unlock();
        //detect_apriltag();
        //Integrating cv::Mat with AprilTags library
        image_u8_t img_header = {
            .width = gray.cols,
            .height = gray.rows,
            .stride = gray.cols,
            .buf = gray.data
        };

        zarray_t *detections = apriltag_detector_detect(detector ,&img_header);
        
        for (int i = 0; i < zarray_size(detections); i++){
            apriltag_detection *detection;
            zarray_get(detections, i, &detection);
            line(consumed_image, cv::Point(detection->p[0][0], detection->p[0][1]),
                     cv::Point(detection->p[1][0], detection->p[1][1]),
                     cv::Scalar(0, 0xff, 0), 2);
            line(consumed_image, cv::Point(detection->p[0][0], detection->p[0][1]),
                     cv::Point(detection->p[3][0], detection->p[3][1]),
                     cv::Scalar(0, 0, 0xff), 2);
            line(consumed_image, cv::Point(detection->p[1][0], detection->p[1][1]),
                     cv::Point(detection->p[2][0], detection->p[2][1]),
                     cv::Scalar(0xff, 0, 0), 2);
            line(consumed_image, cv::Point(detection->p[2][0], detection->p[2][1]),
                     cv::Point(detection->p[3][0], detection->p[3][1]),
                     cv::Scalar(0xff, 0, 0), 2);

            std::stringstream ss;
            ss << detection->id;
            std::string text = ss.str();
            int fontface = cv::FONT_HERSHEY_SCRIPT_SIMPLEX;
            double fontscale = 1.0;
            int baseline;
            cv::Size textsize = cv::getTextSize(text, fontface, fontscale, 2,
                                            &baseline);
            cv::putText(consumed_image, text, cv::Point(detection->c[0]-textsize.width/2,
                                       detection->c[1]+textsize.height/2),
                    fontface, fontscale, cv::Scalar(0xff, 0x99, 0), 2);
            std::cout << "detection" << std::endl;
            std::cout << detection << std::endl;
        }
        apriltag_detections_destroy(detections);
        cv::imshow("Tag Detections", consumed_image);


        //send coordinates here?
        //...
        //std::cout << "The value: " << consumed_item.number << "is now: " << consumed_item.number *2 << std::endl;
        //std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    apriltag_detector_destroy(detector);
    tag36h11_destroy(apriltag_family);

}

int main() {
    std::vector<std::thread> threads;

    threads.emplace_back(producer);
    threads.emplace_back(consumer);


    for(auto& thread: threads) {
        thread.join();
    }
}
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <opencv2/opencv.hpp>
#include <apriltag/apriltag.h>
#include <apriltag/tag36h11.h>

#define buff_max 25
#define mod %


struct item {
    int number;
    cv::VideoCapture myImage;
};

cv::Mat buffer[buff_max];

std::atomic<int> free_index(0);
std::atomic<int> full_index(0);
std::mutex mtx;

cv::VideoCapture initCamera(){
    cv::namedWindow("Video Player");
    cv::VideoCapture cap(0);
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
    std::vector<item> items;
    for(int i = 0; i<5;i++){
        item new_item;
        new_item.number = i;
        items.push_back(new_item);
    }
    cv::Mat image;
    cv::VideoCapture cap = initCamera();
    while(true){
        //Uncomment for time counting
        //auto start_time = std::chrono::high_resolution_clock::now();
        image = getImage(cap);
        cv::Mat converted_img;
        //cv::cvtColor(img, converted_img, cv::COLOR_RGB2BGR);
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
        cv::imshow("Video Player", image);
        mtx.lock();
        //if (!items.empty()){
        //   item item_to_send = items.back();
        //    items.pop_back();
        //    buffer[free_index] = item_to_send;
        //    free_index = (free_index + 1) mod buff_max;
        //}
        buffer[free_index] = image;
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
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
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
            std::cout << detection << std::endl;
        }
        apriltag_detections_destroy(detections);

        //send coordinates here?
        //...
        //std::cout << "The value: " << consumed_item.number << "is now: " << consumed_item.number *2 << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
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
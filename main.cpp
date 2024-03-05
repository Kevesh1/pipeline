#include <iostream>
#include <mutex>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <opencv2/opencv.hpp>

#define buff_max 25
#define mod %


struct item {
    int number;
    cv::VideoCapture myImage;
};

item buffer[buff_max];

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

cv::Mat getImage(){
    cv::Mat image;
    cv::VideoCapture cap;
    cap >> image;
    return image
}

void producer() {
    std::vector<item> items;
    for(int i = 0; i<5;i++){
        item new_item;
        new_item.number = i;
        items.push_back(new_item);
    }
    cv::Mat img;
    cv::VideoCapture cap = initCamera();
    while(true){
        img = getImage();
        if (img.empty()){
            break;
        }
        cv::imshow("Video Player", img);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        while(((free_index + 1) mod buff_max) == full_index) {
            //buffer full
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        mtx.lock();
        if (!items.empty()){
            item item_to_send = items.back();
            items.pop_back();
            buffer[free_index] = item_to_send;
            free_index = (free_index + 1) mod buff_max;
        }
        mtx.unlock();
        char c = (char)cv::waitKey(0);
        if (c==27){
            break;
        }
    }
    cap.release();
}

void consumer() {
    item consumed_item;
    while(true){
        while(free_index == full_index){
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        mtx.lock();
        consumed_item = buffer[full_index];
        full_index = (full_index + 1) mod buff_max;
        mtx.unlock();
        //send coordinates here?
        //...
        std::cout << "The value: " << consumed_item.number << "is now: " << consumed_item.number *2 << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int main() {
    std::vector<std::thread> threads;

    threads.emplace_back(producer);
    threads.emplace_back(consumer);


    for(auto& thread: threads) {
        thread.join();
    }
}
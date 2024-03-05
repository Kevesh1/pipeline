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

void producer() {
    std::vector<item> items;
    for(int i = 0; i<5;i++){
        item new_item;
        new_item.number = i;
        items.push_back(new_item);
    }
    while(true){
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

    }
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
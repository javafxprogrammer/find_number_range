#include <vector>
#include <algorithm>
#include <thread>
#include <mutex>
#include <chrono>
#include <math.h>
#include <iostream>
#include <fstream>
#include <string>
#include <string.h>

std::vector<std::vector<long long>> slice(const std::vector<std::vector<long long>>& arr, int x, int y){
    auto v_begin = std::begin(arr)+x;
    auto v_end = std::begin(arr)+y+1;
    std::vector<std::vector<long long>> new_arr(v_begin, v_end);
    return new_arr;
}

std::mutex mu;
void classify_serial_numbers(const std::vector<std::vector<long long>>& serial_ranges, 
                                     const std::vector<long long>& serial_numbers,
                                     std::ofstream& of){

        std::thread::id t_id = std::this_thread::get_id();
        mu.lock();
        try{
            std::cout << "thread :" << t_id << " started" << std::endl;
        }catch(...){
            mu.unlock();
            throw;
        }
        mu.unlock();

        std::string output;
        for(auto& range: serial_ranges){
            output += std::to_string(range[0]) + "," +std::to_string(range[1]) + ",\"";
            for (auto& serial: serial_numbers){
                if(serial >= range[0] && serial <= range[1]){
                    output +=std::to_string(serial) + "|";
                }
            }   
        output += "\"\n";
        }
        mu.lock();
        try{
            of << output;
            std::cout << "thread :" << t_id << " completed" << std::endl;
        }catch(...){
            mu.unlock();
            throw;
        }
        mu.unlock();
}

int main(){

    std::string serial_numbers_path = R"(C:\Users\username\Projects\C++\classify_numbers\files\serial_numbers.csv)";
    std::string serial_numbers_dest;
    std::ifstream serial_numbers_if(serial_numbers_path);
    std::vector<long long> serial_numbers;

    std::string serial_ranges_path = R"(C:\Users\username\Documents\Projects\C++\classify_numbers\files\serial_ranges.csv)";
    std::string serial_ranges_dest;
    std::ifstream serial_ranges_if(serial_ranges_path);
    std::vector<std::vector<long long>> serial_ranges;

    std::string output_file = R"(C:\Users\username\Documents\Projects\C++\classify_numbers\files\output.csv)";
    std::ofstream output_file_of(output_file);
    output_file_of << "serialStart,serialEnd,serialNumbers\n";

    if(serial_ranges_if.fail() || serial_numbers_if.fail() || output_file_of.fail()){
        std::cout << "failed to open files" << std::endl;
        exit(1);
    }

    // read serial ranges from file
    while(getline(serial_ranges_if, serial_ranges_dest)){

        int len = serial_ranges_dest.length();
        int delimiter_pos = serial_ranges_dest.find(",");

        std::string startingSerial = serial_ranges_dest.substr(0,delimiter_pos);
        std::string endingSerial = serial_ranges_dest.substr(delimiter_pos+1, len-1);

        serial_ranges.push_back({std::stoll(startingSerial), std::stoll(endingSerial)});
    }

    // read serial numbers from file
    while(getline(serial_numbers_if, serial_numbers_dest)){
        serial_numbers.push_back(std::stoll(serial_numbers_dest));
    }
    
    size_t core_count = std::thread::hardware_concurrency();
    size_t serial_ranges_length = serial_ranges.size();
    int n_work_per_core = floor((double)(serial_ranges_length) / (double)core_count);
    std::vector<std::vector<std::vector<long long>>> groups;
    int counter=1;

    // calculate indices to use when slicing vector for each cpu core
    for(int i=0; i<serial_ranges_length; i++){
        if(i%n_work_per_core==0){
            groups.push_back(slice(serial_ranges, i, (i+n_work_per_core)-1));
            if(counter==core_count-1){
                groups.push_back(slice(serial_ranges, (i+n_work_per_core), serial_ranges_length-1));
                break;
            }
            counter=counter+1;
        }
    }

    std::cout << "groups.size = " << groups.size() << std::endl;

    std::vector<std::thread> v_th(core_count);

    std::chrono::system_clock::time_point start_time_point = std::chrono::high_resolution_clock::now();

    for(auto& element: groups){
        v_th.push_back(std::thread(classify_serial_numbers,std::ref(element), 
                                         std::ref(serial_numbers), 
                                         std::ref(output_file_of)));
    }

    for(auto& t: v_th){
        if(t.joinable()){
            t.join();
        }
    }

    serial_ranges_if.close();
    serial_numbers_if.close();
    output_file_of.close();

    std::chrono::system_clock::time_point end_time_point = std::chrono::high_resolution_clock::now();
    std::chrono::seconds duration = std::chrono::duration_cast<std::chrono::seconds>(end_time_point-start_time_point);
    std::cout << "Execution time : " << duration.count() << " seconds" << std::endl;

    return 0;
}

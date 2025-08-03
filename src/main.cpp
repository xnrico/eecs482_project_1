#include <iostream>
#include <fstream>
#include <cassert>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include "disk.h"
#include "cpu.h"
#include "thread.h"
#include "mutex.h"
#include "cv.h"

using std::cout;
using std::endl;

typedef struct requester {
    std::vector<int> requests;
    int requester_id;
} requester;

typedef struct request {
    int track;
    int requester_id;
} request;

std::vector<requester*> requesters;
std::vector<request*> queue;
std::vector<bool> completed;

int max_disk_queue = 0;
int num_requester = 0;
int total_requests = 0;
int serviced_requests = 0;
int num_living_threads = 0;

mutex mu1;
cv cv1, cv2;

void service_func(void* arg) {
    int current_track = 0;

    while (1) {
        mu1.lock();

        if (serviced_requests == total_requests && queue.empty()) {
            // std::cout << "Servicer Finished and Terminated\n";
            mu1.unlock();
            return;
        }

        // std::cout << "********************BEFORE WHILE********************\n";
        // std::cout << "[SERVICER] Num of Living threads: " << num_living_threads << "; Size of QUEUE: " << queue.size() << "/" << max_disk_queue << std::endl;
        // std::cout << "[SERVICER] Max Disk Queue: " << max_disk_queue << std::endl;

        while ((num_living_threads > max_disk_queue && (int)queue.size() < max_disk_queue) || (num_living_threads <= max_disk_queue && (int)queue.size() < num_living_threads)) {
            // std::cout << "********************INSIDE WHILE********************\n";
            // std::cout << "[SERVICER] Num of Living threads: " << num_living_threads << "; Size of QUEUE: " << queue.size() << "/" << max_disk_queue << std::endl;
            cv1.wait(mu1);
        }

        // std::cout << "********************SERVICER STARTED********************\n";
        // std::cout << "[SERVICER] Num of Living threads: " << num_living_threads << "; Size of QUEUE: " << queue.size() << "/" << max_disk_queue << std::endl;
        // std::cout << "[SERVICER] Current track is: " << current_track << std::endl;
        // std::cout << "[SERVICER] Tracks in the queue: [";

        int min_dist = 999999;
        int service_idx = 0;
        for (size_t i=0; i<queue.size(); ++i) { 
            // std::cout << queue[i]->track << " ";
            if (fabs(queue[i]->track - current_track) < min_dist) {
                min_dist = fabs(queue[i]->track - current_track);
                service_idx = i;
            }
        }
        // std::cout << "]\n";

        current_track = queue[service_idx]->track;
        completed[queue[service_idx]->requester_id] = true;
        print_service((unsigned int)queue[service_idx]->requester_id, (unsigned int)queue[service_idx]->track);
        queue.erase(queue.begin() + service_idx);
        ++serviced_requests;

        // std::cout << "[SERVICER] Completion: [";
        // for (size_t i=0; i<completed.size(); ++i) {
            // std::cout << completed[i] << ",";
        // }
        // std::cout << "]\n";

        // std::cout << "[SERVICER] " << serviced_requests << "/" << total_requests << " serviced...\n";
        // std::cout << "********************SERVICER FINISHED********************\n";

        cv2.broadcast();
        mu1.unlock();
    }

    // std::cout << "Here\n";
}

void request_func(void* arg) {
    requester* req = (requester*) arg;

    while (1) {
        mu1.lock();

        while ((completed[req->requester_id] == false) || 
                (!req->requests.empty() && 
                ((num_living_threads >= max_disk_queue && (int)queue.size() >= max_disk_queue) || (num_living_threads < max_disk_queue && (int)queue.size() >= num_living_threads)))) {
            // std::cout << "Requester [" << req->requester_id << "] waiting to issue request, " << req->requests.size() << " remaining\n";
            cv2.wait(mu1);
        }

        if (req->requests.size() > 0) {
            queue.push_back(new request{req->requests.front(), req->requester_id});
            print_request((unsigned int)req->requester_id, (unsigned int)req->requests.front());
            req->requests.erase(req->requests.begin());
            completed[req->requester_id] = false;

            cv1.signal();
            mu1.unlock();
        }

        else {
            --num_living_threads;
            // std::cout << "Requester " << req->requester_id << " thread terminated!\n";
            cv1.signal();
            mu1.unlock();
            return;
        }
    }
}

void parent(void* a) {
    thread t0(service_func, (void*)0);

    for (int i=0; i < num_requester; ++i) {
        thread t(request_func, (void*)(requesters[i]));
    }
}

int main(int argc, char** argv) {
    assert(argc > 2);
    max_disk_queue = atoi(argv[1]);
    assert(max_disk_queue > 0);

    for (int i = argc; i > 2; --i) {
        std::ifstream file(argv[2+num_requester]);
        if (!file) {
            std::cerr << "Error opening file: " << argv[2+num_requester] << std::endl;
            return 1;
        }
        ++num_requester;
        
        std::vector<int> tmp;
        int tmp_num;
        while (file >> tmp_num) {
            tmp.push_back(tmp_num);
            ++total_requests;
        }

        requester* req = new requester{tmp, num_requester-1};
        assert(!tmp.empty());
        requesters.push_back(req);
        completed.push_back(true);
    }

    assert((int)requesters.size() == num_requester);
    assert((int)completed.size() == num_requester);
    num_living_threads = num_requester;

    cpu::boot(parent, nullptr, 0);
    return 0;
}
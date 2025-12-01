/**
* @file interrupts_EP.cpp
* @author Alvan Chaudhury
* @brief External Priority Scheduler (No Preemption) for Assignment 3 Part 1
*/

#include "interrupts_Alvan_Chaudhury.hpp"

// External Priority Scheduler - sorts by priority (lower value = higher priority)
void priority_scheduler(std::vector<PCB> &ready_queue) {
    std::sort(
        ready_queue.begin(),
        ready_queue.end(),
        [](const PCB &first, const PCB &second){
            if(first.priority == second.priority) {
                return (first.arrival_time > second.arrival_time);  // FCFS for same priority
            }
            return (first.priority > second.priority);  // Lower priority number = higher priority
        }
    );
}

std::tuple<std::string> run_simulation(std::vector<PCB> list_processes) {
    std::vector<PCB> ready_queue;
    std::vector<PCB> wait_queue;
    std::vector<PCB> job_list;
    unsigned int current_time = 0;
    PCB running;
    idle_CPU(running);
    std::string execution_status;
    execution_status = print_exec_header();
    
    unsigned int time_since_last_io = 0;
    
    while(!all_process_terminated(job_list) || job_list.empty()) {
        
        // 1) Populate ready queue with arriving processes
        for(auto &process : list_processes) {
            if(process.arrival_time == current_time) {
                if(assign_memory(process)) {
                    process.state = READY;
                    ready_queue.push_back(process);
                    job_list.push_back(process);
                    execution_status += print_exec_status(current_time, process.PID, NEW, READY);
                }
            }
        }
        
        // 2) Manage wait queue 
        std::vector<PCB> temp_wait;
        for(auto &process : wait_queue) {
            process.time_in_io++;
            if(process.time_in_io >= process.io_duration) {
                process.state = READY;
                process.time_in_io = 0;
                ready_queue.push_back(process);
                sync_queue(job_list, process);
                execution_status += print_exec_status(current_time, process.PID, WAITING, READY);
            } else {
                temp_wait.push_back(process);
            }
        }
        wait_queue = temp_wait;
        
        // 3) Schedule processes 
        if(running.state == RUNNING) {
            // Process is running, execute for 1 time unit
            running.remaining_time--;
            time_since_last_io++;
            
            // Check if process needs I/O
            if(running.io_freq > 0 && time_since_last_io >= running.io_freq && running.remaining_time > 0) {
                // Move to I/O
                states old_state = running.state;
                running.state = WAITING;
                running.time_in_io = 0;
                wait_queue.push_back(running);
                sync_queue(job_list, running);
                execution_status += print_exec_status(current_time, running.PID, old_state, WAITING);
                idle_CPU(running);
                time_since_last_io = 0;
            } else if(running.remaining_time == 0) {
                // Process completed
                execution_status += print_exec_status(current_time, running.PID, RUNNING, TERMINATED);
                terminate_process(running, job_list);
                idle_CPU(running);
                time_since_last_io = 0;
            }
        }
        
        // Schedule new process if CPU is idle
        if(running.state == NOT_ASSIGNED && !ready_queue.empty()) {
            priority_scheduler(ready_queue);
            states old_state = ready_queue.back().state;
            run_process(running, job_list, ready_queue, current_time);
            execution_status += print_exec_status(current_time, running.PID, old_state, RUNNING);
            time_since_last_io = 0;
        }
        
        current_time++;
        
        // Safety check to prevent infinite loops
        if(current_time > 100000) {
            std::cerr << "Simulation timeout" << std::endl;
            break;
        }
    }
    
    execution_status += print_exec_footer();
    return std::make_tuple(execution_status);
}

int main(int argc, char** argv) {
    if(argc != 2) {
        std::cout << "ERROR!\nExpected 1 argument, received " << argc - 1 << std::endl;
        std::cout << "To run the program, do: ./interrupts <input_file>" << std::endl;
        return -1;
    }
    
    auto file_name = argv[1];
    std::ifstream input_file;
    input_file.open(file_name);
    
    if (!input_file.is_open()) {
        std::cerr << "Error: Unable to open file: " << file_name << std::endl;
        return -1;
    }
    
    std::string line;
    std::vector<PCB> list_process;
    while(std::getline(input_file, line)) {
        auto input_tokens = split_delim(line, ", ");
        auto new_process = add_process(input_tokens);
        list_process.push_back(new_process);
    }
    
    input_file.close();
    
    auto [exec] = run_simulation(list_process);
    write_output(exec, "execution.txt");
    
    return 0;
}

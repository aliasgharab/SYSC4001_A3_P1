/**
 * @file interrupts.cpp
 * @author Sasisekhar Govind
 * @brief template main.cpp file for Assignment 3 Part 1 of SYSC4001
 * 
 */

#include <interrupts_AliBundookwalla_MohamedGomaa.hpp>

void ExternalPriority(std::vector<PCB> &ready_queue) {
    std::sort( 
        ready_queue.begin(),
        ready_queue.end(),
        []( const PCB &first, const PCB &second ){
            // Higher priority first, then earlier arrival time for same priority
            if(first.PID != second.PID) {
                return first.PID < second.PID;
            }
            return first.arrival_time < second.arrival_time;
        } 
    );
}

std::tuple<std::string> run_simulation(std::vector<PCB> list_processes) {

    std::vector<PCB> ready_queue;   //The ready queue of processes
    std::vector<PCB> wait_queue;    //The wait queue of processes
    std::vector<PCB> job_list;      //A list to keep track of all the processes. This is similar
                                    //to the "Process, Arrival time, Burst time" table that you
                                    //see in questions. You don't need to use it, I put it here
                                    //to make the code easier :).

    unsigned int current_time = 0;
    PCB running;

    //Initialize an empty running process
    idle_CPU(running);

    std::string execution_status;

    //make the output table (the header row)
    execution_status = print_exec_header();

    //Loop while till there are no ready or waiting processes.
    //This is the main reason I have job_list, you don't have to use it.
    while(!all_process_terminated(job_list) || !ready_queue.empty() || !wait_queue.empty() || !list_processes.empty()) {

        //Inside this loop, there are three things you must do:
        // 1) Populate the ready queue with processes as they arrive
        // 2) Manage the wait queue
        // 3) Schedule processes from the ready queue

        //Population of ready queue is given to you as an example.
        //Go through the list of proceeses
        for(auto it = list_processes.begin(); it != list_processes.end(); ) {
            if(it->arrival_time == current_time) {//check if the AT = current time
                //if so, assign memory and put the process into the ready queue
                if(assign_memory(*it)) {
                    it->state = READY;  //Set the process state to READY
                    it->time_slice = 100; // Initialize time quantum for new processes
                    it->remaining_io_time = 0; // Initialize I/O timer
                    ready_queue.push_back(*it); //Add the process to the ready queue
                    job_list.push_back(*it); //Add it to the list of processes

                    execution_status += print_exec_status(current_time, it->PID, NEW, READY);
                    it = list_processes.erase(it);
                    continue;
                }
                // If memory allocation fails, keep in list to try later
            }
            ++it;
        }

        ///////////////////////MANAGE WAIT QUEUE/////////////////////////
        //This mainly involves keeping track of how long a process must remain in the ready queue
        // Process I/O completion in wait queue
        for(auto it = wait_queue.begin(); it != wait_queue.end(); ) {
            if(it->remaining_io_time > 0) {
                it->remaining_io_time--;
                
                if(it->remaining_io_time == 0) {
                    // I/O completed, move back to ready queue
                    it->state = READY;
                    ready_queue.push_back(*it);
                    sync_queue(job_list, *it);
                    execution_status += print_exec_status(current_time, it->PID, WAITING, READY);
                    it = wait_queue.erase(it);
                    continue;
                }
            }
            ++it;
        }  
        
        // 2. Retry memory allocation for processes waiting for memory
        for(auto it = list_processes.begin(); it != list_processes.end(); ) {
            if(it->arrival_time <= current_time) {  // Process has already arrived
                if(assign_memory(*it)) {
                    // Memory now available, add to ready queue
                    it->state = READY;
                    it->time_slice = 100;
                    it->remaining_io_time = 0;
                    ready_queue.push_back(*it);
                    job_list.push_back(*it);
                    execution_status += print_exec_status(current_time, it->PID, NEW, READY);
                    it = list_processes.erase(it);
                    continue;
                }
            }
            ++it;
        }
        /////////////////////////////////////////////////////////////////

        //////////////////////////SCHEDULER//////////////////////////////
        // If CPU is idle and ready queue has processes, schedule one
        if(running.PID == -1 && !ready_queue.empty()) {
            ExternalPriority(ready_queue); // Sort by priority
            running = ready_queue.front();
            ready_queue.erase(ready_queue.begin());
            running.start_time = current_time;
            running.state = RUNNING;
            sync_queue(job_list, running);
            execution_status += print_exec_status(current_time, running.PID, READY, RUNNING);
        }

        // If a process is running, execute it
        if(running.PID != -1) {
            running.remaining_time--;
            
            // Check if process finished
            if(running.remaining_time == 0) {
                execution_status += print_exec_status(current_time, running.PID, RUNNING, TERMINATED);
                terminate_process(running, job_list);
                idle_CPU(running);
            }
            // Check if I/O should be triggered
            else if(running.io_freq > 0 && (running.processing_time - running.remaining_time) % running.io_freq == 0) {
                execution_status += print_exec_status(current_time, running.PID, RUNNING, WAITING);
                running.state = WAITING;
                running.remaining_io_time = running.io_duration;
                wait_queue.push_back(running);
                sync_queue(job_list, running);
                idle_CPU(running);
            }
            // NO TIME QUANTUM CHECK (non-preemptive)
        }
        ///////////////////////////////////////////////////////////////
                
        current_time++;
    }
    
    //Close the output table
    execution_status += print_exec_footer();

    return std::make_tuple(execution_status);
}


int main(int argc, char** argv) {

    //Get the input file from the user
    if(argc != 2) {
        std::cout << "ERROR!\nExpected 1 argument, received " << argc - 1 << std::endl;
        std::cout << "To run the program, do: ./interrupts_RR <your_input_file.txt>" << std::endl;
        return -1;
    }

    //Open the input file
    auto file_name = argv[1];
    std::ifstream input_file;
    input_file.open(file_name);

    //Ensure that the file actually opens
    if (!input_file.is_open()) {
        std::cerr << "Error: Unable to open file: " << file_name << std::endl;
        return -1;
    }

    //Parse the entire input file and populate a vector of PCBs.
    //To do so, the add_process() helper function is used (see include file).
    std::string line;
    std::vector<PCB> list_process;
    while(std::getline(input_file, line)) {
        auto input_tokens = split_delim(line, ", ");
        auto new_process = add_process(input_tokens);
        list_process.push_back(new_process);
    }
    input_file.close();

    //With the list of processes, run the simulation
    auto result = run_simulation(list_process);
    std::string exec = std::get<0>(result);

    write_output(exec, "execution.txt");

    return 0;
}
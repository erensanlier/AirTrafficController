#include <iostream>
#include <iomanip>
#include <stdio.h>
#include <queue>
#include <string>
#include <list>
#include <pthread.h>
#include <cstdlib>
#include <sys/time.h>
#include <unistd.h>

//Time related functions
std::string getAnalogTime(){
    struct timeval tv;
    struct tm* ptm;
    char time_string[40];
    gettimeofday (&tv, nullptr);
    ptm = localtime (&tv.tv_sec);
    strftime (time_string, sizeof (time_string), "%Y-%m-%d %H:%M:%S", ptm);
    std::string temp = "- ";
    temp += time_string;
    temp += " - ";
    return temp;
}
int getCurrentTime(){
    struct timeval tv;
    gettimeofday (&tv, NULL);
    return tv.tv_sec;
}
int getFinishTime(int a){
    struct timeval tv;
    gettimeofday (&tv, NULL);
    return tv.tv_sec + a;
}

int pthread_sleep (int seconds){
    pthread_mutex_t mutex;
    pthread_cond_t conditionvar;
    struct timespec timetoexpire;
    if(pthread_mutex_init(&mutex,NULL))
    {
        return -1;
    }
    if(pthread_cond_init(&conditionvar,NULL))
    {
        return -1;
    }
    struct timeval tp;
    //When to expire is an absolute time, so get the current time and add //it to our delay time
    gettimeofday(&tp, NULL);
    timetoexpire.tv_sec = tp.tv_sec + seconds; timetoexpire.tv_nsec = tp.tv_usec * 1000;

    pthread_mutex_lock (&mutex);
    int res =  pthread_cond_timedwait(&conditionvar, &mutex, &timetoexpire);
    pthread_mutex_unlock (&mutex);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&conditionvar);

    //Upon successful completion, a value of zero shall be returned
    return res;

}

int last_landing_id = 0;
int last_departing_id = -1;
int last_emergency_id = 6666; // initial identifiers
int t = 1; // Time constant
int queues_log_time = 5; //default
double seconds = 120;
double probability = 0.5;

//Plane class
class Plane{
public:
    pthread_mutex_t *mutex;
    pthread_cond_t *cond_var;
    pthread_t *plane_tid;
    pthread_attr_t *plane_attr;
    int planeId;
    int status;
    int requestTime;
    int runwayTime = -1;
    Plane(int planeId, int status, int time) : planeId(planeId), status(status), requestTime(time) {
        mutex = (pthread_mutex_t *)(malloc(sizeof(pthread_mutex_t)));
        cond_var = (pthread_cond_t *)(malloc(sizeof(pthread_cond_t)));
        plane_tid = (pthread_t *)(malloc(sizeof(pthread_t)));
        plane_attr = (pthread_attr_t *)(malloc(sizeof(pthread_attr_t)));
        pthread_attr_init(plane_attr);
        pthread_mutex_init(mutex, nullptr);
        pthread_cond_init(cond_var, nullptr);
    }
};

//Atomic Queue data structure
class atomic_plane_queue{
    pthread_mutex_t mutex;
    std::queue <Plane> queue;
public:

    std::queue<Plane> &getQueue(){
        pthread_mutex_lock(&mutex);
        std::queue<Plane> &temp = queue;
        pthread_mutex_unlock(&mutex);
        return temp;
    }

    atomic_plane_queue(){
        pthread_mutex_init(&mutex, nullptr);
    }
    Plane& front(){
        pthread_mutex_lock(&mutex);
        Plane& temp = queue.front();
        pthread_mutex_unlock(&mutex);
        return temp;
    }
    bool empty(){
        pthread_mutex_lock(&mutex);
        bool temp = queue.empty();
        pthread_mutex_unlock(&mutex);
        return temp;
    }
    void push(Plane& plane){
        pthread_mutex_lock(&mutex);
        queue.push(plane);
        pthread_mutex_unlock(&mutex);
    }
    void pop(){
        pthread_mutex_lock(&mutex);
        queue.pop();
        pthread_mutex_unlock(&mutex);
    }

};

//Atomic Queues
atomic_plane_queue planes_landing;
atomic_plane_queue planes_departing;
atomic_plane_queue planes_emergency; //Even if emergency comes in every 40 sec, why treat different?
std::string get_queue_as_string(std::queue<Plane> q)
{
    std::string temp;
    //printing content of queue
    while (!q.empty()){
        temp += std::to_string(q.front().planeId);
        temp += "<-";
        q.pop();
    }
    return temp;
}

//Logging related functions
void log_final(std::list<Plane *> list, int startTime){

    const char separator = ' ';
    list.reverse();
    std::cout << "------------------------------------------------------------------------------\n";
    std::cout << std::flush << std::left << std::setw(15) << std::setfill(separator) << "Plane ID";
    std::cout << std::left << std::setw(15) << std::setfill(separator) << "Status";
    std::cout << std::left << std::setw(15) << std::setfill(separator) << "Request Time";
    std::cout << std::left << std::setw(15) << std::setfill(separator) << "Runway Time";
    std::cout << std::left << std::setw(15) << std::setfill(separator) << "Turnaround Time\n";

    while(!list.empty()){
        std::string status;
        if(list.front()->status == 0) status = "Landing";
        else if(list.front()->status == 1) status = "Departing";
        else status = "Emergency";
        std::cout << std::left << std::setw(15) << std::setfill(separator) << list.front()->planeId;
        std::cout << std::left << std::setw(15) << std::setfill(separator) << status;
        std::cout << std::left << std::setw(15) << std::setfill(separator) << list.front()->requestTime - startTime;
        if(list.front()->runwayTime == -1)
            std::cout << std::left << std::setw(15) << std::setfill(separator) << "Never";
        else
            std::cout << std::left << std::setw(15) << std::setfill(separator) << list.front()->runwayTime - startTime;
        if(list.front()->runwayTime == -1)
            std::cout << std::left << std::setw(15) << std::setfill(separator) << "∞";
        else
            std::cout << std::left << std::setw(15) << std::setfill(separator) << list.front()->runwayTime - list.front()->requestTime;
        std::cout<<std::endl;
        list.pop_front();
    }
    std::cout << "------------------------------------------------------------------------------\n";
}
void log_queue(int t) {
    std::string landQ = get_queue_as_string(planes_landing.getQueue());
    std::string depQ = get_queue_as_string(planes_departing.getQueue());
    std::string landLog = "Landing Queue at\tt = ";
    std::string depLog = "Departing Queue at\tt = ";
    landLog += std::to_string(t);
    landLog += ": ";
    depLog += std::to_string(t);
    depLog += ": ";
    std::cout << std::flush << landLog + landQ + "\n";
    std::cout << depLog + depQ + "\n";
    //std::cout << "landing Wait: " << getCurrentTime() - planes_landing.front().requestTime << " departing Wait: " << getCurrentTime() - planes_departing.front().requestTime << " Size Ratio: " << (double)planes_landing.size() / planes_departing.size() << std::endl;
}
void log_plane_request(Plane *plane) {
    std::string log = getAnalogTime();
    log += "[Plane ";
    log += std::to_string(plane->planeId);
    if(plane->status == 0){
        log += "] Requesting permission for landing.";
        std::cout << std::flush << log + "\n";
    }
    else if(plane->status == 1){
        log += "] Requesting permission for departing.";
        std::cout << std::flush << log + "\n";
    }
    else{
        log += "] Requesting permission for emergency landing.";
        std::cout << std::flush << log + "\n";
    }
}
void log_tower_finishes(Plane *plane) {
    std::string log = getAnalogTime();
    if(plane->status == 0){
        log += "[ATC] Plane ";
        log += std::to_string(plane->planeId);
        log += " successfully landed.";
        std::cout << std::flush << log + "\n";
    }
    else if(plane->status == 1){
        log += "[ATC] Plane ";
        log += std::to_string(plane->planeId);
        log += " successfully departed.";
        std::cout << std::flush << log + "\n";
    }
    else{
        log += "[ATC] Emergency Plane ";
        log += std::to_string(plane->planeId);
        log += " finished landing.";
        std::cout << std::flush << log + "\n";
    }
}
void log_tower_approvals(Plane *plane) {
    std::string log = getAnalogTime();
    if(plane->status == 0){
        log += "[ATC] Permission given to Plane ";
        log += std::to_string(plane->planeId);
        log += " to land.";
        std::cout << std::flush << log + "\n";
    }
    else if(plane->status == 1){
        log += "[ATC] Permission given to Plane ";
        log += std::to_string(plane->planeId);
        log += " to depart.";
        std::cout << std::flush << log + "\n";
    }
    else{
        log += "[ATC] Permission given to Plane ";
        log += std::to_string(plane->planeId);
        log += " for emergency landing.";
        std::cout << std::flush << log + "\n";
    }
}

//Plane thread functions
void *landing_plane(void *arg){

    Plane *plane = static_cast<Plane *>(arg);

    planes_landing.push(*plane);
    log_plane_request(plane);
    pthread_mutex_lock(plane->mutex);
    pthread_cond_wait(plane->cond_var, plane->mutex);
    pthread_mutex_unlock(plane->mutex);
    plane->runwayTime = getCurrentTime();
    pthread_exit(nullptr);
}
void *departing_plane(void *arg){
    Plane *plane = static_cast<Plane *>(arg);

    planes_departing.push(*plane);
    log_plane_request(plane);
    pthread_mutex_lock(plane->mutex);
    pthread_cond_wait(plane->cond_var, plane->mutex);
    pthread_mutex_unlock(plane->mutex);
    plane->runwayTime = getCurrentTime();
    pthread_exit(nullptr);

}
void *emergency_plane(void *arg){

    Plane *plane = static_cast<Plane *>(arg);
    planes_emergency.push(*plane);
    log_plane_request(plane);
    pthread_mutex_lock(plane->mutex);
    pthread_cond_wait(plane->cond_var, plane->mutex);
    pthread_mutex_unlock(plane->mutex);
    plane->runwayTime = getCurrentTime();
    pthread_exit(nullptr);

}

//Air Trafic Controller helper methods, class and thread function
void land_first_plane(){
    //TODO: Land the first plane in the landing queue.
    log_tower_approvals(&planes_landing.front());
    pthread_sleep(2 * t);
    log_tower_finishes(&planes_landing.front());
    //sending signal to plane
    pthread_mutex_t *mutex = planes_landing.front().mutex;
    pthread_cond_t *cond_var = planes_landing.front().cond_var;
    pthread_mutex_lock(mutex);
    planes_landing.pop();
    pthread_cond_signal(cond_var);
    pthread_mutex_unlock(mutex);
}
void depart_first_plane(){
    //TODO: Make the first plane in departing queue take off.
    log_tower_approvals(&planes_departing.front());
    pthread_sleep(2 * t);
    log_tower_finishes(&planes_departing.front());
    //sending signal to plane
    pthread_mutex_t *mutex = planes_departing.front().mutex;
    pthread_cond_t *cond_var = planes_departing.front().cond_var;
    pthread_mutex_lock(mutex);
    planes_departing.pop();
    pthread_cond_signal(cond_var);
    pthread_mutex_unlock(mutex);
}
void emergency_first_plane(){
    //TODO: Make the first plane in departing queue take off.
    log_tower_approvals(&planes_emergency.front());
    pthread_sleep(2 * t);
    log_tower_finishes(&planes_emergency.front());
    //sending signal to plane
    pthread_mutex_t *mutex = planes_emergency.front().mutex;
    pthread_cond_t *cond_var = planes_emergency.front().cond_var;
    pthread_mutex_lock(mutex);
    planes_emergency.pop();
    pthread_cond_signal(cond_var);
    pthread_mutex_unlock(mutex);
}
void *air_traffic_controller(void *arg){
    int finishTime = getCurrentTime() + *(int *) arg;
    while(getCurrentTime() < finishTime){
        /*
        if(!planes_landing.empty()){
            land_first_plane();
        }else if(!planes_departing.empty()){
            depart_first_plane();
        }
         */ //Prioritize Landing Only
        /*
        if(!planes_landing.empty() && planes_departing.size() < 5){
            land_first_plane();
        }else if(!planes_departing.empty()) {
            depart_first_plane();
        }
         */ //Prioritize Landing if Departing Queue < 5
        //Down below is my implemented scheduling algorithm
        if(!planes_emergency.empty()) emergency_first_plane();
        else if(!planes_landing.empty() && !planes_departing.empty()){
            double priority_multiplier = 0.7;
            double departing_wait_time = getCurrentTime() - planes_departing.front().requestTime;
            double landing_wait_time = getCurrentTime() - planes_landing.front().requestTime;
            bool prioritize_landing = false;
            if(landing_wait_time / departing_wait_time > priority_multiplier) prioritize_landing = true;
            if(prioritize_landing) land_first_plane();
            else depart_first_plane();
        }
        else if(!planes_landing.empty()) land_first_plane();
        else if(!planes_departing.empty()) depart_first_plane();
    }
    pthread_exit(nullptr);
}
class ControlTower{

    pthread_t *ct_tid;
    pthread_attr_t *ct_attr;

public:

    int sim_time{};

    ControlTower() {
        ct_attr = static_cast<pthread_attr_t *>(malloc(sizeof(pthread_attr_t)));
        ct_tid = static_cast<pthread_t *>(malloc(sizeof(pthread_t)));
        pthread_attr_init(ct_attr);
    }

    void init(int time){
        sim_time = time;
        pthread_create(ct_tid, ct_attr, air_traffic_controller, &sim_time);
    }

    pthread_t *getCtTid() const {
        return ct_tid;
    }
};

// I know it was kind of unnecessary to create a thread for a simulation but I liked it this way ^^
//Simulation class and thread function
void *runSimulation(void *args){

    std::list<Plane *> planeList;

    auto * ptr = static_cast<double *>(args);
    int sec = (int)((double *) ptr)[0];
    double prob = ((double *) ptr)[1];
    printf("\nSimulation will take %d seconds. Probability is %f. Operator Name: Walter R. White. -Albuquerque International Sunport\n", sec, prob);

    //Initial planes
    Plane *init;
    last_landing_id += 2;
    init = new Plane(last_landing_id, 0, getCurrentTime());
    planeList.push_front(init);
    pthread_create(init->plane_tid, init->plane_attr, landing_plane, init);
    last_departing_id += 2;
    init = new Plane(last_departing_id, 1, getCurrentTime());
    planeList.push_front(init);
    pthread_create(init->plane_tid, init->plane_attr, departing_plane, init);

    int startTime = getCurrentTime();
    int finishTime = getFinishTime(sec);
    while(getCurrentTime() < finishTime){
        pthread_sleep(1 * t);
        Plane *temp;
        if((getCurrentTime() - startTime) % (40 * t) == 0){
            temp = new Plane(last_emergency_id++, 2, getCurrentTime());
            planeList.push_front(temp);
            pthread_create(temp->plane_tid, temp->plane_attr, emergency_plane, temp);
        }else{
            int random = rand() % 100;
            if(random < 50){
                //TODO: Create one plane thread landing
                last_landing_id += 2;
                temp = new Plane(last_landing_id, 0, getCurrentTime());
                planeList.push_front(temp);
                pthread_create(temp->plane_tid, temp->plane_attr, landing_plane, temp);
            }else{
                //TODO: Create one plane thread departing
                last_departing_id += 2;
                temp = new Plane(last_departing_id, 1, getCurrentTime());
                planeList.push_front(temp);
                pthread_create(temp->plane_tid, temp->plane_attr, departing_plane, temp);
            }
        }
        if((getCurrentTime() - startTime) % queues_log_time == 0){
            log_queue(getCurrentTime() - startTime);
        }
    }
    pthread_sleep(1);
    log_final(planeList, startTime);
    return nullptr;

}
class Simulation{
    ControlTower *control_tower;
    pthread_t *sim_tid;
    pthread_attr_t *sim_attr;
    double s;
    double p;
    double args[2];

public:
    Simulation(double s, double p, ControlTower *ct) : s(s), p(p) {
        sim_attr = static_cast<pthread_attr_t *>(malloc(sizeof(pthread_attr_t)));
        sim_tid = static_cast<pthread_t *>(malloc(sizeof(pthread_t)));
        pthread_attr_init(sim_attr);
        args[0] = s;
        args[1] = p;
        control_tower = ct;
    }
    void run(){
        control_tower->init((int) args[0]);
        pthread_create(sim_tid, sim_attr, runSimulation, args);
    }
    pthread_t *getSimTid() const {
        return sim_tid;
    }
};

int main(int argc, char **args)
{
    //Parse Command Line Args
    //TODO: Parse from CLI

    std::string sArg = "-s";
    std::string pArg = "-p";
    std::string seedArg = "-seed";
    std::string queueLogArg = "-n";

    for(int i = 1; i < argc; i+=2){
        int value = std::atoi(args[i+1]);
        if(sArg == args[i]) seconds = value;
        if(pArg == args[i]) probability = std::stod(args[i+1]);
        if(seedArg == args[i]) srand(value);
        if(queueLogArg == args[i]) queues_log_time = value;
    }

    // Create ControlTower instance, auto creates thread
    auto *ct = new ControlTower();

    //Init simulation
    auto *sim = new Simulation(seconds, probability, ct);
    sim->run();

    pthread_join(*sim->getSimTid(), nullptr);

}
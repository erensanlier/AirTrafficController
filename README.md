# Air Traffic Controller Simulator

This program simulates events occurring an Air Traffic Controller. It was implemented using pthreads library. With only one runway available, multiple Plane threads were scheduled to share the resource. 

mutex and cond_var used to prevent collision on the runway.

## About implemented scheduling algorithm

To prevent starvation of Plane threads, the scheduling algorithm decides which plane to go comparing waiting times of the first elements on the both queues.

```c
if((wait_time_of_landing_plane / wait_time_of_departing_plane) > some_multiplier){
    favor_landing_plane();
}
```

The scheduling algorithms that favors landing planes and favoring landing planes if departing queue < 5 can be found commented out in the air_traffic_control function.

## About implemented atomic queue

A data structure named atomic_plane_queue was implemented. This class has a pthread_mutex and a std::queue inside. For every function call made to the queue, mutex gets locked and released after execution. One thread at a time can access the queue using this method.

## Usage

```shell
g++ main.cpp -pthread
./a.out
```

Default duration is set to 200 seconds.

Default probability of landing plane is set to 0.5.

Default queue log time is set to 5 seconds.

Parameters in the section down below can be added. Also to output can be directed using the notation:

```shell
./a.out > output.log
```

## Parameters

The order of parameters don't matter. Errors may occur if the format was not followed.

Duration : -s (time in seconds)

Probability : -p (double between 0 and 1)

Queue Log Recurrence : -n (time in seconds)

Seed for randomness (if preferred) : -seed (seed value)



## Logging
During the runtime of threads, they log their activities to the standard output. 3 types of logging was implemented:

1- Events occurred at all seconds 

2- Queue situation every n seconds

3- Snapshot of planes after the simulation finishes.

```shell
out.log file prepared with -> ./a.out -s 60 -p 0.5 -n 5 -seed 1
```


## License
[MIT](https://choosealicense.com/licenses/mit/)

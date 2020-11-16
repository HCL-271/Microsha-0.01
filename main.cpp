#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <list>
#include <vector>
#include <iostream>
#include <fcntl.h>
#include <fnmatch.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/times.h>
#include <sys/time.h> //Not a lot libraries

using namespace std; //I know, that it's a bad practise.

#define T_RED   "\033[1;31m" //Let's add some colors
#define T_GRN   "\033[1;32m"
#define T_YEL   "\033[1;33m"
#define T_BLU   "\033[1;34m"
#define T_MAG   "\033[1;35m"
#define T_CYN   "\033[1;36m"
#define T_WHT   "\033[1;37m"
#define T_RES   "\033[0m"

class Microsha { //Lets do it like class
    private:

    bool is_running = true;
    bool is_timing = false;

    class Timer {
        struct timevalue tv1,tv2,dtv;
        struct timezone tz; //Yes, this struct won't be support in future
        struct rusage rusage;
        double utime_st, stime_st;

        public:
        double rtime, utime, stime; //real time, user time and sys time

        int time_start() { 
            gettimeofday(&tv1, &tz);
            utime_st = (double) rusage.ru_utime.tv_sec + (double) rusage.ru_utime.tv_usec / 1000000.0; \\total amou
            stime_st = (double) rusage.ru_stime.tv_sec + (double) rusage.ru_stime.tv_usec / 1000000.0;
            return 0;
        }

        int time_count() { 
            gettimeofday(&tv2, &tz);
            dtv.tv_sec = tv2.tv_sec - tv1.tv_sec;
            dtv.tv_usec = tv2.tv_usec - tv1.tv_usec;
            if(dtv.tv_usec < 0) {
                dtv.tv_sec--; 
                dtv.tv_usec += 1000000;
            }
            if(getrusage(RUSAGE_CHILDREN, &rusage) == -1) return 1;
            rtime = (double) (dtv.tv_sec * 1000 +  dtv.tv_usec / 1000) / 1000.0;
            utime = (double) rusage.ru_utime.tv_sec + (double) rusage.ru_utime.tv_usec / 1000000.0 - utime_st;
            stime = (double) rusage.ru_stime.tv_sec + (double) rusage.ru_stime.tv_usec / 1000000.0 - stime_st;
        }
    };

    Timer timer;

    int sh_time() {
        if(!timing) return 0;
        timing = false;
        int err = timer.time_count();
        if (err != 0) {
            printf("real \t%.4lfs\n", timer.rtime);
            printf("user \t%.4lfs\n", timer.utime);
            printf("sys  \t%.4lfs\n", timer.stime);
            return 0;
        }
        perror("time");
        return err;
    }



int main(int argc, char* argv[], char* env[]) {
    Microsha microshell = Microsha();
    microshell.run();
}

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
        struct timeval tv1,tv2,dtv;
        struct timezone tz; //Yes, this struct won't be support in future
        struct rusage rusage;
        double utime_st, stime_st;

        public:
        double rtime, utime, stime; //real time, user time and sys time

        int time_start() { 
            gettimeofday(&tv1, &tz);
            utime_st = (double) rusage.ru_utime.tv_sec + (double) rusage.ru_utime.tv_usec / 1000000.0; 
            /* ru_utime
              This is the total amount of time spent executing in user mode,
              expressed in a timeval structure (seconds plus microseconds). */
                  /*Текущее время системы, выраженное в количестве секунд и микросекунд,
прошедших с 00:00:00 UTC 1 января 1970 года, может быть получено с
помощью gettimeofday(3) и установлено с помощью settimeofday(3).
Аргументом этого системного вызова является адрес struct timeval,
которая содержит следующие поля:
long tv_sec;  seconds since Jan. 1, 1970 
long tv_usec; microseconds  */
                  
            stime_st = (double) rusage.ru_stime.tv_sec + (double) rusage.ru_stime.tv_usec / 1000000.0;
            return 0;
            
            /* ru_stime
              This is the total amount of time spent executing in kernel
              mode, expressed in a timeval structure (seconds plus microsec‐
              onds). */
        }

        int time_count() { 
            gettimeofday(&tv2, &tz);
            dtv.tv_sec = tv2.tv_sec - tv1.tv_sec;
            dtv.tv_usec = tv2.tv_usec - tv1.tv_usec;
            if(dtv.tv_usec < 0) {
                dtv.tv_sec--; 
                dtv.tv_usec += 1000000;
            }
            /*RUSAGE_CHILDREN
Возвращает статистику использования ресурсов для всех дочерних процессов вызывающего процесса, которые были завершены и ожидали. 
Эта статистика будет включать ресурсы, используемые внуками, и удаленными потомками, если все промежуточные потомки ждали своих завершенных потомков.
*/
            if(getrusage(RUSAGE_CHILDREN, &rusage) == -1) return 1;
            rtime = (double) (dtv.tv_sec * 1000 +  dtv.tv_usec / 1000) / 1000.0;
            utime = (double) rusage.ru_utime.tv_sec + (double) rusage.ru_utime.tv_usec / 1000000.0 - utime_st;
            stime = (double) rusage.ru_stime.tv_sec + (double) rusage.ru_stime.tv_usec / 1000000.0 - stime_st;
        }
    };

    Timer timer;//sounds awful

    int sh_time() {
        if(!is_timing) return 0;
        is_timing = false;
        int err = timer.time_count();
        if (err != 0) {
            printf("real \t%.4lfs\n", timer.rtime);
            printf("user \t%.4lfs\n", timer.utime);
            printf("sys  \t%.4lfs\n", timer.stime);
            return 0;
        }
        perror("time");
        /*
        Описание:

Функция выводи сообщение об ошибке в поток сообщений об ошибке. 
В начале выводится строка, на которую указывает аргумент функции, затем выводится двоеточие и пробел, затем выводится сообщение об ошибке, соответствующей значению errno.

Если на момент вызова функции ошибок не зафиксировано или информация о них удалена, то в качестве сообщения об ошибке будет выведено сообщение: Success. 
*/
        return err;
    }
 int sh_cd(vector<string> & comm) { /* переход по директории(папке) */
        if(comm.size() > 2) {
            perror("cd: to many arguments");
            return 1;
        }
        if(comm.size() == 1) {
            if(chdir(getenv("HOME")) == 0) {
                return 0;
            }
            perror("cd");
            return 1;
        }
        if(chdir(comm[1].c_str()) != 0) {
            perror("cd");
            return 1;
        }
        return 0;
    }

    int execCommand(vector<string> & comm, int in, int out, int convSize, int * fd) {
        if(comm[0] == "cd" && convSize == 2) sh_cd(comm);
        pid_t pid = fork(); 
        if(pid == 0) {
            char ** args = new char *[comm.size() + 1];
            for(int i = 0; i < comm.size(); i++)
                args[i] = &(comm[i][0]);
            args[comm.size()] = NULL;
            
            int din = 0; if(in != 0) din = dup2(in, 0);
            int dout = 1; if(out != 1) dout = dup2(out, 1);

            for(int i = 0; i < (convSize - 2) * 2; i++)
                if(fd[i] != in && fd[i] != out) close(fd[i]);
    
            if(comm[0] == "cd") exit(0);

            execvp(comm[0].c_str(), args);
            perror(comm[0].c_str());
            for(int i = 0; i < comm.size(); i++) delete [] args[i];
            delete [] args;
            exit(0);
        } else if(pid < 0) {
            perror("fork error : pid < 0");
            exit(0);
        }
        return 0;
    }
    
    int runConv(list<vector<string>> & conv) {
        vector<string> in_out = *(conv.begin());
        int in = 0, out = 1;

        if(in_out[1].length() > 0) {
            if(strcmp("<", in_out[0].c_str()) == 0) {
                in = open(in_out[1].c_str(), O_RDONLY, 0666);
                if(in < 0) {
                    perror("< file not found");
                    return 1;
                }
            }
        }
        if(in_out[3].length() > 0) {
            if(strcmp(">", in_out[2].c_str()) == 0) {
                out = open(in_out[3].c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
                if(out < 0) {
                    close(in);
                    perror("> file not found");
                    return 1;
                }
            }
            if(strcmp(">>", in_out[2].c_str()) == 0) {
                out = open(in_out[3].c_str(), O_WRONLY | O_CREAT | O_APPEND, 0666);
                if(out < 0) {
                    close(in);
                    perror(">> file not found");
                    return 1;
                }
            }
        }

        int * fd = new int[(conv.size() - 2) * 2];
        for(int i = 0; i < conv.size() - 2; i++) pipe(fd + i * 2);

        timer.time_start(); 

        if(conv.size() == 2) {
            auto it = conv.begin();
            it++;
            execCommand(*it, in, out, conv.size(), fd);
        } else {
            auto it = conv.end();
            it--;
            for(int i = 0; i < conv.size() - 1; i++, it--) {
                if(i == 0) {
                    execCommand(*it, in, fd[1], conv.size(), fd);
                    continue;
                }
                if(i == conv.size() - 2) {
                    execCommand(*it, fd[2 * i - 2], out, conv.size(), fd);
                    continue;
                }
                execCommand(*it, fd[2 * i - 2], fd[2 * i + 1], conv.size(), fd);
            }
        }

        for(int i = 0; i < (conv.size() - 2) * 2; i++) close(fd[i]);

        delete [] fd;

        int code;
        while(wait(&code) > 0); 

        return 0;
    }

    int nextDepth(vector<string> & path, int dep, string name, vector<string> & res) {
        if(!(dep < path.size())) {
            res.push_back(name);
            return 1;
        }
        struct stat st;
        if (stat(name.c_str(), &st) < 0) {
            perror(name.c_str()); return 1;
        }
        if (S_ISDIR(st.st_mode)) {
            DIR *d = opendir(name.c_str());
            if (d == NULL) {
                perror(name.c_str()); return 1;
            }
            for (dirent *de = readdir(d); de != NULL; de = readdir(d)) {
                string dent = de->d_name;
                string pattern = path[dep];
                if(fnmatch(pattern.c_str(), dent.c_str(), FNM_PATHNAME) == 0) {
                    if(dent == "." && pattern != ".") continue;
                    if(dent == ".." && pattern != "..") continue; 
                    string fname;
                    if(dep == 1) {
                        if(name == "/") {
                            fname = "/" + dent;
                        } else if(name == ".") {
                            fname = dent;
                        } else {
                            fname = name + "/" + dent;
                        }
                    } else {
                        fname = name + "/" + dent;
                    }
                    
                    nextDepth(path, dep + 1, fname, res);
                }
            }
            closedir(d);
        }
        return 0;
    }

    int removeSlash(string & str) {
        string res = "";
        bool sl = false;
        for(const char * cstr = str.c_str(); *cstr != '\0'; cstr++) {
            char c = *cstr;
            switch (c) {
            case '\\' :
                if(!sl) {
                    sl = true;
                    continue;
                }
                break;
            }
            res += c;
            sl = false;
        }
        str = res;
        return 0;
    }

    int updateConv(list<vector<string>> & conv) {
        int err = 0;
        for(auto it = conv.begin(); it != conv.end(); it++) {
            if(it == conv.begin()) it++;
            vector<string> form;
            for(int i = 0; i < it->size(); i++) {
                string & str = it->at(i);
                vector<string> path;

                for(const char * cstr = str.c_str(), * cstr2 = str.c_str(); (cstr2 =  strchr(cstr, '/')) != NULL; cstr = cstr2) {
                    path.push_back(string(cstr, cstr2 - cstr));
                    cstr2++;
                }
                const char * cstr = strrchr(str.c_str(), '/'); 
                string slash = "";
                if(cstr != NULL) {
                    if(cstr[1] != '\0') path.push_back(string(cstr + 1));
                    else if(strcmp(str.c_str(), "/") != 0) slash = "/";
                }
                if(cstr == NULL) {
                    path.push_back(string(str));
                }

                if(strcmp(path[0].c_str(), "") == 0) {
                    path[0] = "/";
                } else if (strcmp(path[0].c_str(), "~") == 0) {
                    char * p = getenv("HOME");
                    if(p == NULL) {
                        perror("getenv('HOME')");
                        return 1;
                    }
                    
                } else {
                    path.insert(path.begin(), ".");
                }

                vector<string> res;
                nextDepth(path, 1, path[0], res);
                if(res.size() > 0) {
                    for(int k = 0; k < res.size(); k++) {
                        string str2 = res[k] + slash;
                        removeSlash(str2);
                        form.push_back(str2);
                    }
                } else {
                    removeSlash(str);
                    form.push_back(str);
                }
            }
            *it = form;
        }
        return 0;
    }

    int nextWord(string & line, int & ind, string & ret) {
        ret = "";
        for(; ind < line.length() && line[ind] == ' '; ind++);
        for(bool ek = false, es = false; ind < line.length(); ind++) {
            char c = line[ind];
            switch (c) {
            case ' ' :
            case '<' :
            case '>' :
            case '|' :
                if(!ek && !es) {
                    if(ret.length() == 0) {
                        ret += c;
                        ind++;
                        if(c == '<' && ind < line.length() && line[ind] == c) {
                            ret += c;
                            ind++;
                        }
                        if(c == '>' && ind < line.length() && line[ind] == c) {
                            ret += c;
                            ind++;
                        }
                    }
                    return 0;
                }
                break;
            case '\\' :
                if(!es && !ek) {
                    es = true;
                    ret += '\\';
                    continue;
                }
                if(ek) ret += '\\';
                break;
            case '*' :
                if(ek) ret += '\\';
                break;
            case '?' :
                if(ek) ret += '\\';
                break;
            case '\'':
                if(!es) {
                    ek = !ek;
                    continue;
                }
            }
            es = false;
            ret += c;
        }
        return 0;
    }

    int parseLine(string line, list<vector<string>> & conv) {
        conv = list<vector<string>>();
        conv.push_front(vector<string>());

        vector<string> in_out = vector<string>(4, string());
        in_out[1] = in_out[3] = "";
        in_out[0] = "<";
        in_out[2] = ">";

        bool in = false;
        bool out = false;
        for(int i = 0; i < line.length();) {
            string word;
            nextWord(line, i, word);
            if(word.length() == 0) continue;
            if(in) {
                in_out[1] = word;
                in = false;
                continue;
            }
            if(out) {
                in_out[3] = word;
                out = false;
                continue;
            }
            if(strcmp(word.c_str(), "<") == 0 || strcmp(word.c_str(), "<<") == 0) {
                if(in || in_out[1].length() > 0) return -1;
                in_out[0] = word;
                in = true;
                continue;
            }
            if(strcmp(word.c_str(), ">") == 0 || strcmp(word.c_str(), ">>") == 0) {
                if(out || in_out[3].length() > 0) return -1; //error
                in_out[2] = word;
                out = true;
                continue;
            }
            if(strcmp(word.c_str(), "|") == 0) {
                conv.push_front(vector<string>());
                continue;
            }
            conv.front().push_back(word);
        }
        conv.push_front(in_out);
        for(auto it = conv.begin(); it != conv.end(); it++) {
            if(it->size() <= 0) return 1;
        }
        return 0;
    }

    int readLine(string & line) {
        char * user = NULL;
        if(getuid() == 0) {
            if((user = getenv("SUDO_USER")) == NULL) return -1;
        } else {
            if((user = getenv("LOGNAME")) == NULL) return -1;
        }

        char * path = new char[1000];
        char * cpath = path;
        if(getcwd(path, 1000 * sizeof(char)) == NULL) return -1;

        string SPath = "";

        if(strstr(path, "/home/") == path && strstr(path, user) == path + strlen("/home/")) {
            path += strlen("/home/");
            path = strchr(path, '/');          

            if(path == NULL || *(path + 1) == '\0') SPath = "~";
            else SPath = "~" + string(path);
        } else {
            SPath = string(path);
        }

        printf(T_GRN "%s" T_RES ":" T_BLU "%s" T_RES "%c ", user, SPath.c_str(), getuid() == 0 ? '!' : '>');
        fflush(stdout);

        delete [] cpath;

        line = "";
        if(!getline(cin, line)) {
            perror("getline");
            return -1;
        }
        if(line == "q") is_running = false;
        if(line.find("time") == 0) {
            is_timing = true;
            line = line.substr(strlen("time"));
        }
        return 0;
    }    

    static void signals (int signal) {
        switch (signal) {
            case SIGINT :
                printf("^C signal\n");
                break;
            case SIGTSTP :
                printf("^Z signal\n");
                break;
            default :
                break;
        } 
    }
    
    int setSignals() {
        signal(SIGINT, signals);
        signal(SIGTSTP, signals);
        return 0;
    }

    
    void printConv(list<vector<string>> & conv) {
        for(auto it = conv.begin(); it != conv.end(); it++) {
            for(int i = 0; i < it->size(); i++) {
                cout << it->at(i) << " ";
            }
            printf("\n");
        }
    }

    public:
    int run() {
        printf("# Microshell v.0.0.1\n\n");
        setSignals();
        int err = 0;
        while(true) {
            string line = "";
            err = readLine(line);
            if(err < 0) return err;
            if(err > 0) continue;

            if(!is_running) break;

            if(line.length() == 0) {
                timer.time_start(); 
                sh_time();
                continue;
            }

            list<vector<string>> conv;
            err = parseLine(line, conv);
            if(err < 0) return err;
            if(err > 0) {
                timer.time_start(); 
                sh_time();
                continue;
            }
            
            err = updateConv(conv);
            if(err < 0) return err;
            if(err > 0) continue;

            timer.time_start();           

            err = runConv(conv);
            if(err < 0) return err;
            if(err > 0) continue;

            err = sh_time();
            if(err < 0) return err;
            if(err > 0) continue;
        }
        return 0;
    }
};


int main(int argc, char* argv[], char* env[]) {
    Microsha microshell = Microsha();
    microshell.run();
}

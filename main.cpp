#include <iostream>
#include <stack>
#include <vector>
#include <queue>
#include <unordered_map>
#include <iomanip> 
#include <string>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>
#include<sys/wait.h> 
#include <grp.h>
#include <stdlib.h>
#include <time.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sstream>
#include<algorithm>
#include<iterator>
#include <fstream>

using namespace std;
#define SIZE 256
#define LIMIT 20
#define clrscr printf("\033[H\033[J") //for clearing the entire screen
#define clrln printf("\033[K") //for clearing a line

//global variables and containers such as stacks, queues used
char cwd[SIZE];
string dest; 
stack<string> back_stack;
stack<string> fwd_stack;
vector<string> vec;  
vector<string> name_vec;
vector<string> cmd_vec;
queue<string> dir_queue;

int start;
int k_flag=0;
int l_flag=0;
int dir_flag=0;
int left_flag=0;
int right_flag=0;
int home_flag=0;
int back_flag=0;
int cmd_mode=0;
int raw_mode=0;
int flag=1;
int search_flag = 0;

struct termios old_terminal;
struct termios new_terminal;


//function declarations
void get_dir(const char *path);
void print_dir(struct stat f_stat);
void file_stat(struct stat f_stat);
const char *human_readable(long long bytes, char *s);
void start_raw_mode(string path);
void set_raw_flags();
void end_raw_mode();
void start_command_mode();
string char_to_string(char *a);
void reset_cursor(int x, int y);
void scrolling();
int min(int a, int b);
void open_file(string filename);
void get_cmd();
void create_file();
void create_dir();
void copy();
void copy_file(const char *source, const char* destination);
void delete_file();
void delete_dir(string to_delete);
void move();
void rename();
void copy_dir(string dest);
void move_dir(string dest);
void search(string search_dir);
void goto_dir();


//for determining minimum of two values
int min(int a, int b)
{
    if(a<b) return a;
    else return b;
}


//for opening file in vi
void open_file(string filename)
{
  
    char path[filename.length() + 1];
    strcpy(path, filename.c_str());
    pid_t pid = fork();
    if(pid == 0)
    {
        fflush(stdin);
        char execname[] = "vi";
        char *exec_arg[] = {execname, path, NULL};
        execv("/usr/bin/vi", exec_arg);
                            
    }
    wait(NULL);

}

//conversion of character to string
string char_to_string(char *a)
{
    string str = "";
    int i;
    for(i=0; a[i]!='\0'; i++) str += a[i];
    return str;
}

//to receive commands from user and store them in vector
void get_cmd()
{
    string str;
    getline(cin, str);
    stringstream ss(str);
    while(ss >> str) cmd_vec.push_back(str);
}

//for setting the flags necessary for our normal (raw) mode
void set_raw_flags()
{
    tcgetattr(STDIN_FILENO, &old_terminal);
    new_terminal = old_terminal;
    new_terminal.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_terminal);
}

// starting the normal mode 
void start_raw_mode(string path)
{
    start = 0;  
    struct stat f_stat;
    const char *dir_path = path.c_str();
    get_dir(dir_path);
    print_dir(f_stat);

}

//end of normal mode; unsetting the flags
void end_raw_mode()
{
    tcsetattr(STDIN_FILENO,TCSANOW,&old_terminal);
}

//start of command mode
void start_command_mode()
{
    
    
    int counter=0;
    reset_cursor(100, 0);
    char ch;

    while(1)
    {
        if(!cmd_vec.empty()) cmd_vec.clear();
        get_cmd();
        if(cmd_vec[0] == "q") break;
        if(cmd_vec[0] == "create_file") 
        {
            counter++;
            create_file();
            reset_cursor(100, 0);
            clrln;
        }
        if(cmd_vec[0] == "create_dir")
        {
            create_dir();
            reset_cursor(100, 0);
            clrln;
        }
        if(cmd_vec[0] == "copy")
        {
            copy();
            reset_cursor(100, 0);
            clrln;
        }
        if(cmd_vec[0] == "delete_file")
        {
            delete_file();
            reset_cursor(100, 0);
            clrln;
        }
        if(cmd_vec[0] == "delete_dir")
        {
            string path;
            if(cmd_vec[1][0] == '~') path = char_to_string(cwd) + "/" + cmd_vec[1].substr(2, cmd_vec[1].size());
            else if(cmd_vec[1][0] == '.' && cmd_vec[1].size() > 1) path = back_stack.top() + "/" + cmd_vec[1].substr(2, cmd_vec[1].size());
            else if(cmd_vec[1][0] == '.' && cmd_vec[1].size() == 1) path = back_stack.top() + "/" + cmd_vec[1];
            else if(cmd_vec[1][0] == '/') path = cmd_vec[1];
            else path = back_stack.top() + "/" + cmd_vec[1];
            delete_dir(path);
            reset_cursor(100, 0);
            clrln;
        }
        if(cmd_vec[0] == "move")
        {
            move();
            reset_cursor(100, 0);
            clrln;
        }
        if(cmd_vec[0] == "rename")
        {
            rename();
            reset_cursor(100, 0);
            clrln;
        }

        if(cmd_vec[0] == "search")
        {
            search(back_stack.top().c_str());
        
            reset_cursor(100, 0);
            clrln;
            if(search_flag) cout << "True"; 
            else cout << "False";
            search_flag=0;
        }

        if(cmd_vec[0] == "goto")
        {
            
            goto_dir();
            reset_cursor(100, 0);
            clrln;
        }

        if(cmd_vec[0] == "esc")
        {
            raw_mode=1;
            reset_cursor(100, 0);
            clrln;
            break;
        }
        
    }

    if(raw_mode)
    {
        raw_mode=0;
        set_raw_flags();
        start_raw_mode(back_stack.top());
    }
}

//for listing all the files and sub-directories inside a directory
void get_dir(const char *path)
{
    DIR *dir = opendir(path);
    if(!dir) return;
    struct dirent *d;
    d = readdir(dir);
    if(!vec.empty()) vec.clear();
    if(!name_vec.empty()) name_vec.clear();
    
    while(d)
    {
        string temp = back_stack.top() + "/" + d->d_name;
        vec.push_back(temp);
        name_vec.push_back(d->d_name);

        d = readdir(dir);
    }

    closedir(dir);
   
}

//printing out the files and sub-directories along with their information
void print_dir(struct stat f_stat)
{
    
    clrscr;
    reset_cursor(0, 0);

    int j;
    
    for(j=start; j < min(name_vec.size(), start+LIMIT); j++)
    {
        const char *name = vec[j].c_str();
        if(name_vec[j].size()>10) cout << setw(20) << left << name_vec[j].substr(0, 10) + "...";
        else cout << setw(20) << left << name_vec[j];
        if(stat(name, &f_stat)==0) file_stat(f_stat);
        cout << endl;
    }

    scrolling();

}

//for navigating through the file explorer
void scrolling()
{
    char ch;
    int i=1;
    if(l_flag)
    {
        reset_cursor(LIMIT, 0);
        i = min(LIMIT, vec.size()-start);
    }
    else reset_cursor(0, 0);
    l_flag=0;
    k_flag=0;
    struct stat f_stat;

    while((ch=cin.get())!='q')
    {
        //up
        if(ch == 65 && i>1)
        {
            i--;
            reset_cursor(i, 0);
        }
        //down
        if(ch == 66 && i<min(vec.size()-start, LIMIT))
        {
            i++;
            reset_cursor(i, 0);
        }
        //k 
        if(ch == 'k' && i == 1 && start!=0)
        {
            start--;
            k_flag=1;
            break;
        }
        //l
        if(ch == 'l' && i == LIMIT)
        {
            start++;
            l_flag=1;
            break;
        }
        //enter
        if(ch == 10)
        {
            const char *temp = vec[start+i-1].c_str();
         
            stat(temp, &f_stat);
            if(S_ISREG(f_stat.st_mode)==1) open_file(vec[start+i-1]);
            if(S_ISREG(f_stat.st_mode)==0)
            {
                dir_flag = 1;
    
                back_stack.push(vec[start+i-1]);
                break;
            }
    
        }

        //left
        if(ch == 68)
        {
            if(!back_stack.empty())
            {
                left_flag = 1;
                string tmp = back_stack.top();
                if(fwd_stack.empty()) fwd_stack.push(tmp);
                else if(tmp != fwd_stack.top()) fwd_stack.push(tmp);
                if(back_stack.size()>1) back_stack.pop();
                break;
            }
        }

        //right
        if(ch == 67)
        {
            if(!fwd_stack.empty())
            {
                right_flag = 1;
                string tmp = fwd_stack.top();
                if(back_stack.empty()) back_stack.push(tmp);
                else if(tmp != back_stack.top()) back_stack.push(tmp);
                if(fwd_stack.size()>1) fwd_stack.pop();
                break;   
            }
        }

        //backspace
        if(ch == 127)
        {
            back_flag = 1;
            while(!fwd_stack.empty()) fwd_stack.pop();
            string tmp = back_stack.top();
            unsigned int n = tmp.length();
            int it = n-1;
            while(it>0 && tmp[it] != '/') it--;
            if(it==0) it++;
            tmp = tmp.substr(0, it);
            if(back_stack.top() != tmp) back_stack.push(tmp);

            break;
        }
        
        //home
        if(ch == 'h')
        {
            home_flag=1;
            while(!fwd_stack.empty()) fwd_stack.pop();
            if(back_stack.top() != cwd) back_stack.push(cwd);
            break;
        }

        //switching to command mode
        if(ch == ':')
        {
            cmd_mode=1;
            break;
        }

       
    }

   

    if(l_flag || k_flag)
    {
        clrscr; 
        print_dir(f_stat);
    }

    if(dir_flag)
    {
       clrscr; 
       dir_flag=0;
       start_raw_mode(back_stack.top());

    }

    if(left_flag)
    {
        clrscr; 
        left_flag = 0;
        if(!back_stack.empty()) start_raw_mode(back_stack.top());
    }

    if(right_flag)
    {
        clrscr; 
        right_flag = 0;
        if(!back_stack.empty()) start_raw_mode(back_stack.top());
    }

    if(back_flag)
    {
        clrscr; 
        back_flag = 0;
        start_raw_mode(back_stack.top());

    }

    if(home_flag)
    {
        clrscr; 
        home_flag = 0;
        start_raw_mode(back_stack.top());
    }

    if(cmd_mode)
    {
        cmd_mode = 0;
        end_raw_mode();
        start_command_mode();
    }
    
}

//for obtaining the information of files and sub-directories such as permission, modification time etc.
void file_stat(struct stat f_stat)
{
    char str[32] = "";
    cout << setw(20) << left << human_readable(f_stat.st_size, str); 
   

    struct passwd *pw= getpwuid(f_stat.st_uid);
    struct group *gr = getgrgid(f_stat.st_gid);
    if(pw) cout << setw(30) << left << pw->pw_name;
    if(gr) cout << setw(20) << left << gr->gr_name; 

  

    if(S_ISDIR(f_stat.st_mode)) cout << "d";
    else cout << "-";

    if(f_stat.st_mode & S_IRUSR) cout << "r";
    else cout << "-";

    if(f_stat.st_mode & S_IWUSR) cout << "w";
    else cout << "-";

    if(f_stat.st_mode & S_IXUSR) cout << "x";
    else cout << "-";

    if(f_stat.st_mode & S_IRGRP) cout << "r";
    else cout << "-";

    if(f_stat.st_mode & S_IWGRP) cout << "w";
    else cout << "-";

    if(f_stat.st_mode & S_IXGRP) cout << "x";
    else cout << "-";

    if(f_stat.st_mode & S_IROTH) cout << "r";
    else cout << "-";

    if(f_stat.st_mode & S_IWOTH) cout << "w";
    else cout << "-";

    if(f_stat.st_mode & S_IXOTH) cout << "x";
    else cout << "-";

    
    cout << "\t";
 
    struct tm m_time;
    m_time = *(gmtime(&f_stat.st_mtime));
    string day, month;
    
    if(m_time.tm_wday == 0) day = "Sun";
    if(m_time.tm_wday == 1) day = "Mon";
    if(m_time.tm_wday == 2) day = "Tue";
    if(m_time.tm_wday == 3) day = "Wed";
    if(m_time.tm_wday == 4) day = "Thu";
    if(m_time.tm_wday == 5) day = "Fri";
    if(m_time.tm_wday == 6) day = "Sat";

    if(m_time.tm_mon == 0) month = "Jan";
    if(m_time.tm_mon == 1) month = "Feb";
    if(m_time.tm_mon == 2) month = "Mar";
    if(m_time.tm_mon == 3) month = "Apr";
    if(m_time.tm_mon == 4) month = "May";
    if(m_time.tm_mon == 5) month = "Jun";
    if(m_time.tm_mon == 6) month = "Jul";
    if(m_time.tm_mon == 7) month = "Aug";
    if(m_time.tm_mon == 8) month = "Sep";
    if(m_time.tm_mon == 9) month = "Oct";
    if(m_time.tm_mon == 10) month = "Nov";
    if(m_time.tm_mon == 11) month = "Dec";

   
    cout << day << " " << m_time.tm_mday << " " <<  month << " " << m_time.tm_year + 1900 << " " << m_time.tm_hour << ":" << m_time.tm_min << ":" << m_time.tm_sec;
   

}

//conversion of size into human readable format such as bytes, kilobytes etc.
const char *human_readable(long long bytes, char *s)
{
    const char *size[5] = { "B", "KB", "MB", "GB", "TB" };
    int i=0;
    double d_bytes = bytes;

    while(i<5 && bytes>=1024)
    {
        d_bytes = bytes/1024.0;
        bytes = bytes/1024;
        i++;
    }
 
    sprintf(s, "%.2f", d_bytes);
 
    return strcat(strcat(s, " "), size[i]);
}

//setting the position of the cursor
void reset_cursor(int x,int y)    
{
    cout<<"\033["<<x<<";"<<y<<"H";
    fflush(stdout);
}

//creating file
void create_file()
{
    int n = cmd_vec[2].size();
    string path, folder;
    
    if(cmd_vec[2][0] == '~')
    {
        folder = cmd_vec[2].substr(2, n);
        path = char_to_string(cwd) + "/" + folder + "/" + cmd_vec[1];
       
    }
    else if (cmd_vec[2][0] == '.')
    {
        if(n>1) 
        {
            folder = cmd_vec[2].substr(2, n);
            path = back_stack.top() + "/" + folder + "/" + cmd_vec[1];
        }
        else path = back_stack.top() + "/" + cmd_vec[1];
    }
    else if(cmd_vec[2][0] == '/') path = cmd_vec[1];
    else path = back_stack.top() + "/" + cmd_vec[1];

    const char *file_path = path.c_str();
    int file = open(file_path, O_RDWR | O_CREAT, S_IRUSR | S_IROTH | S_IRWXU | S_IRWXG | S_IRWXO);
}

//creating directories
void create_dir()
{
    int n = cmd_vec[2].size();
    string path, folder;
    if(cmd_vec[2][0] == '~')
    {
        folder = cmd_vec[2].substr(2, n);
        path = char_to_string(cwd) + "/" + folder + "/" + cmd_vec[1];
       
    }
    else if(cmd_vec[2][0] == '.')
    {
        if(n>1) 
        {
            folder = cmd_vec[2].substr(2, n);
            path = back_stack.top() + "/" + folder + "/" +  cmd_vec[1];
        }
        else path = back_stack.top() + "/" +  cmd_vec[1];
    }
    else if(cmd_vec[2][0] == '/') path = cmd_vec[1];
    else path = back_stack.top() + "/" +  cmd_vec[1];

    const char *dir_path = path.c_str();
    int dir = mkdir(dir_path, S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IXOTH);
}

//helper function for copying files
void copy_file(const char *source, const char* destination)
{
    ifstream src(source, ios::binary);
    ofstream dest(destination, ios::binary);
    dest << src.rdbuf();

    struct stat st;
    stat(source, &st);
    chmod(destination, st.st_mode);
}

//copying files and subdirectories
void copy()
{
    vector<string> temp;
    string source, destination, folder;
    copy(cmd_vec.begin(), cmd_vec.end(), back_inserter(temp));
    if(!cmd_vec.empty()) cmd_vec.clear();
    int n = temp.size();


    for(int i=1; i<n-1; i++)
    {
        if(temp[n-1][0] == '~')
        {
            folder = temp[n-1].substr(2, temp[n-1].size());
            destination = char_to_string(cwd) + "/" + folder + "/";
            
        
        }
        else if(temp[n-1][0] == '.')
        {
            if(temp[n-1].size()>1) 
            {
                folder = temp[n-1].substr(2, temp[n-1].size());
                destination = back_stack.top() + "/" + folder + "/";
                
            }
            else destination = back_stack.top() + "/";      
            
        }
        else if(temp[n-1][0] == '/') destination = temp[n-1] + "/";
        else destination = back_stack.top() + "/"; 

        dest = destination;


       if(temp[i][0] == '~') source = char_to_string(cwd) + "/" + temp[i].substr(2, temp[i].size());
       else if (temp[i][0] == '.' && temp[i].size()>1) source = back_stack.top() + "/" + temp[i].substr(2, temp[i].size());
       else if (temp[i][0] == '.' && temp[i].size()==1) source = back_stack.top() + "/" + temp[i];
       else if(temp[i][0] == '/') source = temp[i];
       else source = back_stack.top() + "/" + temp[i];
       
       if(!cmd_vec.empty()) cmd_vec.clear();
       struct stat s;
       if(stat(source.c_str(), &s)==0)
       {
            if(S_ISREG(s.st_mode)==1)
            {
                cmd_vec.push_back("create_file");
                cmd_vec.push_back(temp[i]);
                cmd_vec.push_back(temp[n-1]);
                create_file();
                int j=temp[i].size()-1;
                while(j>=0 && temp[i][j]!='/') j--;
                destination += temp[i].substr(j+1, temp[i].size());
                copy_file(source.c_str(), destination.c_str());
            }
            else
            {
                dir_queue.push(source);
                int j=temp[i].size()-1;
                while(j>=0 && temp[i][j]!='/') j--;

                cmd_vec.push_back("create_dir");
                cmd_vec.push_back(temp[i].substr(j+1, temp[i].size()));
                cmd_vec.push_back(temp[n-1]);
                create_dir();
            }
       }
    }

    if(!dir_queue.empty() && flag==1) copy_dir(dest);

}
//helper function for copying directories
void copy_dir(string dest)
{
    
    flag=0;
    while(!dir_queue.empty())
    {
        if(!cmd_vec.empty()) cmd_vec.clear();
        DIR *dir = opendir(dir_queue.front().c_str());
        if(!dir) return;
        struct dirent *d;
        d = readdir(dir); 
        cmd_vec.push_back("copy");
        while(d)
        {
            string p; 
            if(strcmp(d->d_name, ".") !=0 && strcmp(d->d_name, "..") !=0)
            {
                p = dir_queue.front() + "/" + d->d_name;
                cmd_vec.push_back(p);
            }
            d = readdir(dir);
        }
        int j = dir_queue.front().size();
        while(j>=0 && dir_queue.front()[j] != '/') j--;
        cmd_vec.push_back(dest + dir_queue.front().substr(j+1, dir_queue.front().size()));
        closedir(dir);
        dir_queue.pop();
        copy();
    }

    flag=1;

}

//deleting files
void delete_file()
{
    string path;
    if(cmd_vec[1][0] == '~') path = char_to_string(cwd) + "/" + cmd_vec[1].substr(2, cmd_vec[1].size());
    else if(cmd_vec[1][0] == '.' && cmd_vec[1].size() > 1) path = back_stack.top() + "/" + cmd_vec[1].substr(2, cmd_vec[1].size());
    else if(cmd_vec[1][0] == '.' && cmd_vec[1].size() == 1) path = back_stack.top() + "/" + cmd_vec[1];
    else if(cmd_vec[1][0] == '/') path = cmd_vec[1];
    else path = back_stack.top() + "/" + cmd_vec[1];
    const char *file_path = path.c_str();
    int delete_status = unlink(file_path);

}
//deleting directories recursively
void delete_dir(string to_delete)
{
   
    DIR* dir = opendir(to_delete.c_str());
    if (dir == NULL) return;

    struct dirent* d;
    d = readdir(dir);
    while (d != NULL) {
        
        if(d->d_type != DT_DIR)
        {
            string t = to_delete + "/" + char_to_string(d->d_name);
            int i = unlink(t.c_str());
        }

        if (d->d_type == DT_DIR && strcmp(d->d_name, ".") != 0 && strcmp(d->d_name, "..") != 0) 
        {
            string path = to_delete + "/" + char_to_string(d->d_name);
            delete_dir(path);
            string t = to_delete + "/" + char_to_string(d->d_name);
            rmdir(t.c_str());
        }
        
        d = readdir(dir);
    }

    closedir(dir);
    
}
//moving files and directories
void move()
{
    vector<string> temp;
    string source, destination, folder;
    copy(cmd_vec.begin(), cmd_vec.end(), back_inserter(temp));
    if(!cmd_vec.empty()) cmd_vec.clear();
    int n = temp.size();


    for(int i=1; i<n-1; i++)
    {
        if(temp[n-1][0] == '~')
        {
            folder = temp[n-1].substr(2, temp[n-1].size());
            destination = char_to_string(cwd) + "/" + folder + "/";
            
        
        }
        else if(temp[n-1][0] == '.')
        {
            if(temp[n-1].size()>1) 
            {
                folder = temp[n-1].substr(2, temp[n-1].size());
                destination = back_stack.top() + "/" + folder + "/";
                
            }
            else destination = back_stack.top() + "/";      
            
        }
        else if(temp[n-1][0] == '/') destination = temp[n-1] + "/";
        else destination = back_stack.top() + "/"; 

        dest = destination; 


       if(temp[i][0] == '~') source = char_to_string(cwd) + "/" + temp[i].substr(2, temp[i].size());
       else if (temp[i][0] == '.' && temp[i].size()>1) source = back_stack.top() + "/" + temp[i].substr(2, temp[i].size());
       else if (temp[i][0] == '.' && temp[i].size()==1) source = back_stack.top() + "/" + temp[i];
       else if(temp[i][0] == '/') source = temp[i];
       else source = back_stack.top() + "/" + temp[i];
       
       if(!cmd_vec.empty()) cmd_vec.clear();
       struct stat s;
       if(stat(source.c_str(), &s)==0)
       {
            if(S_ISREG(s.st_mode)==1)
            {
                cmd_vec.push_back("create_file");
                cmd_vec.push_back(temp[i]);
                cmd_vec.push_back(temp[n-1]);
                create_file();
                int j=temp[i].size()-1;
                while(j>=0 && temp[i][j]!='/') j--;
                destination += temp[i].substr(j+1, temp[i].size());
                copy_file(source.c_str(), destination.c_str());
                //deletion
                if(!cmd_vec.empty()) cmd_vec.clear();
                cmd_vec.push_back("delete_file");
                cmd_vec.push_back(temp[i]);
                delete_file();
               
            }

            else
            {
                dir_queue.push(source);
                int j=temp[i].size()-1;
                while(j>=0 && temp[i][j]!='/') j--;

                cmd_vec.push_back("create_dir");
                cmd_vec.push_back(temp[i].substr(j+1, temp[i].size()));
                cmd_vec.push_back(temp[n-1]);
                create_dir();
            }

        }
    }

    if(!dir_queue.empty() && flag==1) move_dir(dest);
    
}
//helper function for moving directories
void move_dir(string dest)
{
    
    flag=0;
    while(!dir_queue.empty())
    {
        if(!cmd_vec.empty()) cmd_vec.clear();
        DIR *dir = opendir(dir_queue.front().c_str());
        if(!dir) return;
        struct dirent *d;
        d = readdir(dir); 
        cmd_vec.push_back("copy");
        while(d)
        {
            string p; 
            if(strcmp(d->d_name, ".") !=0 && strcmp(d->d_name, "..") !=0)
            {
                p = dir_queue.front() + "/" + d->d_name;
                cmd_vec.push_back(p);
            }
            d = readdir(dir);
        }
        int j = dir_queue.front().size();
        while(j>=0 && dir_queue.front()[j] != '/') j--;
        cmd_vec.push_back(dest + dir_queue.front().substr(j+1, dir_queue.front().size()));
        closedir(dir);
        move();
        rmdir(dir_queue.front().c_str());
        dir_queue.pop();
    }

    flag=1;

}
//renaming 
void rename()
{
    string old_name, new_name;

    if(cmd_vec[1][0] == '~') old_name = char_to_string(cwd) + "/" + cmd_vec[1].substr(2, cmd_vec[1].size());
    else if(cmd_vec[1][0] == '.' && cmd_vec[1].size()>1) old_name = back_stack.top() + "/" + cmd_vec[1].substr(2, cmd_vec[1].size());
    else if(cmd_vec[1][0] == '.' && cmd_vec[1].size()==1) old_name = back_stack.top() + "/" + cmd_vec[1];
    else if(cmd_vec[1][0] == '/') old_name = cmd_vec[1];
    else old_name = back_stack.top() + "/" + cmd_vec[1];

    if(cmd_vec[2][0] == '~') new_name = char_to_string(cwd) + "/" + cmd_vec[2].substr(2, cmd_vec[2].size());
    else if(cmd_vec[2][0] == '.' && cmd_vec[2].size()>1) new_name = back_stack.top() + "/" + cmd_vec[2].substr(2, cmd_vec[1].size());
    else if(cmd_vec[2][0] == '.' && cmd_vec[2].size()==1) new_name = back_stack.top() + "/" + cmd_vec[2];
    else if(cmd_vec[1][0] == '/') new_name = cmd_vec[1];
    else new_name = back_stack.top() + "/" + cmd_vec[2];
   
    rename(old_name.c_str(), new_name.c_str());
    struct stat st;
}
//searching recursively
void search(string search_dir)
{
    DIR* dir = opendir(search_dir.c_str());
    if (dir == NULL) return;

    struct dirent* d;
    d = readdir(dir);
    while (d != NULL) {
        
        if(strcmp(d->d_name, cmd_vec[1].c_str())==0)
        {
            search_flag=1;
            return;
        }

        if (d->d_type == DT_DIR && strcmp(d->d_name, ".") != 0 && strcmp(d->d_name, "..") != 0) 
        {
            string path = search_dir + "/" + char_to_string(d->d_name);
            search(path);
        }

        d = readdir(dir);
    }

    closedir(dir);
    
}
//go to a particular location
void goto_dir()
{
    string path;
    if(cmd_vec[1][0] == '~') path = char_to_string(cwd) + "/" + cmd_vec[1].substr(2, cmd_vec[1].size());
    else if(cmd_vec[1][0] == '.' && cmd_vec[1].size() > 1) path = back_stack.top() + "/" + cmd_vec[1].substr(2, cmd_vec[1].size());
    else if(cmd_vec[1][0] == '.' && cmd_vec[1].size() == 1) path = back_stack.top() + "/" + cmd_vec[1];
    else if(cmd_vec[1][0] == '/') path = cmd_vec[1];
    else path = back_stack.top() + "/" + cmd_vec[1];
    back_stack.push(path);
}

int main()
{
   set_raw_flags();
   getcwd(cwd,SIZE);
   back_stack.push(char_to_string(cwd));
   start_raw_mode(back_stack.top());

}
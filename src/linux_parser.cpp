#include <dirent.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <vector>

#include "linux_parser.h"

using std::stof;
using std::string;
using std::to_string;
using std::vector;

template <typename T>
T readValueFromFile(const std::string &path, const std::string &argumentName) {
  T value;
  std::ifstream filestream(path);

  if (filestream.is_open()) {
    std::string line;
    
    while (std::getline(filestream, line)) {
      std::istringstream lineStream(line);
      std::string argument;
      lineStream >> argument;
      
      if (argument == argumentName) {
        lineStream >> value;
        return value;
      }
    }
  }

  return value;
}


// Read and return the name of the operating system

string LinuxParser::OperatingSystem() {

  string line;
  string key;
  string value;
  std::ifstream filestream(kOSPath);
  if (filestream.is_open()) {
    while (std::getline(filestream, line)) {
      std::replace(line.begin(), line.end(), ' ', '_');
      std::replace(line.begin(), line.end(), '=', ' ');
      std::replace(line.begin(), line.end(), '"', ' ');
      std::istringstream linestream(line);
      while (linestream >> key >> value) {
        if (key == "PRETTY_NAME") {
          std::replace(value.begin(), value.end(), '_', ' ');
          return value;
        }
      }
    }
  }
  return value;
}

// Read and return the kernel of the system
string LinuxParser::Kernel() {
  string os, version, kernel;
  string line;
  std::ifstream stream(kProcDirectory + kVersionFilename);
  if (stream.is_open()) {
    std::getline(stream, line);
    std::istringstream linestream(line);
    linestream >> os >> version >> kernel;
  }
  return kernel;
}

// BONUS: Update this to use std::filesystem
// Read and return the process IDs
vector<int> LinuxParser::Pids() {
  vector<int> pids;
  DIR* directory = opendir(kProcDirectory.c_str());
  struct dirent* file;
  while ((file = readdir(directory)) != nullptr) {
    // Is this a directory?
    if (file->d_type == DT_DIR) {
      // Is every character of the name a digit?
      string filename(file->d_name);
      if (std::all_of(filename.begin(), filename.end(), isdigit)) {
        int pid = stoi(filename);
        pids.push_back(pid);
      }
    }
  }
  closedir(directory);
  return pids;
}

// Read and return the system memory utilization
float LinuxParser::MemoryUtilization() {
  float memTotal = 0.0;
  float memFree = 0.0;
  string line;
  string key;
  string value;

  // read file /proc/meminfo and look for MemTotal and MemFree
  std::ifstream filestream(kProcDirectory + kMeminfoFilename);
  if (filestream.is_open()) {
    while (std::getline(filestream, line)) {
      // remove ' kB' at the end, spaces and :
      // std::remove(line.begin(), line.end(), 'kB');
      std::remove(line.begin(), line.end(), ' ');
      std::replace(line.begin(), line.end(), ':', ' ');
      std::istringstream linestream(line);
      while (linestream >> key >> value) {
        // search for key memTotal
        if (key == "MemTotal") {
          memTotal = std::stof(value);
        }
        // search for key memFree
        else if (key == "MemFree") {
          memFree = std::stof(value);
          break;
        }
      }
    }
  }

  // Total used memory = (memTotal - MemFree) / memTotal
  return ((memTotal - memFree) / memTotal);
}

// Read and return the system uptime
long LinuxParser::UpTime() {
  string line;
  string wholeTime;
  std::ifstream filestream(kProcDirectory + kUptimeFilename);
  if (filestream.is_open()) {
    while (std::getline(filestream, line)) {
      std::istringstream linestream(line);
      while (linestream >> wholeTime) {
        try {
          return std::stol(wholeTime);
        } catch (const std::invalid_argument& arg) {
          return 0;
        }
      }
    }
  }
  return 0;
}

// TODO: Read and return the number of jiffies for the system
long LinuxParser::Jiffies() { return 0; }


// TODO: Read and return the number of active jiffies for a PID
// REMOVE: [[maybe_unused]] once you define the function
long LinuxParser::ActiveJiffies(int pid) { 
  long totalTime = 0;
  std::stringstream path;
  path << kProcDirectory << pid << kStatFilename;
    
  std::ifstream filestream(path.str());
  if (filestream.is_open()) {
    const int position = 13;
    std::string uselessValue;
    for (int i = 0; i < position; i++) {
      if (!(filestream >> uselessValue)) {
        return 10000;
      }
    }
	
    
    long userTimeTicks=0, kernelTimeTicks = 0, userChildrenTimeTicks = 0, kernelChildrenTimeTicks = 0;
   // filestream >> userTimeTicks;
    //std::cout<< "proc13:" << userTimeTicks << std::endl;
    if (filestream >> userTimeTicks >> kernelTimeTicks >> userChildrenTimeTicks >> kernelChildrenTimeTicks) {
        totalTime = (userTimeTicks + kernelTimeTicks + userChildrenTimeTicks + kernelChildrenTimeTicks);
    }
  }
  
  return totalTime/sysconf(_SC_CLK_TCK);
}


long LinuxParser::ActiveJiffies() {
  // Read and return the number of active jiffies for the system
  auto jiffies = CpuUtilization();
   return stol(jiffies[CPUStates::kUser_]) + stol(jiffies[CPUStates::kNice_]) +
         stol(jiffies[CPUStates::kSystem_]) + stol(jiffies[CPUStates::kIRQ_]) +
         stol(jiffies[CPUStates::kSoftIRQ_]) +
         stol(jiffies[CPUStates::kSteal_]);
}
 
// TODO: Read and return the number of idle jiffies for the system
long LinuxParser::IdleJiffies() { 
  
  auto jiffies = CpuUtilization();  
  auto variable = stol(jiffies[CPUStates::kIdle_]) + stol(jiffies[CPUStates::kIOwait_]); 
  return variable;
}


// Read and return the total number of processes
int LinuxParser::TotalProcesses() {
  string line;
  string key;
  string value;
  std::ifstream filestream(kProcDirectory + kStatFilename);
  if (filestream.is_open()) {
    while (std::getline(filestream, line)) {
      std::istringstream linestream(line);
      while (linestream >> key >> value) {
        if (key == "processes") {
          try {
            return std::stol(value);
          } catch (const std::invalid_argument& arg) {
            return 0;
          }
        }
      }
    }
  }
  return 0;
}

// Read and return the number of running processes
int LinuxParser::RunningProcesses() {
  string line;
  string key;
  string value;
  std::ifstream filestream(kProcDirectory + kStatFilename);
  if (filestream.is_open()) {
    while (std::getline(filestream, line)) {
      std::istringstream linestream(line);
      while (linestream >> key >> value) {
        if (key == "procs_running") {
          try {
            return std::stol(value);
          } catch (const std::invalid_argument& arg) {
            return 0;
          }
        }
      }
    }
  }
  return 0;
}

// Read and return the command associated with a process
string LinuxParser::Command(int pid) {
  string value = "";
  std::ifstream filestream(kProcDirectory + std::to_string(pid) +
                           kCmdlineFilename);
  if (filestream.is_open()) {
    std::getline(filestream, value);
    return value;
  }
  return value;
}

// Read and return the CPU usage of a process
// return the values in seconds
vector <string> LinuxParser::CpuUtilization() {
  string nline;
  string key;
  vector<string> jiffies{};
  vector<int> pstate;
  std::ifstream filestream(kProcDirectory + kStatFilename); 
  if (filestream.is_open()) {
    std::getline(filestream, nline);
    std::istringstream linestream(nline);
    linestream >> key;
    
    while (linestream >> key) {
      pstate.emplace_back(std::stoi(key));
      jiffies.emplace_back(key);
      }
    /*idle = pstate[3] + pstate[4];
    active = pstate[0] + pstate[1] + pstate[2] + pstate[4] + pstate[6] + pstate[7];
    cpuValues = active/(float)(float (idle) + float (active));
    jiffies.emplace_back(std::to_string(cpuValues));*/
    
  }
  //else {
  //std::cout << "error" <<std::endl;
  //}
  return jiffies;
}

vector <float> LinuxParser::CpuUtilizationForProcess(int pid) {

  long Hertz = sysconf(_SC_CLK_TCK);
  string line;
  vector<string> columns;
  string column;
  vector <float> cpuValues{0.0};
  std::ifstream stream(kProcDirectory + to_string(pid) + kStatFilename);
  if(stream.is_open()) {
    getline(stream, line);
    std::istringstream linestream(line);
    
    while(linestream.good()) {
      getline(linestream, column, ' ');
      columns.push_back(column);
    }
  long kUtime = stoi(columns[13]);
  long kStime = stoi(columns[14]);
  long kCutime = stoi(columns[15]);
  long kCstime = stoi(columns[16]);
    
  long total_time = kUtime + kStime + kCutime + kCstime;
  float cpuProcessTime = total_time / Hertz;
  long totSeconds = UpTime() - UpTime(pid);
  cpuValues.push_back(totSeconds);
 }
 
  return cpuValues;
}



// Read and return the memory used by a process
string LinuxParser::Ram(int pid) { 
  
  string nline;
  string key;
  long ramInMegaBytes=0;
  
  std::ifstream stream(kProcDirectory + to_string(pid) + kStatusFilename); 
	
      if(stream.is_open()){
        while(std::getline(stream,nline)){
        std::stringstream linestream(nline);
        linestream >> key;
          
        if (key == "VmSize:"){
          linestream >> ramInMegaBytes;
          break;
        }
      }
	}
  	else {
    std::cout<< "error"<<std::endl;
    
    }
	stream.close(); 

	return std::to_string(ramInMegaBytes / 1024);
}

// Read and return the user ID associated with a process
string LinuxParser::Uid(int pid) {
  string line;
  string key;
  string value = "";
  std::ifstream filestream(kProcDirectory  + std::to_string(pid) +
                           kStatusFilename);
  if (filestream.is_open()) {
    while (std::getline(filestream, line)) {
      std::replace(line.begin(), line.end(), ':', ' ');
      std::istringstream linestream(line);
      while (linestream >> key >> value) {
        if (key == "Uid") {
          return value;
        }
      }
    }
  }
  return value;
}

// Read and return the user associated with a process
string LinuxParser::User(int pid) {
  // read the user ID for this process
  string uid = Uid(pid);
  string line;
  string key;
  string value = "";
  string other;
  // find user name for this user ID in /etc/passwd
  std::ifstream filestream(kPasswordPath);
  if (filestream.is_open()) {
    while (std::getline(filestream, line)) {
      std::replace(line.begin(), line.end(), ':', ' ');
      std::istringstream linestream(line);
      while (linestream >> value >> other >> key) {
        if (key == uid) {
          return value;
        }
      }
    }
  }
  return value;
}

// Read and return the uptime of a process
long LinuxParser::UpTime(int pid) {
  vector<string> cstate;
  string nline;
  string total;
  std::ifstream filestream(kProcDirectory  + std::to_string(pid) +
                           kStatFilename);
  if (filestream.is_open()) {
    while (std::getline(filestream, nline)) {
      std::istringstream linestream(nline);
         while (linestream >> total) {
      	   cstate.emplace_back(total);
   	  }
    }
  } 
  long Hertz = sysconf(_SC_CLK_TCK);
  long time_long = (cstate.size() >= 21 ? 0 :  stol(cstate[21])/Hertz);
  return time_long;
  //return LinuxParser::UpTime() - std::stol(cstate[21])/Hertz;
}

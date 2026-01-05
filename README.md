# grpc_core
This project is a light-weight publisher-subscriber/service(Server-Client) communicate protocol based on gRPC. For cases having difficulty on installing ROS, or needy to combine control system to web page.

# Changelog

## [dev branch] - December 23, 2025

### Added
- **Comprehensive Logger System**: Implemented a full-featured logging system with ROS-compatible log levels (DEBUG, INFO, WARN, ERROR, FATAL)
  - Stream-style logging API with modern C++ operators
  - Advanced logging macros: conditional (`LOG_*_IF`), throttled (`LOG_*_THROTTLE`), periodic (`LOG_*_EVERY_N`), once (`LOG_*_ONCE`), and state-change (`LOG_*_CHANGED`) logging
  - Configurable minimum log level filtering
  - Thread-safe operations
  - Remote publishing support via gRPC
  - Both local console and remote output options
  - New files: `include/Logger.h` (300 lines), `src/Logger.cpp` (212 lines)
  
- **Robot Protocol Definitions**: 
  - `Config.proto`: Configuration message definitions (40 lines)
  - `Log.proto`: Log message definitions for the logging system (21 lines)
  - `Robot.proto`: Robot mode and command messages with ROBOTMODE enum (SYSTEM_ON, INIT, IDLE, STANDBY, MOTORCONFIG)
  
- **Enhanced Motor and Power Protocols**:
  - Motor.proto: Added 9 new lines of motor control definitions
  - Steering.proto: Added 1 line of steering-related definitions

### Changed
- **NodeHandler API**: Updated Publisher and Subscriber constructors to include `maxSize` parameter with default value for better buffer management
- **Message Queue Order**: Fixed message retrieval order in Communicator (changed from back to front of the queue for correct FIFO behavior)
- **Power.proto Refactoring**: Streamlined and reorganized (reduced from 68 lines to more efficient structure)
- **Log Output Formatting**: Improved log level alignment and readability in console output

### Removed
- **Robot.proto**: Removed `RobotRequestUpdate` message (previously had `mode_update` field) - functionality consolidated into `RobotCmdStamped`

### Fixed
- ROBOTMODE enum order: Corrected the ordering by swapping MOTORCONFIG and STANDBY values
- Package declarations: Fixed namespace consistency in Robot.proto
- Field indices: Corrected error and error_code field indices in RobotStateStamped message
- Message queue handling: Multiple iterations to ensure correct FIFO behavior

### Statistics
- **11 files changed**: 869 insertions(+), 59 deletions(-)
- **Major additions**: Logger system (512 lines), extended README documentation (196 lines)
- **16 commits** ahead of main branch with iterative improvements and fixes

### Migration Notes
- If using `Publisher` or `Subscriber`, note the new optional `maxSize` parameter in constructors
- Applications using `RobotRequestUpdate` message should migrate to using `RobotCmdStamped` with the `request_robot_mode` field
- Logger system is fully backward compatible - existing code will continue to work

# preliminary
Install gRPC: https://grpc.io/docs/languages/cpp/quickstart/  

**Note:**
* The first step change to "export MY_INSTALL_DIR={your installation path}", e.g. "export MY_INSTALL_DIR=$HOME/corgi_ws/install".
* There is no need to reinstall cmake if it is version 3.13 or later.

***The following all use "$HOME/corgi_ws/install" to replace {your installation path}.***
# compile
***Need Compiler of C++ 17 or higher***
```
cd grpc_core
mkdir build
cd build
cmake .. -DCMAKE_PREFIX_PATH=$HOME/corgi_ws/install -DCMAKE_INSTALL_PREFIX=$HOME/corgi_ws/install
make -j16
sudo make install
```

# local environment setting
These will write setting into your bash file.
```
echo export PATH=\$HOME/corgi_ws/install/bin:\$PATH >> ~/.bashrc
echo export CORE_LOCAL_IP="127.0.0.1" >> ~/.bashrc
echo export CORE_MASTER_ADDR="127.0.0.1:10010" >> ~/.bashrc
source ~/.bashrc
```
**"CORE_LOCAL_IP"** should be your local device IP, and **"CORE_MASTER_ADDR"** should be set to the same as master IP, i.e. IP of the device which runs **grpccore**.  
For example, if you have device A (**192.168.0.106**) and device B (**192.168.0.172**), and you run your core master node on device A on port **10010**. Then on each device A and B, the **"CORE_MASTER_ADDR"** should be **"192.168.0.106:10010"**, and **"CORE_LOCAL_IP"** on device A is **"192.168.0.106"**, **"CORE_LOCAL_IP"** on device B is **"192.168.0.172"**. 

# example compile
```
cd grpc_core/example/c++ 
mkdir build 
cd build 
cmake .. -DCMAKE_PREFIX_PATH=$HOME/corgi_ws/install
make -j16
```

# run example
### Publisher & Listener
```
grpccore      // terminal 1
./NodeTestPub // terminal 2 (run in the build file of example/c++)
./NodeTestSub // terminal 3 (run in the build file of example/c++)
```
This is the basic Publisher/Subscriber protocol, it support multiple subscribers subscribe to one topic, and also multiple publishers publish to a topic is legal but not recommended.
### Server & Client
```
grpccore                // terminal 1
./NodeTestServiceServer // terminal 2 (run in the build file of example/c++)
./NodeTestServiceClient // terminal 3 (run in the build file of example/c++)
```
This is the basic ServiceServer/Client protocol, if you launch multiple Server on one service, only the last one works functionally.

# Use self-defined message defined in grpc_core
The self-defined messages are put in the *robot_protos* file.  
Please refer to [grpc_node_test](https://github.com/kyle1548/grpc_node_test) for usage instructions.

# Logger System
grpc_core provides a simplified global logging system. Just include `Logger.h` and use `LOG_*` macros anywhere - no complex setup required!

## Quick Start

### 1. Initialize (once in main.cpp)
```cpp
#include "Logger.h"

int main() {
    LOG_INIT("my_node");  // Initialize with node name, auto-publishes to /log
    
    LOG_INFO << "Program started!";
    // ...
}
```

### 2. Use anywhere (no additional setup needed!)
```cpp
// any_file.cpp
#include "Logger.h"

void anyFunction() {
    LOG_INFO << "Hello world: " << 123;
    LOG_WARN << "Temperature: " << temp;
    LOG_ERROR << "Error code: " << err;
}

class AnyClass {
public:
    void method() {
        LOG_DEBUG << "Inside class method";  // Works directly!
    }
};
```

## Log Levels
- **DEBUG**: Detailed debugging information
- **INFO**: General informational messages  
- **WARN**: Warning messages
- **ERROR**: Error messages
- **FATAL**: Fatal error messages

## Basic Macros

```cpp
LOG_DEBUG << "Debug message";
LOG_INFO  << "Info message";
LOG_WARN  << "Warning message";
LOG_ERROR << "Error message";
LOG_FATAL << "Fatal error!";
```

## Advanced Macros

### Conditional Logging
```cpp
LOG_INFO_IF(x > 0) << "x is positive: " << x;
LOG_WARN_IF(battery < 20) << "Low battery!";
```

### Log Only Once
```cpp
LOG_WARN_ONCE << "This appears only once";
LOG_INFO_ONCE << "Initialization complete";
```

### Log Every N Occurrences
```cpp
LOG_INFO_EVERY_N(100) << "Processed " << count << " items";
LOG_DEBUG_EVERY_N(50) << "Periodic debug";
```

### Log When Condition Changes
```cpp
LOG_WARN_CHANGED(voltage < 10.0) << "Low voltage!";
LOG_INFO_CHANGED(motor_active) << "Motor started";
```

### Throttled Logging (rate-limited)
```cpp
LOG_WARN_THROTTLE(1000) << "At most once per second";
LOG_THROTTLE_END
```

## Configuration (Optional)

```cpp
// Set minimum log level (filter out lower levels)
core::GlobalLoggerImpl::instance().setMinLevel(core::LogLevel::INFO);

// Enable/disable console output
core::GlobalLoggerImpl::instance().setLocalOutput(true);

// Enable/disable remote publishing to /log
core::GlobalLoggerImpl::instance().setRemoteOutput(true);
```

## Output Format
```
[HH:MM:SS.USEC] [LEVEL] [node_name] [file.cpp:line] message
```

Example:
```
[10:30:45.123456] [INFO ] [fpga_driver] [main.cpp:42] Program started!
[10:30:45.234567] [WARN ] [fpga_driver] [motor.cpp:128] Temperature high: 85°C
```

## Features
- **Zero configuration** - Just `#include "Logger.h"` and use
- **Auto file/line info** - Automatically includes source location
- **Thread-safe** - Safe for multi-threaded applications
- **Auto /log publishing** - Logs published to `/log` topic automatically
- **ROS-compatible levels** - DEBUG, INFO, WARN, ERROR, FATAL

# On sbRIO
If you are compiling on sbRIO (Single-Board RIO), you need to make the following modifications.
* Due to the old version of libraries on sbRIO, use c++14 instead of c++17 by modifying **CMakeLists.txt**.
```
set (CMAKE_CXX_STANDARD 14)  # change 17 to 14
```
* Because OpenSSL is installed additionally, its root directory must be specified when running cmake for projects that use gRPC,  
i.e. add **"-DOPENSSL_ROOT_DIR={your OpenSSL installation path}"** to cmake command.
```
cmake .. -DCMAKE_PREFIX_PATH=$HOME/corgi_ws/install -DCMAKE_INSTALL_PREFIX=$HOME/corgi_ws/install -DOPENSSL_ROOT_DIR=$HOME/corgi_ws/install/ssl
```
*  Because some libraries (e.g. c-ares) are installed in a non-standard location, add the library path to the **LD_LIBRARY_PATH** environment variable in the bash file.
```
echo export LD_LIBRARY_PATH=$HOME/corgi_ws/install/lib:$LD_LIBRARY_PATH >> ~/.bashrc
```
* Due to memory limitations on sbRIO, you may need to use **"make"** instead of **"make -j16"** when compiling.
```
make
```


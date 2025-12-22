# grpc_core
This project is a light-weight publisher-subscriber/service(Server-Client) communicate protocol based on gRPC. For cases having difficulty on installing ROS, or needy to combine control system to web page.

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
grpc_core provides a comprehensive logging system with ROS-compatible log levels and stream-style output.

## Log Levels
- **DEBUG**: Detailed debugging information
- **INFO**: General informational messages
- **WARN**: Warning messages
- **ERROR**: Error messages
- **FATAL**: Fatal error messages

## Basic Usage

### Creating a Logger
```cpp
#include "Logger.h"

// Create logger instance
core::Logger logger("node_name");

// Or use global logger (optional)
core::GlobalLogger::init("global_node");
```

### Stream-Style Logging (Recommended)
```cpp
LOG_DEBUG(logger) << "Debug message: " << variable;
LOG_INFO(logger) << "Info message: " << value;
LOG_WARN(logger) << "Warning: " << warning_msg;
LOG_ERROR(logger) << "Error: " << error_code;
LOG_FATAL(logger) << "Fatal error!";

// Using global logger
GLOG_INFO << "Global info message";
GLOG_WARN << "Global warning";
```

### Direct Method Logging
```cpp
logger.debug("Debug message");
logger.info("Info message");
logger.warn("Warning message");
logger.error("Error message");
logger.fatal("Fatal error");
```

## Advanced Logging Macros

### Conditional Logging
Log only when condition is true:
```cpp
LOG_INFO_IF(logger, temperature > 100) << "High temperature: " << temperature;
LOG_WARN_IF(logger, battery < 20) << "Low battery: " << battery;
```

### Throttled Logging
Log at most once per time interval (in milliseconds):
```cpp
// Log at most once per second (1000ms)
LOG_WARN_THROTTLE(logger, 1000) << "Periodic warning message";
LOG_THROTTLE_END

LOG_INFO_THROTTLE(logger, 500) << "This logs every 500ms maximum";
LOG_THROTTLE_END
```

### Log Every N Occurrences
```cpp
// Log every 100 times
LOG_INFO_EVERY_N(logger, 100) << "Periodic info message";

// Log every 50 times
LOG_DEBUG_EVERY_N(logger, 50) << "Debug every 50 calls";
```

### Log Only Once
Log message only on first occurrence:
```cpp
LOG_WARN_ONCE(logger) << "This warning appears only once";
LOG_INFO_ONCE(logger) << "Initialization complete";
```

### Log When Condition Changes
Log only when condition transitions from false to true:
```cpp
// Logs only when voltage drops below 10.0 (state change)
LOG_WARN_CHANGED(logger, voltage < 10.0) << "Low voltage: " << voltage;

// Logs when motor becomes active
LOG_INFO_CHANGED(logger, motor_active) << "Motor started";
```

## Logger Configuration

### Set Minimum Log Level
Filter out log messages below specified level:
```cpp
// Only show INFO and above (filters out DEBUG)
logger.setMinLevel(core::LogLevel::INFO);

// Show all messages including DEBUG
logger.setMinLevel(core::LogLevel::DEBUG);

// Show only errors
logger.setMinLevel(core::LogLevel::ERROR);
```

### Enable/Disable Outputs
```cpp
// Enable/disable console output
logger.setLocalOutput(true);   // Show in console
logger.setLocalOutput(false);  // Hide console output

// Enable/disable remote publishing
logger.setRemoteOutput(true);  // Publish to remote
logger.setRemoteOutput(false); // Disable remote publishing
```

### Remote Publishing Callback
Set custom callback for remote logging:
```cpp
// Create publisher for log messages
core::Publisher<log_msg::LogEntry>& log_pub = 
    nh.advertise<log_msg::LogEntry>("log_topic", [buffer size]);

// Set publish callback
logger.setPublishCallback([&log_pub](const log_msg::LogEntry& entry) {
    log_pub.publish(entry);
});
```

## Complete Example
```cpp
#include "NodeHandler.h"
#include "Logger.h"

int main() {
    // Initialize node handler
    core::NodeHandler nh;
    
    // Create logger
    core::Logger logger("my_node");
    
    // Configure logger
    logger.setMinLevel(core::LogLevel::DEBUG);
    logger.setLocalOutput(true);
    logger.setRemoteOutput(true);
    
    // Setup remote publishing
    core::Publisher<log_msg::LogEntry>& log_pub = 
        nh.advertise<log_msg::LogEntry>("/logs", [buffer size]);
    logger.setPublishCallback([&log_pub](const log_msg::LogEntry& entry) {
        log_pub.publish(entry);
    });
    
    // Example usage
    int counter = 0;
    double temperature = 25.0;
    
    while (true) {
        counter++;
        
        // Basic logging
        LOG_INFO(logger) << "Loop iteration: " << counter;
        
        // Conditional logging
        LOG_WARN_IF(logger, temperature > 80) << "High temp: " << temperature;
        
        // Throttled logging (once per second)
        LOG_DEBUG_THROTTLE(logger, 1000) << "Periodic debug";
        LOG_THROTTLE_END
        
        // Log every N times
        LOG_INFO_EVERY_N(logger, 100) << "Processed " << counter << " iterations";
        
        // Log only once
        LOG_INFO_ONCE(logger) << "System initialized";
        
        // Log on state change
        LOG_WARN_CHANGED(logger, temperature > 90) << "Overheating!";
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    return 0;
}
```

## Features
- **Thread-safe**: Safe to use across multiple threads
- **Real-time output**: Console and remote publishing support
- **ROS-compatible**: Compatible with ROS logging levels
- **Flexible filtering**: Configurable minimum log level
- **Rich macros**: Conditional, throttled, periodic, and state-change logging
- **Stream-style API**: Modern C++ stream operators for easy formatting
- **Remote publishing**: Publish logs via gRPC for distributed systems

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


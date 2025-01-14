# UBMonitor

UBMonitor is a basic system monitoring tool built on top of [UBUS](https://openwrt.org/docs/techref/ubus).

## Getting Started

### Installation
1. Clone the repository:
    ```sh
    git clone https://github.com/ImportFlask/ubmonitor
    ```
2. Navigate to the project directory:
    ```sh
    cd ubmonitor
    ```
3. Compile the application:
    ```sh
    make
    ```
4. (Optional) Install the application:
    ```sh
    sudo make install
    ```

### Running UBMonitor
1. Launch the application in the background:
    ```sh
    sudo UBMonitor &
    ```
2. Verify that UBMonitor is running successfully:
    ```sh
    sudo ubus -v list
    ```
3. Make your first UBUS call:
    ```sh
    sudo ubus call ubm info
    ```

### UBUS on Desktop
> Use the following guide to set up UBUS and its dependencies: [libubox-on-ubuntu](https://github.com/robbie-cao/note/blob/master/libubox-on-ubuntu.md)

### Available Methods

UBMonitor provides several methods for monitoring:

- **info**: Displays system information.
- **cpu**: Provides details about CPU.
- **mem**: Shows memory usage.
- **net**: Lists active network interfaces.
- **signal**: Sends a signal to a specified process.
  - Parameters:
    - `pid`: Process ID (Integer)
    - `sig_id`: Signal ID (Integer)
- **lookup**: Retrieves information about a specific process.
  - Parameters:
    - `pid`: Process ID (Integer)

Example usage with arguments:
```sh
sudo ubus call ubm lookup "{'pid': 1000}"
```

### End note

This project was created as part of my learning journey with UBus during my internship at Teltonika Networks. Initially, I struggled to understand UBus, which motivated me to develop this small monitoring tool. While the project is functional, it is not perfect — it lacks proper error handling in some instances and does not return detailed insights via `blobmsg` when something fails, often requiring a look into syslog for debugging.

That said, this project can serve as a helpful starting point or example for those also struggling to understand UBus. With a few relatively minor adjustments, you can adapt it to include proper error handling and make it more robust for practical use.
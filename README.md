## **Project Title**: **Modular Embedded OS Kernel with Real-Time Capabilities**  

---

### **Objective**  
I am currently working on developing a lightweight, modular OS kernel for resource-constrained microcontrollers (no MMU) with features like real-time scheduling, IPC, and peripheral drivers, integrating concepts from bare-metal development, RTOS design, and kernel architecture.  

---

### **Key Features**  

| **Feature**                 | **Description**                                                                 |
|-----------------------------|---------------------------------------------------------------------------------|
| **Custom Bootloader**        | Minimal bootloader to initialize hardware, load the kernel, and support secure firmware updates. |
| **Task Scheduler**           | Preemptive, priority-based scheduler supporting real-time tasks and dynamic task management. |
| **Memory Management**        | Simple heap allocator and memory segmentation for task isolation without MMU.  |
| **Device Drivers**           | Drivers for UART, I2C, and SPI communication, and a virtual filesystem (e.g., FAT). |
| **Inter-Process Communication (IPC)** | Mechanisms like message queues, semaphores, and event flags for efficient task communication. |
| **Hardware Abstraction Layer** | Abstraction layer for portability across microcontroller families.            |
| **Debugging and Monitoring** | Logging, task tracing, and performance metrics via a connected PC interface.    |
| **Extensibility**            | Support for user-defined modules (e.g., sensor integration or motor control).   |

---

### **How to build and run the project** 

 - WIP

---

### **Technologies and Tools**  

| **Category**            | **Tools/Technologies**                                     |
|--------------------------|-----------------------------------------------------------|
| **Programming Languages**| C (kernel, drivers), Assembly (bootloader, low-level initialization). |
| **Hardware**             | ARM Cortex-M microcontrollers (e.g., STM32, TI MSP430), RISC-V (e.g., SiFive HiFive). |
| **Debugging/Testing**    | OpenOCD, GDB, JTAG/SWD debugger, QEMU for emulation.      |
| **Build System**         | GCC ARM toolchain, Clang, Makefile, CMake.               |
| **Communication Protocols**| UART (console/debugging), I2C, SPI.                     |
| **Optional Tools**       | Python (PC-side monitoring tool), Docker (build environment). |

---

### **Scope and Challenges**  

1. **Integration**: Combine bare-metal drivers, RTOS schedulers, and kernel IPC into a cohesive system.  
2. **Efficiency**: Optimize the kernel to achieve a memory footprint under 16 KB.  
3. **Debugging**: Address stack overflows and synchronization issues in real-time environments.  
4. **Portability**: Abstract hardware-specific code for cross-platform compatibility.  

---  

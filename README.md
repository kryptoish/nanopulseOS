<picture>
  <source media="(prefers-color-scheme: dark)" srcset="/screenshot/NanoPulseOS-Dark.png">
  <source media="(prefers-color-scheme: light)" srcset="/screenshot/NanoPulseOS-Light.png">
  <img alt="NanoPulse OS Logo" src="/screenshot/NanoPulse-OS-Logo.png">
</picture>

# **NanoPulse OS**: **A NANO Programmed Unique Live Scripting Environment**

---

### **Objective**  
**NanoPulse OS** is a lightweight, portable, live operating system that runs directly from a USB stick. It is designed for creative users, offering tools for generative art, simple retro games, an experimental IDE with a custom esoteric programming language (soon), and fun easter eggs. NanoPulse OS is all about providing a minimal, fun, and interactive environment for exploration and creation. It leaves absolutely no trace on your computer after shutdown.

---

### **Key Features**  

| **Feature**                 | **Description**                                                                 |
|-----------------------------|---------------------------------------------------------------------------------|
| **Live System**              | Boots directly from a USB stick with no need for installation or hard drive modification. |
| **Generative Art**           | Create hardware footprint-style generative art which is unique to every computer. |
| **Retro Games**              | Includes simple retro games like Tetris, CS2 cases, and Portal (credits lol). |
| **Simple Scripting**         | Supports an easy-to-use esoteric scripting language for creating small programs. |
| **Portable**                 | Fully functional on any computer that supports USB boot, without affecting the host system. |
| **Minimalist Design**        | Focuses on simplicity and accessibility for both novice and advanced users (literally just all in VGA text mode). |
| **Cross-Platform Compatibility** | Compatible with most x86-based systems and works on a wide range of hardware without installation. |
| **Creativity**     | Built to inspire. |

---

### **How to Build and Run the Project**

1. Download the NanoPulse OS image (once available).
2. Flash the image onto a USB stick using a tool like [Rufus](https://rufus.ie/) or [Balena Etcher](https://www.balena.io/etcher/).
3. Insert the USB stick into any computer and reboot, ensuring the BIOS/UEFI settings allow booting from USB.
4. NanoPulse OS will boot directly from the USB and provide a graphical interface for generative art, retro games, and coding.

---

### **Basic Usage Instructions**

Type ```help``` into the terminal to see all available commands.

---

### **Technologies and Tools**  

| **Category**            | **Tools/Technologies**                                     |
|--------------------------|-----------------------------------------------------------|
| **Programming Languages**| C (kernel and OS development), Assembly (low-level initialization), custom esoteric language. |
| **Graphics**             | Simple pixel-based graphics engine (just VGA text mode cuz Im lazy) for retro games and generative art. |
| **Build System**         | GCC (compiling), Makefile, custom tools for building and flashing the OS image. |
| **Bootloader**           | GRUB or Syslinux for booting from USB and handling the OS kernel loading. |
| **User Interface**       | Lightweight simple unix style shell for art creation, game playing, and coding. |

---

### **Scope and Challenges**

1. **Minimalist Design**: Creating a lean, efficient OS that can run entirely from a USB stick without needing installation or hard drive changes.
2. **Generative Art**: Developing easy-to-use tools for users to create complex generative hardware footprint art with limited resources.
3. **Custom Language Interpreter**: Designing and implementing a lightweight interpreter for a minimalist esoteric programming language that is fun and easy to use, but not intended for serious development.
4. **Retro Game Creation**: Implementing simple game mechanics and a framework for creating and playing retro-style games (e.g., Space Invaders).
5. **Cross-Platform Compatibility**: Ensuring the OS works on a wide range of hardware, including supporting booting from various USB interfaces and configurations.

---

### **Future Plans**

- **Custom Bootloader, Custom Programming language, Custom Compiler**: To make the whole OS unique and mine. Right now I am using GRUB, C, GCC for these purposes.
- **Expanded Game Library**: Add more retro games and interactive challenges for users.
- **Advanced Scripting Features**: Extend the custom language to support more complex functionality for users who want to create more advanced programs.
- **Art Tools**: Improve and expand the generative art tools to allow for more creative control and complexity.
- **Performance Optimizations**: Fine-tune the OS to ensure smooth performance on a wide range of hardware configurations.
- **Project: Artifact**: Theme it around being a lost old OS, put it on a cheap USB then scatter it around places ARG-style to find.

---

### **Contributing**

If you’re interested in contributing to NanoPulse OS, feel free to fork the repository and submit pull requests. Contributions are welcome, whether it’s improving my (probably) terribly written code or adding cool new features for fun!

---

### Personal note for next steps in work: 

Custom Interperator IDE cmd. 


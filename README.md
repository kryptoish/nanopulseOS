<picture>
  <source media="(prefers-color-scheme: dark)" srcset="/screenshot/NanoPulseOS-Dark.png">
  <source media="(prefers-color-scheme: light)" srcset="/screenshot/NanoPulseOS-Light.png">
  <img alt="NanoPulse OS Logo" src="/screenshot/NanoPulse-OS-Logo.png">
</picture>

# **NanoPulse OS**: **A NANO Programmed Unique Live Scripting Environment**

---

### **Objective**  
**NanoPulse OS** is a lightweight, portable, live operating system that runs directly from a USB stick. It is designed for creative users, offering tools for generative art, simple retro game creation (like Space Invaders), and an experimental IDE with a custom "brainfuck"-like programming language interpreter. NanoPulse OS is all about providing a minimal, fun, and interactive environment for exploration and creation. It leaves absolutely no trace on your computer after shutdown.

---

### **Key Features**  

| **Feature**                 | **Description**                                                                 |
|-----------------------------|---------------------------------------------------------------------------------|
| **Live System**              | Boots directly from a USB stick with no need for installation or hard drive modification. |
| **Generative Art**           | Create hardware footprint-style generative art with simple, intuitive controls. |
| **Retro Games**              | Includes built-in tools for designing and playing simple retro games like Space Invaders. |
| **Custom IDE**               | Features a lightweight IDE for coding in a custom, minimalist programming language, inspired by Brainfuck. |
| **Simple Scripting**         | Supports an easy-to-use scripting language for creating small programs and games. |
| **Portable**                 | Fully functional on any computer that supports USB boot, without affecting the host system. |
| **Minimalist Design**        | Focuses on simplicity and accessibility for both novice and advanced users. |
| **Cross-Platform Compatibility** | Compatible with most x86-based systems and works on a wide range of hardware without installation. |
| **Creative Environment**     | Built to inspire users to experiment with code, art, and retro gaming. |

---

### **How to Build and Run the Project**

1. Download the NanoPulse OS image (once available).
2. Flash the image onto a USB stick using a tool like [Rufus](https://rufus.ie/) or [Balena Etcher](https://www.balena.io/etcher/).
3. Insert the USB stick into any computer and reboot, ensuring the BIOS/UEFI settings allow booting from USB.
4. NanoPulse OS will boot directly from the USB and provide a graphical interface for generative art, retro games, and coding.

---

### **Basic Usage Instructions**

- **Generative Art**: Once the OS is loaded, navigate to the "Art" section where you can create generative hardware footprint art using the built-in tools. The system allows for real-time previewing and modification.
  
- **Retro Games**: Access the "Games" menu to play or modify classic games like Space Invaders. You can also create new retro games with simple scripting commands.

- **Custom IDE**: The built-in IDE is minimalist but powerful enough to write, test, and run small programs in a custom, brainfuck-inspired language. Open the IDE from the main menu and start coding!

- **Programming**: Use the custom language interpreter to run programs written in a simple, low-level syntax. **Note**: The language is intentionally a joke and not meant to be used for serious programming. It's a fun, minimalist tool to experiment with coding, but it lacks the features and capabilities of a full-fledged programming language.

---

### **Persistent Storage**

- **Persistent Art**: The only data that persists across reboots on the host system is the **generative unique art footprint** created by the user. This allows you to save your creative works and reuse them in future sessions.
- **No Host Hard Drive Modification**: NanoPulse OS does not make any permanent changes to the host system's hard drive. All files and configurations are stored in RAM, ensuring that the OS remains lightweight and non-invasive.

---

### **Technologies and Tools**  

| **Category**            | **Tools/Technologies**                                     |
|--------------------------|-----------------------------------------------------------|
| **Programming Languages**| C (kernel and OS development), Assembly (low-level initialization), custom interpreter for brainfuck-like language. |
| **Graphics**             | Simple pixel-based graphics engine for retro games and generative art. |
| **Build System**         | GCC, Makefile, custom tools for building and flashing the OS image. |
| **Bootloader**           | GRUB or Syslinux for booting from USB and handling the OS kernel loading. |
| **User Interface**       | Lightweight graphical interface for art creation, game playing, and coding. |
| **External Libraries**   | SDL2 (for graphics and input handling), custom libraries for scripting and game creation. |

---

### **Scope and Challenges**

1. **Minimalist Design**: Creating a lean, efficient OS that can run entirely from a USB stick without needing installation or hard drive changes.
2. **Generative Art**: Developing easy-to-use tools for users to create complex generative hardware footprint art with limited resources.
3. **Custom Language Interpreter**: Designing and implementing a lightweight interpreter for a minimalist programming language (inspired by Brainfuck) that is fun and easy to use, but not intended for serious development.
4. **Retro Game Creation**: Implementing simple game mechanics and a framework for creating and playing retro-style games (e.g., Space Invaders).
5. **Cross-Platform Compatibility**: Ensuring the OS works on a wide range of hardware with no need for installation, including supporting booting from various USB interfaces and configurations.

---

### **Future Plans**

- **Expanded Game Library**: Add more retro games and interactive challenges for users.
- **Advanced Scripting Features**: Extend the custom language to support more complex functionality for users who want to create more advanced programs.
- **Art Tools**: Improve and expand the generative art tools to allow for more creative control and complexity.
- **Performance Optimizations**: Fine-tune the OS to ensure smooth performance on a wide range of hardware configurations.

---

### **Contributing**

If you’re interested in contributing to NanoPulse OS, feel free to fork the repository and submit pull requests. Contributions are welcome, whether it’s improving the IDE, adding more game templates, or creating new art tools!


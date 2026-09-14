<picture>
  <source media="(prefers-color-scheme: dark)" srcset="/screenshot/NanoPulseOS-Dark.png">
  <source media="(prefers-color-scheme: light)" srcset="/screenshot/NanoPulseOS-Light.png">
  <img alt="NanoPulse OS Logo" src="/screenshot/NanoPulse-OS-Logo.png">
</picture>

# **NanoPulse OS**: **A NANO Programmed OS that has Pulse**

---

### **Objective**  
**NanoPulse OS** is a lightweight, portable, live operating system that runs directly from a USB stick (x86-32 i686). It is designed for learning/hobby usage and includes -> generative art, simple retro games, an experimental IDE with a custom esoteric programming language (soon), and fun easter eggs. NanoPulse OS is all about providing a minimal, fun, and interactive environment for exploration and creation. It leaves absolutely no trace on your computer after shutdown.

---

### **Key Features**  
Live system booted from USB, pseudo-random/hardware-based generative art, retro games like Tetris, esoteric scripting language (not yet implemented), lightweight portability, simple flat text based design reminiscent of older operating systems, and pretty decent x86 laptop compatibility (as long as you have legacy boot mode).

---

### **How to Build and Run the Project**

1. [Download](https://www.nanopulseos.org/) the NanoPulse OS image (once available).
2. Flash the image onto a USB stick using a tool like [Rufus](https://rufus.ie/) or [Balena Etcher](https://www.balena.io/etcher/).
3. Keep "Partition scheme" to MBR and select "Write in DD image mode" after starting (Your device will most likely need Legacy boot ability).
4. Insert the USB stick into any computer and reboot, ensuring the BIOS/UEFI settings allow booting from USB (secure boot off).
5. NanoPulse OS will boot directly from the USB be functional.

You can also try it out on the website which I have added a v86 emulator in a website.

---

### **Basic Usage Instructions**

Type ```help``` into the terminal to see all available commands.

---

### **Technologies and Tools**  

- Kernel written in C & x86 Assembly
- Build system uses GCC, Makefiele & grubmkrescue for .iso generation
- Graphics is a Double framebuffer 32-bit mode (for that old effect)
- Uses GRUB for bootloading
- Some posix cmds and mostly just fun little extra features (use ```help```)

---

### **Scope and Challenges/Goals**

1. Minimalist Design
2. Generative Art
3. Custom Language Interpreter
4. Retro Game Creation
5. Wide-range of x86 laptop Compatibility

---

### **Future Plans?**

- **Custom Bootloader, Custom Programming language, Custom Compiler**: To make the whole OS unique and mine. Right now I am using GRUB, C, GCC for these purposes. (This is farfetched timeline wise)
- **Expanded Game Library**: Add more retro games and interactive challenges for users.
- **Advanced Scripting Features**: Extend the custom language to support more complex functionality for users who want to create more advanced programs.
- **Art Tools**: Improve and expand the generative art tools to allow for more creative control and complexity.
- **Performance Optimizations**: Fine-tune the OS to ensure smooth performance on a wide range of hardware configurations/compatibility.
- **Project: Artifact**: Theme it around being a lost old OS, put it on a cheap USB then scatter it around places ARG-style to find.

---

### **Contributing**

If you’re interested in contributing to NanoPulse OS, feel free to fork the repository and submit pull requests. Contributions are welcome, whether it’s improving my (probably) terribly written code or adding cool new features for fun!

---

### Personal note for next steps in work: 

Custom Interperator IDE cmd. 


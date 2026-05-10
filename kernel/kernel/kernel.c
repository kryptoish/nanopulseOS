#include <stdio.h>

#include <kernel/tty.h>
#include <kernel/ramfs.h>
#include <drivers/screen.h>
#include <drivers/keyboard.h>
#include <drivers/ports.h>
#include <drivers/fb.h>
#include <interrupts/isr.h>
#include <interrupts/timer.h>
#include <kernel/gamble.h>

extern const char sc_ascii[];
extern const int SC_MAX;

/* Multiboot2 tag header. */
struct mb2_tag { u32 type; u32 size; };

/* Type 8: framebuffer info tag. */
struct mb2_fb_tag {
    u32 type; u32 size;
    u64 addr;
    u32 pitch;
    u32 width;
    u32 height;
    u8  bpp;
    u8  fb_type;
    u8  reserved[2];
};

static void init_framebuffer_from_mb2(u32 magic, u32 mbi_addr) {
    if (magic != 0x36d76289 || mbi_addr == 0) return;

    u8 *p = (u8 *)mbi_addr;
    u32 total = *(u32 *)p;
    u8 *end   = p + total;
    p += 8; /* skip total_size + reserved */

    while (p < end) {
        struct mb2_tag *t = (struct mb2_tag *)p;
        if (t->type == 0) break;
        if (t->type == 8) {
            struct mb2_fb_tag *fb = (struct mb2_fb_tag *)t;
            fb_init((u32)fb->addr, fb->pitch, fb->width, fb->height, fb->bpp);
            return;
        }
        u32 step = (t->size + 7) & ~7u;
        p += step;
    }
}

void kernel_main(u32 magic, u32 mbi_addr) {
    init_framebuffer_from_mb2(magic, mbi_addr);

	terminal_initialize();
	isr_install();
	kprint("=== nanopulseOS Kernel Starting ===\n");

	keyboard_init();
	kprint("Keyboard init success\n");

	init_timer(100); /* 100 Hz PIT tick - game loop/animation base. */
	kprint("Timer init (100 Hz)\n");

	ramfs_init();
	kprint("RAM filesystem init\n");

	__asm__ __volatile__("sti");
	kprint("Interrupts enabled\n");
	kprint("OS initialization successful!\n");
	delay_ms(500);
	clear_screen();
	
	#define LP "               "
	kprint(
		"                            Welcome to nanopulseOS!\n"
		"\n"
		LP "               +B$$$$$$$$$$$$$$r.\n"
		LP "           ^q$$@O+ .+/xcct]` ,nB$$#!\n"
		LP "         ~@$$_ [8$$$$$$$$$$$$$BU ,W$@r\n"
		LP "       ,$$M.<@$$$$$$$$$$$$$$$$$$$$j J$$]\n"
		LP "      C$B;>@$$$$$$$$$$$$$$$$$$$$$$$$c a$*\n"
		LP "     W$*     &&\\   @$$$$$$$$.  ,&&-   .j$$\n"
		LP "    b$m $-  ,$$Q   $$$$$$$$$\"  +$$x   $i_$B\n"
		LP "   r$@ 8w @$$$$$$${'$$$$$$$u @$$$$$$$;_$'z$m\n"
		LP "   $$\"   '$$$$$$$$x   -j/   '$$$$$$$$?    $$`\n"
		LP "  t$k }  j$$$$$$$$W   8$@i  Q$$$$$$$$M  ^~t$q\n"
		LP "  0$vI$$@^\\w@$$@Mz *$$$$$$$B fb@$$@*v M$$X`$$\n"
		LP "  O$/]$$@    ;;'   o$$$$$$$@    ;;.   o$$O'$$\n"
		LP "  J$c:$$$d  0$$8`  $$$$$$$$$\"  d$$#  r$$$z`$$\n"
		LP "  1$h $$$$$$$$$$$$Z <Q$$$h-\"[$$$$$$$$$$$$<x$k\n"
		LP "   $$\"n$$$$$$$$$$$/          $$$$$$$$$$$# $$^\n"
		LP "   {$$ M$$$$$$$$$$b `\\$$$m`.t$$$$$$$$$$$ J$m\n"
		LP "    O$p B$$$$$$8i  $$$$$$$$$\" ^d$$$$$$$;1$B\n"
		LP "     #$& d$$$$$<   h$$$$$$$@   .$$$$$$.U$$\n"
		LP "      v$@i\"B$$$$0{ M$$$$$$$%.]n$$$$$t.8$a\n"
		LP "       ^@$%I;8$$$$W  ^B$@+  Q$$$$@[ d$$~\n"
		LP "         ;B$$u :W@x   '^^    $%{ !$$@[\n"
		LP "           .C$$$h|'  +1)1-   ]Z$$$d`\n"
		LP "               !#$$$$$$$$$$$$$B]."
	);
	#undef LP

	delay_ms(3000);
	clear_screen();
	kprint("nanopulseOS\n");
	kprint("> ");
	
	while(1) {
        // Check for keyboard input
		if (check_keyboard_interrupt()) {
			// Keyboard input is handled in the interrupt callback
			// This just ensures we process any pending input
		}
		__asm__ __volatile__("hlt");
	}
}

#define VIDEO_MEMORY 0xB8000
#define MAX_COMMAND_LENGTH 64

// Реализация strncmp (чтобы не было ошибки линковки)
int strncmp(const char* s1, const char* s2, int n) {
    while (n-- && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return n < 0 ? 0 : *(unsigned char*)s1 - *(unsigned char*)s2;
}

// Multiboot info structure
struct multiboot_info {
    unsigned long flags;
    unsigned long mem_lower;
    unsigned long mem_upper;
    unsigned long boot_device;
    unsigned long cmdline;
    unsigned long mods_count;
    unsigned long mods_addr;
    unsigned long syms[4];
    unsigned long mmap_length;
    unsigned long mmap_addr;
    unsigned long drives_length;
    unsigned long drives_addr;
    unsigned long config_table;
    unsigned long boot_loader_name;
    unsigned long apm_table;
    unsigned long vbe_control_info;
    unsigned long vbe_mode_info;
    unsigned short vbe_mode;
    unsigned short vbe_interface_seg;
    unsigned short vbe_interface_off;
    unsigned short vbe_interface_len;
};

// Global variables
char command_buffer[MAX_COMMAND_LENGTH];
int cursor_pos = 0;

// Function prototypes
void clear_screen();
void print(const char* str);
void print_char(char c);
void print_hex(unsigned long n);
void print_dec(unsigned long n);
void print_prompt();
void execute_command(const char* cmd);

// Port I/O functions
void outb(unsigned short port, unsigned char data) {
    asm volatile ("outb %0, %1" : : "a"(data), "Nd"(port));
}

unsigned char inb(unsigned short port) {
    unsigned char ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// String functions
int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

void strcpy(char* dest, const char* src) {
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
}

void memset(void* ptr, int value, unsigned long num) {
    unsigned char* p = ptr;
    while (num--) {
        *p++ = value;
    }
}

// Структуры для IDT
struct idt_entry {
    unsigned short base_lo;
    unsigned short sel;
    unsigned char always0;
    unsigned char flags;
    unsigned short base_hi;
} __attribute__((packed));

struct idt_ptr {
    unsigned short limit;
    unsigned int base;
} __attribute__((packed));

struct idt_entry idt[256];
struct idt_ptr idtp;

// Внешняя функция из assembly (загрузка IDT)
extern void idt_load();

void idt_set_gate(unsigned char num, unsigned long base, unsigned short sel, unsigned char flags) {
    idt[num].base_lo = base & 0xFFFF;
    idt[num].base_hi = (base >> 16) & 0xFFFF;
    idt[num].sel = sel;
    idt[num].always0 = 0;
    idt[num].flags = flags;
}

void idt_init() {
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
    idtp.base = (unsigned int)&idt;
    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, 0, 0, 0);
    }
    idt_load();
}

// Kernel main function
void kernel_main(unsigned long magic, unsigned long mb_info_addr) {
    // Check multiboot magic
    if (magic != 0x2BADB002) {
        print("Error: Not booted by Multiboot loader!");
        return;
    }
    
    clear_screen();
    
    print("===============================================\n");
    print("              OUROS OS - Multiboot Edition     \n");
    print("===============================================\n\n");
    
    // Show multiboot info
    struct multiboot_info* mb_info = (struct multiboot_info*)mb_info_addr;
    
    print("Multiboot magic: 0x");
    print_hex(magic);
    print("\n");
    
    if (mb_info->flags & 0x01) {
        print("Memory: ");
        print_dec((mb_info->mem_lower + mb_info->mem_upper) / 1024);
        print("KB available\n");
    }
    
    print("\nType 'help' for available commands\n");
    print("===============================================\n\n");
    
    print_prompt();
    
    // Main loop
    while (1) {
        // Check for keyboard input
        if (inb(0x64) & 0x01) {
            unsigned char key = inb(0x60);
            
            if (key == 0x1C) { // Enter
                command_buffer[cursor_pos] = '\0';
                print("\n");
                execute_command(command_buffer);
                cursor_pos = 0;
                memset(command_buffer, 0, MAX_COMMAND_LENGTH);
                print_prompt();
            } 
            else if (key == 0x0E) { // Backspace
                if (cursor_pos > 0) {
                    cursor_pos--;
                    command_buffer[cursor_pos] = '\0';
                    print_char('\b');
                    print_char(' ');
                    print_char('\b');
                }
            }
            else if (key < 0x80 && cursor_pos < MAX_COMMAND_LENGTH - 1) {
                // Simple keyboard map
                const char* keyboard_map = "??1234567890-=??qwertyuiop[]??asdfghjkl;'`?\\zxcvbnm,./?*? ?";
                if (key < 64 && keyboard_map[key] != '?') {
                    char character = keyboard_map[key];
                    command_buffer[cursor_pos++] = character;
                    print_char(character);
                }
            }
            
            // Acknowledge keyboard
            outb(0x20, 0x20);
        }
        
        // Save CPU cycles
        asm volatile ("hlt");
    }
}

void clear_screen() {
    unsigned char* vidmem = (unsigned char*)VIDEO_MEMORY;
    for (int i = 0; i < 80 * 25 * 2; i += 2) {
        vidmem[i] = ' ';
        vidmem[i+1] = 0x07;
    }
}

void print(const char* str) {
    while (*str) {
        print_char(*str);
        str++;
    }
}

void print_char(char c) {
    static int cursor = 0;
    unsigned char* vidmem = (unsigned char*)VIDEO_MEMORY;
    
    if (c == '\n') {
        cursor = ((cursor / 160) + 1) * 160;
    } else if (c == '\b') {
        if (cursor > 0) {
            cursor -= 2;
            vidmem[cursor] = ' ';
            vidmem[cursor+1] = 0x07;
        }
    } else {
        vidmem[cursor++] = c;
        vidmem[cursor++] = 0x07;
    }
    
    // Scroll if needed
    if (cursor >= 80 * 25 * 2) {
        for (int i = 160; i < 80 * 25 * 2; i++) {
            vidmem[i - 160] = vidmem[i];
        }
        for (int i = 80 * 24 * 2; i < 80 * 25 * 2; i += 2) {
            vidmem[i] = ' ';
            vidmem[i+1] = 0x07;
        }
        cursor = 80 * 24 * 2;
    }
}

void print_hex(unsigned long n) {
    char hex_chars[] = "0123456789ABCDEF";
    char hex_str[16];
    int i = 0;
    
    if (n == 0) {
        print("0");
        return;
    }
    
    while (n > 0) {
        hex_str[i++] = hex_chars[n & 0xF];
        n >>= 4;
    }
    
    for (int j = i - 1; j >= 0; j--) {
        print_char(hex_str[j]);
    }
}

void print_dec(unsigned long n) {
    char dec_str[16];
    int i = 0;
    
    if (n == 0) {
        print("0");
        return;
    }
    
    while (n > 0) {
        dec_str[i++] = '0' + (n % 10);
        n /= 10;
    }
    
    for (int j = i - 1; j >= 0; j--) {
        print_char(dec_str[j]);
    }
}

void print_prompt() {
    print("ouros> ");
}

void execute_command(const char* cmd) {
    if (strcmp(cmd, "help") == 0) {
        print("Available commands:\n");
        print("  help    - Show this help\n");
        print("  clear   - Clear screen\n");
        print("  info    - System information\n");
        print("  reboot  - Reboot system\n");
        print("  echo    - Echo text\n");
    }
    else if (strcmp(cmd, "clear") == 0) {
        clear_screen();
    }
    else if (strcmp(cmd, "info") == 0) {
        print("OUROS OS v1.0 - Multiboot Edition\n");
        print("32-bit Protected Mode\n");
        print("Video: 80x25 text mode\n");
    }
    else if (strcmp(cmd, "reboot") == 0) {
        print("Rebooting...\n");
        // Trigger reboot
        outb(0x64, 0xFE);
        while (1) asm volatile ("hlt");
    }
    else if (strncmp(cmd, "echo ", 5) == 0) {
        print(cmd + 5);
        print("\n");
    }
    else if (cmd[0] != '\0') {
        print("Unknown command: ");
        print(cmd);
        print("\nType 'help' for available commands\n");
    }
}

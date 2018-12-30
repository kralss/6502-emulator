#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <iomanip>

bool
get_bit(char x, int n)
{
    return x & (1u << n);
}

void
set_bit(char& x, int n, bool v)
{
    if (v) {
        x |= 1u << n;
    } else {
        x &= ~(1u << n);
    }
}

unsigned
make_addr(char a, char b)
{
    return (unsigned(a) << 8) | b;
}

class vm_c
{
    bool running;

    // command table
    char* arg;
    void (vm_c::*fetch_arg[0x100])();
    void (vm_c::*command[0x100])();

    // helper functions

    void set_carry_flag(bool x) {
        set_bit(status, 0, x);
    }

    bool get_carry_flag() {
        return get_bit(status, 0);
    }

    void set_zero_flag(bool x) {
        set_bit(status, 1, x);
    }

    bool get_zero_flag() {
        return get_bit(status, 1);
    }

    void set_overflow_flag(bool x) {
        set_bit(status, 6, x);
    }

    bool get_overflow_flag() {
        return get_bit(status, 6);
    }

    void set_negative_flag(bool x) {
        set_bit(status, 7, x);
    }

    bool get_negative_flag() {
        return get_bit(status, 7);
    }

    void set_with_flags(char& u, char v) {
        u = v;
        set_zero_flag(u == 0);
        set_negative_flag(get_bit(u, 7));
    }

    void push(char x) {
        memory[0x100u + rs] = x;
        rs--;
    }

    char pull() {
        rs++;
        return memory[0x100u + rs];
    }

    void add_signed(unsigned& x, char y) {
        if (y < 0x80u) {
            x += y;
        } else {
            x -= ~y + 1;
        }
    }

    // addressing modes

    void implicit() {
        pc += 1;
    }

    void accumulator() {
        pc += 1;
        arg = &ra;
    }

    void immediate() {
        arg = &memory[pc + 1];
        pc += 2;
    }

    void zero_page() {
        arg = &memory[memory[pc + 1]];
        pc += 2;
    }

    void zero_page_x() {
        arg = &memory[memory[pc + 1] + rx];
        pc += 2;
    }

    void zero_page_y() {
        arg = &memory[memory[pc + 1] + ry];
        pc += 2;
    }

    void absolute() {
        auto a = &memory[pc];
        arg = &memory[make_addr(a[2], a[1])];
        pc += 3;
    }

    void absolute_x() {
        auto a = &memory[pc];
        arg = &memory[make_addr(a[2], a[1]) + rx];
        pc += 3;
    }

    void absolute_y() {
        auto a = &memory[pc];
        arg = &memory[make_addr(a[2], a[1]) + ry];
        pc += 3;
    }

    void indirect() {
        auto a = &memory[pc];
        auto ptr = &memory[make_addr(a[2], a[1])];
        arg = &memory[make_addr(*(ptr + 1), *ptr)];
        pc += 3;
    }

    void indirect_x() {
        auto a = &memory[pc];
        auto addr = &memory[a[1] + rx];
        arg = &memory[make_addr(*(addr + 1), *addr)];
        pc += 2;
    }

    void indirect_y() {
        auto a = &memory[pc];
        arg = &memory[make_addr(memory[a[1] + 1], memory[a[1]]) + ry];
        pc += 2;
    }

    void relative() {
        arg = &memory[pc + 1];
        pc += 2;
    }

    // instructions

    void exclusive_or() {
        set_with_flags(ra, ra ^ *arg);
    }

    void inclusive_or() {
        set_with_flags(ra, ra | *arg);
    }

    void logical_and() {
        set_with_flags(ra, ra & *arg);
    }

    void bit_test() {
        set_zero_flag((ra & *arg) == 0);
        set_overflow_flag(get_bit(*arg, 6));
        set_negative_flag(get_bit(*arg, 7));
    }

    void arithmetic_shift_left() {
        auto& x = *arg;
        set_carry_flag(get_bit(x, 7));
        x <<= 1;
        set_zero_flag(x == 0);
        set_negative_flag(get_bit(x, 7));
    }

    void logical_shift_right() {
        auto& x = *arg;
        set_carry_flag(get_bit(x, 0));
        x >>= 1;
        set_zero_flag(x == 0);
        set_negative_flag(get_bit(x, 7));
    }

    void rotate_left() {
        auto& x = *arg;
        auto ca = get_carry_flag();
        set_carry_flag(get_bit(x, 7));
        x <<= 1;
        set_bit(x, 0, ca);
        set_zero_flag(x == 0);
        set_negative_flag(get_bit(x, 7));
    }

    void rotate_right() {
        auto& x = *arg;
        auto ca = get_carry_flag();
        set_carry_flag(get_bit(x, 0));
        x >>= 1;
        set_bit(x, 7, ca);
        set_zero_flag(x == 0);
        set_negative_flag(get_bit(x, 7));
    }

    void load_accumulator() {
        set_with_flags(ra, *arg);
    }

    void load_x_register() {
        set_with_flags(rx, *arg);
    }

    void load_y_register() {
        set_with_flags(ry, *arg);
    }

    void store_accumulator() {
        *arg = ra;
    }

    void store_x_register() {
        *arg = rx;
    }

    void store_y_register() {
        *arg = ry;
    }

    void transfer_accumulator_to_x() {
        set_with_flags(rx, ra);
    }

    void transfer_accumulator_to_y() {
        set_with_flags(ry, ra);
    }

    void transfer_x_to_accumulator() {
        set_with_flags(ra, rx);
    }

    void transfer_y_to_accumulator() {
        set_with_flags(ra, ry);
    }

    void increment_memory() {
        set_with_flags(*arg, (*arg) + 1);
    }

    void increment_x_register() {
        set_with_flags(rx, rx + 1);
    }

    void increment_y_register() {
        set_with_flags(ry, ry + 1);
    }

    void decrement_memory() {
        set_with_flags(*arg, (*arg) - 1);
    }

    void decrement_x_register() {
        set_with_flags(rx, rx - 1);
    }

    void decrement_y_register() {
        set_with_flags(ry, ry - 1);
    }

    void transfer_stack_pointer_to_x() {
        set_with_flags(rx, rs);
    }

    void transfer_x_to_stack_pointer() {
        rs = rx;
    }

    void push_accumulator() {
        push(ra);
    }

    void push_status() {
        auto s = status;
        set_bit(s, 5, 1);
        set_bit(s, 4, 1);
        push(s);
    }

    void pull_accumulator() {
        set_with_flags(ra, pull());
    }

    void pull_status() {
        auto s = pull();
        set_bit(s, 5, 0);
        set_bit(s, 4, 0);
        status = s;
    }

    void jump() {
        pc = arg - &memory[0];
    }

    void jump_to_subroutine() {
        pc -= 3;
        push(char(pc >> 8));
        push(char(pc));
        pc = arg - &memory[0];
    }

    void return_from_subroutine() {
        auto v = pull();
        auto u = pull();
        pc = make_addr(u, v) + 3;
    }

    void branch_if_carry_clear() {
        if (get_carry_flag() == false) {
            add_signed(pc, *arg);
        }
    }

    void branch_if_carry_set() {
        if (get_carry_flag() == true) {
            add_signed(pc, *arg);
        }
    }

    void branch_if_minus() {
        if (get_negative_flag() == true) {
            add_signed(pc, *arg);
        }
    }

    void branch_if_equal() {
        if (get_zero_flag() == true) {
            add_signed(pc, *arg);
        }
    }

    void branch_if_not_equal() {
        if (get_zero_flag() == false) {
            add_signed(pc, *arg);
        }
    }

    void branch_if_positive() {
        if (get_negative_flag() == false) {
            add_signed(pc, *arg);
        }
    }

    void branch_if_overflow_clear() {
        if (get_overflow_flag() == false) {
            add_signed(pc, *arg);
        }
    }

    void branch_if_overflow_set() {
        if (get_overflow_flag() == true) {
            add_signed(pc, *arg);
        }
    }

    void clear_carry_flag() {
        set_carry_flag(false);
    }

    void set_carry_flag() {
        set_carry_flag(true);
    }

    void clear_overflow_flag() {
        set_overflow_flag(false);
    }

    void clear_decimal_mode() {
        set_bit(status, 3, false);
    }

    void set_decimal_flag() {
        set_bit(status, 3, true);
    }

    void clear_interrupt_disable() {
        set_bit(status, 2, false);
    }

    void set_interrupt_disable() {
        set_bit(status, 2, true);
    }

    void no_operation() {
    }

    void add_with_carry() {
        unsigned res = ra;
        unsigned v = *arg;
        v += get_carry_flag();
        auto a7 = get_bit(ra, 7);
        auto b7 = get_bit(v, 7);
        res += v;
        set_with_flags(ra, res);
        auto c7 = get_bit(ra, 7);
        set_overflow_flag(a7 == b7 && a7 != c7);
        set_carry_flag(res >= 0x100);
    }

    void subtract_with_carry() {
        unsigned res = ra;
        unsigned xx = *arg;
        xx += !get_carry_flag();
        auto a7 = get_bit(ra, 7);
        auto b7 = get_bit(xx, 7);
        res -= xx;
        set_with_flags(ra, res);
        auto c7 = get_bit(ra, 7);
        set_overflow_flag(a7 != b7 && b7 == c7);
        set_carry_flag(res < 0x100);
    }

    void compare() {
        set_carry_flag(ra >= *arg);
        set_zero_flag(ra == *arg);
        set_negative_flag(get_bit(ra - *arg, 7));
    }

    void compare_x_register() {
        set_carry_flag(rx >= *arg);
        set_zero_flag(rx == *arg);
        set_negative_flag(get_bit(rx - *arg, 7));
    }

    void compare_y_register() {
        set_carry_flag(ry >= *arg);
        set_zero_flag(ry == *arg);
        set_negative_flag(get_bit(ry - *arg, 7));
    }

    void force_interrupt() {
        std::cout << "warning : brk is not supported\n";
    }

    void return_from_interrupt() {
        status = pull();
        auto u = pull();
        auto v = pull();
        pc = make_addr(v, u);
    }

    void undefined() {
        unsigned cmd = memory[pc - 1];
        std::cout << "warning : unknown instruction 0x" << cmd << "\n";
        running = false;
    }

public:

    std::vector<char> memory;

    // registers
    unsigned pc;
    char rs;
    char ra;
    char rx;
    char ry;
    char status;

    vm_c() : memory(0x10000, 0xff) {
        running = true;

        pc = 0x0200;
        rs = 0xff;
        ra = 0;
        rx = 0;
        ry = 0;
        status = 0;

        std::fill(fetch_arg, fetch_arg + 0x100u, &vm_c::implicit);
        std::fill(command, command + 0x100u, &vm_c::undefined);

#define instr(opcode, op, mode) do {              \
            fetch_arg[opcode] = &vm_c::mode;      \
            command[opcode] = &vm_c::op;          \
        } while (0);                              \

        instr(0x29, logical_and, immediate);
        instr(0x25, logical_and, zero_page);
        instr(0x35, logical_and, zero_page_x);
        instr(0x2d, logical_and, absolute);
        instr(0x3d, logical_and, absolute_x);
        instr(0x39, logical_and, absolute_y);
        instr(0x21, logical_and, indirect_x);
        instr(0x31, logical_and, indirect_y);

        instr(0x49, exclusive_or, immediate);
        instr(0x45, exclusive_or, zero_page);
        instr(0x55, exclusive_or, zero_page_x);
        instr(0x4d, exclusive_or, absolute);
        instr(0x5d, exclusive_or, absolute_x);
        instr(0x59, exclusive_or, absolute_y);
        instr(0x41, exclusive_or, indirect_x);
        instr(0x51, exclusive_or, indirect_y);

        instr(0x09, inclusive_or, immediate);
        instr(0x05, inclusive_or, zero_page);
        instr(0x15, inclusive_or, zero_page_x);
        instr(0x0d, inclusive_or, absolute);
        instr(0x1d, inclusive_or, absolute_x);
        instr(0x19, inclusive_or, absolute_y);
        instr(0x01, inclusive_or, indirect_x);
        instr(0x11, inclusive_or, indirect_y);

        instr(0x24, bit_test, zero_page);
        instr(0x2c, bit_test, absolute);

        instr(0xa9, load_accumulator, immediate);
        instr(0xa5, load_accumulator, zero_page);
        instr(0xb5, load_accumulator, zero_page_x);
        instr(0xad, load_accumulator, absolute);
        instr(0xbd, load_accumulator, absolute_x);
        instr(0xb9, load_accumulator, absolute_y);
        instr(0xa1, load_accumulator, indirect_x);
        instr(0xb1, load_accumulator, indirect_y);

        instr(0xa2, load_x_register, immediate);
        instr(0xa6, load_x_register, zero_page);
        instr(0xb6, load_x_register, zero_page_y);
        instr(0xae, load_x_register, absolute);
        instr(0xbe, load_x_register, absolute_y);

        instr(0xa0, load_y_register, immediate);
        instr(0xa4, load_y_register, zero_page);
        instr(0xb4, load_y_register, zero_page_x);
        instr(0xac, load_y_register, absolute);
        instr(0xbc, load_y_register, absolute_x);

        instr(0x85, store_accumulator, zero_page);
        instr(0x95, store_accumulator, zero_page_x);
        instr(0x8d, store_accumulator, absolute);
        instr(0x9d, store_accumulator, absolute_x);
        instr(0x99, store_accumulator, absolute_y);
        instr(0x81, store_accumulator, indirect_x);
        instr(0x91, store_accumulator, indirect_y);

        instr(0x86, store_x_register, zero_page);
        instr(0x96, store_x_register, zero_page_y);
        instr(0x8e, store_x_register, absolute);

        instr(0x84, store_y_register, zero_page);
        instr(0x94, store_y_register, zero_page_x);
        instr(0x8c, store_y_register, absolute);

        instr(0xaa, transfer_accumulator_to_x, implicit);
        instr(0xa8, transfer_accumulator_to_y, implicit);
        instr(0x8a, transfer_x_to_accumulator, implicit);
        instr(0x98, transfer_y_to_accumulator, implicit);

        instr(0xe6, increment_memory, zero_page);
        instr(0xf6, increment_memory, zero_page_x);
        instr(0xee, increment_memory, absolute);
        instr(0xfe, increment_memory, absolute_x);
        instr(0xe8, increment_x_register, implicit);
        instr(0xc8, increment_y_register, implicit);

        instr(0xc6, decrement_memory, zero_page);
        instr(0xd6, decrement_memory, zero_page_x);
        instr(0xce, decrement_memory, absolute);
        instr(0xde, decrement_memory, absolute_x);
        instr(0xca, decrement_x_register, implicit);
        instr(0x88, decrement_y_register, implicit);

        instr(0x0a, arithmetic_shift_left, accumulator);
        instr(0x06, arithmetic_shift_left, zero_page);
        instr(0x16, arithmetic_shift_left, zero_page_x);
        instr(0x0e, arithmetic_shift_left, absolute);
        instr(0x1e, arithmetic_shift_left, absolute_x);

        instr(0x4a, logical_shift_right, accumulator);
        instr(0x46, logical_shift_right, zero_page);
        instr(0x56, logical_shift_right, zero_page_x);
        instr(0x4e, logical_shift_right, absolute);
        instr(0x5e, logical_shift_right, absolute_x);

        instr(0x2a, rotate_left, accumulator);
        instr(0x26, rotate_left, zero_page);
        instr(0x36, rotate_left, zero_page_x);
        instr(0x2e, rotate_left, absolute);
        instr(0x3e, rotate_left, absolute_x);

        instr(0x6a, rotate_right, accumulator);
        instr(0x66, rotate_right, zero_page);
        instr(0x76, rotate_right, zero_page_x);
        instr(0x6e, rotate_right, absolute);
        instr(0x7e, rotate_right, absolute_x);

        instr(0xba, transfer_stack_pointer_to_x, implicit);
        instr(0x9a, transfer_x_to_stack_pointer, implicit);
        instr(0x48, push_accumulator, implicit);
        instr(0x08, push_status, implicit);
        instr(0x68, pull_accumulator, implicit);
        instr(0x28, pull_status, implicit);

        instr(0x4c, jump, absolute);
        instr(0x6c, jump, indirect);
        instr(0x20, jump_to_subroutine, absolute);
        instr(0x60, return_from_subroutine, implicit);

        instr(0x90, branch_if_carry_clear, relative);
        instr(0xb0, branch_if_carry_set, relative);
        instr(0xf0, branch_if_equal, relative);
        instr(0x30, branch_if_minus, relative);
        instr(0xd0, branch_if_not_equal, relative);
        instr(0x10, branch_if_positive, relative);
        instr(0x50, branch_if_overflow_clear, relative);
        instr(0x70, branch_if_overflow_set, relative);

        instr(0x18, clear_carry_flag, implicit);
        instr(0xd8, clear_decimal_mode, implicit);
        instr(0x58, clear_interrupt_disable, implicit);
        instr(0xb8, clear_overflow_flag, implicit);
        instr(0x38, set_carry_flag, implicit);
        instr(0xf8, set_decimal_flag, implicit);
        instr(0x78, set_interrupt_disable, implicit);

        instr(0x69, add_with_carry, immediate);
        instr(0x65, add_with_carry, zero_page);
        instr(0x75, add_with_carry, zero_page_x);
        instr(0x6d, add_with_carry, absolute);
        instr(0x7d, add_with_carry, absolute_x);
        instr(0x79, add_with_carry, absolute_y);
        instr(0x61, add_with_carry, indirect_x);
        instr(0x71, add_with_carry, indirect_y);

        instr(0xe9, subtract_with_carry, immediate);
        instr(0xe5, subtract_with_carry, zero_page);
        instr(0xf5, subtract_with_carry, zero_page_x);
        instr(0xed, subtract_with_carry, absolute);
        instr(0xfd, subtract_with_carry, absolute_x);
        instr(0xf9, subtract_with_carry, absolute_y);
        instr(0xe1, subtract_with_carry, indirect_x);
        instr(0xf1, subtract_with_carry, indirect_y);

        instr(0xc9, compare, immediate);
        instr(0xc5, compare, zero_page);
        instr(0xd5, compare, zero_page_x);
        instr(0xcd, compare, absolute);
        instr(0xdd, compare, absolute_x);
        instr(0xd9, compare, absolute_y);
        instr(0xc1, compare, indirect_x);
        instr(0xd1, compare, indirect_y);

        instr(0xe0, compare_x_register, immediate);
        instr(0xe4, compare_x_register, zero_page);
        instr(0xec, compare_x_register, absolute);

        instr(0xc0, compare_y_register, immediate);
        instr(0xc4, compare_y_register, zero_page);
        instr(0xcc, compare_y_register, absolute);

        instr(0xea, no_operation, implicit);
        instr(0x00, force_interrupt, implicit);
        instr(0x40, return_from_interrupt, implicit);

#undef instr
    }

    int load_file(const std::string& file) {
        std::ifstream input(file, std::ios::binary);
        if (!input.good()) {
            return -1;
        }
        input.read(&memory[pc], memory.size() - pc);
        return 0;
    }

    void load(const std::vector<char>& v) {
        std::copy(v.begin(), v.end(), memory.begin() + pc);
    }

    void execute() {
        if (pc >= memory.size()) {
            running = false;
            return;
        }

        auto a = &memory[pc];
        ((*this).*(fetch_arg[a[0]]))();
        ((*this).*(command[a[0]]))();
    }

    void run() {
        while (running) {
            execute();
        }
    }

    void test() {
        pc = 0x4000;
        load_file("test.bin");
        while (pc != 0x45c0) {
            execute();
        }
        std::cout << "answer : 0x" << unsigned(memory[0x0210]) << "\n";
        if (memory[0x0210] == 0xff) {
            std::cout << "test pass\n";
        } else {
            std::cout << "test fail\n";
        }
    }
};

int
main()
{
    std::cout << std::setfill('0') << std::setw(2) << std::hex;
    vm_c vm;
    vm.test();
}

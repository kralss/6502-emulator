#include <iostream>
#include <string>
#include <vector>

#include "test.hpp"
#include "machine.hpp"
#include "misc.hpp"

static t_machine mach;

static int pass_count;
static int total_count;

static void tst(const std::string& label, const std::vector<char>& prog) {
    std::cout << "test : " << label << "\n";
    mach.init();
    mach.load_program(prog, 0x200);
    mach.run();
}

static void vfy(bool cond) {
    if (cond) {
        std::cout << "pass\n";
        pass_count++;
    } else {
        std::cout << "fail\n";
    }
    std::cout << "\n";
    total_count++;
}

static char mem(t_addr addr) {
    return mach.read_memory(addr);
}

static void
test_logical_and()
{
    tst("and", {0xa9, 0x25, 0x29, 0x36, 0x85, 0x99});
    vfy(mem(0x99) == (0x25u & 0x36u));
    tst("and", {0xa9, 0x0f, 0x29, 0xf0, 0x08});
    vfy(get_bit(mem(0x1ff), 1) == 1);
    tst("and", {0xa9, 0xff, 0x29, 0xf0, 0x08});
    vfy(get_bit(mem(0x1ff), 7) == 1);
}

static void
test_exclusive_or()
{
    tst("xor", {0xa9, 0x25, 0x49, 0x36, 0x85, 0x99});
    vfy(mem(0x99) == (0x25u ^ 0x36u));
    tst("xor", {0xa9, 0xff, 0x49, 0xff, 0x08});
    vfy(get_bit(mem(0x1ff), 1) == 1);
    tst("xor", {0xa9, 0xff, 0x49, 0x0f, 0x08});
    vfy(get_bit(mem(0x1ff), 7) == 1);
}

static void
test_inclusive_or()
{
    tst("or", {0xa9, 0x25, 0x09, 0x36, 0x85, 0x99});
    vfy(mach.read_memory(0x99) == (0x25u | 0x36u));
    tst("or", {0xa9, 0x00, 0x09, 0x00, 0x08});
    vfy(get_bit(mem(0x1ff), 1) == 1);
    tst("or", {0xa9, 0x80, 0x09, 0x00, 0x08});
    vfy(get_bit(mem(0x1ff), 7) == 1);
}

static void
test_shift()
{
    std::vector<char> prog = {
        0xa9, 0xf5,
        0x0a, 0x85, 0x00,
        0xa9, 0xf5,
        0x4a, 0x85, 0x01,
        0xa9, 0xf5,
        0x2a, 0x85, 0x02,
        0x6a, 0x85, 0x03,
        0x08
    };
    tst("shift", prog);
    auto tmp = mem(0) == 0xea && mem(1) == 0x7a && mem(2) == 0xeb && mem(3) == 0xf5;
    vfy(get_bit(mem(0x1ff), 0) == 1 && tmp);
}

static void
test_bit_test()
{
    tst("bit", {0xa9, 0x01, 0x2c, 0x00, 0x02, 0x08});
    auto tmp = mem(0x1ff);
    vfy(get_bit(tmp, 1) == 0 && get_bit(tmp, 6) == 0);
}

static void
test_load_store()
{
    std::vector<char> prog = {
        0xa9, 0x12, 0xa2, 0x34, 0xa0, 0x56,
        0x85, 0x00, 0x86, 0x01, 0x84, 0x02
    };
    tst("load store", prog);
    vfy(mem(0) == 0x12 && mem(1) == 0x34 && mem(2) == 0x56);
}

static void
test_register_transfer()
{
    std::vector<char> prog = {
        0xa9, 0x12, 0xaa, 0x0a, 0xa8, 0x0a,
        0x8a, 0x85, 0x00, 0x98, 0x85, 0x01,
        0x86, 0x02, 0x84, 0x03
    };
    tst("register transfer", prog);
    vfy(mem(0) == 0x12 && mem(1) == 0x24 && mem(2) == 0x12 && mem(3) == 0x24);
}

static void
test_increment_decrement()
{
    std::vector<char> prog = {
        0xa9, 0xb3, 0x85, 0x00,
        0xe6, 0x00, 0xe6, 0x00, 0xc6, 0x00, 0xe6, 0x00, // m $b5
        0xa2, 0xac, 0xca, 0xe8, 0xe8, 0xca, 0xca, 0x86, 0x01, // x $ab
        0xa0, 0x9d, 0xc8, 0xc8, 0xc8, 0x88, 0x84, 0x02, // y $9f
    };
    tst("inc dec", prog);
    vfy(mem(0) == 0xb5 && mem(1) == 0xab && mem(2) == 0x9f);
}

static void
test_stack()
{
    // std::vector<char> prog = {
    //     0xba, 0x86, 0x00, 0xa2, 0xf0, 0x9a, 0xe8, 0xba, 0x86, 0x01,
    //     0xa9, 0xc5, 0x48, 0x08, 0x68, 0x28, 0x85, 0x02,
    //     0x08, 0x68, 0x85, 0x03
    // };
    // tst("stack", prog);
    // auto tmp = get_bit(mem(2), 7) == 1;
    // vfy(mem(0) == 0xff && mem(1) == 0xf0 && mem(3) == 0xc5 && tmp);
}

static void
test_jump()
{
    std::vector<char> prog = {
        0x4c, 0x04, 0x02, 0xe8,
        0x20, 0x0b, 0x02, 0x20, 0x0d, 0x02, 0x86, 0x06, 0xff,
        0xe8, 0xe8, 0x60
    };
    tst("jmp", prog);
    vfy(mem(6) == 0x04);
}

static void
test_branch()
{
    std::vector<char> prog = {
        0x38, 0xb8, 0xa9, 0x80,
        0x90, 0x80, 0xb0, 0x01, 0xff, 0x10, 0x80, 0x30, 0x02,
        0xff, 0xff, 0x70, 0x80, 0x50, 0x02, 0xff, 0xff, 0xf0,
        0x80, 0xd0, 0x02, 0xff, 0xff, 0xa9, 0x37, 0x85, 0x05
    };
    tst("branch", prog);
    vfy(mem(5) == 0x37);
}

static void
test_mode()
{
    std::vector<char> prog = {
        0xa0, 0x03, 0xa9, 0x02, 0x85, 0x01, 0xa9, 0xfd, 0x85, 0x00,
        0x91, 0x00
    };
    tst("addr mode", prog);
    vfy(mem(0x0300) == 0xfd);
}

void begin_testing() {
    pass_count = 0;
    total_count = 0;
}

void end_testing() {
    std::cout << "fail : " << (total_count - pass_count);
    std::cout << " / " << total_count << "\n";
}

void modular_test() {
    test_logical_and();
    test_shift();
    test_exclusive_or();
    test_inclusive_or();
    test_bit_test();
    test_load_store();
    test_register_transfer();
    test_increment_decrement();
    test_stack();
    test_jump();
    test_branch();
    test_mode();
}

void full_test() {
    mach.init();
    mach.load_program_from_file("test.bin", 0x4000);
    while (mach.get_program_counter() != 0x45c0u) {
        mach.step();
    }
    auto answer = mach.read_memory(0x0210);
    std::cout << "answer : "; print_hex(answer); std::cout << "\n";
    if (answer == 0xff) {
        std::cout << "full test pass\n";
    } else {
        std::cout << "full test fail\n";
    }
}

void func_test() {
    mach.init();
    auto ret = mach.load_program_from_file("func_test_no_dec.bin", 0x0000);
    if (ret < 0) {
        std::cout << "load program fail\n";
        return;
    }
    mach.set_program_counter(0x0400);
    while (true) {
        auto pc = mach.get_program_counter();
        auto sc = mach.get_step_counter();
        if (pc == 0x3469ul) {
            std::cout << "success\n";
            break;
        }
        if (sc % 0x1000u == 0) {
            mach.print_info();
        }
        auto ret = mach.step();
        if (ret < 0) {
            std::cout << "bad instruction\n";
            break;
        }
    }
}

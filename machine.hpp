#pragma once

#include <array>
#include <vector>

using t_addr = unsigned long;

class t_machine
{
    t_addr arg;
    std::array<char, 0x10000> memory;

    // registers

    t_addr pc; // program counter
    char sp; // stack pointer
    char ra; // accumulator
    char rx; // register x
    char ry; // register y
    char rp; // processor status

    // addressing modes

    void m_imp();
    void m_acc();
    void m_imm();
    void m_rel();
    void m_zpg();
    void m_zpx();
    void m_zpy();
    void m_abs();
    void m_abx();
    void m_aby();
    void m_ind();
    void m_inx();
    void m_iny();

    // instructions

    void i_lda();
    void i_ldx();
    void i_ldy();

    void i_sta();
    void i_stx();
    void i_sty();

    void i_tax();
    void i_tay();
    void i_txa();
    void i_tya();
    void i_tsx();
    void i_txs();

    void i_pha();
    void i_pla();

    void i_php();
    void i_plp();

    void i_and();
    void i_eor();
    void i_ora();
    void i_bit();

    void i_inc();
    void i_dec();

    void i_inx();
    void i_dex();
    void i_iny();
    void i_dey();

    void i_jmp();
    void i_jsr();
    void i_rts();

    void i_clc();
    void i_sec();
    void i_clv();
    void i_cld();
    void i_sed();
    void i_cli();
    void i_sei();

    void i_bcc();
    void i_bcs();
    void i_bpl();
    void i_bmi();
    void i_bne();
    void i_beq();
    void i_bvc();
    void i_bvs();

    void i_brk();
    void i_rti();
    void i_nop();

    void i_asl();
    void i_lsr();
    void i_rol();
    void i_ror();

    void i_adc();
    void i_sbc();
    void i_cmp();
    void i_cpx();
    void i_cpy();

    void set_carry_flag(bool);
    bool get_carry_flag();
    void set_zero_flag(bool);
    bool get_zero_flag();
    void set_overflow_flag(bool);
    bool get_overflow_flag();
    void set_negative_flag(bool);
    bool get_negative_flag();
    void set_break_flag(bool);
    bool get_break_flag();

    char read_mem(t_addr);
    t_addr read_mem_2(t_addr);
    void write_mem(t_addr, char);
    void set_with_flags(t_addr, char);
    void set_arg(t_addr, int);
    void push(char);
    char pull();
    void push_pc();
    void pull_pc();
    void short_jump_if(bool);

public:

    t_machine();
    void reset();
    t_addr get_program_counter();
    char read_memory(t_addr);
    void load_program(const std::vector<char>&, t_addr);
    int load_program_from_file(const std::string&, t_addr);
    int step();
    void run();
};

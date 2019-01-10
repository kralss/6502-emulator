#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <functional>

#include "machine.hpp"
#include "misc.hpp"

t_addr make_addr(char hi, char lo) {
    return (t_addr(hi) << 8) | lo;
}

void add_with_carry(char& x, char y, bool& c) {
    unsigned z = x;
    z += y;
    c = (z >= 0x100u);
    x = z;
}

const t_addr addr_ra = 0x10000;
const t_addr addr_rx = 0x10001;
const t_addr addr_ry = 0x10002;
const t_addr addr_rp = 0x10003;
const t_addr addr_sp = 0x10004;

void t_machine::process_interrupt() {
    if (nmi_flag == 1) {
        nmi_flag = 0;
        push_addr(pc);
        auto val = rp;
        set_bit(val, 5, 1);
        set_bit(val, 4, 0);
        push(val);
        set_interrupt_disable_flag(1);
        pc = read_mem_2(0xfffa);
    }
    if (reset_flag == 1) {
        reset_flag = 0;
        set_interrupt_disable_flag(1);
        pc = read_mem_2(0xfffc);
    } else if (irq_flag == 1) {
        irq_flag = 0;
        push_addr(pc);
        auto val = rp;
        set_bit(val, 5, 1);
        set_bit(val, 4, 0);
        push(val);
        set_interrupt_disable_flag(1);
        pc = read_mem_2(0xfffe);
    }
}

int t_machine::step() {
    auto idf = get_interrupt_disable_flag();
    if (nmi_flag || (idf == 0 && (reset_flag || irq_flag))) {
        process_interrupt();
        step_count++;
        return 0;
    }

    // fetch an instruction
    auto opcode = read_mem(pc);
    pc++;

    // execute the given instruction
    cyc = 0;
    switch (opcode) {
    case 0x29: m_imm(); i_and(); break;
    case 0x25: m_zpg(); i_and(); break;
    case 0x35: m_zpx(); i_and(); break;
    case 0x2d: m_abs(); i_and(); break;
    case 0x3d: m_abx(); i_and(); break;
    case 0x39: m_aby(); i_and(); break;
    case 0x21: m_inx(); i_and(); break;
    case 0x31: m_iny(); i_and(); break;

    case 0x49: m_imm(); i_eor(); break;
    case 0x45: m_zpg(); i_eor(); break;
    case 0x55: m_zpx(); i_eor(); break;
    case 0x4d: m_abs(); i_eor(); break;
    case 0x5d: m_abx(); i_eor(); break;
    case 0x59: m_aby(); i_eor(); break;
    case 0x41: m_inx(); i_eor(); break;
    case 0x51: m_iny(); i_eor(); break;

    case 0x09: m_imm(); i_ora(); break;
    case 0x05: m_zpg(); i_ora(); break;
    case 0x15: m_zpx(); i_ora(); break;
    case 0x0d: m_abs(); i_ora(); break;
    case 0x1d: m_abx(); i_ora(); break;
    case 0x19: m_aby(); i_ora(); break;
    case 0x01: m_inx(); i_ora(); break;
    case 0x11: m_iny(); i_ora(); break;

    case 0x24: m_zpg(); i_bit(); break;
    case 0x2c: m_abs(); i_bit(); break;

    case 0xa9: m_imm(); i_lda(); break;
    case 0xa5: m_zpg(); i_lda(); break;
    case 0xb5: m_zpx(); i_lda(); break;
    case 0xad: m_abs(); i_lda(); break;
    case 0xbd: m_abx(); i_lda(); break;
    case 0xb9: m_aby(); i_lda(); break;
    case 0xa1: m_inx(); i_lda(); break;
    case 0xb1: m_iny(); i_lda(); break;

    case 0xa2: m_imm(); i_ldx(); break;
    case 0xa6: m_zpg(); i_ldx(); break;
    case 0xb6: m_zpy(); i_ldx(); break;
    case 0xae: m_abs(); i_ldx(); break;
    case 0xbe: m_aby(); i_ldx(); break;

    case 0xa0: m_imm(); i_ldy(); break;
    case 0xa4: m_zpg(); i_ldy(); break;
    case 0xb4: m_zpx(); i_ldy(); break;
    case 0xac: m_abs(); i_ldy(); break;
    case 0xbc: m_abx(); i_ldy(); break;

    case 0x85: m_zpg(); i_sta(); break;
    case 0x95: m_zpx(); i_sta(); break;
    case 0x8d: m_abs(); i_sta(); break;
    case 0x9d: m_abx(); i_sta(); break;
    case 0x99: m_aby(); i_sta(); break;
    case 0x81: m_inx(); i_sta(); break;
    case 0x91: m_iny(); i_sta(); break;

    case 0x86: m_zpg(); i_stx(); break;
    case 0x96: m_zpy(); i_stx(); break;
    case 0x8e: m_abs(); i_stx(); break;

    case 0x84: m_zpg(); i_sty(); break;
    case 0x94: m_zpx(); i_sty(); break;
    case 0x8c: m_abs(); i_sty(); break;

    case 0xaa: m_imp(); i_tax(); break;
    case 0xa8: m_imp(); i_tay(); break;
    case 0x8a: m_imp(); i_txa(); break;
    case 0x98: m_imp(); i_tya(); break;

    case 0xe6: m_zpg(); i_inc(); break;
    case 0xf6: m_zpx(); i_inc(); break;
    case 0xee: m_abs(); i_inc(); break;
    case 0xfe: m_abx(); i_inc(); break;
    case 0xe8: m_imp(); i_inx(); break;
    case 0xc8: m_imp(); i_iny(); break;

    case 0xc6: m_zpg(); i_dec(); break;
    case 0xd6: m_zpx(); i_dec(); break;
    case 0xce: m_abs(); i_dec(); break;
    case 0xde: m_abx(); i_dec(); break;
    case 0xca: m_imp(); i_dex(); break;
    case 0x88: m_imp(); i_dey(); break;

    case 0x0a: m_acc(); i_asl(); break;
    case 0x06: m_zpg(); i_asl(); break;
    case 0x16: m_zpx(); i_asl(); break;
    case 0x0e: m_abs(); i_asl(); break;
    case 0x1e: m_abx(); i_asl(); break;

    case 0x4a: m_acc(); i_lsr(); break;
    case 0x46: m_zpg(); i_lsr(); break;
    case 0x56: m_zpx(); i_lsr(); break;
    case 0x4e: m_abs(); i_lsr(); break;
    case 0x5e: m_abx(); i_lsr(); break;

    case 0x2a: m_acc(); i_rol(); break;
    case 0x26: m_zpg(); i_rol(); break;
    case 0x36: m_zpx(); i_rol(); break;
    case 0x2e: m_abs(); i_rol(); break;
    case 0x3e: m_abx(); i_rol(); break;

    case 0x6a: m_acc(); i_ror(); break;
    case 0x66: m_zpg(); i_ror(); break;
    case 0x76: m_zpx(); i_ror(); break;
    case 0x6e: m_abs(); i_ror(); break;
    case 0x7e: m_abx(); i_ror(); break;

    case 0xba: m_imp(); i_tsx(); break;
    case 0x9a: m_imp(); i_txs(); break;
    case 0x48: m_imp(); i_pha(); break;
    case 0x08: m_imp(); i_php(); break;
    case 0x68: m_imp(); i_pla(); break;
    case 0x28: m_imp(); i_plp(); break;

    case 0x4c: m_abs(); i_jmp(); break;
    case 0x6c: m_ind(); i_jmp(); break;
    case 0x20: m_abs(); i_jsr(); break;
    case 0x60: m_imp(); i_rts(); break;

    case 0x90: m_rel(); i_bcc(); break;
    case 0xb0: m_rel(); i_bcs(); break;
    case 0xf0: m_rel(); i_beq(); break;
    case 0x30: m_rel(); i_bmi(); break;
    case 0xd0: m_rel(); i_bne(); break;
    case 0x10: m_rel(); i_bpl(); break;
    case 0x50: m_rel(); i_bvc(); break;
    case 0x70: m_rel(); i_bvs(); break;

    case 0x18: m_imp(); i_clc(); break;
    case 0xd8: m_imp(); i_cld(); break;
    case 0x58: m_imp(); i_cli(); break;
    case 0xb8: m_imp(); i_clv(); break;
    case 0x38: m_imp(); i_sec(); break;
    case 0xf8: m_imp(); i_sed(); break;
    case 0x78: m_imp(); i_sei(); break;

    case 0x69: m_imm(); i_adc(); break;
    case 0x65: m_zpg(); i_adc(); break;
    case 0x75: m_zpx(); i_adc(); break;
    case 0x6d: m_abs(); i_adc(); break;
    case 0x7d: m_abx(); i_adc(); break;
    case 0x79: m_aby(); i_adc(); break;
    case 0x61: m_inx(); i_adc(); break;
    case 0x71: m_iny(); i_adc(); break;

    case 0xe9: m_imm(); i_sbc(); break;
    case 0xe5: m_zpg(); i_sbc(); break;
    case 0xf5: m_zpx(); i_sbc(); break;
    case 0xed: m_abs(); i_sbc(); break;
    case 0xfd: m_abx(); i_sbc(); break;
    case 0xf9: m_aby(); i_sbc(); break;
    case 0xe1: m_inx(); i_sbc(); break;
    case 0xf1: m_iny(); i_sbc(); break;

    case 0xc9: m_imm(); i_cmp(); break;
    case 0xc5: m_zpg(); i_cmp(); break;
    case 0xd5: m_zpx(); i_cmp(); break;
    case 0xcd: m_abs(); i_cmp(); break;
    case 0xdd: m_abx(); i_cmp(); break;
    case 0xd9: m_aby(); i_cmp(); break;
    case 0xc1: m_inx(); i_cmp(); break;
    case 0xd1: m_iny(); i_cmp(); break;

    case 0xe0: m_imm(); i_cpx(); break;
    case 0xe4: m_zpg(); i_cpx(); break;
    case 0xec: m_abs(); i_cpx(); break;

    case 0xc0: m_imm(); i_cpy(); break;
    case 0xc4: m_zpg(); i_cpy(); break;
    case 0xcc: m_abs(); i_cpy(); break;

    case 0xea: m_imp(); i_nop(); break;
    case 0x00: m_imp(); i_brk(); break;
    case 0x40: m_imp(); i_rti(); break;

    default:
        return -1;
    }

    step_count++;
    return 0;
}

void t_machine::m_imp() {
    rcyc = 0;
    wcyc = 0;
}

void t_machine::m_acc() {
    rcyc = 0;
    wcyc = 0;
    set_arg(addr_ra, 0);
}

void t_machine::m_imm() {
    rcyc = 0;
    wcyc = 0;
    set_arg(pc, 1);
}

void t_machine::m_rel() {
    set_arg(pc, 1);
    rcyc = 0;
    wcyc = 0;
}

void t_machine::m_zpg() {
    rcyc = 1;
    wcyc = 1;
    set_arg(read_mem(pc), 1);
}

void t_machine::m_zpx() {
    rcyc = 2;
    wcyc = 2;
    set_arg(char(read_mem(pc) + rx), 1);
}

void t_machine::m_zpy() {
    rcyc = 2;
    wcyc = 2;
    set_arg(char(read_mem(pc) + ry), 1);
}

void t_machine::m_abs() {
    rcyc = 2;
    wcyc = 2;
    set_arg(read_mem_2(pc), 2);
}

void t_machine::m_abx() {
    auto lo = read_mem(pc);
    auto hi = read_mem(pc + 1);
    bool carry;
    add_with_carry(lo, rx, carry);
    hi += carry;
    set_arg(make_addr(hi, lo), 2);
    rcyc = 2 + carry;
    wcyc = 3;
}

void t_machine::m_aby() {
    auto lo = read_mem(pc);
    auto hi = read_mem(pc + 1);
    bool carry;
    add_with_carry(lo, ry, carry);
    hi += carry;
    set_arg(make_addr(hi, lo), 2);
    rcyc = 2 + carry;
    wcyc = 3;
}

void t_machine::m_ind() {
    set_arg(read_mem_2(read_mem_2(pc)), 2);
    rcyc = 4;
}

void t_machine::m_inx() {
    rcyc = 4;
    wcyc = 4;
    set_arg(read_mem_2(char(read_mem(pc) + rx)), 1);
}

void t_machine::m_iny() {
    auto addr = read_mem(pc);
    auto lo = read_mem(addr);
    addr++;
    auto hi = read_mem(addr);
    bool carry;
    add_with_carry(lo, ry, carry);
    hi += carry;
    set_arg(make_addr(hi, lo), 1);
    rcyc = 3 + carry;
    wcyc = 4;
}

void t_machine::i_lda() {
    set_with_flags(addr_ra, read_mem(arg));
    cyc += 2 + rcyc;
}

void t_machine::i_ldx() {
    set_with_flags(addr_rx, read_mem(arg));
    cyc += 2 + rcyc;
}

void t_machine::i_ldy() {
    set_with_flags(addr_ry, read_mem(arg));
    cyc += 2 + rcyc;
}

void t_machine::i_sta() {
    write_mem(arg, ra);
    cyc += 2 + wcyc;
}

void t_machine::i_stx() {
    write_mem(arg, rx);
    cyc += 2 + rcyc;
}

void t_machine::i_sty() {
    write_mem(arg, ry);
    cyc += 2 + rcyc;
}

void t_machine::i_tax() {
    set_with_flags(addr_rx, ra);
    cyc += 2;
}

void t_machine::i_tay() {
    set_with_flags(addr_ry, ra);
    cyc += 2;
}

void t_machine::i_txa() {
    set_with_flags(addr_ra, rx);
    cyc += 2;
}

void t_machine::i_tya() {
    set_with_flags(addr_ra, ry);
    cyc += 2;
}

void t_machine::i_tsx() {
    set_with_flags(addr_rx, sp);
    cyc += 2;
}

void t_machine::i_txs() {
    sp = rx;
    cyc += 2;
}

void t_machine::i_pha() {
    push(ra);
    cyc += 3;
}

void t_machine::i_pla() {
    set_with_flags(addr_ra, pull());
    cyc += 4;
}

void t_machine::i_php() {
    auto val = rp;
    set_bit(val, 5, 1);
    set_bit(val, 4, 1);
    push(val);
    cyc += 3;
}

void t_machine::i_plp() {
    rp = pull();
    cyc += 4;
}

void t_machine::i_and() {
    set_with_flags(addr_ra, ra & read_mem(arg));
    cyc += 2 + rcyc;
}

void t_machine::i_eor() {
    set_with_flags(addr_ra, ra ^ read_mem(arg));
    cyc += 2 + rcyc;
}

void t_machine::i_ora() {
    set_with_flags(addr_ra, ra | read_mem(arg));
    cyc += 2 + rcyc;
}

void t_machine::i_bit() {
    auto val = read_mem(arg);
    set_zero_flag((ra & val) == 0);
    set_overflow_flag(get_bit(val, 6));
    set_negative_flag(get_bit(val, 7));
    cyc += 2 + rcyc;
};

void t_machine::i_inc() {
    set_with_flags(arg, read_mem(arg) + 1);
    cyc += 4 + wcyc;
}

void t_machine::i_dec() {
    set_with_flags(arg, read_mem(arg) - 1);
    cyc += 4 + wcyc;
}

void t_machine::i_inx() {
    set_with_flags(addr_rx, rx + 1);
    cyc += 2;
}

void t_machine::i_dex() {
    set_with_flags(addr_rx, rx - 1);
    cyc += 2;
}

void t_machine::i_iny() {
    set_with_flags(addr_ry, ry + 1);
    cyc += 2;
}

void t_machine::i_dey() {
    set_with_flags(addr_ry, ry - 1);
    cyc += 2;
}

void t_machine::i_jmp() {
    pc = arg;
    cyc += 1 + rcyc;
}

void t_machine::i_jsr() {
    push_addr(pc - 1);
    pc = arg;
    cyc += 4 + rcyc;
}

void t_machine::i_rts() {
    pc = pull_addr() + 1;
    cyc += 6;
}

void t_machine::i_clc() {
    set_carry_flag(0);
    cyc += 2;
}

void t_machine::i_sec() {
    set_carry_flag(1);
    cyc += 2;
}

void t_machine::i_clv() {
    set_overflow_flag(0);
    cyc += 2;
}

void t_machine::i_cld() {
    set_bit(rp, 3, 0);
    cyc += 2;
}

void t_machine::i_sed() {
    set_bit(rp, 3, 1);
    cyc += 2;
}

void t_machine::i_cli() {
    set_bit(rp, 2, 0);
    cyc += 2;
}

void t_machine::i_sei() {
    set_bit(rp, 2, 1);
    cyc += 2;
}

void t_machine::i_bcc() {
    short_jump_if(get_carry_flag() == 0);
}

void t_machine::i_bcs() {
    short_jump_if(get_carry_flag() == 1);
}

void t_machine::i_bpl() {
    short_jump_if(get_negative_flag() == 0);
}

void t_machine::i_bmi() {
    short_jump_if(get_negative_flag() == 1);
}

void t_machine::i_bne() {
    short_jump_if(get_zero_flag() == 0);
};

void t_machine::i_beq() {
    short_jump_if(get_zero_flag() == 1);
}

void t_machine::i_bvc() {
    short_jump_if(get_overflow_flag() == 0);
}

void t_machine::i_bvs() {
    short_jump_if(get_overflow_flag() == 1);
}

void t_machine::i_brk() {
    push_addr(pc + 1);
    auto val = rp;
    set_bit(val, 5, 1);
    set_bit(val, 4, 1);
    push(val);
    pc = read_mem_2(0xfffe);
    set_break_flag(1);
    set_interrupt_disable_flag(1);
    cyc += 7;
}

void t_machine::i_rti() {
    rp = pull();
    pc = pull_addr();
    cyc += 6;
}

void t_machine::i_nop() {
    cyc += 2;
}

void t_machine::i_asl() {
    auto val = read_mem(arg);
    set_carry_flag(get_bit(val, 7));
    set_with_flags(arg, val << 1);
    cyc += (arg == addr_ra) ? 2 : (4 + wcyc);
}

void t_machine::i_lsr() {
    auto val = read_mem(arg);
    set_carry_flag(get_bit(val, 0));
    set_with_flags(arg, val >> 1);
    cyc += (arg == addr_ra) ? 2 : (4 + wcyc);
}

void t_machine::i_rol() {
    auto val = read_mem(arg);
    auto ca = get_carry_flag();
    set_carry_flag(get_bit(val, 7));
    val <<= 1;
    set_bit(val, 0, ca);
    set_with_flags(arg, val);
    cyc += (arg == addr_ra) ? 2 : (4 + wcyc);
}

void t_machine::i_ror() {
    auto val = read_mem(arg);
    auto ca = get_carry_flag();
    set_carry_flag(get_bit(val, 0));
    val >>= 1;
    set_bit(val, 7, ca);
    set_with_flags(arg, val);
    cyc += (arg == addr_ra) ? 2 : (4 + wcyc);
}

void t_machine::i_adc() {
    unsigned res = ra;
    unsigned v = read_mem(arg);
    auto ca = get_carry_flag();
    v += ca;
    auto a7 = get_bit(ra, 7);
    auto b7 = get_bit(v, 7);
    res += v;
    set_with_flags(addr_ra, res);
    auto c7 = get_bit(ra, 7);
    if (ca == 1 && v == 0x80u) {
        set_overflow_flag(a7 == 0);
    } else {
        set_overflow_flag(a7 == b7 && a7 != c7);
    }
    set_carry_flag(res >= 0x100u);
    cyc += 2 + rcyc;
}

void t_machine::i_sbc() {
    unsigned res = ra;
    unsigned xx = read_mem(arg);
    auto nc = !get_carry_flag();
    xx += nc;
    auto a7 = get_bit(ra, 7);
    auto b7 = get_bit(xx, 7);
    res -= xx;
    set_with_flags(addr_ra, res);
    auto c7 = get_bit(ra, 7);
    if (nc == 1 && xx == 0x80u) {
        set_overflow_flag(a7 == 1);
    } else {
        set_overflow_flag(a7 != b7 && b7 == c7);
    }
    set_carry_flag(res < 0x100);
    cyc += 2 + rcyc;
}

void t_machine::i_cmp() {
    auto val = read_mem(arg);
    set_carry_flag(ra >= val);
    set_zero_flag(ra == val);
    set_negative_flag(get_bit(ra - val, 7));
    cyc += 2 + rcyc;
}

void t_machine::i_cpx() {
    auto val = read_mem(arg);
    set_carry_flag(rx >= val);
    set_zero_flag(rx == val);
    set_negative_flag(get_bit(rx - val, 7));
    cyc += 2 + rcyc;
}

void t_machine::i_cpy() {
    auto val = read_mem(arg);
    set_carry_flag(ry >= val);
    set_zero_flag(ry == val);
    set_negative_flag(get_bit(ry - val, 7));
    cyc += 2 + rcyc;
}

void t_machine::run() {
    while (true) {
        auto ret = step();
        if (ret < 0) {
            break;
        }
    }
}

char t_machine::read_mem(t_addr addr) {
    if (addr == addr_ra) {
        return ra;
    } else if (addr == addr_rx) {
        return rx;
    } else if (addr == addr_ry) {
        return ry;
    } else if (addr == addr_rp) {
        return rp;
    } else if (addr == addr_sp) {
        return sp;
    } else {
        return memory[addr];
    }
}

void t_machine::write_mem(t_addr addr, char val) {
    if (addr == addr_ra) {
        ra = val;
    } else if (addr == addr_rx) {
        rx = val;
    } else if (addr == addr_ry) {
        ry = val;
    } else if (addr == addr_rp) {
        rp = val;
    } else if (addr == addr_sp) {
        sp = val;
    } else {
        memory[addr] = val;
    }
}

t_addr t_machine::read_mem_2(t_addr addr) {
    auto v = read_mem(addr);
    auto u = read_mem(addr + 1);
    return make_addr(u, v);
}

void t_machine::set_with_flags(t_addr addr, char v) {
    write_mem(addr, v);
    set_zero_flag(v == 0);
    set_negative_flag(get_bit(v, 7));
}

void t_machine::push(char val) {
    // std::cout << "push "; print_hex(val); std::cout << "\n";
    write_mem(0x100u + sp, val);
    sp--;
}

char t_machine::pull() {
    sp++;
    return read_mem(0x100u + sp);
}

void t_machine::push_addr(t_addr addr) {
    push(char(addr >> 8));
    push(char(addr));
}

t_addr t_machine::pull_addr() {
    auto v = pull();
    auto u = pull();
    return make_addr(u, v);
}

void t_machine::short_jump_if(bool cond) {
    cyc += 2;
    if (cond) {
        cyc++;
        auto offset = read_mem(arg);
        char old_page = pc >> 8;
        if (offset < 0x80u) {
            pc += offset;
        } else {
            offset = ~offset;
            pc -= offset + 1u;
        }
        char new_page = pc >> 8;
        if (new_page != old_page) {
            cyc++;
        }
    }
}

void t_machine::set_arg(t_addr addr, int n) {
    arg = addr;
    pc += n;
}

void t_machine::set_carry_flag(bool x) {
    set_bit(rp, 0, x);
}

bool t_machine::get_carry_flag() {
    return get_bit(rp, 0);
}

void t_machine::set_zero_flag(bool x) {
    set_bit(rp, 1, x);
}

bool t_machine::get_zero_flag() {
    return get_bit(rp, 1);
}

void t_machine::set_interrupt_disable_flag(bool x) {
    set_bit(rp, 2, x);
}

bool t_machine::get_interrupt_disable_flag() {
    return get_bit(rp, 2);
}

void t_machine::set_overflow_flag(bool x) {
    set_bit(rp, 6, x);
}

bool t_machine::get_overflow_flag() {
    return get_bit(rp, 6);
}

void t_machine::set_negative_flag(bool x) {
    set_bit(rp, 7, x);
}

bool t_machine::get_negative_flag() {
    return get_bit(rp, 7);
}

void t_machine::set_break_flag(bool x) {
    set_bit(rp, 4, x);
}

bool t_machine::get_break_flag() {
    return get_bit(rp, 4);
}

t_addr t_machine::get_program_counter() {
    return pc;
}

void t_machine::set_program_counter(t_addr addr) {
    pc = addr;
}

int t_machine::load_program_from_file(const std::string& file, t_addr addr) {
    std::ifstream input(file, std::ios::binary);
    if (!input.good()) {
        return -1;
    }
    pc = addr;
    input.read(&memory[pc], memory.size() - pc);
    return 0;
}

void t_machine::load_program(const std::vector<char>& v, t_addr addr) {
    pc = addr;
    std::copy(v.begin(), v.end(), memory.begin() + pc);
}

char t_machine::read_memory(t_addr addr) {
    return read_mem(addr);
}

void t_machine::print_info() {
    std::cout << "| a : "; print_hex(ra);
    std::cout << " | x : "; print_hex(rx);
    std::cout << " | y : "; print_hex(ry);
    std::cout << " | sp : "; print_hex(sp);
    std::cout << " | pc : "; print_hex(pc);
    std::cout << " | p : "; print_hex(rp);
    std::cout << " | sc : "; print_hex(step_count);
    std::cout << " |\n";
}

unsigned long t_machine::get_step_counter() {
    return step_count;
}

void t_machine::init() {
    pc = 0x0200;
    sp = 0xff;
    ra = 0x00;
    rx = 0x00;
    ry = 0x00;
    rp = 0x24;
    std::fill(memory.begin(), memory.end(), 0xff);
    nmi_flag = 0;
    irq_flag = 0;
    reset_flag = 0;
    step_count = 0;
}

t_machine::t_machine() {
    init();
}

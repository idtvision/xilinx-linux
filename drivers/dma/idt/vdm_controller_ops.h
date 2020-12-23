#pragma once
// VDM controller instructions
//
// A 32-bit instruction is structured as 4 8-bit fields
// logic[7:0] op,arg0,arg1,arg2
// {op, arg0, arg1, arg2}
//
// op 0 -- No operation
// TODO: need to fix noop in the FSM, right now it just does into the IDLE state
//
static const unsigned op_noop = 0x00000000;
inline unsigned noop(void) { return op_noop; }
// op 1 -- add immediate
// arg0 -- mem_ptr
// arg1, arg2 -- {arg1,arg2} value
// result -- *mem_ptr += value
static const unsigned op_add_imm = 0x01000000;
inline unsigned add_imm(unsigned char mem_ptr, unsigned short value) {
    return op_add_imm | (mem_ptr << 16) | value;
}
// op 2 -- branch immediate
// arg0 -- ignored
// arg1, arg2 -- {arg1, arg2} addr
// result -- IP = addr (next instruction pointer set to addr)
static const unsigned op_br_imm = 0x02000000;
inline unsigned br_imm(unsigned short addr) {
    return op_br_imm | addr;
}
// op 3 -- add memory location
// arg0 -- mem_ptr
// arg1 -- src_ptr
// result -- *mem_ptr += *src_ptr
static const unsigned op_add_mem = 0x03000000;
inline unsigned add_mem(unsigned char mem_ptr, unsigned char src_ptr) {
    return op_add_mem | (mem_ptr << 16) | (src_ptr << 8);
}
// op 4 -- branch if less than
// arg0 -- lhs_ptr
// arg1 -- rhs_ptr
// arg2 -- addr
// result -- if (*lhs_ptr < *rhs_ptr) IP = addr
static const unsigned op_brl = 0x04000000;
inline unsigned brl(unsigned char lhs_ptr, unsigned char rhs_ptr, unsigned char addr) {
    return op_brl | (lhs_ptr << 16) | (rhs_ptr << 8) | addr;
}
// op 5,6,7,8 -- issue data mover command
// arg0 -- addr_ptr
// arg1 -- size_ptr
// result -- initiate data move from *addr_ptr of size *size_ptr
static const unsigned op_dma_0 = 0x05000000;
static const unsigned op_dma_1 = 0x06000000;
static const unsigned op_dma_2 = 0x07000000;
static const unsigned op_dma_3 = 0x08000000;
inline unsigned dma(unsigned which, unsigned char addr_ptr, unsigned char size_ptr) {
    switch (which) {
        case 0: return op_dma_0 | (addr_ptr << 16) | (size_ptr << 8);
        case 1: return op_dma_1 | (addr_ptr << 16) | (size_ptr << 8);
        case 2: return op_dma_2 | (addr_ptr << 16) | (size_ptr << 8);
        case 3: return op_dma_3 | (addr_ptr << 16) | (size_ptr << 8);
        default: return noop();
    }
}
// op 9 -- output value to verilog port out0
// arg0 -- addr_ptr
// result -- out0 = *addr_ptr
static const unsigned op_out0 = 0x09000000;
inline unsigned out0(unsigned addr_ptr) {
    return op_out0 | (addr_ptr << 16);
}
// op 10 -- input value from verilog port in0
// arg0 -- addr_ptr
// result -- *addr_ptr = in0
static const unsigned op_in0 = 0x0a000000;
inline unsigned in0(unsigned addr_ptr) {
    return op_in0 | (addr_ptr << 16);
}
// op 11 -- zero memory
// arg0 -- addr_ptr
// result -- *addr_ptr = 0
static const unsigned op_zero = 0x0b000000;
inline unsigned zero(unsigned addr_ptr) {
    return op_zero | (addr_ptr << 16);
}
// op 12 -- assign immediate
// arg0 -- mem_ptr
// arg1, arg2 -- {arg1,arg2} value
// result -- *mem_ptr = value
static const unsigned op_assign_imm = 0x0c000000;
inline unsigned assign_imm(unsigned char mem_ptr, unsigned short value) {
    return op_assign_imm | (mem_ptr << 16) | value;
}
// TODO: not this op is broken, waiting on rebuild
// op 13 -- assign from memory
// arg0 -- mem_ptr
// arg1 -- src_ptr
// result -- *mem_ptr = *src_ptr
static const unsigned op_assign_mem = 0x0d000000;
inline unsigned assign_mem(unsigned char mem_ptr, unsigned char src_ptr) {
    return op_assign_mem | (mem_ptr << 16) | (src_ptr << 8);
}
// TODO: add branch if less with immediate to support counters!
// This hangs the FSM
static const unsigned op_bad = 0xff000000;
inline unsigned hang(void){ return op_bad; }

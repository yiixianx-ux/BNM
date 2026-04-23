#include "BNM/AssemblerUtils.hpp"
#include "BNM/DebugMessages.hpp"
#include <cstdio>

namespace BNM {

namespace AssemblerUtils {

#if defined(__ARM_ARCH_7A__)

    // Check if the assembly is `bl ...` or `b ...`
    static bool IsBranch(BNM_PTR address) {
        uint32_t insn = *(uint32_t*)address;
        return (insn & 0x0A000000) == 0x0A000000;
    }

#elif defined(__aarch64__)

    enum class Arm64InsnType {
        UNKNOWN,
        B,
        BL,
        BR,
        BLR,
        ADRP,
        ADD_IMM,
        LDR_LIT,
        RET
    };

    struct Arm64Insn {
        Arm64InsnType type = Arm64InsnType::UNKNOWN;
        uint32_t rd = 0;
        uint32_t rn = 0;
        int64_t imm = 0;
        BNM_PTR address = 0;
    };

    static Arm64Insn DecodeArm64Insn(BNM_PTR address) {
        uint32_t insn = *(uint32_t*)address;
        Arm64Insn out;
        out.address = address;

        // B: 0001 01imm26
        if ((insn & 0xFC000000) == 0x14000000) {
            out.type = Arm64InsnType::B;
            out.imm = (int64_t)((int32_t)((insn & 0x03FFFFFF) << 6) >> 4);
        }
        // BL: 1001 01imm26
        else if ((insn & 0xFC000000) == 0x94000000) {
            out.type = Arm64InsnType::BL;
            out.imm = (int64_t)((int32_t)((insn & 0x03FFFFFF) << 6) >> 4);
        }
        // BR: 1101 0110 0001 1111 0000 00rn 0000 0000
        else if ((insn & 0xFFFFFC1F) == 0xD61F0000) {
            out.type = Arm64InsnType::BR;
            out.rn = (insn >> 5) & 0x1F;
        }
        // BLR: 1101 0110 0011 1111 0000 00rn 0000 0000
        else if ((insn & 0xFFFFFC1F) == 0xD63F0000) {
            out.type = Arm64InsnType::BLR;
            out.rn = (insn >> 5) & 0x1F;
        }
        // ADRP: 1 immlo:2 10000 immhi:19 rd:5
        else if ((insn & 0x9F000000) == 0x90000000) {
            out.type = Arm64InsnType::ADRP;
            int64_t imm = ((insn >> 29) & 0x03) | (((insn >> 5) & 0x7FFFF) << 2);
            if (insn & 0x800000) imm |= ~0x1FFFFF; // Sign extend
            out.imm = imm << 12;
            out.rd = insn & 0x1F;
        }
        // LDR (literal): 01 011 0 00 imm19 rd:5 (64-bit)
        else if ((insn & 0xFF000000) == 0x58000000) {
            out.type = Arm64InsnType::LDR_LIT;
            out.rd = insn & 0x1F;
            int64_t imm = (int32_t)((insn & 0x00FFFFE0) << 8) >> 13;
            out.imm = imm << 2;
        }
        // ADD (immediate): sf:1 0010001 sh:1 imm12:12 rn:5 rd:5
        else if ((insn & 0xFF000000) == 0x91000000) {
            out.type = Arm64InsnType::ADD_IMM;
            out.rd = insn & 0x1F;
            out.rn = (insn >> 5) & 0x1F;
            out.imm = (insn >> 10) & 0xFFF;
            if (insn & (1 << 22)) out.imm <<= 12; // Shifted
        }
        // RET: 1101 0110 0101 1111 0000 00rn 0000 0000
        else if ((insn & 0xFFFFFC1F) == 0xD65F0000) {
            out.type = Arm64InsnType::RET;
            out.rn = (insn >> 5) & 0x1F;
        }

        return out;
    }

    // Check if the assembly is `bl ...` or `b ...`
    static bool IsBranch(BNM_PTR address) {
        auto insn = DecodeArm64Insn(address);
        return insn.type == Arm64InsnType::B || insn.type == Arm64InsnType::BL;
    }

#elif defined(__i386__) || defined(__x86_64__)

    // Check if the assembly is `call ...`
    static bool IsCall(BNM_PTR address) { return *(uint8_t*)address == 0xE8; }
#endif

    // Decode b or bl and get the address it goes to
    static bool DecodeBranchOrCall(BNM_PTR address, BNM_PTR &outOffset) {
#if defined(__ARM_ARCH_7A__)
        if (!IsBranch(address)) return false;
        uint32_t insn = *(uint32_t*)address;
        uint8_t add = 8;
        int32_t offset = (int32_t)((insn & 0x00FFFFFF) << 8) >> 6;
        outOffset = address + offset + add;
#elif defined(__aarch64__)
        auto insn = DecodeArm64Insn(address);
        if (insn.type == Arm64InsnType::B || insn.type == Arm64InsnType::BL) {
            outOffset = address + insn.imm;
            return true;
        }

        if (insn.type == Arm64InsnType::BR || insn.type == Arm64InsnType::BLR) {
            uint32_t targetReg = insn.rn;
            BNM_PTR adrpAddr = 0;
            BNM_PTR addAddr = 0;
            BNM_PTR ldrAddr = 0;
            int64_t adrpImm = 0;
            int64_t addImm = 0;
            int64_t ldrImm = 0;

            // Scan backward up to 10 instructions to find ADRP and ADD or LDR
            for (int i = 1; i <= 10; ++i) {
                auto prevInsn = DecodeArm64Insn(address - i * 4);
                if (prevInsn.rd != targetReg) continue;

                if (prevInsn.type == Arm64InsnType::ADRP) {
                    adrpAddr = address - i * 4;
                    adrpImm = prevInsn.imm;
                } else if (prevInsn.type == Arm64InsnType::ADD_IMM && prevInsn.rn == targetReg) {
                    addAddr = address - i * 4;
                    addImm = prevInsn.imm;
                } else if (prevInsn.type == Arm64InsnType::LDR_LIT) {
                    ldrAddr = address - i * 4;
                    ldrImm = prevInsn.imm;
                }

                if ((adrpAddr && addAddr) || ldrAddr) break;
            }

            if (ldrAddr) {
                outOffset = *(BNM_PTR*)(ldrAddr + ldrImm);
                return true;
            } else if (adrpAddr && addAddr) {
                // ADRP computes (PC & ~0xFFF) + offset
                BNM_PTR base = (adrpAddr & ~0xFFFULL) + adrpImm;
                outOffset = base + addImm;
                return true;
            }
        }
        return false;
#elif defined(__i386__) || defined(__x86_64__)
        if (!IsCall(address)) return false;
        // Address + address from the `call` + size of the instruction
        outOffset = address + *(int32_t*)(address + 1) + 5;
#else
#error "BNM only supports arm64, arm, x86 and x86_64"
        return false;
#endif
        return true;
    }

    void DiagnosticDump(BNM_PTR address, size_t size) {
        if (!address) return;
        char buf[size * 3 + 1];
        for (size_t i = 0; i < size; ++i) {
            sprintf(buf + i * 3, "%02X ", *(uint8_t*)(address + i));
        }
        BNM_LOG_DEBUG("BNM: Diagnostic dump at %p: %s", (void*)address, buf);
    }

    BNM_PTR FindNextJump(BNM_PTR start, uint8_t index) {
        BNM_PTR outOffset = 0;
        BNM_PTR currentPos = start;

        for (uint8_t i = 0; i < index; ++i) {
            bool found = false;
            // Scan up to 200 bytes for the next jump
            for (int j = 0; j < 200; j += (sizeof(BNM_PTR) == 8 ? 4 : 1)) {
                if (DecodeBranchOrCall(currentPos + j, outOffset)) {
                    currentPos = outOffset;
                    found = true;
                    break;
                }
            }
            if (!found) {
                BNM_LOG_WARN("BNM: Failed to find jump #%d starting from %p", i + 1, (void*)currentPos);
                DiagnosticDump(currentPos, 32);
                return 0;
            }
        }
        return currentPos;
    }

} // namespace AssemblerUtils

} // namespace BNM

//
//  opernames.c
//  aodec
//
//  Created by zlot on 05/02/2020.
//  Copyright © 2020 zlot. All rights reserved.
//

#include "alis.h"
#include "alis_private.h"
#include "utils.h"

#include "experimental.h"

u16 loctp_common(u16 offset) {
    debug(EDebugWarning, " /* STUBBED */");
    return offset;
}

u16 locti_common(u16 offset) {
    debug(EDebugWarning, " /* STUBBED */");
    return offset;
}

u16 loctc_common(u16 offset) {
    
    s16 d1w = vram_read8ext16(offset - 1);
    offset += alis.varD7;
    while(--d1w > 0) {
        u16 d2 = *(alis.acc++);
        u16 mul = vram_read16(offset - 2);
        d2 *= mul;
        offset += d2;
    }
    
    return offset;
}


// =============================================================================
#pragma mark - TODO: opernames
// =============================================================================

void oimmb() {
    // reads a byte, extends into r7
    alis.varD7 = script_read8ext16();
}

void oimmw() {
    // reads a word into r7
    alis.varD7 = script_read16();
}

void oimmp() {
    // reads null-terminated data into bssChunk3
    script_read_until_zero(alis.bsd6);
}

void olocb() {
    // read word offset, copy extended byte from ram[offset] into r7
    u16 offset = script_read16();
    alis.varD7 = vram_read8ext16(offset);
}

void olocw() {
    // read word offset, copy word from ram[offset] into r7
    u16 offset = script_read16();
    alis.varD7 = vram_read16(offset);
}

void olocp() {
    u16 offset = script_read16();
    vram_writep(offset, alis.bsd7bis);
}

void oloctp() {
    debug(EDebugWarning, " /* STUBBED */");
}

void oloctc() {
    u16 offset = script_read16();
    u16 ret = loctc_common(offset);
    alis.varD7 = vram_read8(ret);
}

void olocti() {
    u16 offset = script_read16();
    s16 result = tabint(offset);
    alis.varD7 = read16(vram_ptr(result), alis.platform.is_little_endian);
}

// reads a byte offset from script,
// then reads an extended byte from vram[offset] into r7
void odirb(/* u8 offset */) {
    u8 offset = script_read8();
    alis.varD7 = vram_read8ext16(offset);
}

// reads a byte offset from script,
// then reads a word from vram[offset] into r7
void odirw(/* u8 offset */) {
    u8 offset = script_read8();
    alis.varD7 = vram_read16(offset);
    // printf("\nXXodirw: 0x%.6x > 0x%.2x\n", (u16)alis.varD7, offset);
}

// reads a byte offset from script,
// then reads a null-terminated data stream from vram[offset] into bssChunk3
void odirp(/* u8 offset */) {
    u8 offset = script_read8();
    vram_readp(offset, alis.bsd7bis);
}

void odirtp() {
    debug(EDebugWarning, " /* MISSING */");
}

void odirtc() {
    debug(EDebugWarning, " /* MISSING */");
}

void odirti() {
    u8 offset = script_read8();
    s16 result = tabint(offset);
    alis.varD7 = read16(vram_ptr(result), alis.platform.is_little_endian);
}

void omainb() {
    debug(EDebugWarning, " /* CHECK */");
    u16 offset = script_read16();
    alis.varD7 = *(u8 *)(alis.mem + alis.basemain + offset);
}

void omainw() {
    debug(EDebugWarning, " /* CHECK */");
    u16 offset = script_read16();
    alis.varD7 = *(u16 *)(alis.mem + alis.basemain + offset);
}

void omainp() {
    debug(EDebugWarning, " /* MISSING */");
}

void omaintp() {
    debug(EDebugWarning, " /* MISSING */");
}

void omaintc() {
    debug(EDebugWarning, " /* MISSING */");
}

void omainti() {
    debug(EDebugWarning, " /* MISSING */");
}

void ohimb() {
    debug(EDebugWarning, " /* CHECK */");
    u16 offset = script_read16(); // 51ce
    u16 vram_idx = vram_read16(offset); // 6
    u32 vram_offset = alis.script_vram_orgs[vram_idx / sizeof(sScriptLoc)].vram_offset;
    u32 vram_addr = alis.scripts[vram_offset]->vram_org;

    u16 index = script_read16(); // 0d
    alis.varD7 = *(u8 *)(alis.mem + vram_addr + index);
}

void ohimw() {
    debug(EDebugWarning, " /* MISSING */");
}

void ohimp() {
    debug(EDebugWarning, " /* MISSING */");
}

void ohimtp() {
    debug(EDebugWarning, " /* MISSING */");
}

void ohimtc() {
    debug(EDebugWarning, " /* MISSING */");
}

void ohimti() {
    debug(EDebugWarning, " /* MISSING */");
}

// pop from accumulator into r6
void opile() {
    // save r6 into r7
    alis.varD7 = alis.varD6;
    alis.varD6 = *(alis.acc++);
}

// start eval loop, will stop after ofin() is called
void oeval() {
    alis.oeval_loop = 1;
    while(alis.oeval_loop) {
        readexec_opername();
    }
}

// stop eval loop
void ofin() {
    alis.oeval_loop = 0;
}

// push value from r7 register to accumulator
void opushacc() {
    *(--alis.acc) = alis.varD7;
}

// r7 = variable AND r7
void oand() {
    alis.varD6 = alis.varD7;
    readexec_opername();
    alis.varD7 &= alis.varD6;
}

// r7 = variable OR r7
void oor() {
    alis.varD6 = alis.varD7;
    readexec_opername();
    alis.varD7 |= alis.varD6;
}

// r7 = variable XOR r7
void oxor() {
    alis.varD6 = alis.varD7;
    readexec_opername();
    alis.varD7 ^= alis.varD6;
}

// r7 = variable EQV r7
void oeqv() {
    alis.varD6 = alis.varD7;
    readexec_opername();
    alis.varD7 ^= alis.varD6;
    alis.varD7 = ~alis.varD7;
}

// r6 == r7
void oegal() {
    alis.varD6 = alis.varD7;
    readexec_opername();
    alis.varD7 = (alis.varD6 == alis.varD7) ? 0xff : 0x0;
}

// r6 != r7
void odiff() {
    alis.varD6 = alis.varD7;
    readexec_opername();
    alis.varD7 = (alis.varD6 != alis.varD7) ? 0xff : 0x0;
}

// r6 <= r7
void oinfeg() {
    alis.varD6 = alis.varD7;
    readexec_opername();
    alis.varD7 = (alis.varD6 <= alis.varD7) ? 0xff : 0x0;
}

// r6 >= r7
void osupeg() {
    alis.varD6 = alis.varD7;
    readexec_opername();
    alis.varD7 = (alis.varD6 >= alis.varD7) ? 0xff : 0x0;
}

// r6 < r7
void oinf() {
    alis.varD6 = alis.varD7;
    readexec_opername();
    alis.varD7 = (alis.varD6 < alis.varD7) ? 0xff : 0x0;
}

// r6 > r7
void osup() {
    alis.varD6 = alis.varD7;
    readexec_opername();
    alis.varD7 = (alis.varD6 > alis.varD7) ? 0xff : 0x0;
}

// r7 += variable
void oadd() {
//00017932 61 00 fc 38     bsr.w      FUN_READEXEC_OPERNAME_SAVE_D7                    undefined FUN_READEXEC_OPERNAME_
//00017936 de 46           add.w      D6w,D7w
//00017938 4e 75           rts
    readexec_opername_saveD7();
    alis.varD7 += alis.varD6;
}

// r7 -= variable
void osub() {
//0001793a 61 00 fc 30     bsr.w      FUN_READEXEC_OPERNAME_SAVE_D7                    undefined FUN_READEXEC_OPERNAME_
//0001793e 9c 47           sub.w      D7w,D6w
//00017940 3e 06           move.w     D6w,D7w
//00017942 4e 75           rts
    readexec_opername_saveD7();
    alis.varD6 -= alis.varD7;
    alis.varD7 = alis.varD6;
}

// r7 %= variable
void omod() {
//00017944 61 00 fc 26     bsr.w      FUN_READEXEC_OPERNAME_SAVE_D7                    undefined FUN_READEXEC_OPERNAME_
//00017948 48 c6           ext.l      D6
//0001794a 8d c7           divs.w     D7w,D6
//0001794c 2e 06           move.l     D6,D7
//0001794e 48 47           swap       D7
//00017950 4e 75           rts
    readexec_opername_saveD7();
    alis.varD6 %= alis.varD7;
    alis.varD7 = alis.varD6;
}

// r7 /= variable
void odiv() {
//00017952 61 00 fc 18     bsr.w      FUN_READEXEC_OPERNAME_SAVE_D7                    undefined FUN_READEXEC_OPERNAME_
//00017956 48 c6           ext.l      D6
//00017958 8d c7           divs.w     D7w,D6
//0001795a 3e 06           move.w     D6w,D7w
//0001795c 4e 75           rts
    readexec_opername_saveD7();
    alis.varD6 /= alis.varD7;
    alis.varD7 = alis.varD6;
}

// r7 *= variable
void omul() {
//0001795e 61 00 fc 0c     bsr.w      FUN_READEXEC_OPERNAME_SAVE_D7                    undefined FUN_READEXEC_OPERNAME_
//00017962 cf c6           muls.w     D6w,D7
//00017964 4e 75           rts
    readexec_opername_saveD7();
    alis.varD7 *= alis.varD6;
}


void oneg() {
    alis.varD7 = -alis.varD7;
}


void oabs() {
    if(alis.varD7 < 0) {
        alis.varD7 = -alis.varD7;
    }
}

void ornd() {
    alis.varD7 = sys_random() % alis.varD7;
}

void osgn() {
    if(alis.varD7 > 0) {
        alis.varD7 = 1;
    }
    else if(alis.varD7 < 0) {
        alis.varD7 = 0xffff;
    }
}

void onot() {
    alis.varD7 = ~alis.varD7;
}

void oinkey() {
    if (alis.automode == 0)
    {
        alis.varD7 = io_inkey();
    }
}

void okeyon() {
    debug(EDebugWarning, " /* MISSING */");
}

void ojoy() {
    if (alis.automode == 0)
    {
        alis.varD7 = io_joy(alis.varD7);
    }
}

void oprnd() {
    debug(EDebugWarning, " /* MISSING */");
}

void oscan() {
    if (alis.script->context._0x1e_scan_clr == alis.script->context._0x1c_scan_clr)
        return;

    s16 scan_clr = alis.script->context._0x1e_scan_clr + 2;
    if (-0x35 < scan_clr)
        scan_clr -= *(short *)(alis.mem + alis.script->context._0x14_script_org_offset + 0x16);

    alis.script->context._0x1e_scan_clr = scan_clr;
    if (scan_clr == alis.script->context._0x1c_scan_clr)
        alis.script->context._0x24_scan_inter.scan_clr_bit_7 &= 0x7f;
}

void oshiftkey() {
    if (alis.automode == 0)
    {
        alis.varD7 = io_shiftkey();
    }
}

void ofree() {
    debug(EDebugWarning, "\n%s SIMULATED\n", __FUNCTION__);
    alis.varD7 = 0x321;
}

void omodel() {
    alis.varD7 = sys_get_model();
}

void ogetkey() {
    debug(EDebugWarning, " /* MISSING */");
}

void oleft() {
    debug(EDebugWarning, " /* MISSING */");
}

void oright() {
    debug(EDebugWarning, " /* MISSING */");
}

void omid() {
    debug(EDebugWarning, " /* MISSING */");
}

void olen() {
    alis.varD7 = strlen((const char *)alis.bsd7bis);
}

void oasc() {
    alis.varD7 = 0;
    alis.varD7 = alis.bsd7bis[0];
}

void ostr() {
    debug(EDebugWarning, " /* MISSING */");
}

void osadd() {
    // TODO: strcat ??
    debug(EDebugWarning, " /* STUBBED */");
    
    u8 cVar1;
    u8 *pcVar3;
    u8 *pcVar4;
    
    sparam();
    
    u8 *pcVar2 = alis.sd6;
    
    do
    {
        pcVar3 = pcVar2;
        pcVar2 = pcVar3 + 1;
        pcVar4 = alis.sd7;
    }
    while (*pcVar3 != '\0');
    
    do
    {
        cVar1 = *pcVar4;
        *pcVar3 = cVar1;
        pcVar2 = alis.sd6;
        pcVar3 ++;
        pcVar4 ++;
    }
    while (cVar1 != '\0');
    
    alis.sd6 = alis.sd7;
    alis.sd7 = pcVar2;
}

void osegal() {
    // TODO: strcmp ??
    debug(EDebugWarning, " /* MISSING */");
}

void osdiff() {
    // TODO: !strcmp ??
    debug(EDebugWarning, " /* MISSING */");
}

void osinfeg() {
    // TODO: string equ or < ??
    debug(EDebugWarning, " /* MISSING */");
}

void ossupeg() {
    // TODO: string equ or > ??
    debug(EDebugWarning, " /* MISSING */");
}

void osinf() {
    // TODO: string < ??
    debug(EDebugWarning, " /* MISSING */");
}

void ossup() {
    // TODO: string > ??
    debug(EDebugWarning, " /* MISSING */");
}

void ospushacc() {
    debug(EDebugWarning, " /* MISSING */");
}

void ospile() {
    u8 * tmp = alis.bsd7bis;
    alis.bsd7bis = alis.bsd6;
    alis.bsd6 = tmp;
    
    debug(EDebugWarning, " /* STUBBED */");
}

void oval() {
    // TODO: compute int value of bssChunk3 string -> d7 ??
    debug(EDebugWarning, " /* STUBBED */");
    
    s8 cVar1;
    u8 bVar2;
    
    s16 sVar3 = 0;
    u8 neg = 0;
    if (alis.sd7[0] != 0)
    {
        u8 *string = alis.sd7;

        neg = alis.sd7[0] == 0x2d;
        if (neg)
        {
            string ++;
        }
        
        if (*string == 0x24)
        {
            while (1)
            {
                string ++;
                bVar2 = *string;
                if ((bVar2 == 0) || ((s8)(bVar2 - 0x30) < 0))
                    break;
                
                if (bVar2 < 0x3a)
                {
                    cVar1 = bVar2 - 0x30;
                }
                else
                {
                    bVar2 = bVar2 & 0xdf;
                    if (((s8)(bVar2 + 0xbf) < '\0') || (0x46 < bVar2))
                        break;
                    
                    cVar1 = bVar2 - 0x37;
                }
                
                sVar3 = sVar3 * 0x10 + cVar1;
            }
        }
        else
        {
            while (1)
            {
                bVar2 = *string;
                if (((bVar2 == 0) || ((s8)(bVar2 - 0x30) < '\0')) || (0x39 < bVar2))
                    break;
                
                sVar3 = sVar3 * 10 + (s8)(bVar2 - 0x30);
                string ++;
            }
        }
    }

    alis.varD7 = neg ? -sVar3 : sVar3;
}

void oexistf() {
    char path[kPathMaxLen] = {0};
    strcpy(path, alis.platform.path);
    strcat(path, (char *)alis.bsd7bis);
    alis.varD7 = sys_fexists(path) ? 0xffff : 0x0;
}

void ochr() {
    alis.bsd7bis[0] = (u8)alis.varD7;
}

void ochange() {
    // TODO: change le drive courant ??
    debug(EDebugWarning, " /* MISSING */");
}

void ocountry() {
    debug(EDebugWarning, " /* MISSING */");
}

void omip() {
    alis.varD7 = 0x64;
}

void ojoykey() {
    if (alis.automode == 0)
    {
        alis.varD7 = io_joykey(alis.varD7);
    }
}

void oconfig() {
    alis.varD7 = 0;
}

void cnul() {
}

// =============================================================================
#pragma mark - Opername Pointer table (85 values)
// In the disasm, the table JTAB_OPERNAMES is at address $10ee8 -> $10f92.
// So it contains 0xaa / 170 bytes, or 85 words, it matches the opernames count.
// So the routine FUN_READEXEC_OPERNAME() gets a BYTE code from script pc,
//  then retrieves a WORD offset at (JTAB_OPERNAMES[code]), then
//  jumps at the address (JTAB_OPERAMES + offset).
// =============================================================================
sAlisOpcode opernames[] = {
    DECL_OPCODE(0x00, oimmb,
                "read immediate byte from script, extend to word, copy into r7"),
    {},
    DECL_OPCODE(0x01, oimmw,
                "read immediate word from script, copy into r7"),
    {},
    DECL_OPCODE(0x02, oimmp,
                "read immediate bytes from script until zero, copy into str1"),
    {},
    DECL_OPCODE(0x03, olocb,
                "read offset word from script, extend read byte at vram[offset] to word, copy into r7"),
    {},
    DECL_OPCODE(0x04, olocw,
                "read offset word from script, read word at vram[offset], copy into r7"),
    {},
    DECL_OPCODE(0x05, olocp,
                "read offset word from script, read bytes at vram[offset] until zero, copy into str1"),
    {},
    DECL_OPCODE(0x06, oloctp, "TODO add desc"),
    {},
    DECL_OPCODE(0x07, oloctc, "TODO add desc"),
    {},
    DECL_OPCODE(0x08, olocti, "TODO add desc"),
    {},
    DECL_OPCODE(0x09, odirb, "TODO add desc"),
    {},
    DECL_OPCODE(0x0A, odirw, "TODO add desc"),
    {},
    DECL_OPCODE(0x0B, odirp, "TODO add desc"),
    {},
    DECL_OPCODE(0x0C, odirtp, "TODO add desc"),
    {},
    DECL_OPCODE(0x0D, odirtc, "TODO add desc"),
    {},
    DECL_OPCODE(0x0E, odirti, "TODO add desc"),
    {},
    DECL_OPCODE(0x0F, omainb, "TODO add desc"),
    {},
    DECL_OPCODE(0x10, omainw, "TODO add desc"),
    {},
    DECL_OPCODE(0x11, omainp, "TODO add desc"),
    {},
    DECL_OPCODE(0x12, omaintp, "TODO add desc"),
    {},
    DECL_OPCODE(0x13, omaintc, "TODO add desc"),
    {},
    DECL_OPCODE(0x14, omainti, "TODO add desc"),
    {},
    DECL_OPCODE(0x15, ohimb, "TODO add desc"),
    {},
    DECL_OPCODE(0x16, ohimw, "TODO add desc"),
    {},
    DECL_OPCODE(0x17, ohimp, "TODO add desc"),
    {},
    DECL_OPCODE(0x18, ohimtp, "TODO add desc"),
    {},
    DECL_OPCODE(0x19, ohimtc, "TODO add desc"),
    {},
    DECL_OPCODE(0x1A, ohimti, "TODO add desc"),
    {},
    DECL_OPCODE(0x1B, opile,
                "copy r6 to r7, pop word form virtual accumulator into r6"),
    {},
    DECL_OPCODE(0x1C, oeval,
                "starts an expression evaluation loop"),
    {},
    DECL_OPCODE(0x1D, ofin,
                "ends an expression evaluation loop"),
    {},
    DECL_OPCODE(0x1E, cnul,
                "[N/I]"),
    {},
    DECL_OPCODE(0x1F, cnul,
                "[N/I]"),
    {},
    DECL_OPCODE(0x20, opushacc,
                "push word from r7 into virtual accumulator"),
    {},
    DECL_OPCODE(0x21, oand,
                "AND binary operator"),
    {},
    DECL_OPCODE(0x22, oor,
                "OR binary operator"),
    {},
    DECL_OPCODE(0x23, oxor,
                "XOR binary operator"),
    {},
    DECL_OPCODE(0x24, oeqv,
                "EQV binary operator"),
    {},
    DECL_OPCODE(0x25, oegal,
                "is equal test"),
    {},
    DECL_OPCODE(0x26, odiff,
                "is non-equal test"),
    {},
    DECL_OPCODE(0x27, oinfeg,
                "is inferior or equal test"),
    {},
    DECL_OPCODE(0x28, osupeg,
                "is superior or equal test"),
    {},
    DECL_OPCODE(0x29, oinf,
                "is inferior test"),
    {},
    DECL_OPCODE(0x2A, osup,
                "is superior test"),
    {},
    DECL_OPCODE(0x2B, oadd,
                "addition operator"),
    {},
    DECL_OPCODE(0x2C, osub,
                "substraction operator"),
    {},
    DECL_OPCODE(0x2D, omod,
                "modulo operator"),
    {},
    DECL_OPCODE(0x2E, odiv,
                "division operator"),
    {},
    DECL_OPCODE(0x2F, omul,
                "multiplication operator"),
    {},
    DECL_OPCODE(0x30, oneg,
                "negation operator"),
    {},
    DECL_OPCODE(0x31, oabs,
                "absolute value operator"),
    {},
    DECL_OPCODE(0x32, ornd,
                "get random number"),
    {},
    DECL_OPCODE(0x33, osgn,
                "sign test operator"),
    {},
    DECL_OPCODE(0x34, onot,
                "binary NOT operator"),
    {},
    DECL_OPCODE(0x35, oinkey, "TODO add desc"),
    {},
    DECL_OPCODE(0x36, okeyon, "TODO add desc"),
    {},
    DECL_OPCODE(0x37, ojoy, "TODO add desc"),
    {},
    DECL_OPCODE(0x38, oprnd, "TODO add desc"),
    {},
    DECL_OPCODE(0x39, oscan, "TODO add desc"),
    {},
    DECL_OPCODE(0x3A, oshiftkey, "TODO add desc"),
    {},
    DECL_OPCODE(0x3B, ofree, "TODO add desc"),
    {},
    DECL_OPCODE(0x3C, omodel,
                "get host device model"),
    {},
    DECL_OPCODE(0x3D, ogetkey, "TODO add desc"),
    {},
    DECL_OPCODE(0x3E, oleft, "TODO add desc"),
    {},
    DECL_OPCODE(0x3F, oright, "TODO add desc"),
    {},
    DECL_OPCODE(0x40, omid, "TODO add desc"),
    {},
    DECL_OPCODE(0x41, olen, "TODO add desc"),
    {},
    DECL_OPCODE(0x42, oasc, "TODO add desc"),
    {},
    DECL_OPCODE(0x43, ostr, "TODO add desc"),
    {},
    DECL_OPCODE(0x44, osadd, "TODO add desc"),
    {},
    DECL_OPCODE(0x45, osegal, "TODO add desc"),
    {},
    DECL_OPCODE(0x46, osdiff, "TODO add desc"),
    {},
    DECL_OPCODE(0x47, osinfeg, "TODO add desc"),
    {},
    DECL_OPCODE(0x48, ossupeg, "TODO add desc"),
    {},
    DECL_OPCODE(0x49, osinf, "TODO add desc"),
    {},
    DECL_OPCODE(0x4A, ossup, "TODO add desc"),
    {},
    DECL_OPCODE(0x4B, ospushacc, "TODO add desc"),
    {},
    DECL_OPCODE(0x4C, ospile, "TODO add desc"),
    {},
    DECL_OPCODE(0x4D, oval, "TODO add desc"),
    {},
    DECL_OPCODE(0x4E, oexistf,
                "if file name in bssChunk3 exists, returns 0xffff in varD7, else 0"),
    {},
    DECL_OPCODE(0x4F, ochr, "TODO add desc"),
    {},
    DECL_OPCODE(0x50, ochange, "TODO add desc"),
    {},
    DECL_OPCODE(0x51, ocountry, "TODO add desc"),
    {},
    DECL_OPCODE(0x52, omip, "TODO add desc"),
    {},
    DECL_OPCODE(0x53, ojoykey, "TODO add desc"),
    {},
    DECL_OPCODE(0x54, oconfig,
                "unknown"),
    {}
};

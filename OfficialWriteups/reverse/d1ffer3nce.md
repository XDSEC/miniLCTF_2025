## 题解

UPX解压，载入IDA后发现是GoLang写的程序，定位到关键代码

```c
 v22 = main_sub_1145141919(
          v17,
          ptr,
          v18,
          (__int64)"0123456789abcdefexpected integerexpected newline: value of type 23841857910156250123456789ABCDEFTerminateProcessinteger overflowgcshrinkstackofftracefpunwindoffGC scavenge waitGC worker (idle)page trace flush/gc/gogc:percent, not a functiongc: unswept span KiB work (bg),  mheap.sweepgen=runtime: nelems=workbuf is emptymSpanList.removemSpanList.insertbad special kindbad summary dataruntime: addr = runtime: base = runtime: head = timeBeginPeriod",
          16LL,
          16LL,
          v19,
          v20,
          v21);
```

由于GoLang和Rust一样，把字符串常量全部放一起而不是用\x00分割，而下面参数的16LL则是字符串的长度。

即为：0123456789abcdef

点进去后可以观察到XXTEA

魔改点1，DELTA

```c
  if ( v28 > 0 )
  {
    LODWORD(v16) = v30 + 0x4D696E69;
    v32 = ((unsigned int)(v30 + 0x4D696E69) >> 2) & 3;
    v33 = 0LL;
    goto LABEL_34;
  }
```

魔改点2，rounds计算

```c
v28 = 2025 / (__int64)j + 6;
```
最后在函数外可以找到结果比对

```c
  if ( v38 == 64
    && (unsigned __int8)runtime_memequal(
                          v39,
                          "729daebea2e3845b310f01f1b3e703c24c810a9ca0ed2c4d9252a214882d7721runtime.SetFinalizer: first argument was allocated into an arenacompileCallback: expected function with one uintptr-sized r..................省略一大坨",
                          64LL) )
  {
```

即为 729daebea2e3845b310f01f1b3e703c24c810a9ca0ed2c4d9252a214882d7721

最后就可以写出Exp了。

## Exp
```python
from regadgets import *
enc = bytes.fromhex('729daebea2e3845b310f01f1b3e703c24c810a9ca0ed2c4d9252a214882d7721')
dec = xxtea_decrypt(enc, b'0123456789abcdef', delta=0x4d696e69, round_addi=2025)
print(dec)
# b'miniLCTF{W3lc0m3~MiN1Lc7F_2O25}\x01'
```
最后的\x01是PKCS5/7 Padding。

## Flag
miniLCTF{W3lc0m3~MiN1Lc7F_2O25}
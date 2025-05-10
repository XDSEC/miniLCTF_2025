# 题解
32位程序运行64位代码
init函数通过mmap映射内存并复制机器码到0xdead000处
![image.png](/api/media?hash=307da183cd0abf097d4af40c337aaf8cfddc92fd784948b3c215437a3b75d90b)

whathappened函数通过retf跳转到0xdead000,并将cs改为33，进入到64位代码中
![image.png](/api/media?hash=2f6ab0469cba9817eab117f749d5814c1d97dd9d8e24b9e31481c037102dae38)

64位代码的加密逻辑很简单，只是对input进行xor ”L“，有个小坑是最后两位没有异或
之后进行未魔改的sm4加密操作，并将结果与enc数组进行对比

```text
miniLCTF{3ac159d665b4ccfb25c0927c1a23edb3}
```
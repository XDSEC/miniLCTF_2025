随手出了个签到题，希望大家玩的开心。

> 本题解较为简略，一些具体的细节留给读者自行补充。

```python
from Crypto.Util.number import getPrime
from Crypto.Util.Padding import pad
from Crypto.Cipher import AES
from random import getrandbits, randint
from hashlib import md5


class Noisy_cipher:
    def __init__(self, params):
        self.nbits = params["nbits"]
        self.pbits = params["nbits"]//2
        self.Mbits = params["Mbits"]
        self.k0bits = params["k0bits"]
        self.k1bits = params["k1bits"]
        self.samples = params["samples"]
        self.p = getPrime(self.pbits)
        self.q = getPrime(self.nbits)
        self.n = self.p * self.q
        self.s = randint(0, self.n)
        self.M = getrandbits(self.Mbits)
        self.pubKey = [self.n]
        self.priKey = [self.s, self.p, self.M]

    def encrypt(self,msg):
        res = []
        for i in range(self.samples):
            k0 = getrandbits(self.k0bits)
            k1 = getrandbits(self.k1bits)
            ci = self.s * (msg[i] + k0*self.M)*(1 + k1*self.p) % self.n
            res.append(ci)

        return res


if __name__ == '__main__':
    params = {
        "nbits":1024,
        "Mbits":30,
        "k0bits":30,
        "k1bits":512,
        "samples":20,
    }
    mbits = 24
    Noise = Noisy_cipher(params)
    n = Noise.n
    msg = [getrandbits(mbits) for _ in range(params["samples"])]
    cipher = Noise.encrypt(msg)
    with open('secret.txt', 'r') as file:
        flag = file.readlines()[0].encode()

    file.close()
    key = md5(str(msg).encode()).digest()
    aes = AES.new(key, AES.MODE_ECB)
    encrypted_flag = aes.encrypt(pad(flag, 16)).hex()
    with open('output.txt', 'a') as file:
        file.write('n = ' + str(n) + '\n')
        file.write('c = ' + str(cipher) + '\n')
        file.write('encrypted_flag = "' + encrypted_flag + '"\n')
    file.close()
```

简单分析下`Noisy_cipher`，容易看出关键在于如下表达式：

```python
ci = self.s * (msg[i] + k0*self.M)*(1 + k1*self.p) % self.n
```

参数`self.s`,`self.M`,`self.p`构成了私钥，`k0,k1`则是额外添加的噪声。

我们先做一个热身练习：

首先将 $s$ 丢掉，仅考虑 $(m + k_{0}\cdot M)\cdot (1 + k_{1}\cdot p) \bmod{n}$，展开便可以得到 $x + k_{1}x\cdot p\bmod{n},x=m+k_{0}\cdot M$。

> 实际上后续的分析表明参数`self.s`的限制很容易被我们绕过

由于我们使用了 $n=p\cdot q$，那么上式可进一步改写为：

$$x + (k_{1}x\bmod{q})\cdot p \bmod {n}$$

接下来，我们需要注意到一些简单的大小关系：

1. $x = m + k_{0}\cdot M<2^{60} + m$
2. $k_{1}x\bmod q\approx q$

这实际上是一个经典的Partially Approximate Common Divisor Problem，笔者这里使用了正交格的方法来恢复 $x$ 与 分解大整数 $n$。

在得到了 $x$ 以后，如何恢复出原始的消息 $m$？实际上我们只需稍加思索即可得到问题的答案。

回顾一下 $x$ 的定义： $x = m + k_{0}\cdot M$

细心的读者应该能够看出上式实际上是经典著名的Approximate Common Divisor Problem的一个实例。

在本题的参数条件下，具体的求解方法有很多，笔者所用的仍然是正交格的玩法。

一点感想：设计密码算法好难啊！

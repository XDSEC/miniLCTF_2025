# rsasign

本题目为签到难度，所以只给出解题思路，代码部分留给读者实现

查看关键函数

```
def get_gift(prikey):
    a = bytes_to_long(b'miniL')
    b = bytes_to_long(b'mini7')
    p, q = prikey[1:]
    phi = (p - 1)*(q - 1)
    giftp = p + a
    giftq = q + b
    gift = pow((giftp + giftq + a*b), 2, phi)
    return gift >> 740

```

我们大概能得到
$$
(p+q+a+b+a\cdot b)^2~~	\approx~~gift~<<740~~mod~phi \\ (p+q+a+b+a\cdot b)^2-k\cdot (n-(p+q)+1)~~\approx ~~gift~<<740\\
$$
数量级相差不大，可以直接爆破出k，从而解得（p+q）的高位，进而得到p或q的高位，然后进行copper






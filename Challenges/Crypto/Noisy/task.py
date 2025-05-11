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

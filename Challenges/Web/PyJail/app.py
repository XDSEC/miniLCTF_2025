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
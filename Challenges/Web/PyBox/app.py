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
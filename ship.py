#!/usr/bin/env python3
import subprocess
import os
import shutil
import sys
import threading
import time
import platform
import zipfile
if platform.system() == "Windows":
    os.system("")
class Colors:
    HEADER = '\033[95m'
    BLUE = '\033[94m'
    CYAN = '\033[96m'
    GREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    DIM = '\033[90m'
class Symbols:
    CHECK = f"{Colors.GREEN}✔{Colors.ENDC}"
    CROSS = f"{Colors.FAIL}✖{Colors.ENDC}"
    ARROW = f"{Colors.CYAN}➜{Colors.ENDC}"
    INFO = f"{Colors.BLUE}i{Colors.ENDC}"
class Spinner:
    def __init__(self, message="Processing..."):
        self.message = message
        self.stop_event = threading.Event()
        self.thread = threading.Thread(target=self._spin)
        self.frames = ["⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"]
        self.start_time = time.time()
    def start(self):
        self.stop_event.clear()
        self.thread.start()
    def stop(self, success=True):
        self.stop_event.set()
        self.thread.join()
        elapsed = time.time() - self.start_time
        sys.stdout.write("\r" + " " * (len(self.message) + 10) + "\r")
        sys.stdout.flush()
        return elapsed
    def _spin(self):
        idx = 0
        while not self.stop_event.is_set():
            frame = self.frames[idx % len(self.frames)]
            sys.stdout.write(f"\r{Colors.CYAN}{frame}{Colors.ENDC} {self.message}")
            sys.stdout.flush()
            idx += 1
            time.sleep(0.08)
def print_header(title):
    print(f"\n{Colors.HEADER}{Colors.BOLD}{'=' * 60}{Colors.ENDC}")
    print(f"{Colors.HEADER}{Colors.BOLD}   {title.upper()}{Colors.ENDC}")
    print(f"{Colors.HEADER}{Colors.BOLD}{'=' * 60}{Colors.ENDC}\n")
def print_verbose_block(title, content):
    print(f"{Colors.DIM}   ┌─ [{title}] ──────────────────────────")
    for line in content.splitlines():
        print(f"   │ {line}")
    print(f"   └──────────────────────────────────────────{Colors.ENDC}")
class ShipRegistry:
    _functions = {}
    _display_names = {}
    @classmethod
    def register(cls, name: str, display_name: str = None):
        def decorator(func):
            cls._functions[name] = func
            cls._display_names[name] = display_name or name
            return func
        return decorator
    @classmethod
    def get(cls, name: str):
        return cls._functions.get(name)
    @classmethod
    def exists(cls, name: str) -> bool:
        return name in cls._functions
    @classmethod
    def get_display_name(cls, name: str) -> str:
        return cls._display_names.get(name, name)
    @classmethod
    def all_functions(cls) -> dict:
        return cls._functions.copy()
    @classmethod
    def list_functions(cls) -> list:
        return list(cls._functions.keys())
@ShipRegistry.register("run", "Run Command")
def ship_run(command: str, verbose: bool = False):
    try:
        result = subprocess.run(command, shell=True, capture_output=True, text=True)
        return {
            "stdout": result.stdout.strip(),
            "stderr": result.stderr.strip(),
            "returncode": result.returncode
        }
    except Exception as e:
        return {"stdout": "", "stderr": str(e), "returncode": -1}
@ShipRegistry.register("delete", "Delete")
def ship_delete(path: str, forgive_missing: bool = True):
    try:
        if os.path.isdir(path):
            shutil.rmtree(path)
        elif os.path.isfile(path):
            os.remove(path)
        else:
            if not forgive_missing:
                return {"stdout": "", "stderr": f"Path not found: {path}", "returncode": 1}
            return {"stdout": "Path not found (ignored)", "stderr": "", "returncode": 0}
        return {"stdout": f"Deleted: {os.path.basename(path)}", "stderr": "", "returncode": 0}
    except Exception as e:
        return {"stdout": "", "stderr": str(e), "returncode": -1}
@ShipRegistry.register("mkdir", "Create Directory")
def ship_mkdir(path: str):
    try:
        if os.path.exists(path):
            return {"stdout": f"Directory exists: {path}", "stderr": "", "returncode": 0}
        os.makedirs(path, exist_ok=True)
        return {"stdout": f"Created directory: {path}", "stderr": "", "returncode": 0}
    except Exception as e:
        return {"stdout": "", "stderr": str(e), "returncode": -1}
@ShipRegistry.register("copy", "Copy")
def ship_copy(src: str, dst: str):
    try:
        if not os.path.isfile(src):
            return {"stdout": "", "stderr": f"Source file not found: {src}", "returncode": 1}
        os.makedirs(os.path.dirname(dst) if os.path.dirname(dst) else ".", exist_ok=True)
        shutil.copy2(src, dst)
        return {"stdout": f"Copied {os.path.basename(src)}", "stderr": "", "returncode": 0}
    except Exception as e:
        return {"stdout": "", "stderr": str(e), "returncode": -1}
@ShipRegistry.register("move", "Move")
def ship_move(src: str, dst: str):
    try:
        shutil.move(src, dst)
        return {"stdout": f"Moved {os.path.basename(src)}", "stderr": "", "returncode": 0}
    except Exception as e:
        return {"stdout": "", "stderr": str(e), "returncode": -1}
@ShipRegistry.register("move_all", "Move Contents")
def ship_move_all(src: str, dst: str):
    try:
        if not os.path.exists(src):
            return {"stdout": "", "stderr": f"Source not found: {src}", "returncode": 1}
        os.makedirs(dst, exist_ok=True)
        count = 0
        for item in os.listdir(src):
            shutil.move(os.path.join(src, item), os.path.join(dst, item))
            count += 1
        return {"stdout": f"Moved {count} items from {src}", "stderr": "", "returncode": 0}
    except Exception as e:
        return {"stdout": "", "stderr": str(e), "returncode": -1}
@ShipRegistry.register("zip", "Create ZIP")
def ship_zip(src: str, zip_path: str):
    try:
        if not os.path.exists(src):
            return {"stdout": "", "stderr": f"Source directory not found: {src}", "returncode": 1}
        os.makedirs(os.path.dirname(zip_path) if os.path.dirname(zip_path) else ".", exist_ok=True)
        file_count = 0
        with zipfile.ZipFile(zip_path, 'w', zipfile.ZIP_DEFLATED) as zipf:
            for root, dirs, files in os.walk(src):
                for file in files:
                    file_path = os.path.join(root, file)
                    arcname = os.path.relpath(file_path, src)
                    zipf.write(file_path, arcname)
                    file_count += 1
        zip_size = os.path.getsize(zip_path) / (1024 * 1024)
        return {"stdout": f"Zipped {file_count} files ({zip_size:.2f} MB)", "stderr": "", "returncode": 0}
    except Exception as e:
        return {"stdout": "", "stderr": str(e), "returncode": -1}
@ShipRegistry.register("list", "List Directory")
def ship_list(path: str):
    try:
        contents = os.listdir(path)
        return {"stdout": "\n".join(contents), "stderr": "", "returncode": 0}
    except Exception as e:
        return {"stdout": "", "stderr": str(e), "returncode": -1}
@ShipRegistry.register("echo", "Echo")
def ship_echo(message: str):
    print(f"  {Colors.CYAN}>{Colors.ENDC} {message}")
    return {"stdout": message, "stderr": "", "returncode": 0}
def _get_task_name(func, args):
    fname = func.__name__
    if fname == "ship_run":
        cmd = args.get("command", "")
        return f"Run: {Colors.BOLD}{cmd[:40]}{'...' if len(cmd) > 40 else ''}{Colors.ENDC}"
    elif fname == "ship_delete":
        return f"Delete: {os.path.basename(args.get('path', 'unknown'))}"
    elif fname == "ship_copy":
        return f"Copy: {os.path.basename(args.get('src', 'unknown'))}"
    elif fname == "ship_move_all":
        return f"Move contents: {os.path.basename(args.get('src', 'unknown'))} → {os.path.basename(args.get('dst', ''))}"
    elif fname == "ship_mkdir":
        return f"MkDir: {args.get('path', '')}"
    elif fname == "ship_zip":
        return f"Zip: {os.path.basename(args.get('zip_path', 'unknown'))}"
    elif fname == "ship_echo":
        return f"Echo: {args.get('message', '')[:30]}"
    else:
        display = ShipRegistry.get_display_name(fname.replace("ship_", ""))
        return f"{display}: {list(args.values())[0] if args else ''}"[:50]
def build(task_name: str, tasks, dry_run: bool = False):
    results = []
    print_header(task_name)
    total_start = time.time()
    print(f"{Colors.BOLD}Plan: {len(tasks)} steps to execute.{Colors.ENDC}\n")
    for i, (func, args) in enumerate(tasks, start=1):
        readable_name = _get_task_name(func, args)
        step_prefix = f"{Colors.DIM}[{i}/{len(tasks)}]{Colors.ENDC}"
        if dry_run:
            print(f"{step_prefix} {Symbols.INFO} {readable_name} {Colors.DIM}(Skipped){Colors.ENDC}")
            results.append({"stdout": "Dry run", "stderr": "", "returncode": 0})
            continue
        spinner = Spinner(message=f"{readable_name}...")
        spinner.start()
        try:
            result = func(**args)
        except Exception as e:
            result = {"stdout": "", "stderr": str(e), "returncode": -1}
        elapsed = spinner.stop()
        time_str = f"{elapsed:.2f}s"
        if result["returncode"] == 0:
            print(f"{step_prefix} {Symbols.CHECK} {readable_name} {Colors.DIM}({time_str}){Colors.ENDC}")
            if args.get("verbose", False) and result["stdout"]:
                print_verbose_block("STDOUT", result["stdout"])
        else:
            print(f"{step_prefix} {Symbols.CROSS} {readable_name} {Colors.FAIL}(FAILED in {time_str}){Colors.ENDC}")
            print(f"\n{Colors.FAIL}>> ERROR DETAILS:{Colors.ENDC}")
            if result["stdout"]:
                print(f"{Colors.DIM}{result['stdout']}{Colors.ENDC}")
            if result["stderr"]:
                print(f"{Colors.WARNING}{result['stderr']}{Colors.ENDC}")
            results.append(result)
            break
        results.append(result)
    total_time = time.time() - total_start
    success_count = len([r for r in results if r['returncode'] == 0])
    print(f"\n{Colors.DIM}{'-' * 60}{Colors.ENDC}")
    if len(results) == len(tasks) and len(results) > 0 and results[-1]['returncode'] == 0:
        print(f"{Colors.GREEN}{Colors.BOLD}BUILD SUCCESSFUL{Colors.ENDC}")
    elif len(results) == 0:
        print(f"{Colors.WARNING}{Colors.BOLD}NO TASKS EXECUTED{Colors.ENDC}")
    else:
        print(f"{Colors.FAIL}{Colors.BOLD}BUILD FAILED{Colors.ENDC}")
    print(f"Total Time: {total_time:.2f}s | Steps: {success_count}/{len(tasks)}")
    print(f"{Colors.DIM}{'-' * 60}{Colors.ENDC}\n")
    return results
class ShipToken:
    LBRACE = 'LBRACE'
    RBRACE = 'RBRACE'
    LPAREN = 'LPAREN'
    RPAREN = 'RPAREN'
    COLON = 'COLON'
    COMMA = 'COMMA'
    EQUALS = 'EQUALS'
    EQ = 'EQ'
    NE = 'NE'
    LT = 'LT'
    LE = 'LE'
    GT = 'GT'
    GE = 'GE'
    AND = 'AND'
    OR = 'OR'
    NOT = 'NOT'
    IDENT = 'IDENT'
    CUSTOM = 'CUSTOM'
    STRING = 'STRING'
    NUMBER = 'NUMBER'
    BOOL = 'BOOL'
    NULL = 'NULL'
    EOF = 'EOF'
class ShipLexer:
    """Tokenizer for Ship DSL."""
    def __init__(self, text):
        self.text = text
        self.pos = 0
        self.line = 1
        self.col = 1
    def _advance(self):
        if self.pos < len(self.text):
            if self.text[self.pos] == '\n':
                self.line += 1
                self.col = 1
            else:
                self.col += 1
            self.pos += 1
    def _peek(self, offset=0):
        pos = self.pos + offset
        return self.text[pos] if pos < len(self.text) else ''
    def _skip_whitespace_and_comments(self):
        while self.pos < len(self.text):
            c = self._peek()
            if c in ' \t\n\r':
                self._advance()
            elif c == '/' and self._peek(1) == '/':
                while self.pos < len(self.text) and self._peek() != '\n':
                    self._advance()
            elif c == '/' and self._peek(1) == '*':
                self._advance()
                self._advance()
                while self.pos < len(self.text):
                    if self._peek() == '*' and self._peek(1) == '/':
                        self._advance()
                        self._advance()
                        break
                    self._advance()
            elif c == '#':
                while self.pos < len(self.text) and self._peek() != '\n':
                    self._advance()
            else:
                break
    def _read_string(self, quote):
        self._advance()
        result = []
        while self.pos < len(self.text):
            c = self._peek()
            if c == quote:
                self._advance()
                break
            elif c == '\\':
                self._advance()
                escaped = self._peek()
                escape_map = {'n': '\n', 't': '\t', '\\': '\\', quote: quote}
                result.append(escape_map.get(escaped, escaped))
                self._advance()
            else:
                result.append(c)
                self._advance()
        return ''.join(result)
    def _read_identifier(self):
        result = []
        while self.pos < len(self.text):
            c = self._peek()
            if c.isalnum() or c in '_-./':
                result.append(c)
                self._advance()
            else:
                break
        return ''.join(result)
    def _read_number(self):
        result = []
        has_dot = False
        while self.pos < len(self.text):
            c = self._peek()
            if c.isdigit():
                result.append(c)
                self._advance()
            elif c == '.' and not has_dot:
                has_dot = True
                result.append(c)
                self._advance()
            else:
                break
        num_str = ''.join(result)
        return float(num_str) if has_dot else int(num_str)
    def tokenize(self):
        tokens = []
        while self.pos < len(self.text):
            self._skip_whitespace_and_comments()
            if self.pos >= len(self.text):
                break
            c = self._peek()
            line = self.line
            if c == '=' and self._peek(1) == '=':
                tokens.append((ShipToken.EQ, '==', line))
                self._advance()
                self._advance()
            elif c == '!' and self._peek(1) == '=':
                tokens.append((ShipToken.NE, '!=', line))
                self._advance()
                self._advance()
            elif c == '<' and self._peek(1) == '=':
                tokens.append((ShipToken.LE, '<=', line))
                self._advance()
                self._advance()
            elif c == '>' and self._peek(1) == '=':
                tokens.append((ShipToken.GE, '>=', line))
                self._advance()
                self._advance()
            elif c == '&' and self._peek(1) == '&':
                tokens.append((ShipToken.AND, '&&', line))
                self._advance()
                self._advance()
            elif c == '|' and self._peek(1) == '|':
                tokens.append((ShipToken.OR, '||', line))
                self._advance()
                self._advance()
            elif c == '{':
                tokens.append((ShipToken.LBRACE, '{', line))
                self._advance()
            elif c == '}':
                tokens.append((ShipToken.RBRACE, '}', line))
                self._advance()
            elif c == '(':
                tokens.append((ShipToken.LPAREN, '(', line))
                self._advance()
            elif c == ')':
                tokens.append((ShipToken.RPAREN, ')', line))
                self._advance()
            elif c == ':':
                tokens.append((ShipToken.COLON, ':', line))
                self._advance()
            elif c == ',':
                tokens.append((ShipToken.COMMA, ',', line))
                self._advance()
            elif c == '=':
                tokens.append((ShipToken.EQUALS, '=', line))
                self._advance()
            elif c == '<':
                tokens.append((ShipToken.LT, '<', line))
                self._advance()
            elif c == '>':
                tokens.append((ShipToken.GT, '>', line))
                self._advance()
            elif c == '!':
                tokens.append((ShipToken.NOT, '!', line))
                self._advance()
            elif c == '"' or c == "'":
                s = self._read_string(c)
                tokens.append((ShipToken.STRING, s, line))
            elif c == '$':
                self._advance()
                name = self._read_identifier()
                tokens.append((ShipToken.CUSTOM, name, line))
            elif c.isdigit() or (c == '-' and self._peek(1).isdigit()):
                if c == '-':
                    self._advance()
                    num = -self._read_number()
                else:
                    num = self._read_number()
                tokens.append((ShipToken.NUMBER, num, line))
            elif c.isalpha() or c == '_':
                ident = self._read_identifier()
                if ident.lower() in ('true', 'false'):
                    tokens.append((ShipToken.BOOL, ident.lower() == 'true', line))
                elif ident.lower() in ('null', 'none'):
                    tokens.append((ShipToken.NULL, None, line))
                else:
                    tokens.append((ShipToken.IDENT, ident, line))
            else:
                self._advance()
        tokens.append((ShipToken.EOF, None, self.line))
        return tokens
class ShipParser:
    def __init__(self, variables=None):
        self.variables = variables or {}
        self.tasks = []
        self.title = "Ship Build"
        self.tokens = []
        self.pos = 0
    def _current(self):
        return self.tokens[self.pos] if self.pos < len(self.tokens) else (ShipToken.EOF, None, 0)
    def _peek(self, offset=0):
        idx = self.pos + offset
        return self.tokens[idx] if idx < len(self.tokens) else (ShipToken.EOF, None, 0)
    def _advance(self):
        self.pos += 1
    def _expect(self, token_type):
        tok = self._current()
        if tok[0] != token_type:
            raise SyntaxError(f"Expected {token_type} but got {tok[0]} '{tok[1]}' at line {tok[2]}")
        self._advance()
        return tok[1]
    def _resolve_identifier(self, name):
        if name in self.variables:
            return self.variables[name]
        return name
    def _parse_primary(self):
        tok = self._current()
        if tok[0] == ShipToken.STRING:
            self._advance()
            return tok[1]
        elif tok[0] == ShipToken.NUMBER:
            self._advance()
            return tok[1]
        elif tok[0] == ShipToken.BOOL:
            self._advance()
            return tok[1]
        elif tok[0] == ShipToken.NULL:
            self._advance()
            return None
        elif tok[0] == ShipToken.IDENT:
            self._advance()
            return self._resolve_identifier(tok[1])
        elif tok[0] == ShipToken.LPAREN:
            self._advance()
            result = self._parse_expression()
            self._expect(ShipToken.RPAREN)
            return result
        elif tok[0] == ShipToken.NOT:
            self._advance()
            value = self._parse_primary()
            return not self._to_bool(value)
        else:
            raise SyntaxError(f"Unexpected token {tok[0]} '{tok[1]}' at line {tok[2]}")
    def _to_bool(self, value):
        if isinstance(value, bool):
            return value
        if isinstance(value, str):
            return value.lower() not in ('false', '0', '', 'null', 'none')
        if isinstance(value, (int, float)):
            return value != 0
        return value is not None
    def _parse_comparison(self):
        left = self._parse_primary()
        tok = self._current()
        if tok[0] in (ShipToken.EQ, ShipToken.NE, ShipToken.LT, ShipToken.LE, ShipToken.GT, ShipToken.GE):
            op = tok[0]
            self._advance()
            right = self._parse_primary()
            if op == ShipToken.EQ:
                return left == right
            elif op == ShipToken.NE:
                return left != right
            elif op == ShipToken.LT:
                return left < right
            elif op == ShipToken.LE:
                return left <= right
            elif op == ShipToken.GT:
                return left > right
            elif op == ShipToken.GE:
                return left >= right
        return left
    def _parse_and(self):
        left = self._parse_comparison()
        while self._current()[0] == ShipToken.AND:
            self._advance()
            right = self._parse_comparison()
            left = self._to_bool(left) and self._to_bool(right)
        return left
    def _parse_expression(self):
        left = self._parse_and()
        while self._current()[0] == ShipToken.OR:
            self._advance()
            right = self._parse_and()
            left = self._to_bool(left) or self._to_bool(right)
        return left
    def _parse_value(self):
        return self._parse_expression()
    def _parse_function_args(self):
        args = {}
        self._expect(ShipToken.LBRACE)
        while self._current()[0] != ShipToken.RBRACE:
            if self._current()[0] == ShipToken.EOF:
                raise SyntaxError("Unexpected end of file in function arguments")
            key_tok = self._current()
            if key_tok[0] not in (ShipToken.IDENT, ShipToken.STRING):
                raise SyntaxError(f"Expected argument name at line {key_tok[2]}, got {key_tok[0]}")
            key = key_tok[1]
            self._advance()
            if self._current()[0] != ShipToken.COLON:
                raise SyntaxError(f"Expected ':' after argument name at line {self._current()[2]}. Use ':' for function arguments, '=' is only for var blocks.")
            self._advance()
            value = self._parse_value()
            args[key] = value
            if self._current()[0] == ShipToken.COMMA:
                self._advance()
        self._expect(ShipToken.RBRACE)
        return args
    def _parse_var_block(self):
        self._expect(ShipToken.LBRACE)
        script_vars = {}
        while self._current()[0] != ShipToken.RBRACE:
            if self._current()[0] == ShipToken.EOF:
                raise SyntaxError("Unexpected end of file in var block")
            name_tok = self._current()
            if name_tok[0] != ShipToken.IDENT:
                raise SyntaxError(f"Expected variable name at line {name_tok[2]}")
            name = name_tok[1]
            self._advance()
            if self._current()[0] != ShipToken.EQUALS:
                raise SyntaxError(f"Expected '=' after variable name at line {self._current()[2]}. Use '=' for var blocks.")
            self._advance()
            value = self._parse_value()
            if name not in self.variables:
                script_vars[name] = value
            if self._current()[0] == ShipToken.COMMA:
                self._advance()
        self._expect(ShipToken.RBRACE)
        self.variables = {**script_vars, **self.variables}
    def _parse_if_block(self):
        condition = self._parse_expression()
        condition_met = self._to_bool(condition)
        self._expect(ShipToken.LBRACE)
        if_tasks = []
        if condition_met:
            if_tasks = self._parse_block_body()
        else:
            self._skip_block_body()
        self._expect(ShipToken.RBRACE)
        result_tasks = if_tasks if condition_met else []
        already_matched = condition_met
        while self._current()[0] == ShipToken.IDENT:
            tok = self._current()
            if tok[1] == 'elif':
                self._advance()
                elif_condition = self._parse_expression()
                elif_met = self._to_bool(elif_condition) and not already_matched
                self._expect(ShipToken.LBRACE)
                if elif_met:
                    result_tasks = self._parse_block_body()
                    already_matched = True
                else:
                    self._skip_block_body()
                self._expect(ShipToken.RBRACE)
            elif tok[1] == 'else':
                self._advance()
                self._expect(ShipToken.LBRACE)
                if not already_matched:
                    result_tasks = self._parse_block_body()
                else:
                    self._skip_block_body()
                self._expect(ShipToken.RBRACE)
                break
            else:
                break
        return result_tasks
    def _skip_block_body(self):
        depth = 1
        while depth > 0 and self._current()[0] != ShipToken.EOF:
            tok = self._current()
            if tok[0] == ShipToken.LBRACE:
                depth += 1
            elif tok[0] == ShipToken.RBRACE:
                depth -= 1
                if depth == 0:
                    return
            self._advance()
    def _parse_block_body(self):
        tasks = []
        while self._current()[0] not in (ShipToken.RBRACE, ShipToken.EOF):
            tok = self._current()
            if tok[0] == ShipToken.IDENT:
                ident = tok[1]
                self._advance()
                if ident == 'title':
                    if self._current()[0] == ShipToken.COLON:
                        self._advance()
                    self.title = str(self._parse_value())
                elif ident == 'var':
                    self._parse_var_block()
                elif ident == 'if':
                    if_tasks = self._parse_if_block()
                    tasks.extend(if_tasks)
                elif ShipRegistry.exists(ident):
                    func = ShipRegistry.get(ident)
                    args = self._parse_function_args()
                    tasks.append((func, args))
                else:
                    if self._current()[0] == ShipToken.LBRACE:
                        self._advance()
                        self._skip_block_body()
                        self._expect(ShipToken.RBRACE)
            elif tok[0] == ShipToken.CUSTOM:
                custom_name = tok[1]
                self._advance()
                if self._current()[0] == ShipToken.LBRACE:
                    self._advance()
                    self._skip_block_body()
                    self._expect(ShipToken.RBRACE)
                print(f"{Colors.DIM}Custom task: ${custom_name}{Colors.ENDC}")
            else:
                self._advance()
        return tasks
    def parse(self, script_content):
        lexer = ShipLexer(script_content)
        self.tokens = lexer.tokenize()
        self.pos = 0
        tok = self._current()
        if tok[0] == ShipToken.IDENT and tok[1] == 'ship':
            self._advance()
            self._expect(ShipToken.LBRACE)
            self.tasks = self._parse_block_body()
            self._expect(ShipToken.RBRACE)
        else:
            self.tasks = self._parse_block_body()
        return self
    def execute(self, dry_run=False):
        return build(self.title, self.tasks, dry_run=dry_run)
def run_ship(script_path: str, dry_run: bool = False):
    with open(script_path, 'r', encoding='utf-8') as f:
        script_content = f.read()
    parser = ShipParser()
    parser.parse(script_content)
    return parser.execute(dry_run=dry_run)
def main():
    import argparse

    VERSION = "1.0.0"

    cli_parser = argparse.ArgumentParser(
        prog='ship',
        description='Ship Build System - "Ship It!" A simple scripting DSL for build automation.',
        epilog='Example: ship build.ship --dry-run'
    )
    cli_parser.add_argument(
        'script',
        nargs='?',
        help='Path to the .ship build script (default: Shipfile or build.ship)'
    )
    cli_parser.add_argument(
        '--dry-run',
        action='store_true',
        help='Show what would be executed without running anything'
    )
    cli_parser.add_argument(
        '--version',
        action='version',
        version=f'%(prog)s {VERSION}'
    )
    cli_parser.add_argument(
        '--list-functions',
        action='store_true',
        help='List all available Ship DSL functions'
    )
    args = cli_parser.parse_args()
    if args.list_functions:
        print_header("Available Functions")
        functions = ShipRegistry.all_functions()
        for name in sorted(functions.keys()):
            display = ShipRegistry.get_display_name(name)
            print(f"  {Colors.CYAN}{name}{Colors.ENDC} -> {display}")
        return
    script_path = args.script
    if not script_path:
        for default in ['Shipfile', 'build.ship', 'ship.ship']:
            if os.path.exists(default):
                script_path = default
                break
    if not script_path:
        cli_parser.print_help()
        print(f"\n{Colors.FAIL}Error: No build script specified and no default (Shipfile, build.ship) found.{Colors.ENDC}")
        sys.exit(1)
    if not os.path.exists(script_path):
        script_with_extension = script_path + '.ship'
        if os.path.exists(script_with_extension):
            script_path = script_with_extension
        else:
            print(f"{Colors.FAIL}Error: Script not found: {script_path}{Colors.ENDC}")
            sys.exit(1)
    try:
        results = run_ship(script_path, dry_run=args.dry_run)
        if any(r.get('returncode', 0) != 0 for r in results):
            sys.exit(1)
    except SyntaxError as e:
        print(f"{Colors.FAIL}Syntax Error: {e}{Colors.ENDC}")
        sys.exit(1)
    except Exception as e:
        print(f"{Colors.FAIL}Error: {e}{Colors.ENDC}")
        sys.exit(1)
if __name__ == "__main__":
    main()

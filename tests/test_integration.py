"""Offline tests: only a loopback HTTP server, no credentials or transit API."""
import http.server
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile
import threading
import time


class Handler(http.server.BaseHTTPRequestHandler):
    def log_message(self, *_args):
        pass

    def do_GET(self):
        status, body = 200, b"{}"
        if self.path == "/http-error":
            status = 503
        elif self.path == "/empty":
            body = b""
        elif self.path == "/redirect":
            status = 302
        elif self.path == "/large":
            body = b"x" * (2 * 1024 * 1024 + 1)
        elif self.path == "/nul":
            body = b"{}\0hidden"
        elif self.path == "/slow":
            time.sleep(2)
        try:
            self.send_response(status)
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
        except (BrokenPipeError, ConnectionResetError):
            pass


def main():
    ticker, network = (str(Path(arg).resolve()) for arg in sys.argv[1:])
    env = {key: value for key, value in os.environ.items()
           if key not in {"API_KEY", "STOP_IDS", "API_FEED", "POLL_SECONDS", "MATRIX_FONT"}}
    env["NO_PROXY"] = "127.0.0.1,localhost"
    with http.server.ThreadingHTTPServer(("127.0.0.1", 0), Handler) as server:
        worker = threading.Thread(target=server.serve_forever, daemon=True)
        worker.start()
        subprocess.run([network, f"http://127.0.0.1:{server.server_port}"],
                       env=env, check=True, timeout=15)
        server.shutdown()
        worker.join()
    with tempfile.TemporaryDirectory() as cwd:
        def run(*args, overrides=None):
            return subprocess.run([ticker, *args], cwd=cwd, env=env | (overrides or {}),
                                  capture_output=True, text=True, timeout=10)
        assert run("--help").returncode == 0
        missing = run("--once")
        assert missing.returncode == 1 and "API_KEY" in missing.stderr
        demo = run("--demo", "--once")
        assert demo.returncode == 0, demo.stderr
        assert "DEMO" in demo.stdout and "Stop 0454" in demo.stdout and "3 min" in demo.stdout
        assert run("--unknown").returncode == 1
        assert run("--demo", "--once", overrides={"STOP_IDS": "1,,2"}).returncode == 1
        assert run("--demo", "--once", overrides={"POLL_SECONDS": "-1"}).returncode == 1
        (Path(cwd) / ".env").write_text("API_KEY=\"unterminated\n")
        malformed = run("--once")
        assert malformed.returncode == 1 and "unterminated" not in malformed.stderr
        (Path(cwd) / ".env").unlink()
        process = subprocess.Popen([ticker, "--demo"], cwd=cwd, env=env,
                                   stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        try:
            time.sleep(0.3)
            process.send_signal(signal.SIGTERM)
            stdout, stderr = process.communicate(timeout=5)
            assert process.returncode == 0, stderr
            assert "DEMO" in stdout
        finally:
            if process.poll() is None:
                process.kill()
                process.wait()
    print("CLI regressions passed (demo, configuration, missing key, SIGTERM).")


if __name__ == "__main__":
    main()

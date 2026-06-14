#! /usr/bin/env python3

import subprocess
import sys

DEBUG = True

def run(arguments, **kwargs):
    if DEBUG:
        print(" ".join(arguments))
    return subprocess.run(arguments, capture_output=True, text=True, check=True, **kwargs)

def main():
    # Run jlc with -### to get the individual commands to be executed
    jlcArguments = ['jlc'] + sys.argv[1:] + ['-###']
    jlcOutput = run(jlcArguments)

    for command in jlcOutput.stdout.splitlines():
        commandArguments = command.split()
        if commandArguments[0].find('clang') != -1:
            if '-Werror' in commandArguments:
                commandArguments.remove('-Werror')
            if '-no-pie' in commandArguments:
                commandArguments.remove('-no-pie')
            commandArguments.append('--target=riscv64')
#            commandArguments.append('-march=rv64gc')
            commandArguments.append('-march=rv64ifd')
            commandArguments.append('-mabi=lp64d')
            commandArguments.append('-I/usr/riscv64-linux-gnu/include')
            commandArguments.append('-L/usr/riscv64-linux-gnu/lib')
            commandArguments.append('-L/usr/lib/gcc-cross/riscv64-linux-gnu/13')
            run(commandArguments)
        else:
            run(commandArguments)

if __name__ == "__main__":
    try:
        main()
    except subprocess.CalledProcessError as e:
        print(f"Command failed with return code {e.returncode}: {' '.join(e.cmd)}", file=sys.stderr)
        print(f"Stdout:\n{e.stdout}", file=sys.stderr)
        print(f"Stderr:\n{e.stderr}", file=sys.stderr)
        sys.exit(1)

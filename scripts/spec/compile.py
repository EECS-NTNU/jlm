#!/bin/python3

import json
import subprocess
import os.path

try:
    with open('sources.json', 'r') as file:
        jsonData = json.load(file)
    
except FileNotFoundError:
    print("Error: The file 'sources.json' was not found.")

def compileFile(cFile, jlm, scriptDirectory):
    workingDirectory = scriptDirectory + "/" + cFile['working_dir']
    os.chdir(workingDirectory)

    outputDirectory = scriptDirectory + "/" + os.path.dirname(cFile["ofile"])
    mkdirCommand = "mkdir -p " + outputDirectory
    subprocess.run(mkdirCommand.split(" "))

    clangCommand = "clang-18"
    for argument in cFile["arguments"]:
        clangCommand = clangCommand + " " + argument

    outputFile = scriptDirectory + "/" + cFile["ofile"]
    if jlm:
        clangCommand = clangCommand + " -c " + cFile["cfile"] + " -S -emit-llvm -Xclang -disable-O0-optnone -o " + outputFile + ".ll"
        print(clangCommand)
        result = subprocess.run(clangCommand.split(" "), check=True, stdout = subprocess.DEVNULL, stderr = subprocess.DEVNULL)

        jlmCommand = "jlm-opt --output-format=llvm -s /tmp -o " + outputFile + ".jlm-opt.ll " + outputFile + ".ll"
        print(jlmCommand)
        result = subprocess.run(jlmCommand.split(" "), check=True)

        llcCommand = "llc-18 -O0 --relocation-model=static -filetype=obj -o " + outputFile + " " + outputFile + ".jlm-opt.ll"
        print(llcCommand)
        result = subprocess.run(llcCommand.split(" "), check=True)
    else:
        clangCommand = clangCommand + " -c " + cFile["cfile"] + " -Xclang -disable-O0-optnone -O0 -o " + outputFile
        print(clangCommand)
        result = subprocess.run(clangCommand.split(" "), check=True, stdout = subprocess.DEVNULL, stderr = subprocess.DEVNULL)

def main():
    scriptDirectory = os.path.dirname(os.path.realpath(__file__))

    for benchmark, info in jsonData.items():
        if benchmark != "502.gcc":
#        if benchmark != "526.blender":
            print(benchmark + " skipped")
            continue
        else:
            print(benchmark)

        # Compile all files with clang and jlm-opt
        for cFile in info["cfiles"]:
            compileFile(cFile, False, scriptDirectory)
            compileFile(cFile, True, scriptDirectory)

        # Compile one file at a time with jlm-opt and check if the binary is still working
#        for withJlm in info["cfiles"]:
            # if withJlm['cfile'] == 'haifa-sched.c':
            #     compileFile(withJlm, True, scriptDirectory)
#            if withJlm['cfile'] != "c-parser.c" and withJlm['cfile'] != 'haifa-sched.c':
#                compileFile(withJlm, True, scriptDirectory)
            # for cFile in info["cfiles"]:
            #     if withJlm['cfile'] == cFile['cfile']:
            #         compileFile(cFile, True, scriptDirectory)
            #     else:
            #         compileFile(cFile, False, scriptDirectory)

        # Debug gcc
#        compileFile("c-parser-jlm.c", True, scriptDirectory) 

        os.chdir(scriptDirectory)
#        linkCommand = "clang-18 -no-pie -O0"
        linkCommand = "llvm-link-18"
#        for argument in jsonData[benchmark]['linker_arguments']:
#            linkCommand = linkCommand + " " + argument
        for ofile in jsonData[benchmark]['ofiles']:
            linkCommand = linkCommand + " " + ofile
        linkCommand = linkCommand + " -o " + jsonData[benchmark]['elffile'] 
        print(linkCommand)
        result = subprocess.run(linkCommand.split(" "), check=True)

        runCommand = jsonData[benchmark]['elffile'] + " test.c"
        print(runCommand)
        result = subprocess.run(runCommand.split(" "), check=True)

main()

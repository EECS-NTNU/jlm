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
        clangCommand1 = clangCommand + " -c " + cFile["cfile"] + " -S -emit-llvm -Xclang -disable-O0-optnone -o " + outputFile + ".ll"
        print(clangCommand1)
        result = subprocess.run(clangCommand1.split(" "), check=True, stdout = subprocess.DEVNULL, stderr = subprocess.DEVNULL)
        clangCommand2 = clangCommand + " -c " + cFile["cfile"] + " -Xclang -disable-O0-optnone -O0 -o " + outputFile
        print(clangCommand2)
        result = subprocess.run(clangCommand2.split(" "), check=True, stdout = subprocess.DEVNULL, stderr = subprocess.DEVNULL)

def main():
    scriptDirectory = os.path.dirname(os.path.realpath(__file__))

    for benchmark, info in jsonData.items():
        if benchmark != "502.gcc":
            print(benchmark + " skipped")
            continue
        else:
            print(benchmark)

        # Compile one file at a time with jlm-opt and check if the binary is still working
#        for cFile in info["cfiles"]:
#            if cFile['cfile'] != "c-parser.c" and cFile['cfile'] != 'haifa-sched.c':
#                compileFile(cFile, True, scriptDirectory)
#            else:
#                compileFile(cFile, False, scriptDirectory)

        # Compile one file at a time with jlm-opt and check if the binary is still working
        for cFile in info["cfiles"]:
            if cFile['cfile'] == "c-parser.c" or cFile['cfile'] == 'haifa-sched.c':
                compileFile(cFile, False, scriptDirectory)

        os.chdir(scriptDirectory)
        
        # Link all LLVM-IR files to a single file with llvm-link
        llvmLinkCommand = "llvm-link-18"
        for ofile in jsonData[benchmark]['ofiles']:
            if ofile == 'benchspec/CPU/502.gcc_r/build/build_base_clang-build-m64.0000/c-parser.o' or ofile == "benchspec/CPU/502.gcc_r/build/build_base_clang-build-m64.0000/haifa-sched.o":
                llvmLinkCommand = llvmLinkCommand + " " + ofile + ".jlm-opt.ll"
#                llvmLinkCommand = llvmLinkCommand + " --override " + ofile + ".ll"
        llvmLinkCommand = llvmLinkCommand + " -o " + jsonData[benchmark]['elffile']  + ".ll"
        print(llvmLinkCommand)
        result = subprocess.run(llvmLinkCommand.split(" "), check=True)

        # Compile the LLVM-IR file
#        clangCommand = "clang18 -o " + jsonData[benchmark]['elffile']  + ".ll" + " -Xclang -disable-O0-optnone -O0 -o " +  scriptDirectory + "/" + jsonData[benchmark]['elffile']
        clangCommand = "llc-18 -O0 --relocation-model=static -filetype=obj " + jsonData[benchmark]['elffile']  + ".ll" + " -o " +  jsonData[benchmark]['elffile'] + ".o"
        print(clangCommand)
        result = subprocess.run(clangCommand.split(" "), check=True, stdout = subprocess.DEVNULL, stderr = subprocess.DEVNULL)

        # Link all object files
        linkCommand = "clang-18 -no-pie -O0"
        for argument in jsonData[benchmark]['linker_arguments']:
            linkCommand = linkCommand + " " + argument
        for ofile in jsonData[benchmark]['ofiles']:
            if ofile != 'benchspec/CPU/502.gcc_r/build/build_base_clang-build-m64.0000/c-parser.o' and ofile != "benchspec/CPU/502.gcc_r/build/build_base_clang-build-m64.0000/haifa-sched.o":
                linkCommand = linkCommand + " " + ofile
        linkCommand = linkCommand + " " + jsonData[benchmark]['elffile'] + ".o" 
        linkCommand = linkCommand + " -o " + jsonData[benchmark]['elffile'] 
        result = subprocess.run(linkCommand.split(" "), check=True)

        runCommand = jsonData[benchmark]['elffile'] + " test.c"
        print(runCommand)
        result = subprocess.run(runCommand.split(" "), check=True)

main()

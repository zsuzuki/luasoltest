//
// Copyright 2018 Yoshinori Suzuki<wave.suzuki.z@gmail.com>
//
package main

import (
	"flag"
	"fmt"
	"io/ioutil"
	"log"
	"os"
	"os/exec"
)

const (
	macosPath = "/Applications/Xcode.app/Contents/Developer/"
)

func main() {
	outputName := flag.String("o", "", "output name(base)")
	flag.Parse()

	remainArgs := flag.Args()
	if len(remainArgs) < 1 {
		fmt.Fprintf(os.Stderr, "need header file.\n")
		flag.Usage()
		os.Exit(1)
	}

	cmdline := make([]string, 32)
	cmdline[0] = "./build/default/Debug/extract/luaextract"
	cmdline[1] = remainArgs[0]

	argindex := 1
	if *outputName != "" {
		cmdline[argindex+1] = "-o"
		cmdline[argindex+2] = *outputName
		argindex = argindex + 2
	}
	argindex++
	cmdline[argindex] = "-b"
	argindex++
	cmdline[argindex] = "modules"
	argindex++
	cmdline[argindex] = "--"
	argindex++
	cmdline[argindex] = "-x"
	argindex++
	cmdline[argindex] = "c++"
	argindex++
	cmdline[argindex] = "-std=c++14"
	argindex++
	cmdline[argindex] = "-I./modules"
	argindex++
	cmdline[argindex] = "-I" + macosPath + "Toolchains/XcodeDefault.xctoolchain/usr/include/c++/v1"
	argindex++
	cmdline[argindex] = "-I" + macosPath + "Toolchains/XcodeDefault.xctoolchain/usr/lib/clang/9.0.0/include"
	argindex++
	cmdline[argindex] = "-I" + macosPath + "Toolchains/XcodeDefault.xctoolchain/usr/include"
	argindex++
	cmdline[argindex] = "-I" + macosPath + "Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.13.sdk/usr/include"

	cmd := exec.Command(cmdline[0], cmdline[1:argindex+9]...)

	stdout, err := cmd.StdoutPipe()
	if err != nil {
		log.Fatal(err)
	}
	if err := cmd.Start(); err != nil {
		log.Fatal(err)
	}
	slurp, _ := ioutil.ReadAll(stdout)
	fmt.Printf("%s\n", slurp)
	if err := cmd.Wait(); err != nil {
		log.Fatal(err)
	}

	fmt.Println(*outputName)
}

//
// Copyright 2018 Yoshinori Suzuki<wave.suzuki.z@gmail.com>
//
package main

import (
	"bufio"
	"flag"
	"fmt"
	"io"
	"io/ioutil"
	"log"
	"os"
	"os/exec"
	"strings"
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

	includeDir := []string{}

	// clang setup
	clang := make([]string, 32)
	clang[0] = "-Wp,-v"
	clang[1] = remainArgs[0]
	clangCmd := exec.Command("clang++", clang...)
	stderr, err := clangCmd.StderrPipe()
	if err != nil {
		log.Fatal(err)
		os.Exit(1)
	}
	if err := clangCmd.Start(); err != nil {
		log.Fatal(err)
		os.Exit(1)
	}
	errReader := bufio.NewReader(stderr)
	for {
		cb, _, err := errReader.ReadLine()
		lineStr := string(cb)
		if strings.Index(lineStr, " /") != -1 {
			n := strings.Index(lineStr, " (framework directory)")
			if n != -1 {
				// fmt.Println("Framework: ", lineStr[:n])
				includeDir = append(includeDir, lineStr[1:n])
			} else {
				// fmt.Println(lineStr)
				includeDir = append(includeDir, lineStr[1:])
			}
		}
		if err == io.EOF {
			// fmt.Println("done.")
			break
		}
		if err != nil {
			log.Fatal(err)
			os.Exit(1)
		}
	}

	// extrace command
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
	for _, up := range remainArgs[1:] {
		argindex++
		cmdline[argindex] = "-I" + up
		// fmt.Println("---", up)
	}
	for _, ip := range includeDir {
		argindex++
		cmdline[argindex] = "-I" + ip
		// fmt.Println("***", ip)
	}
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

	fmt.Println("output:", *outputName)
}

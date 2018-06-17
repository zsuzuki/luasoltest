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
	"path/filepath"
	"strings"
)

// clangのシステムインクルードパス等を取得する
func getClangSettings(fname string) []string {
	includeDir := []string{}

	clangCmd := exec.Command("clang++", "-Wp,-v", fname)
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

	return includeDir
}

//
func makeCommandArguments(baseDir string, inputName string, outputName string) []string {
	commandLine := []string{inputName}
	// output filename
	if outputName != "" {
		commandLine = append(commandLine, "-o", outputName)
	}
	// base directory
	if baseDir != "" {
		commandLine = append(commandLine, "-b", baseDir)
	}
	// clang options
	commandLine = append(commandLine, "--", "-x", "c++", "-std=c++17")

	return commandLine
}

//
func addIncludeDir(list []string, modulePath []string, includeDir []string) []string {
	addList := func(al []string) {
		for _, p := range al {
			list = append(list, "-I", p)
		}
	}
	addList(modulePath)
	addList(includeDir)
	return list
}

// Application
//
func main() {
	outputName := flag.String("o", "", "output name(base)")
	commandPath := flag.String("p", "", "luaextract exist directory")
	baseDir := flag.String("b", ".", "modules base directory")
	flag.Parse()

	remainArgs := flag.Args()
	if len(remainArgs) < 1 {
		fmt.Fprintf(os.Stderr, "need header file.\n")
		flag.Usage()
		os.Exit(1)
	}
	inputName := remainArgs[0]

	// clangの情報を取得
	includeDir := getClangSettings(inputName)
	// コマンド名
	commandName := filepath.Join(*commandPath, "luaextract")
	// 解析コマンド
	cmdline := makeCommandArguments(*baseDir, inputName, *outputName)
	incdirs := append(remainArgs[1:], *baseDir)
	cmdline = addIncludeDir(cmdline, incdirs, includeDir)
	cmd := exec.Command(commandName, cmdline...)

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

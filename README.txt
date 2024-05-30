mini shell.

input: 
	string (max length 510) no blank space authorized except space 
	support all regular terminal command except "cd",
	support history command execution use "!<i>" when i is the index of command stored in the file.txt file,
	support pipe using '|' or '||'
	support '&' at end of command (last character only) - run in background
	support 'nohup' at the beginning of the command - the command will be executing and ignoring all signals 	or inputs from the user, the output of the command can be found in "nohup.txt"
	special command "done" summarize total commands and total pipes of the current run and terminate the program	
output: 
	output of the command, or summarize total command, and total word in the current run.
list of file:
	ex3.c
	README.txt
	file.txt [created in the ex3.c folder after first run]
	nohup.txt [created after running 'nohup' command]
	tmp [buffer file, created while program running removed after running]
compilation: 
		gcc ex3.c -o ex3
run: ./ex3
    
by author Elya Athlan.
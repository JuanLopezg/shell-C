# msh – Minimal UNIX Shell  

This project implements a small UNIX-like shell written in C.  
It is mainly focused on understanding process creation, pipes, redirections, and basic shell behavior.  

The shell supports execution of external commands, internal commands, pipes, background execution, and simple expansions.  

---

## Features  

- Execution of external programs using `fork` and `execvp`  
- Built-in commands:  
  - `cd`  
  - `umask`  
  - `time`  
  - `read`  
- Input, output, and error redirection (`<`, `>`, `2>`)  
- Pipes between commands  
- Background execution (`&`)  
- Environment variable expansion (`$VAR`)  
- Home expansion (`~`, `~user`)  
- Wildcard expansion using `?`  
- Special environment variables (`mypid`, `status`, `bgpid`, `prompt`)  

---

## How it works  

- The shell reads commands using a parser (`obtain_order`)  
- Metacharacter substitutions are applied before execution  
- Internal commands are executed inside the shell process when possible  
- External commands are executed using child processes  
- Pipes and redirections are handled with file descriptor duplication  
- Background processes are not waited for and zombies are cleaned periodically  

---

## Prompt  

The default prompt is:  

msh>

yaml
Copiar código

It can be changed using the `prompt` environment variable.  

---

## Notes  

- This is an educational project  
- Error handling is minimal and focused on learning system calls  
- The implementation prioritizes clarity over completeness  

---

## Disclaimer  

This project is for learning purposes only and is not intended to be a full-featured shell.  

#include <iostream>
#include <string>
#include <sys/wait.h>
#include <chrono>
#include <vector>
#include <algorithm>
#include <stdio.h>
#include <unistd.h>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <functional>
#include <signal.h>

const int PIPE_COUNT = 2;
const int PIPE_READ_END = 0;
const int PIPE_WRITE_END = 1;

const int STDIN = 0;
const int STDOUT = 1;

void stringSplit(std::string s, std::vector<std::string>& v)
{
	std::stringstream ss;
	ss << s;

	std::string temp;

	while (std::getline(ss, temp, ' '))
		v.push_back(temp);

	return;
}//stringSplit

void history(std::vector<std::string> v)
{
	for (int i = 0; i < v.size() - 1; i ++)
		std::cout << (i + 1) << ":\t" << v[i] << std::endl;

	return;
}//history

std::string ctrl(int cNum, std::vector<std::string> v)
{
	return v[cNum - 1];
}//ctrl

int cd(std::string adr)
{
	char* newAddress = new char[adr.length() + 1];

	std::strcpy(newAddress, adr.c_str());
	
	char* dir = newAddress;

	return chdir(dir);
}//cd

int findPipe(std::vector<std::string>& v)
{
	for (int i = 0; i < v.size(); i++)
	{
		if (v[i] == "|")
			return i;
	
	}//returns the index value if | is present

	return -1;
}//findPipe

void pipeExec(std::vector<std::string>& left, std::vector<std::string>& right)
{
	int pids[PIPE_COUNT];
	pipe(pids);

	int savedStdout = dup(STDOUT);
	int savedStdin = dup(STDIN);

	pid_t pid = fork();
	std::cout << pid << std::endl;
	if (pid == 0)
	{ 
		dup2(pids[PIPE_WRITE_END], STDOUT);

		char** argv = new char*[left.size() + 1];
		
		for (int i = 0; i < left.size(); i++)
		{
			argv[i] = new char[right[i].length() + 1];
			std::strcpy(argv[i], left[i].c_str());
		}

		argv[left.size()] = NULL;
		execvp(argv[0], argv);
	}
	
	pid_t pid2 = fork();
	if (pid2 == 0)
	{
		dup2(pids[PIPE_READ_END], STDIN);
		
		close(pids[PIPE_WRITE_END]);

		char** argv = new char*[right.size() + 1];

		for (int i = 0; i < right.size(); i++)
		{
			argv[i] = new char[right[i].length() + 1];
			std::strcpy(argv[i], right[i].c_str());
		}

		argv[right.size()] = NULL;
		execvp(argv[0], argv);
	}

	int status;
	waitpid(pid, &status, 0);

	close(pids[PIPE_WRITE_END]);
	close(pids[PIPE_READ_END]);

	waitpid(pid2, &status, 0);

	dup2(savedStdout, STDOUT);
	dup2(savedStdin, STDIN);
}//pipeExec


int main()
{
	bool shell = true;
	bool exec;

	std::string command;
	
	int pipeI;

	std::vector<std::string> argv;
	std::vector<std::string> History;

	std::chrono::time_point<std::chrono::high_resolution_clock> s = std::chrono::high_resolution_clock::now();
	std::chrono::time_point<std::chrono::high_resolution_clock> e = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> totalTime = e - s;

	//displaying the address and sets up for command line input
	std::cout << get_current_dir_name() <<  "⌘\t";
	std::getline(std::cin, command);
	History.push_back(command);
	stringSplit(command, argv);
	pipeI = findPipe(argv);
	signal(SIGINT, SIG_IGN);
	signal(SIGTERM, SIG_DFL);

	//while in the shell
	while(shell)
	{
		exec = true;

		if (argv[0] == "exit")
		{
			shell = false;
			exec = false;

			History.clear();
			argv.clear();
		
			std::cout << "Good-bye" << std::endl << std::endl;

			return EXIT_SUCCESS;
		}//basis check--avoiding fork bombs

		if (argv[0] == "ptime")
		{
			exec = false;
			std::cout << "Total Time: " << totalTime.count() << " seconds" << std::endl;
			History.pop_back();
		}//displays process time

		if (argv[0] == "^")
		{
			int comNum = std::stoi(argv[1]);
			argv.pop_back();
			argv.clear();
			
			std::string temp = ctrl(comNum, History);
			stringSplit(temp, argv);
		}//initializes and executes a command from History
		
		if (argv[0] == "history")
		{
			exec = false;
			history(History);
		}//displays history

		if (argv[0] == "cd")
		{
			exec = false;
			int cdRet = cd(argv[1]);
			
			if (cdRet == -1)
				std::cout << "Invalid address...\nPlease check and try again.\n";
		}//cd set-up and error catching
		
		std::chrono::time_point<std::chrono::high_resolution_clock> start = std::chrono::high_resolution_clock::now();

		if (exec && pipeI == -1)
		{
		       	if (fork())
			{
				wait(NULL);
			
				std::chrono::time_point<std::chrono::high_resolution_clock> end = std::chrono::high_resolution_clock::now();

				std::chrono::duration<double> tempTime = end - start;
				totalTime = totalTime + tempTime;
			}//parent--time

			else
			{
				char** argv2 = new char*[argv.size() + 1];
			
				for (int i = 0; i < argv.size(); i++)
				{
					argv2[i] = new char[1000];
					std::strcpy(argv2[i],argv[i].c_str());
				}

				argv2[argv.size()] = NULL;

				execvp(argv2[0], argv2);
			
				std::cout << "Error! '" << argv[0] << "' not a valid command!" << std::endl;

				argv.clear();

				return EXIT_SUCCESS;
			}//executing proces
		}//if executable params

		//addapted from PipeDemo.cpp as portrayed by Dean Mathias in class
		if (exec && pipeI >= 0)
		{
			std::vector<std::string> l;
			std::vector<std::string> r;

			for (int i = 0; i < pipeI/* + 1*/; i++)
				l.push_back(argv[i].c_str());

			for (int i = pipeI + 1; i < argv.size(); i++)
				r.push_back(argv[i].c_str());
			
			pipeExec(l, r);
		}			
		
		argv.clear();

		std::cout << get_current_dir_name() << "⌘\t";
		std::getline(std::cin, command);
		History.push_back(command);
		stringSplit(command, argv);
		pipeI = findPipe(argv);
		signal(SIGINT, SIG_IGN);
		signal(SIGTERM, SIG_DFL);
	}//while in the shell

	return EXIT_SUCCESS;
}//main


/*
 *
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */

#include "command.h"
#include "tokenizer.h"
#include <cctype>
#include <fcntl.h>
#include <glob.h>
#include <iostream>
#include <signal.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <vector>
#include <wordexp.h>

using namespace std;
void parse(vector<Token> &tokens) {
  Command &currentCommand = Command::_currentCommand;
  // call by ref
  currentCommand.clear();
  SimpleCommand *currentSimpleCommand = new SimpleCommand();

  int size = tokens.size();

  if (size == 0 || tokens[0].type == TOKEN_PIPE ||
      tokens[size - 1] == TOKEN_PIPE) {
    // invalid command
    cout << "invalid command" << endl;
    return;
  }

  for (int i = 0; i < token_size; i++) {
    Token token = tokens[i];

    if (token.type == TOKEN_COMMAND) {
      if (currentSimpleCommand) {
        currentCommand.insertSimpleCommand(currentSimpleCommand);
      }
      currentSimpleCommand = new SimpleCommand();
      char *arg = strdup(token.value.c_str());
      currentSimpleCommand->insertArgument(arg);
    } else if (token.type == TOKEN_ARGUMENT) {
      char *arg = strdup(token.value.c_str());
      currentSimpleCommand->insertArgument(arg);
    } else if (token.type == TOKEN_PIPE) {
      if (!currentSimpleCommand) {
        cerr << "invalid command\n";
        return;
      }
      currentCommand.insertSimpleCommand(currentSimpleCommand);
      currentSimpleCommand = nullptr;
      if (i + 1 >= size) {
        cerr << "invalid command\n";
        currentCommand.clear();
        return;
      }
      if (tokens[i + 1].type == TOKEN_PIPE) {
        cerr << "invalid command\n";
        currentCommand.clear();
        return;
      }
    } else if (token.type == TOKEN_REDIRECT || token.type == TOKEN_APPEND ||
               token.type == TOKEN_INPUT || token.type == TOKEN_ERROR ||
               token.type == TOKEN_REDIRECT_AND_ERROR) {
      if (i + 1 >= size || tokens[i + 1].type != TOKEN_ARGUMENT) {
        cerr << "invalid command\n";
        if (currentSimpleCommand)
          delete currentSimpleCommand;
        currentCommand.clear();
        return;
      }
      {
        i++;
        char *fileNameCstring = tokens[i].value.c_str();
        if (token.type == TOKEN_REDIRECT) {
          currentCommand._outFile = strdup(fileNameCstring);
          currentCommand._append = 0;
        } else if (token.type == TOKEN_APPEND) {
          currentCommand._outFile = strdup(fileNameCstring);
          currentCommand._append = 1;
        } else if (token.type == TOKEN_INPUT)
          currentCommand._inputFile = strdup(fileNameCstring);
        else if (token.type == TOKEN_ERROR)
          currentCommand._errFile = strdup(fileNameCstring);
        else if (token.type == TOKEN_REDIRECT_AND_ERROR) {
          currentCommand._errFile = strdup(fileNameCstring);
          currentCommand._out_error = 1;
          currentCommand._outFile = strdup(fileNameCstring);
          currentCommand._append = 1;
        }
      }
    } else if (token.type == TOKEN_BACKGROUND) {
      currentCommand._background = 1;
      if (i + 1 < size) {
        bool onlyEOForEmpty = true;
        for (int j = i + 1; j < size; ++j) {
          if (tokens[j].type != TOKEN_EOF) {
            onlyEOForEmpty = false;
            break;
          }
        }
        if (!onlyEOForEmpty) {
          cerr << "Syntax error: '&' should appear at the end of the command\n";
          if (currentSimpleCommand)
            delete currentSimpleCommand;
          currentCommand.clear();
          return;
        }
      }
    } else if (token.type == TOKEN_EOF) {
      break;
    } else {
      cerr << "Syntax error: unexpected token '" << token.value << "'\n";
      if (currentSimpleCommand)
        delete currentSimpleCommand;
      currentCommand.clear();
      return;
    }
  }

  if (currentSimpleCommand->_numberOfArguments > 0 || currentSimpleCommand) {
    currentCommand.insertSimpleCommand(currentSimpleCommand);
  } else {
    delete currentSimpleCommand;
  }
  /*
    Implement this function
    you must parse for all possible tokens scenario
    right now command.execute only print the format , you will inject it in your
    logic.

  */
  currentCommand.execute();
  Command::_currentCommand.clear();
}

void child_handler(int sig)
{
    int saved_errno = errno;
    pid_t pid;
    int status;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        FILE *log = fopen("log.txt", "a");
        if (log)
        {
            fprintf(log, "%d\n", pid);
            fclose(log);
        }
    }

    errno = saved_errno;
}

    vector<string> expandWildcards(const string &arg)
    {
        wordexp_t p;
        vector<string> results;

        if (wordexp(arg.c_str(), &p, 0) == 0)
        {
            for (size_t i = 0; i < p.we_wordc; ++i)
            {
                results.push_back(p.we_wordv[i]);
            }
        }
        wordfree(&p);
        if (results.empty())
            results.push_back(arg);
        return results;
    }

SimpleCommand::SimpleCommand() {
  _numberOfAvailableArguments = 5;
  _numberOfArguments = 0;
  _arguments = (char **)malloc(_numberOfAvailableArguments * sizeof(char *));
}

void SimpleCommand::insertArgument(char *argument) {
    string argStr(argument);

    if (argStr.find('*') != string::npos || argStr.find('?') != string::npos)
    {
        wordexp_t p;
        if (wordexp(argument, &p, 0) == 0)
        {
            for (size_t i = 0; i < p.we_wordc; ++i)
            {
                SimpleCommand::insertArgument(strdup(p.we_wordv[i]));
            }
            wordfree(&p);
            return;
        }
    }


  if (_numberOfAvailableArguments == _numberOfArguments + 1) {
    _numberOfAvailableArguments *= 2;
    _arguments = (char **)realloc(_arguments,
                                  _numberOfAvailableArguments * sizeof(char *));
  }

  _arguments[_numberOfArguments] = argument;
  _arguments[_numberOfArguments + 1] = NULL;

  _numberOfArguments++;
}

Command::Command() {
  _numberOfAvailableSimpleCommands = 1;
  _simpleCommands = (SimpleCommand **)malloc(_numberOfSimpleCommands *
                                             sizeof(SimpleCommand *));

  _numberOfSimpleCommands = 0;
  _outFile = 0;
  _inputFile = 0;
  _errFile = 0;
  _background = 0;
}

void Command::insertSimpleCommand(SimpleCommand *simpleCommand) {
  if (_numberOfAvailableSimpleCommands == _numberOfSimpleCommands) {
    _numberOfAvailableSimpleCommands *= 2;
    _simpleCommands = (SimpleCommand **)realloc(
        _simpleCommands,
        _numberOfAvailableSimpleCommands * sizeof(SimpleCommand *));
  }

  _simpleCommands[_numberOfSimpleCommands] = simpleCommand;
  _numberOfSimpleCommands++;
}

void Command::clear() {
  for (int i = 0; i < _numberOfSimpleCommands; i++) {
    for (int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++) {
      free(_simpleCommands[i]->_arguments[j]);
    }

    free(_simpleCommands[i]->_arguments);
    free(_simpleCommands[i]);
  }

  if (_outFile) {
    free(_outFile);
  }

  if (_inputFile) {
    free(_inputFile);
  }

  if (_errFile) {
    free(_errFile);
  }

  _numberOfSimpleCommands = 0;
  _outFile = 0;
  _inputFile = 0;
  _errFile = 0;
  _background = 0;
}

void Command::print() {
  printf("\n\n");
  printf("              COMMAND TABLE                \n");
  printf("\n");
  printf("  #   Simple Commands\n");
  printf("  --- ----------------------------------------------------------\n");

  for (int i = 0; i < _numberOfSimpleCommands; i++) {
    printf("  %-3d ", i);
    for (int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++) {
      printf("\"%s\" \t", _simpleCommands[i]->_arguments[j]);
    }
  }

  printf("\n\n");
  printf("  Output       Input        Error        Err&Out       Background\n");
  printf(
      "  ------------ ------------ ------------ ------------ ------------\n");
  printf("  %-12s %-12s %-12s %-12s %-12s\n", _outFile ? _outFile : "default",
         _inputFile ? _inputFile : "default", _errFile ? _errFile : "default",
         _out_error == 1 ? _errFile : "default", _background ? "YES" : "NO");
  printf("\n\n");
}

void Command::execute() {
  print();
  /*
      Write your code here
  */
    if (_numberOfSimpleCommands == 0)
        return;

    if (strcmp(_simpleCommands[0]->_arguments[0], "exit") == 0)
    {
        printf("Program exiting\n");
        exit(0);
    }
    if (strcmp(_simpleCommands[0]->_arguments[0], "cd") == 0)
    {
        const char *path = (_simpleCommands[0]->_numberOfArguments > 1) ? _simpleCommands[0]->_arguments[1] : getenv("HOME");

        if (chdir(path) == 0)
        {
            printf("Changing to directory '%s'\n", path);
            char cwd[1024];
            if (getcwd(cwd, sizeof(cwd)) != NULL)
                printf("You are now in %s\n", cwd);
        }
        else
        {
            perror("cd failed");
        }
        clear();
        return;
    }

    int tmpin = dup(0);
    int tmpout = dup(1);
    int tmperr = dup(2);
    int fdin;
    if (_inputFile)
    {
        fdin = open(_inputFile, O_RDONLY);
    }
    else
    {
        fdin = dup(tmpin);
    }
    if (fdin < 0)
    {
        perror("Error opening input file");
        clear();
        return;
    }
    for (int i = 0; i < _numberOfSimpleCommands; i++)
    {
        int fdout;
        int fdpipe[2];
        int fdin_next = -1;

        if (i == _numberOfSimpleCommands - 1)
        {
            if (_outFile)
            {
                int flags = O_WRONLY | O_CREAT | (_append ? O_APPEND : O_TRUNC);
                fdout = open(_outFile, flags, 0666);
                if (fdout < 0)
                {
                    perror("Error opening output file");
                    clear();
                    return;
                }
            }
            else
                fdout = tmpout;
        }
        else
        {
            if (pipe(fdpipe) < 0)
            {
                perror("pipe failed");
                clear();
                return;
            }
            fdout = fdpipe[1];
            fdin_next = fdpipe[0];
        }

        pid_t pid = fork();
        if (pid < 0)
        {
            perror("fork failed");
            clear();
            return;
        }
        else if (pid == 0) // child
        {
            dup2(fdin, 0);

            dup2(fdout, 1);

            if (i == _numberOfSimpleCommands - 1)
            {
                if (_errFile && !_out_error)
                {
                    int fderr = open(_errFile, O_WRONLY | O_CREAT | (_append ? O_APPEND : O_TRUNC), 0666);
                    if (fderr < 0)
                    {
                        perror("Error opening error file");
                        exit(1);
                    }
                    dup2(fderr, 2);
                    close(fderr);
                }

                if (_out_error && _errFile)
                {
                    int fderr = open(_errFile, O_WRONLY | O_CREAT | (_append ? O_APPEND : O_TRUNC), 0666);
                    if (fderr < 0)
                    {
                        perror("Error opening error file");
                        exit(1);
                    }
                    dup2(fderr, 1);
                    dup2(fderr, 2);
                    close(fderr);
                }
            }

            if (fdin != tmpin)
                close(fdin);
            if (fdout != tmpout)
                close(fdout);
            if (fdin_next != -1)
                close(fdin_next);

            execvp(_simpleCommands[i]->_arguments[0], _simpleCommands[i]->_arguments);
            perror("Command not found");
            exit(1);
        }
        else
        {
            FILE *log = fopen("log.txt", "a");
            if (log)
            {
                fprintf(log, "%d\n", pid);
                fclose(log);
            }
            if (!_background)
            {
                int status;
                waitpid(pid, &status, 0);
            }
            else
            {
                printf("[Background pid] %d\n", (int)pid);
            }
        }
        if (fdin != tmpin)
            close(fdin);
        if (fdout != tmpout)
            close(fdout);

        fdin = fdin_next;
    }
    dup2(tmpin, 0);
    dup2(tmpout, 1);
    dup2(tmperr, 2);
    close(tmpin);
    close(tmpout);
    close(tmperr);
  clear();
  prompt();
}

void Command::prompt() {
  printf("myshell>");
  fflush(stdout);
  string input;
  while (true) {
    getline(cin, input);
    vector<Token> tokens = tokenize(input);
    parse(tokens);
  }
}

Command Command::_currentCommand;
SimpleCommand *Command::_currentSimpleCommand;

int main() {
  Command::_currentCommand.prompt();
  return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DISK_driver.h"
#include "kernel.h"
#include "memorymanager.h"
#include "ram.h"
#include "shell.h"
#include "shellmemory.h"

#define TRUE 1
#define FALSE 0

/*
This function prints the commands available
*/
int help() {
  printf(
      "========================================================================"
      "===============================\n");
  printf("COMMANDS\t\t\tDESCRIPTIONS\n");
  printf(
      "========================================================================"
      "===============================\n");
  printf("help\t\t\t\t| Displays all commands.\n");
  printf(
      "----------------------------------------------------------------------"
      "---------------------------------\n");
  printf("quit\t\t\t\t| Terminates the shell.\n");
  printf(
      "----------------------------------------------------------------------"
      "---------------------------------\n");
  printf(
      "set VAR STRING\t\t\t| Assigns the value STRING to the shell memory "
      "variable, VAR.\n");
  printf(
      "----------------------------------------------------------------------"
      "---------------------------------\n");
  printf(
      "print VAR\t\t\t| Displays the STRING value assigned to the shell memory "
      "variable, VAR.\n");
  printf(
      "----------------------------------------------------------------------"
      "---------------------------------\n");
  printf("run SCRIPT.TXT\t\t\t| Executes the file SCRIPT.txt.\n");
  printf(
      "----------------------------------------------------------------------"
      "---------------------------------\n");
  printf("exec P1 P2 P3\t\t\t| Executes programs P1 P2 P3 concurrently.\n");
  printf(
      "----------------------------------------------------------------------"
      "---------------------------------\n");
  printf(
      "mount NAME BLOCKS BLOCK_SIZE\t| Mounts an existing partition or"
      "creates a new partition if the\n"
      "\t\t\t\t| partition name does not exist.\n");
  printf(
      "----------------------------------------------------------------------"
      "---------------------------------\n");
  printf(
      "write FILENAME [STRING]\t\t| Writes the inputed string, enclosed by "
      "[], to a file.\n");
  printf(
      "----------------------------------------------------------------------"
      "---------------------------------\n");
  printf(
      "read FILENAME VAR\t\t| Reads the contents of a file into the shell "
      "variable, VAR.\n");
  printf(
      "========================================================================"
      "===============================\n");

  return 0;
}

/*
This function takes an array of string.
First string is the "set" command
Second string is a variable name
Third string is a value
It assigns that value to a environment variable with that variable name in the
shell memory array. Return ERRORCODE -1 if out of memory else 0
*/
int set(char* words[]) {
  char* varName = words[1];
  char* value = words[2];
  int errorCode = setVariable(varName, value);
  return errorCode;
}

/*
This function takes an array of string.
First string is the "print command".
Second string is the variable name.
It will print the value associated with that variable if it exists.
Else it will print an appropriate message.
Return 0 if successful.
*/
int print(char* words[]) {
  char* varName = words[1];
  char* value = getValue(varName);

  if (strcmp(value, "_NONE_") == 0) {
    // If no variable with such name exists, display this message
    printf("Variable does not exist\n");
  } else {
    // else display the variable's value
    printf("%s\n", value);
  }
  return 0;
}

/*
This function takes an array of string.
First string is the "run" command
Second string is script filename to execute
Returns errorCode
*/
static int run(char* words[]) {
  char* filename = words[1];
  FILE* fp = fopen(filename, "r");
  int errorCode = 0;
  // if file cannot be opened, return ERRORCODE -3
  if (fp == NULL) return -3;
  char buffer[1000];
  printf(
      "/////////////////////////////// STARTING EXECUTION OF %s "
      "///////////////////////////////\n",
      filename);
  while (!feof(fp)) {
    fgets(buffer, 999, fp);
    errorCode = parse(buffer);
    // User input the "quit" command. Terminate execution of this script file.
    if (errorCode == 1) {
      // Run command successfully executed so ERRORCODE 0. Stop reading file.
      errorCode = 0;
      break;
    } else if (errorCode != 0) {
      // An error occurred. Display it and stop reading the file.
      // removing the extra carriage return
      buffer[strlen(buffer) - 2] = '\0';
      displayCode(errorCode, buffer);
      break;
    }
  }
  printf(
      "/////////////////////////////// Terminating execution of %s "
      "///////////////////////////////\n",
      filename);
  fclose(fp);
  return 0;
}

int exec(char* words[]) {
  char* filename[3] = {"_NONE_", "_NONE_", "_NONE_"};
  int errorCode = 0;
  for (int i = 0; i < 3; i++) {
    if (strcmp(words[i + 1], "_NONE_") != 0) {
      filename[i] = strdup(words[i + 1]);
      FILE* fp = fopen(filename[i], "r");
      if (fp == NULL) {
        errorCode = -3;
      } else {
        int err = launcher(fp);
        // if launcher failed, set errorCode to -6 for LAUNCHING ERROR
        if (err == 0) {
          errorCode = -6;
        }
      }
      if (errorCode < 0) {
        displayCode(errorCode, words[i]);
        printf("EXEC COMMAND ABORTED...\n");
        emptyReadyQueue();
        clearRAM();
        return 0;
      }
    }
  }

  printf("|----------| ");
  printf("\tSTARTING CONCURRENT PROGRAMS ( ");
  for (int i = 0; i < 3; i++) {
    if (strcmp(filename[i], "_NONE_") != 0) {
      printf("%s ", filename[i]);
    }
  }
  printf(")\t|----------|\n");

  scheduler();

  printf("|----------| ");
  printf("\tTERMINATING ALL CONCURRENT PROGRAMS");
  printf("\t|----------|\n");
  return 0;
}

int mount(char* words[]) {
  int errorCode;
  char partitionPath[100];
  sprintf(partitionPath, "PARTITION/%s.txt", words[1]);
  FILE* f = fopen(partitionPath, "r");

  // create a partition if it doesnt exist
  if (f == NULL) {
    int numOfBlocks = atoi(words[2]);
    int blockSize = atoi(words[3]);

    if (numOfBlocks == 0 || blockSize == 0) return -2;

    errorCode = partition(words[1], blockSize, numOfBlocks);

    if (errorCode == 0) {
      displayCode(-15, words[1]);
      return 0;
    }
  } else {
    fclose(f);
  }

  errorCode = mountFS(words[1]);
  if (errorCode == 0) displayCode(-14, words[1]);

  return 0;
}

int write(char* words[]) {
  // Get value passed
  char value[2048] = "";
  char buffer[512];
  int i = 2;
  while (i != 50 && strcmp(words[i], "_NONE_")) {
    sprintf(buffer, "%s ", words[i]);
    strcat(value, buffer);
    i++;
  }

  // Check data is wrapped in square brackets
  if (value[0] != '[' || value[strlen(value) - 2] != ']') {
    displayCode(-7, "WRAP DATA WITH []");
  }

  // Remove '[' and ']'
  memmove(value, value + 1, strlen(value));
  value[strlen(value) - 2] = '\0';

  // open file (only opens if file does not exist)
  int file = openfile(words[1]);
  if (file < 0) return file;

  // call write driver
  int blockSize = getBlockSize();
  char writeBuffer[blockSize];
  for (int j = 0; j < strlen(value); j += blockSize) {
    strncpy(writeBuffer, &value[j], blockSize);

    // write block to file
    int errorCode = writeBlock(file, writeBuffer);
    if (errorCode == -11) {
      printf(
          "WARNING : Partition data is full, the first %d bytes were "
          "written to '%s'\n",
          j, words[1]);
      break;
    } else if (errorCode < 0) {
      return errorCode;
    }
  }

  return 0;
}

int read(char* words[]) {
  // open file (only opens if file does not exist)
  int file = openfile(words[1]);
  if (file < 0) return file;

  // read block from file
  char* data = readBlock(file);
  if (data == NULL) {
    displayCode(-16, "");
    return 0;
  }

  // set variable to data read from file
  int errorCode = setVariable(words[2], data);
  return errorCode;
}

/*
This functions takes a parsed version of the user input.
It will interpret the valid commands or return a bad error code if the command
failed for some reason Returns: ERRORCODE  0 : No error and user wishes to
continue ERRORCODE  1 : Users wishes to quit the shell / terminate script
ERRORCODE -1 : RAN OUT OF SHELL MEMORY
ERRORCODE -2 : INCORRECT NUMBER OF ARGUMENTS
ERRORCODE -3 : FILE DOES NOT EXIST
ERRORCODE -4 : UNKNOWN COMMAND. TRY "help"
*/
int interpreter(char* words[]) {
  // default errorCode if no error occurred AND user did not enter the "quit"
  // command
  int errorCode = 0;
  // At this point, we are checking for each possible commands entered
  if (strcmp(words[0], "_NONE_") == 0 || words[0][0] == '#') {
    // if it's an empty line, return
    return 0;
  } else if (strcmp(words[0], "help") == 0) {
    // if it's the "help" command, we display the description of every commands
    help();
  } else if (strcmp(words[0], "quit") == 0) {
    // if it's the "quit" command
    // errorCode is 1 when user voluntarily wants to quit the program.
    errorCode = 1;
  } else if (strcmp(words[0], "set") == 0) {
    // if it's the "set VAR STRING" command
    // check for the presence or 2 more arguments
    // If one argument missing, return ERRORCODE -2 for invalid number of
    // arguments
    if ((strcmp(words[1], "_NONE_") == 0) ||
        (strcmp(words[2], "_NONE_") == 0)) {
      errorCode = -2;
    } else {
      // ERRORCODE -1 : Out of Memory might occur
      errorCode = set(words);
    }
  } else if (strcmp(words[0], "print") == 0) {
    // if it's the "print VAR" command
    // if there's no second argument, return ERRORCODE -2 for invalid number of
    // arguments
    if (strcmp(words[1], "_NONE_") == 0) return -2;

    // Call the print function
    errorCode = print(words);

  } else if (strcmp(words[0], "run") == 0) {
    // if it's the "run SCRIPT.TXT" command
    // check if there's a second argument, return ERRORCODE -2 for invalid
    // number of arguments
    if (strcmp(words[1], "_NONE_") == 0) return -2;

    // Error will be handled in the run function. We can assume that after the
    // run function terminate, the errorCode is 0.
    errorCode = run(words);
  } else if (strcmp(words[0], "exec") == 0) {
    // if it's the "exec" command
    // check if there's at least 2 arguments and not >= 4 arguments
    if (strcmp(words[1], "_NONE_") == 0 || strcmp(words[4], "_NONE_") != 0)
      return -2;

    errorCode = exec(words);
  } else if (strcmp(words[0], "mount") == 0) {
    // if it's the "mount" command
    // check if there's 4 arguments
    if (strcmp(words[3], "_NONE_") == 0 || strcmp(words[4], "_NONE_") != 0)
      return -2;

    errorCode = mount(words);
  } else if (strcmp(words[0], "write") == 0) {
    // if it's the "write" command
    // check if there's at least 3 arguments
    if (strcmp(words[1], "_NONE_") == 0 || strcmp(words[2], "_NONE_") == 0)
      return -2;

    errorCode = write(words);
  } else if (strcmp(words[0], "read") == 0) {
    // if it's the "read" command
    // check if there's 3 arguments
    if (strcmp(words[2], "_NONE_") == 0 || strcmp(words[3], "_NONE_") != 0)
      return -2;

    errorCode = read(words);
  } else {
    // Error code for unknown command
    errorCode = -4;
  }

  return errorCode;
}
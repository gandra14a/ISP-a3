//-----------------------------------------------------------------------------
// a3.c
//
// A program for the third assignment of ESP (WS 21/22), which implements
// a simplified version of a famous game "Scrabble".
//
// Group: 3 (David Andrawes)
//
// Author: 01431708
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <string.h>
#include "framework.h"

#define MAX_INPUT_LENGTH 50  // maximum length for the command input
#define FILE_PATH_ARGUMENT 1 // second argument with index 1 represents the path to the config file
#define LOWER_UPPER_DIFFERENCE 32 // used to convert the lowercase to uppercase letters or vice versa


typedef struct _Tile_
{
  char letter_;
  int points_;
} Tile;



int checkConfigFile(int number_of_arguments, char* file_path);

int fieldSize(FILE* fp);

int getMaximumPoints(int field_width);

int getField(FILE* fp, char* field[], int field_width);

int getNumberOfTiles(FILE* fp, int* last_line_length);

int getUsableTiles(FILE* fp, Tile* tiles[], int number_of_tiles, int* last_line_length);

int getPlayerOnTheMove(FILE* fp);

int getFirstPlayerPoints(FILE* fp);

int getSecondPlayerPoints(FILE* fp);

void freeFieldMemory(char* field[],  int field_width);

void freeTilesMemory(Tile* tiles[], int number_of_tiles);

void printScreen(char* field[], int field_width, Tile* tiles[], int number_of_tiles,
                 int player_1_points, int player_2_points);

void printHelp(void);

int insertCommand(char* field[], int field_width, Tile* tiles[], int number_of_tiles, int* player_on_the_move,
                  int* player_1_points, int* player_2_points, char row, char column,
                  Orientation orientation, char* word, int* double_used, int* double_active);

void saveCommand(FILE* fp, char* field[], int field_width, Tile* tiles[], int number_of_tiles,
                 int player_on_the_move, int player_1_points, int player_2_points);

Input askForCommand(FILE* fp, char* field[], int field_width, Tile* tiles[],
                    int number_of_tiles, int player_on_the_move, int* return_value);

int executeCommand(FILE* fp, char* field[], int field_width, Tile* tiles[], int number_of_tiles,
                   int* player_on_the_move, int* player_1_points, int* player_2_points, int max_points, Input input,
                   int* double_used, int* double_active);

void endWithWinner(FILE* fp, char* field[], int field_width, Tile* tiles[], int number_of_tiles,
                   int player_1_points, int player_2_points);



//------------------------------------------------------------------------------
///
/// The main program.
/// Reads an existing "Scrabble" game position from a file and asks for commands
/// to perform respective game moves.
///
///
/// @param argc number of command line arguments
/// @param argv vector of command line arguments; second argument should be a path to the config file
///
/// @return 0 if no problems occur, otherwise some value from 1 to 4
//
int main(int argc, char* argv[])
{


  int return_value_main = checkConfigFile(argc, argv[FILE_PATH_ARGUMENT]);

  if (return_value_main != 0)
  {
    return return_value_main;
  }

  FILE* file_pointer = fopen(argv[FILE_PATH_ARGUMENT], "r+");

  int field_width_var = fieldSize(file_pointer);

  char* playing_field[field_width_var];
  //char* playing_field = malloc(field_width_var*sizeof(char*));

  return_value_main = getField(file_pointer, playing_field, field_width_var);

  if (return_value_main != 0)
  {
    return return_value_main;
  }

  int player_on_the_move_var = getPlayerOnTheMove(file_pointer);
  int first_player_points = getFirstPlayerPoints(file_pointer);
  int second_player_points = getSecondPlayerPoints(file_pointer);

  int length_of_the_last_line = 0; // used to go back to the beginning of the last line
  int number_of_usable_tiles = getNumberOfTiles(file_pointer, &length_of_the_last_line);

  Tile* usable_tiles[number_of_usable_tiles];
  //Tile* usable_tiles = malloc(number_of_usable_tiles*sizeof(Tile*));

  return_value_main = getUsableTiles(file_pointer, usable_tiles, number_of_usable_tiles, &length_of_the_last_line);

  if (return_value_main != 0)
  {
    freeFieldMemory(playing_field, field_width_var);
    return return_value_main;
  }

  printScreen(playing_field, field_width_var, usable_tiles, number_of_usable_tiles, first_player_points,
              second_player_points);


  int maximum_points = getMaximumPoints(field_width_var);

  int double_used = 0;
  int double_active = 0;


  while (first_player_points < maximum_points && second_player_points < maximum_points)
  {

    Input input_instance = askForCommand(file_pointer, playing_field, field_width_var, usable_tiles,
                                         number_of_usable_tiles, player_on_the_move_var, &return_value_main);


    // in case of control+D we set return_value to 5 and use it to return 0 from the main 
    if (return_value_main == 5) 
    {
      return 0;
    }

    return_value_main = executeCommand(file_pointer, playing_field, field_width_var, usable_tiles,
                                       number_of_usable_tiles, &player_on_the_move_var, &first_player_points,
                                       &second_player_points, maximum_points, input_instance, &double_used,
                                       &double_active);

    // in case of QUIT command, the executeCommand returns 6, which we use to return 0 from the main
    if (return_value_main == 6) 
    {
      return 0;
    }
  }

  endWithWinner(file_pointer, playing_field, field_width_var, usable_tiles,
                number_of_usable_tiles, first_player_points, second_player_points);

  return return_value_main;

}



//------------------------------------------------------------------------------
///
/// Checks if correct number of command line arguments is passed, trys to open the
/// file and checks if the opened file is a valid config file.
/// At the end, the opened file is again closed.
///
///
/// @param number_of_arguments number of command line arguments
/// @param file_path path to the desired config file
///
/// @return 0 if no problems occur, otherwise some value from 1 to 3
//
int checkConfigFile(int number_of_arguments, char* file_path)
{
  if (number_of_arguments != 2)
  {
    printf("Usage: ./a3 configfile\n");
    return 1;
  }
  else
  {

    FILE* fp = fopen(file_path, "r+");
    if (fp == NULL)
    {
      printf("Error: Cannot open file: %s\n", file_path);
      return 2;
    }
    else
    {
      int magic_number_length = 0;
      while (fgetc(fp) != '\n')
      {
        magic_number_length = magic_number_length + 1;
      }

      char magic_number[magic_number_length + 1];
      char valid_magic_number[] = "Scrabble";
      fseek(fp, 0, SEEK_SET);
      fgets(magic_number, magic_number_length + 1, fp);
      if (strcmp(magic_number, valid_magic_number))
      {
        printf("Error: Invalid file: %s\n", file_path);
        fclose(fp);
        return 3;
      }
      fclose(fp);
      return 0;
    }
  }
}
//------------------------------------------------------------------------------
///
/// Reveals the size of the playing field.
///
///
/// @param fp file pointer to the config file
///
/// @return field width
///
//
int fieldSize(FILE* fp)
{
  fseek(fp, 0, SEEK_SET);
  while (fgetc(fp) != '\n')
  {

  }
  int field_width = 0;
  while (fgetc(fp) != '\n')
  {
    field_width = field_width + 1;
  }
  return field_width;
}
//------------------------------------------------------------------------------
///
/// Calculates a maximum number of points to be achieved in the game.
///
///
/// @param field_width width and also height of the playing field
///
/// @return calculated number of maximum points
///
//
int getMaximumPoints(int field_width)
{
  return (field_width * field_width / 2);
}
//------------------------------------------------------------------------------
///
/// Takes the playing field from the config file and puts it into a string array
/// on the heap.
///
///
/// @param fp file pointer to the config file
/// @param field vector of strings that holds the current playing field on the heap
/// @param field_width width and also height of the playing field
///
/// @return 0 if no problems occur, 4 if there is not enough memory on the heap
///
//
int getField(FILE* fp, char* field[], int field_width)
{
  fseek(fp, 0 ,SEEK_SET);
  while (fgetc(fp) != '\n')
  {

  }


  for (int row = 0; row < field_width; row++)
  {
    field[row] = (char*) malloc((field_width + 1) * sizeof(char));
    if (field[row] == NULL)
    {
      printf("Error: Out of memory\n");
      for (int row_new = 0; row_new < row - 1; row_new++)
      {
        free(field[row_new]);
      }
      return 4;
    }
    else
    {
      fgets(field[row], field_width + 1, fp);
      fseek(fp, 1, SEEK_CUR);
    }
  }
  return 0;
}
//------------------------------------------------------------------------------
///
/// Reaveals the number of useable tiles, which are given in the config file
///
///
/// @param fp file pointer to the config file
/// @param last_line_length length of the last line of the config file
/// 
/// @return number of the given tiles from the config file
///
//
int getNumberOfTiles(FILE* fp, int* last_line_length)
{
  int number_of_tiles = 0;

  char last_line_character = fgetc(fp);
  while (last_line_character != EOF)
  {
    *last_line_length = *last_line_length + 1;
    if (last_line_character == ' ')
    {
      number_of_tiles = number_of_tiles + 1;
    }
    last_line_character = fgetc(fp);
  }

  number_of_tiles = number_of_tiles + 1;

  return number_of_tiles;
}
//------------------------------------------------------------------------------
///
/// Takes all given tiles from the config file and puts them into an array of
/// type Tile*. Each tile is represented as a instance of a struct Tile, that has
/// 2 members, namely letter and points. All tiles are on the heap.
///
///
/// @param fp file pointer to the config file
/// @param tiles vector of pointers of type Tile, that holds all tiles on the heap
/// @param number_of_tiles number of all tiles
/// @param last_line_length length of the last line of the config file
///
/// @return 0 if no problems occur, 4 if there is not enough memory on the heap
///
//
int getUsableTiles(FILE* fp, Tile* tiles[], int number_of_tiles, int* last_line_length)
{
  fseek(fp, -(*last_line_length), SEEK_END);


  for (int tile_counter = 0; tile_counter < number_of_tiles; tile_counter++)
  {
    tiles[tile_counter] = (Tile*) malloc(sizeof(Tile));
    if (tiles[tile_counter] == NULL)
    {
      printf("Error: Out of memory\n");
      for (int tile_counter_new = 0; tile_counter_new < tile_counter; tile_counter_new++)
      {
        free(tiles[tile_counter_new]);
      }
      return 4;
    }
    tiles[tile_counter] -> letter_ = fgetc(fp);
    tiles[tile_counter] -> points_ = fgetc(fp) - '0';
    fgetc(fp);

  }
 return 0;
}
//------------------------------------------------------------------------------
///
/// Reaveal the player that is on the move, which is given in the config file.
///
///
/// @param fp file pointer to the config file
///
/// @return 1 if the first player is on the move, 2 if the second player is on the move
///
//
int getPlayerOnTheMove(FILE* fp)
{
  int player_on_the_move = fgetc(fp) - '0';
  fseek(fp, 1, SEEK_CUR);
  return player_on_the_move;
}
//------------------------------------------------------------------------------
///
/// Reveals the points achieved by the first player so far. Reads it from the 
/// config file.
/// 
///
/// @param fp file pointer to the config file
/// 
/// @return number of points of the first player
///
//
int getFirstPlayerPoints(FILE* fp)
{
  int number_of_digits = 0;
  while (fgetc(fp) != '\n')
  {
    number_of_digits = number_of_digits + 1;
  }
  fseek(fp, -number_of_digits - 1, SEEK_CUR);
  int first_player_points = 0;
  switch (number_of_digits)
  {
  case 1:
    first_player_points = fgetc(fp) - '0';
    break;
  case 2:
    first_player_points = 10 * (int) (fgetc(fp) - '0') + (int) (fgetc(fp) - '0');
    break;
  case 3:
    first_player_points = 100 * (int) (fgetc(fp) - '0') + 10 * (int) (fgetc(fp) - '0') + (int) (fgetc(fp) - '0');
    break;
  }
  fseek(fp, 1, SEEK_CUR);
  return first_player_points;
}
//------------------------------------------------------------------------------
///
/// Reveals the points achieved by the second player so far. Reads it from the
/// config file.
///
///
/// @param fp file pointer to the config file
/// 
/// @return number of points of the second player
///
//
int getSecondPlayerPoints(FILE* fp)
{
  int number_of_digits = 0;
  while (fgetc(fp) != '\n')
  {
    number_of_digits = number_of_digits + 1;
  }
  fseek(fp, -number_of_digits - 1, SEEK_CUR);
  int layer_2_points = 0;
  switch (number_of_digits)
  {
  case 1:
    layer_2_points = fgetc(fp) - '0';
    break;
  case 2:
    layer_2_points = 10 * (int) (fgetc(fp) - '0') + (int) (fgetc(fp) - '0');
    break;
  case 3:
    layer_2_points = 100 * (int) (fgetc(fp) - '0') + 10 * (int) (fgetc(fp) - '0') + (int) (fgetc(fp) - '0');
    break;
  }
  fseek(fp, 1, SEEK_CUR);
  return layer_2_points;
}
//------------------------------------------------------------------------------
///
/// Frees the memory on the heap, that is allocated for the playing field
/// 
/// 
/// @param field vector of strings that holds the current playing field on the heap
/// @param field_width width and also height of the playing field
///
//
void freeFieldMemory(char* field[], int field_width)
{
  for (int row = 0; row < field_width; row++)
  {
    free(field[row]);
  }
}
//------------------------------------------------------------------------------
///
/// Frees the memory on the heap, that is alocated for tiles
/// 
/// 
/// @param tiles vector of pointers of type Tile, that holds all tiles on the heap
/// @param number_of_tiles number of all tiles
///
//
void freeTilesMemory(Tile* tiles[], int number_of_tiles)
{
  for (int tile = 0; tile < number_of_tiles; tile++)
  {
    free(tiles[tile]);
  }

}
//------------------------------------------------------------------------------
///
/// Prints the current situation on the playing field and also the number of each
/// players points. This function is called after each valid command.
/// 
///
/// @param field vector of strings that holds the current playing field on the heap
/// @param field_width width and also height of the playing field
/// @param tiles vector of pointers of type Tile, that holds all tiles on the heap
/// @param number_of_tiles number of all tiles
/// @param player_1_points number of points achieved by the first player
/// @param player_2_points number of points achieved by the second player
///
//
void printScreen(char* field[], int field_width, Tile* tiles[], int number_of_tiles,
                 int player_1_points, int player_2_points)
{
  for (int tile = 0; tile < number_of_tiles; tile++)
  {
    if ((tile % 9) == 0)
    {
      printf("\n");
    }
    if (tile == (number_of_tiles - 1))
    {
      printf("%c%d", tiles[tile] -> letter_ - LOWER_UPPER_DIFFERENCE, tiles[tile] -> points_);
    } 
    else
    {
      printf("%c%d, ", tiles[tile] -> letter_ - LOWER_UPPER_DIFFERENCE, tiles[tile] -> points_);
    }
  }

  if (player_1_points < 10) 
  {
    printf("\n  P1:    %d Points\n", player_1_points);
  }
  else if (player_1_points < 100)
  {
    printf("\n  P1:   %d Points\n", player_1_points);
  }
  else
  {
    printf("\n  P1:  %d Points\n", player_1_points);
  }

  if (player_2_points < 10)
  {
    printf("  P2:    %d Points\n", player_2_points);
  }
  else if (player_2_points < 100)
  {
    printf("  P2:   %d Points\n", player_2_points);
  }
  else
  {
    printf("  P2:  %d Points\n", player_2_points);
  }

  printf(" |");
  for (int letter = 65; letter < 65 + field_width; letter++)
  {
    printf("%c", letter);
  }
  printf("\n");
  printf("--");
  for (int dash = 0; dash < field_width; dash++)
  {
    printf("-");
  }
  printf("\n");
  for (int row = 0; row < field_width; row++)
  {
    printf("%c|", row + 65);
    printf("%s\n", field[row]);
  }
  printf("\n");

}
//------------------------------------------------------------------------------
///
/// Prints the text of the help command.
/// 
//
void printHelp(void)
{
  printf("Commands:\n");
  printf(" - insert <ROW> <COLUMN> <H/V> <WORD>\n");
  printf("    <H/V> stands for H: horizontal, V: vertical.\n\n");
  printf(" - help\n");
  printf("    Prints this help text.\n\n");
  printf(" - quit\n");
  printf("    Terminates the game.\n\n");
  printf(" - save\n");
  printf("    Saves the game to the current config file.\n\n");
  printf(" - load <CONFIGFILE>\n");
  printf("    load config file and start game.\n");
}
//------------------------------------------------------------------------------
///
/// Checks if the insert command is correct regarding the move. If the move does 
/// not follow the rules of the game, this function prints error message and returns
/// 0, otherwise updates the playing field, the player that is on the move and also
/// the points of the player that just played and returns 1
///
///
/// @param field vector of strings that holds the current playing field on the heap
/// @param field_width width and also height of the playing field
/// @param tiles vector of pointers of type Tile, that holds all tiles on the heap
/// @param number_of_tiles number of all tiles
/// @param player_on_the_move current player that is on the move
/// @param player_1_points number of points achieved by the first player
/// @param player_2_points number of points achieved by the second player
/// @param row row of the first letter of the word
/// @param column column of the first letter of the word
/// @param orientation vertical or horizontal orientation of the word
/// @param word the new word
///
/// @return 0 if invalid move, 1 if valid
///
//
int insertCommand(char* field[], int field_width, Tile* tiles[], int number_of_tiles, int* player_on_the_move,
                  int* player_1_points, int* player_2_points, char row, char column,
                  Orientation orientation, char* word, int* double_used, int* double_active)
{


  int word_length = 0;
  for (int letter = 0; word[letter] != '\0'; letter++)
  {
    word_length = word_length + 1;
  }

  for (int letter = 0; letter < word_length; letter++)
  {
    if (word[letter] < 'a' || word[letter] > 'z')
    {
      printf("Error: Impossible move!\n");
      return 0;
    }
  }
  int new_points = 0;
  int letter_points[word_length];
  int valid_letter;
  for (int letter = 0; letter < word_length; letter++)
  {
    valid_letter = 0;
    for (int tile = 0; tile < number_of_tiles; tile++)
    {
      if (word[letter] == tiles[tile] -> letter_)
      {
        valid_letter = 1;
        letter_points[letter] = tiles[tile] -> points_;
        break;
      }
    }
    if (valid_letter == 0)
    {
      printf("Error: Impossible move!\n");
      return 0;
    }

  }

  int new_letters = 0;

  if (orientation == HORIZONTAL)
  {
    int starting_point = (column - LOWER_UPPER_DIFFERENCE)  - 'A';
    int row_index = (row - LOWER_UPPER_DIFFERENCE) - 'A';
    char current_row[field_width];
    for (int column = 0; column < field_width; column++)
    {
      current_row[column] = field[row_index][column];
    }
    if (starting_point + word_length > field_width)
    {
      printf("Error: Impossible move!\n");
      return 0;
    }
    else if ((*player_1_points + *player_2_points) == 0)
    {
      for (int horizontal_position = starting_point; horizontal_position < starting_point + word_length;
          horizontal_position++)
      {
        field[row_index][horizontal_position] = word[horizontal_position - starting_point] - LOWER_UPPER_DIFFERENCE;
        new_letters = new_letters + 1;
        new_points = new_points + letter_points[horizontal_position - starting_point];
      }

    }
    else
    {
      for (int horizontal_position = starting_point; horizontal_position < starting_point + word_length;
          horizontal_position++)
      {
        if (field[row_index][horizontal_position] == ' ')
        {
          field[row_index][horizontal_position] = word[horizontal_position - starting_point] - LOWER_UPPER_DIFFERENCE;
          new_letters = new_letters + 1;
          new_points = new_points + letter_points[horizontal_position - starting_point];
        }
        else
        {

          if ((field[row_index][horizontal_position] + LOWER_UPPER_DIFFERENCE) != word[horizontal_position - starting_point])
          {
            printf("Error: Impossible move!\n");
            for (int column = 0; column < field_width; column++)
            {
              field[row_index][column] = current_row[column];
            }
            return 0;
          }
        }
      }
      if (new_letters == word_length)
      {
        printf("Error: Impossible move!\n");
        for (int column = 0; column < field_width; column++)
        {
          field[row_index][column] = current_row[column];
        }
      return 0;
      }
    }
  }
  else if (orientation == VERTICAL)
  {
    int starting_point = (row - LOWER_UPPER_DIFFERENCE) - 'A';
    int column_index = (column - LOWER_UPPER_DIFFERENCE) - 'A';
    char current_column[field_width];
    for (int row = 0; row < field_width; row++)
    {
      current_column[row] = field[row][column_index];
    }
    if (starting_point + word_length > field_width)
    {
      printf("Error: Impossible move!\n");
      return 0;
    }
    else if ((*player_1_points + *player_2_points) == 0)
    {
      for (int vertical_position = starting_point; vertical_position < starting_point + word_length;
          vertical_position++)
      {
        field[vertical_position][column_index] = word[vertical_position - starting_point] - LOWER_UPPER_DIFFERENCE;
        new_letters = new_letters + 1;
        new_points = new_points + letter_points[vertical_position - starting_point];
      }
    }
    else
    {
      int new_letters = 0;
      for (int vertical_position = starting_point; vertical_position < starting_point + word_length;
          vertical_position++)
      {
        if (field[vertical_position][column_index] == ' ')
        {
          field[vertical_position][column_index] = word[vertical_position - starting_point] - LOWER_UPPER_DIFFERENCE;
          new_letters = new_letters + 1;
          new_points = new_points + letter_points[vertical_position - starting_point];
        }
        else
        {
          if ((field[vertical_position][column_index] + LOWER_UPPER_DIFFERENCE) != word[vertical_position - starting_point])
          {
            printf("Error: Impossible move!\n");
            for (int row = 0; row < field_width; row++)
            {
              field[row][column_index] = current_column[row];
            }
            return 0;
          }
        }
      }
      if (new_letters == word_length)
      {
        printf("Error: Impossible move!\n");
        for (int row = 0; row < field_width; row++)
        {
          field[row][column_index] = current_column[row];
        }
        return 0;
      }
    }

  }

  switch (*player_on_the_move)
  {
  case 1:
    if (*double_active == 1)
    {
      *player_1_points = *player_1_points + 2*new_points;
      *player_on_the_move = 2;
      *double_active = 0;
    }
    else
    {
      *player_1_points = *player_1_points + new_points;
      *player_on_the_move = 2;
    }

    break;
  case 2:
    if (*double_active == 1)
    {
      *player_2_points = *player_2_points + 2*new_points;
      *player_on_the_move = 1;
      *double_active = 0;
    }
    else
    {
      *player_2_points = *player_2_points + new_points;
      *player_on_the_move = 1;
    }
    break;

  }
  return 1;
}
//------------------------------------------------------------------------------
///
/// Saves current playing field, player on the move and both players points on
/// the config file.
/// 
/// 
/// @param fp file pointer to the config file
/// @param field vector of strings that holds the current playing field on the heap
/// @param field_width width and also height of the playing field
/// @param tiles vector of pointers of type Tile, that holds all tiles on the heap
/// @param number_of_tiles number of all tiles
/// @param player_on_the_move current player that is on the move
/// @param player_1_points number of points achieved by the first player
/// @param player_2_points number of points achieved by the second player
///
//
void saveCommand(FILE* fp, char* field[], int field_width, Tile* tiles[], int number_of_tiles,
                 int player_on_the_move, int player_1_points, int player_2_points)
{
  fseek(fp, 0, SEEK_SET);
  while (fgetc(fp) != '\n')
  {

  }
  for (int row = 0; row < field_width; row++)
  {
    for (int column = 0; column < field_width; column++)
    {
      fputc(field[row][column], fp);
    }
    fputc('\n', fp);
  }

  char player = player_on_the_move + '0';
  fputc(player, fp);
  fputc('\n', fp);

  if (player_1_points < 10)
  {
    char points_player_1 = player_1_points + '0';
    fputc(points_player_1, fp);
    fputc('\n', fp);
  }
  else if (player_1_points < 100)
  {
    char first_digit = (player_1_points / 10) + '0';
    char second_digit = (player_1_points % 10) + '0';

    fputc(first_digit, fp);
    fputc(second_digit, fp);
    fputc('\n', fp);
  }
  else if (player_1_points < 1000)
  {
    char first_digit = (player_1_points / 100) + '0';
    char second_digit = ((player_1_points / 10) % 10) + '0';
    char third_digit = (player_1_points % 10) + '0';

    fputc(first_digit, fp);
    fputc(second_digit, fp);
    fputc(third_digit, fp);
    fputc('\n', fp);

  }

  if (player_2_points < 10)
  {
    char points_player_2 = player_2_points + '0';
    fputc(points_player_2, fp);
    fputc('\n', fp);
  }
  else if (player_2_points < 100)
  {
    char first_digit = (player_2_points / 10) + '0';
    char second_digit = (player_2_points % 10) + '0';

    fputc(first_digit, fp);
    fputc(second_digit, fp);
    fputc('\n', fp);
  }
  else if (player_2_points < 1000)
  {
    char first_digit = (player_2_points / 100) + '0';
    char second_digit = ((player_2_points / 10) % 10) + '0';
    char third_digit = (player_2_points % 10) + '0';

    fputc(first_digit, fp);
    fputc(second_digit, fp);
    fputc(third_digit, fp);
    fputc('\n', fp);
  }

  for (int tile = 0; tile < number_of_tiles; tile++)
  {
    char letter = tiles[tile] -> letter_;
    char points = (tiles[tile] -> points_) + '0';

    fputc(letter, fp );
    fputc(points, fp);
    if (tile < number_of_tiles - 1)
    {
      fputc(' ', fp);
    }
  }

}
//------------------------------------------------------------------------------
///
/// Asks for command and checks if the given command is valid regarding the
/// parameters. Also checks if the command is control+D and in case of that
/// changes the return value of the main function to 5. In case of a valid
/// command it returns an instance of struct Input that holds the information
/// of the current command.
/// 
///
/// @param fp file pointer to the config file
/// @param field vector of strings that holds the current playing field on the heap
/// @param field_width width and also height of the playing field
/// @param tiles vector of pointers of type Tile, that holds all tiles on the heap
/// @param number_of_tiles number of all tiles
/// @param player_on_the_move current player that is on the move
/// @param return_value return value of the main function
///
/// @return input struct instance, that holds info about the command
///
//
Input askForCommand(FILE* fp, char* field[], int field_width, Tile* tiles[],
                    int number_of_tiles, int player_on_the_move, int* return_value)
{
  char stdin_line[MAX_INPUT_LENGTH];
  Input input;
  int invalid_command = 1;
  while (invalid_command)
  {
    printf("Player %d > ", player_on_the_move);
    if (fgets(stdin_line, MAX_INPUT_LENGTH, stdin) == NULL)
    {
      fclose(fp);
      freeFieldMemory(field, field_width);
      freeTilesMemory(tiles, number_of_tiles);
      *return_value = 5;
      invalid_command = 0;
    }
    else
    {
      parseCommand(stdin_line, &input);

      if (input.row_ < 'a' || input.row_ > 'z')
      {
        input.is_error_ = 1;
      }

      if (input.column_ < 'a' || input.column_ > 'z')
      {
        input.is_error_ = 1;
      }

      if (input.orientation_ != HORIZONTAL && input.orientation_ != VERTICAL)
      {
        input.is_error_ = 1;
      }

      if (input.only_whitespaces_ == 1)
      {

      }
      else if (input.command_ == UNKNOWN)
      {
        printf("Error: Unknown command: %s\n", stdin_line);
      }
      else if (input.command_ == INSERT && input.is_error_ == 1)
      {
        printf("Error: Insert parameters not valid!\n");
        free(input.word_);
      }
      else
      {
        invalid_command = 0;
      }
    }
  }
  return input;

}
//------------------------------------------------------------------------------
///
/// Based on the given valid command, this function decides which function to 
/// be called.
/// 
///
/// @param fp file pointer to the config file
/// @param field vector of strings that holds the current playing field on the heap
/// @param field_width width and also height of the playing field
/// @param tiles vector of pointers of type Tile, that holds all tiles on the heap
/// @param number_of_tiles number of all tiles
/// @param player_on_the_move current player that is on the move
/// @param player_1_points number of points achieved by the first player
/// @param player_2_points number of points achieved by the second player
/// @param max_points maximum number of points to be achieved
/// @param input input struct instance, that holds info about the command
///
/// @return 0 if any command other than QUIT, 6 if QUIT
///
//
int executeCommand(FILE* fp, char* field[], int field_width, Tile* tiles[], int number_of_tiles,
                   int* player_on_the_move, int* player_1_points, int* player_2_points, int max_points, Input input,
                   int* double_used, int* double_active)
{
  int return_value = 0;
  int valid_move;

  switch (input.command_)
  {
    case INSERT:
      valid_move = insertCommand(field, field_width, tiles, number_of_tiles, player_on_the_move,
                                 player_1_points, player_2_points, input.row_, input.column_,
                                 input.orientation_, input.word_, double_used, double_active);
        if (valid_move == 1 && *player_1_points < max_points && *player_2_points < max_points)
        {
          printScreen(field, field_width, tiles, number_of_tiles, *player_1_points, *player_2_points);
        }
        free(input.word_);
        break;
      case SAVE:
        saveCommand(fp, field, field_width, tiles, number_of_tiles,
                    *player_on_the_move, *player_1_points, *player_2_points);
        break;
      case QUIT:
        fclose(fp);
        freeFieldMemory(field, field_width);
        freeTilesMemory(tiles, number_of_tiles);
        return_value = 6;
        break;
      case HELP:
        printHelp();
        break;
      case DOUBLE:
        if (*double_used == 0)
        {
          *double_active = 1;
          *double_used = 1;
        }
        break;
      case UNKNOWN:
        break;
  }
  return return_value;
}
//------------------------------------------------------------------------------
///
/// If game ends with winner, this function prints out the player that won and 
/// his/her points. Also frees all alocated memory and closes the config file
/// 
///
/// @param fp file pointer to the config file
/// @param field vector of strings that holds the current playing field on the heap
/// @param field_width width and also height of the playing field
/// @param tiles vector of pointers of type Tile, that holds all tiles on the heap
/// @param number_of_tiles number of all tiles
/// @param player_1_points number of points achieved by the first player
/// @param player_2_points number of points achieved by the second player
///
//
void endWithWinner(FILE* fp, char* field[], int field_width, Tile* tiles[], int number_of_tiles,
                   int player_1_points, int player_2_points)
{
  if (player_1_points  > player_2_points)
  {
    printf("Player 1 has won the game with %d points!\n", player_1_points);
  }
  else
  {
    printf("Player 2 has won the game with %d points!\n", player_2_points);
  }

  freeFieldMemory(field, field_width);
  freeTilesMemory(tiles, number_of_tiles);
  fclose(fp);
}



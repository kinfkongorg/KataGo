/*
 * board.h
 * Originally from an unreleased project back in 2010, modified since.
 * Authors: brettharrison (original), David Wu (original and later modifications).
 */

#ifndef GAME_BOARD_H_
#define GAME_BOARD_H_

#include "../core/global.h"
#include "../core/hash.h"

//TYPES AND CONSTANTS-----------------------------------------------------------------

struct Board;

//Player
typedef int8_t Player;
static const Player P_BLACK = 1;
static const Player P_WHITE = 2;

//Color of a point on the board
typedef int8_t Color;
static const Color C_EMPTY = 0;
static const Color C_BLACK = 1;
static const Color C_WHITE = 2;
static const Color C_WALL = 3;

static inline Color getOpp(Color c)
{return c ^ 3;}

//Conversions for players and colors
namespace PlayerIO {
  char colorToChar(Color c);
  std::string playerToString(Player p);
  bool tryParsePlayer(const std::string& s, Player& pla);
}

//Location of a point on the board
//(x,y) is represented as (x+1) + (y+1)*(x_size+1)
typedef short Loc;
namespace Location
{
  Loc getLoc(int x, int y, int x_size);
  int getX(Loc loc, int x_size);
  int getY(Loc loc, int x_size);

  void getAdjacentOffsets(short adj_offsets[8], int x_size);
  bool isAdjacent(Loc loc0, Loc loc1, int x_size);
  int distance(Loc loc0, Loc loc1, int x_size);
  int euclideanDistanceSquared(Loc loc0, Loc loc1, int x_size);

  std::string toString(Loc loc, int x_size, int y_size);
  std::string toString(Loc loc, const Board& b);
  std::string toStringMach(Loc loc, int x_size);
  std::string toStringMach(Loc loc, const Board& b);

  bool tryOfString(const std::string& str, int x_size, int y_size, Loc& result);
  bool tryOfString(const std::string& str, const Board& b, Loc& result);
  Loc ofString(const std::string& str, int x_size, int y_size);
  Loc ofString(const std::string& str, const Board& b);

  std::vector<Loc> parseSequence(const std::string& str, const Board& b);
}

//Simple structure for storing moves. Not used below, but this is a convenient place to define it.
STRUCT_NAMED_PAIR(Loc,loc,Player,pla,Move);

//Fast lightweight board designed for playouts and simulations, where speed is essential.
//Simple ko rule only.
//Does not enforce player turn order.

struct Board
{
  //Initialization------------------------------
  //Initialize the zobrist hash.
  //MUST BE CALLED AT PROGRAM START!
  static void initHash();

  //Board parameters and Constants----------------------------------------

  static const int MAX_LEN = 8;  //Maximum edge length allowed for the board
  static const int MAX_PLAY_SIZE = MAX_LEN * MAX_LEN;  //Maximum number of playable spaces
  static const int MAX_ARR_SIZE = (MAX_LEN+1)*(MAX_LEN+2)+1; //Maximum size of arrays needed

  //Location used to indicate an invalid spot on the board.
  static const Loc NULL_LOC = 0;
  //Location used to indicate a pass move is desired.
  static const Loc PASS_LOC = 1;

  //Zobrist Hashing------------------------------
  static bool IS_ZOBRIST_INITALIZED;
  static Hash128 ZOBRIST_SIZE_X_HASH[MAX_LEN+1];
  static Hash128 ZOBRIST_SIZE_Y_HASH[MAX_LEN+1];
  static Hash128 ZOBRIST_BOARD_HASH[MAX_ARR_SIZE][4];
  static Hash128 ZOBRIST_PLAYER_HASH[4];
  static const Hash128 ZOBRIST_GAME_IS_OVER;

  //Structs---------------------------------------


  //Tracks locations for fast random selection
  /* struct PointList { */
  /*   PointList(); */
  /*   PointList(const PointList&); */
  /*   void operator=(const PointList&); */
  /*   void add(Loc); */
  /*   void remove(Loc); */
  /*   int size() const; */
  /*   Loc& operator[](int); */
  /*   bool contains(Loc loc) const; */

  /*   Loc list_[MAX_PLAY_SIZE];   //Locations in the list */
  /*   int indices_[MAX_ARR_SIZE]; //Maps location to index in the list */
  /*   int size_; */
  /* }; */

  //Move data passed back when moves are made to allow for undos
  struct MoveRecord {
    Player pla;
    Loc loc;
  };

  //Constructors---------------------------------
  Board();  //Create Board of size (19,19)
  Board(int x, int y); //Create Board of size (x,y)
  Board(const Board& other);

  Board& operator=(const Board&) = default;

  //Functions------------------------------------

  //Check if moving here is legal.
  bool isLegal(Loc loc, Player pla, bool isStrict) const;
  //Check if this location is on the board
  bool isOnBoard(Loc loc) const;
  //Check if this location is adjacent to stones of the specified color
  bool isAdjacentToPla(Loc loc, Player pla) const;
  //Is this board empty?
  bool isEmpty() const;

  
  //Sets the specified stone if possible. Returns true usually, returns false location or color were out of range.
  bool setStone(Loc loc, Color color);

  //Attempts to play the specified move. Returns true if successful, returns false if the move was illegal.
  bool playMove(Loc loc, Player pla, bool isMultiStoneSuicideLegal);

  //Plays the specified move, assuming it is legal.
  void playMoveAssumeLegal(Loc loc, Player pla);

  //Plays the specified move, assuming it is legal, and returns a MoveRecord for the move
  MoveRecord playMoveRecorded(Loc loc, Player pla);

  //Undo the move given by record. Moves MUST be undone in the order they were made.
  //Undos will NOT typically restore the precise representation in the board to the way it was. The heads of chains
  //might change, the order of the circular lists might change, etc.
  void undo(MoveRecord record);

  //Get what the position hash would be if we were to play this move and resolve captures and suicides.
  //Assumes the move is on an empty location.
  Hash128 getPosHashAfterMove(Loc loc, Player pla) const;

  //Get a random legal move that does not fill a simple eye.
  /* Loc getRandomMCLegal(Player pla); */


  //If a point is a pass-alive stone or pass-alive territory for a color, mark it that color.
  //If nonPassAliveStones, also marks non-pass-alive stones that are not part of the opposing pass-alive territory.
  //If safeBigTerritories, also marks for each pla empty regions bordered by pla stones and no opp stones, where all pla stones are pass-alive.
  //If unsafeBigTerritories, also marks for each pla empty regions bordered by pla stones and no opp stones, regardless.
  //All other points are marked as C_EMPTY.
  //[result] must be a buffer of size MAX_ARR_SIZE and will get filled with the result
  void calculateArea(
    Color* result,
    bool nonPassAliveStones,
    bool safeBigTerritories,
    bool unsafeBigTerritories,
    bool isMultiStoneSuicideLegal
  ) const;


  //Calculates the area (including non pass alive stones, safe and unsafe big territories)
  //However, strips out any "seki" regions.
  //Seki regions are that are adjacent to any remaining empty regions.
  //If keepTerritories, then keeps the surrounded territories in seki regions, only strips points for stones.
  //If keepStones, then keeps the stones, only strips points for surrounded territories.
  //whiteMinusBlackIndependentLifeRegionCount - multiply this by two for a group tax.
  void calculateIndependentLifeArea(
    Color* result,
    int& whiteMinusBlackIndependentLifeRegionCount,
    bool keepTerritories,
    bool keepStones,
    bool isMultiStoneSuicideLegal
  ) const;

  //Run some basic sanity checks on the board state, throws an exception if not consistent, for testing/debugging
  void checkConsistency() const;

  static Board parseBoard(int xSize, int ySize, const std::string& s);
  static void printBoard(std::ostream& out, const Board& board, Loc markLoc, const std::vector<Move>* hist);

  //Data--------------------------------------------

  int x_size;                  //Horizontal size of board
  int y_size;                  //Vertical size of board
  Color colors[MAX_ARR_SIZE];  //Color of each location on the board.


  Hash128 pos_hash; //A zobrist hash of the current board position (does not include ko point or player to move)


  short adj_offsets[8]; //Indices 0-3: Offsets to add for adjacent points. Indices 4-7: Offsets for diagonal points.

  private:
  void init(int xS, int yS);
  void removeSingleStone(Loc loc);


  friend std::ostream& operator<<(std::ostream& out, const Board& board);

};




#endif // GAME_BOARD_H_

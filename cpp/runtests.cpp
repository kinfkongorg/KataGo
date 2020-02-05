
#include <sstream>
#include "core/global.h"
#include "core/bsearch.h"
#include "core/rand.h"
#include "core/elo.h"
#include "core/fancymath.h"
#include "game/board.h"
#include "game/rules.h"
#include "game/boardhistory.h"
#include "neuralnet/nninputs.h"
#include "tests/tests.h"
#include "main.h"

using namespace std;

int MainCmds::runtests(int argc, const char* const* argv) {
  (void)argc;
  (void)argv;
  testAssert(sizeof(size_t) == 8);
  Board::initHash();
  ScoreValue::initTables();

  BSearch::runTests();
  Rand::runTests();
  FancyMath::runTests();
  ComputeElos::runTests();


  ScoreValue::freeTables();

  cout << "All tests passed" << endl;
  return 0;
}

int MainCmds::runoutputtests(int argc, const char* const* argv) {
  (void)argc;
  (void)argv;
  Board::initHash();
  ScoreValue::initTables();

  ScoreValue::freeTables();

  return 0;
}

int MainCmds::runsearchtests(int argc, const char* const* argv) {
  Board::initHash();
  ScoreValue::initTables();

  if(argc != 6) {
    cerr << "Must supply exactly five arguments: MODEL_FILE INPUTSNHWC CUDANHWC SYMMETRY FP16" << endl;
    return 1;
  }
  ScoreValue::freeTables();

  return 0;
}

int MainCmds::runsearchtestsv3(int argc, const char* const* argv) {
  Board::initHash();
  ScoreValue::initTables();

  if(argc != 6) {
    cerr << "Must supply exactly five arguments: MODEL_FILE INPUTSNHWC CUDANHWC SYMMETRY FP16" << endl;
    return 1;
  }

  ScoreValue::freeTables();

  return 0;
}

int MainCmds::runsearchtestsv8(int argc, const char* const* argv) {
  Board::initHash();
  ScoreValue::initTables();

  if(argc != 5) {
    cerr << "Must supply exactly four arguments: MODEL_FILE INPUTSNHWC CUDANHWC FP16" << endl;
    return 1;
  }
  ScoreValue::freeTables();

  return 0;
}

int MainCmds::runselfplayinittests(int argc, const char* const* argv) {
  if(argc != 2) {
    cerr << "Must supply exactly one argument: MODEL_FILE" << endl;
    return 1;
  }

  Board::initHash();
  ScoreValue::initTables();


  ScoreValue::freeTables();

  return 0;
}

int MainCmds::runsekitrainwritetests(int argc, const char* const* argv) {
  if(argc != 2) {
    cerr << "Must supply exactly one argument: MODEL_FILE" << endl;
    return 1;
  }

  Board::initHash();
  ScoreValue::initTables();


  ScoreValue::freeTables();

  return 0;
}

int MainCmds::runnnlayertests(int argc, const char* const* argv) {
  (void)argc;
  (void)argv;
  return 0;
}

int MainCmds::runnnontinyboardtest(int argc, const char* const* argv) {
  if(argc != 6) {
    cerr << "Must supply exactly five arguments: MODEL_FILE INPUTSNHWC CUDANHWC SYMMETRY FP16" << endl;
    return 1;
  }
  Board::initHash();
  ScoreValue::initTables();


  ScoreValue::freeTables();

  return 0;
}

int MainCmds::runnnsymmetriestest(int argc, const char* const* argv) {
  if(argc != 5) {
    cerr << "Must supply exactly four arguments: MODEL_FILE INPUTSNHWC CUDANHWC FP16" << endl;
    return 1;
  }
  Board::initHash();
  ScoreValue::initTables();


  ScoreValue::freeTables();

  return 0;
}

int MainCmds::runnnonmanyposestest(int argc, const char* const* argv) {
  if(argc != 6 && argc != 7) {
    cerr << "Must supply five or six arguments: MODEL_FILE INPUTSNHWC CUDANHWC SYMMETRY FP16 [COMPARISONFILE]" << endl;
    return 1;
  }
  Board::initHash();
  ScoreValue::initTables();


  ScoreValue::freeTables();

  return 0;
}

int MainCmds::runownershiptests(int argc, const char* const* argv) {
  if(argc != 3) {
    cerr << "Must supply exactly two arguments: GTP_CONFIG MODEL_FILE" << endl;
    return 1;
  }
  Board::initHash();
  ScoreValue::initTables();


  ScoreValue::freeTables();
  return 0;
}

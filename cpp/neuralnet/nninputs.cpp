#include "../neuralnet/nninputs.h"

using namespace std;

int NNPos::xyToPos(int x, int y, int nnXLen) {
  return y * nnXLen + x;
}
int NNPos::locToPos(Loc loc, int boardXSize, int nnXLen, int nnYLen) {
  if(loc == Board::PASS_LOC)
    return nnXLen * nnYLen;
  else if(loc == Board::NULL_LOC)
    return nnXLen * (nnYLen + 1);
  return Location::getY(loc,boardXSize) * nnXLen + Location::getX(loc,boardXSize);
}
Loc NNPos::posToLoc(int pos, int boardXSize, int boardYSize, int nnXLen, int nnYLen) {
  if(pos == nnXLen * nnYLen)
    return Board::PASS_LOC;
  int x = pos % nnXLen;
  int y = pos / nnXLen;
  if(x < 0 || x >= boardXSize || y < 0 || y >= boardYSize)
    return Board::NULL_LOC;
  return Location::getLoc(x,y,boardXSize);
}

bool NNPos::isPassPos(int pos, int nnXLen, int nnYLen) {
  return pos == nnXLen * nnYLen;
}

int NNPos::getPolicySize(int nnXLen, int nnYLen) {
  return nnXLen * nnYLen + 1;
}

//-----------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------

const Hash128 MiscNNInputParams::ZOBRIST_CONSERVATIVE_PASS =
  Hash128(0x0c2b96f4b8ae2da9ULL, 0x5a14dee208fec0edULL);
const Hash128 MiscNNInputParams::ZOBRIST_PLAYOUT_DOUBLINGS =
  Hash128(0xa5e6114d380bfc1dULL, 0x4160557f1222f4adULL);

//-----------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------

double ScoreValue::whiteWinsOfWinner(Player winner, double drawEquivalentWinsForWhite) {
  if(winner == P_WHITE)
    return 1.0;
  else if(winner == P_BLACK)
    return 0.0;

  assert(winner == C_EMPTY);
  return drawEquivalentWinsForWhite;
}

static const double twoOverPi = 0.63661977236758134308;
static const double piOverTwo = 1.57079632679489661923;

double ScoreValue::whiteScoreDrawAdjust(double finalWhiteMinusBlackScore, double drawEquivalentWinsForWhite, const BoardHistory& hist) {
  return finalWhiteMinusBlackScore + hist.whiteKomiAdjustmentForDraws(drawEquivalentWinsForWhite);
}

double ScoreValue::whiteScoreValueOfScoreSmooth(double finalWhiteMinusBlackScore, double center, double scale, double drawEquivalentWinsForWhite, const Board& b, const BoardHistory& hist) {
  double adjustedScore = finalWhiteMinusBlackScore + hist.whiteKomiAdjustmentForDraws(drawEquivalentWinsForWhite) - center;
  if(b.x_size == b.y_size)
    return atan(adjustedScore / (scale*b.x_size)) * twoOverPi;
  else
    return atan(adjustedScore / (scale*sqrt(b.x_size*b.y_size))) * twoOverPi;
}

double ScoreValue::whiteScoreValueOfScoreSmoothNoDrawAdjust(double finalWhiteMinusBlackScore, double center, double scale, const Board& b) {
  double adjustedScore = finalWhiteMinusBlackScore - center;
  if(b.x_size == b.y_size)
    return atan(adjustedScore / (scale*b.x_size)) * twoOverPi;
  else
    return atan(adjustedScore / (scale*sqrt(b.x_size*b.y_size))) * twoOverPi;
}

static double inverse_atan(double x) {
  if(x >= piOverTwo - 1e-6) return 1e6;
  if(x <= -piOverTwo + 1e-6) return -1e6;
  return tan(x);
}

double ScoreValue::approxWhiteScoreOfScoreValueSmooth(double scoreValue, double center, double scale, const Board& b) {
  assert(scoreValue >= -1 && scoreValue <= 1);
  double scoreUnscaled = inverse_atan(scoreValue*piOverTwo);
  if(b.x_size == b.y_size)
    return scoreUnscaled * (scale*b.x_size) + center;
  else
    return scoreUnscaled * (scale*sqrt(b.x_size*b.y_size)) + center;
}

double ScoreValue::whiteScoreMeanSqOfScoreGridded(double finalWhiteMinusBlackScore, double drawEquivalentWinsForWhite) {
  assert((int)(finalWhiteMinusBlackScore * 2) == finalWhiteMinusBlackScore * 2);
  bool finalScoreIsInteger = ((int)finalWhiteMinusBlackScore == finalWhiteMinusBlackScore);
  if(!finalScoreIsInteger)
    return finalWhiteMinusBlackScore * finalWhiteMinusBlackScore;

  double lower = finalWhiteMinusBlackScore - 0.5;
  double upper = finalWhiteMinusBlackScore + 0.5;
  double lowerSq = lower * lower;
  double upperSq = upper * upper;

  return lowerSq + (upperSq - lowerSq) * drawEquivalentWinsForWhite;
}


static bool scoreValueTablesInitialized = false;
static double* expectedSVTable = NULL;
static const int svTableAssumedBSize = NNPos::MAX_BOARD_LEN;
static const int svTableMeanRadius = svTableAssumedBSize*svTableAssumedBSize + NNPos::EXTRA_SCORE_DISTR_RADIUS;
static const int svTableMeanLen = svTableMeanRadius*2;
static const int svTableStdevLen = svTableAssumedBSize*svTableAssumedBSize + NNPos::EXTRA_SCORE_DISTR_RADIUS;

void ScoreValue::freeTables() {
  if(scoreValueTablesInitialized) {
    delete[] expectedSVTable;
    expectedSVTable = NULL;
    scoreValueTablesInitialized = false;
  }
}

void ScoreValue::initTables() {
  assert(!scoreValueTablesInitialized);
  expectedSVTable = new double[svTableMeanLen*svTableStdevLen];

  //Precompute normal PDF
  const int stepsPerUnit = 10; //Must be divisible by 2. This is both the number of segments that we divide points into, and that we divide stdevs into
  const int boundStdevs = 5;
  int minStdevSteps = -boundStdevs*stepsPerUnit;
  int maxStdevSteps = boundStdevs*stepsPerUnit;
  double* normalPDF = new double[(maxStdevSteps-minStdevSteps)+1];
  for(int i = minStdevSteps; i <= maxStdevSteps; i++) {
    double xInStdevs = (double)i / stepsPerUnit;
    double w = exp(-0.5 * xInStdevs * xInStdevs);
    normalPDF[i-minStdevSteps] = w;
  }
  //Precompute scorevalue at increments of 1/stepsPerUnit points
  Board board(svTableAssumedBSize,svTableAssumedBSize);
  int minSVSteps = - (svTableMeanRadius*stepsPerUnit + stepsPerUnit/2 + boundStdevs * svTableStdevLen * stepsPerUnit);
  int maxSVSteps = -minSVSteps;
  double* svPrecomp = new double[(maxSVSteps-minSVSteps)+1];
  for(int i = minSVSteps; i <= maxSVSteps; i++) {
    double mean = (double)i / stepsPerUnit;
    double sv = whiteScoreValueOfScoreSmoothNoDrawAdjust(mean, 0.0, 1.0, board);
    svPrecomp[i-minSVSteps] = sv;
  }

  //Perform numeric integration
  for(int meanIdx = 0; meanIdx < svTableMeanLen; meanIdx++) {
    int meanSteps = (meanIdx - svTableMeanRadius) * stepsPerUnit - stepsPerUnit/2;
    for(int stdevIdx = 0; stdevIdx < svTableStdevLen; stdevIdx++) {
      double wSum = 0.0;
      double wsvSum = 0.0;
      for(int i = minStdevSteps; i <= maxStdevSteps; i++) {
        int xSteps = meanSteps + stdevIdx * i;
        double w = normalPDF[i-minStdevSteps];
        assert(xSteps >= minSVSteps && xSteps <= maxSVSteps);
        double sv = svPrecomp[xSteps-minSVSteps];
        wSum += w;
        wsvSum += w*sv;
      }
      expectedSVTable[meanIdx*svTableStdevLen + stdevIdx] = wsvSum / wSum;
    }
  }

  delete[] normalPDF;
  delete[] svPrecomp;
  scoreValueTablesInitialized = true;
}

double ScoreValue::expectedWhiteScoreValue(double whiteScoreMean, double whiteScoreStdev, double center, double scale, const Board& b) {
  assert(scoreValueTablesInitialized);

  double scaleFactor;
  if(b.x_size == b.y_size)
    scaleFactor = (double)svTableAssumedBSize / (scale * b.x_size);
  else
    scaleFactor = (double)svTableAssumedBSize / (scale * sqrt(b.x_size*b.y_size));

  double meanScaled = (whiteScoreMean - center) * scaleFactor;
  double stdevScaled = whiteScoreStdev * scaleFactor;

  double meanRounded = round(meanScaled);
  double stdevFloored = floor(stdevScaled);
  int meanIdx0 = (int)meanRounded + svTableMeanRadius;
  int stdevIdx0 = (int)stdevFloored;
  int meanIdx1 = meanIdx0+1;
  int stdevIdx1 = stdevIdx0+1;

  if(meanIdx0 < 0) { meanIdx0 = 0; meanIdx1 = 0; }
  if(meanIdx1 >= svTableMeanLen) { meanIdx0 = svTableMeanLen-1; meanIdx1 = svTableMeanLen-1; }
  assert(stdevIdx0 >= 0);
  if(stdevIdx1 >= svTableStdevLen) { stdevIdx0 = svTableStdevLen-1; stdevIdx1 = svTableStdevLen-1; }

  double lambdaMean = meanScaled - meanRounded + 0.5;
  double lambdaStdev = stdevScaled - stdevFloored;

  double a00 = expectedSVTable[meanIdx0*svTableStdevLen + stdevIdx0];
  double a01 = expectedSVTable[meanIdx0*svTableStdevLen + stdevIdx1];
  double a10 = expectedSVTable[meanIdx1*svTableStdevLen + stdevIdx0];
  double a11 = expectedSVTable[meanIdx1*svTableStdevLen + stdevIdx1];

  double b0 = a00 + lambdaStdev*(a01-a00);
  double b1 = a10 + lambdaStdev*(a11-a10);
  return b0 + lambdaMean*(b1-b0);
}

//-----------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------

void NNInputs::fillOwnership(
  const Board& board,
  const Color* area,
  bool groupTax,
  int nnXLen,
  int nnYLen,
  float* ownership
) {
  if(!groupTax) {
    std::fill(ownership, ownership + nnXLen*nnYLen, 0.0f);
    for(int y = 0; y<board.y_size; y++) {
      for(int x = 0; x<board.x_size; x++) {
        Loc loc = Location::getLoc(x,y,board.x_size);
        int pos = NNPos::locToPos(loc,board.x_size,nnXLen,nnYLen);
        Color areaColor = area[loc];
        if(areaColor == P_BLACK)
          ownership[pos] = -1.0f;
        else if(areaColor == P_WHITE)
          ownership[pos] = 1.0f;
        else {
          assert(areaColor == C_EMPTY);
          ownership[pos] = 0;
        }
      }
    }
  }
  
}


//-----------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------


NNOutput::NNOutput()
  :whiteOwnerMap(NULL),noisedPolicyProbs(NULL)
{}
NNOutput::NNOutput(const NNOutput& other) {
  nnHash = other.nnHash;
  whiteWinProb = other.whiteWinProb;
  whiteLossProb = other.whiteLossProb;
  whiteNoResultProb = other.whiteNoResultProb;
  whiteScoreMean = other.whiteScoreMean;
  whiteScoreMeanSq = other.whiteScoreMeanSq;
  whiteLead = other.whiteLead;
  varTimeLeft = other.varTimeLeft;

  nnXLen = other.nnXLen;
  nnYLen = other.nnYLen;
  if(other.whiteOwnerMap != NULL) {
    whiteOwnerMap = new float[nnXLen * nnYLen];
    std::copy(other.whiteOwnerMap, other.whiteOwnerMap + nnXLen * nnYLen, whiteOwnerMap);
  }
  else
    whiteOwnerMap = NULL;

  if(other.noisedPolicyProbs != NULL) {
    noisedPolicyProbs = new float[NNPos::MAX_NN_POLICY_SIZE];
    std::copy(other.noisedPolicyProbs, other.noisedPolicyProbs + NNPos::MAX_NN_POLICY_SIZE, noisedPolicyProbs);
  }
  else
    noisedPolicyProbs = NULL;

  std::copy(other.policyProbs, other.policyProbs+NNPos::MAX_NN_POLICY_SIZE, policyProbs);
}

NNOutput::NNOutput(const vector<shared_ptr<NNOutput>>& others) {
  int len = others.size();
  float floatLen = (float)len;
  assert(len > 0);
  for(int i = 1; i<len; i++) {
    assert(others[i]->nnHash == others[0]->nnHash);
  }
  nnHash = others[0]->nnHash;

  whiteWinProb = 0.0f;
  whiteLossProb = 0.0f;
  whiteNoResultProb = 0.0f;
  whiteScoreMean = 0.0f;
  whiteScoreMeanSq = 0.0f;
  whiteLead = 0.0f;
  varTimeLeft = 0.0f;
  for(int i = 0; i<len; i++) {
    const NNOutput& other = *(others[i]);
    whiteWinProb += other.whiteWinProb;
    whiteLossProb += other.whiteLossProb;
    whiteNoResultProb += other.whiteNoResultProb;
    whiteScoreMean += other.whiteScoreMean;
    whiteScoreMeanSq += other.whiteScoreMeanSq;
    whiteLead += other.whiteLead;
    varTimeLeft += other.varTimeLeft;
  }
  whiteWinProb /= floatLen;
  whiteLossProb /= floatLen;
  whiteNoResultProb /= floatLen;
  whiteScoreMean /= floatLen;
  whiteScoreMeanSq /= floatLen;
  whiteLead /= floatLen;
  varTimeLeft /= floatLen;

  nnXLen = others[0]->nnXLen;
  nnYLen = others[0]->nnYLen;

  {
    float whiteOwnerMapCount = 0.0f;
    whiteOwnerMap = NULL;
    for(int i = 0; i<len; i++) {
      const NNOutput& other = *(others[i]);
      if(other.whiteOwnerMap != NULL) {
        if(whiteOwnerMap == NULL) {
          whiteOwnerMap = new float[nnXLen * nnYLen];
          std::fill(whiteOwnerMap, whiteOwnerMap + nnXLen * nnYLen, 0.0f);
        }
        whiteOwnerMapCount += 1.0f;
        for(int pos = 0; pos<nnXLen*nnYLen; pos++)
          whiteOwnerMap[pos] += other.whiteOwnerMap[pos];
      }
    }
    if(whiteOwnerMap != NULL) {
      assert(whiteOwnerMapCount > 0);
      for(int pos = 0; pos<nnXLen*nnYLen; pos++)
        whiteOwnerMap[pos] /= whiteOwnerMapCount;
    }
  }

  noisedPolicyProbs = NULL;

  //For technical correctness in case of impossibly rare hash collisions:
  //Just give up if they don't all match in move legality
  {
    bool mismatch = false;
    std::fill(policyProbs, policyProbs + NNPos::MAX_NN_POLICY_SIZE, 0.0f);
    for(int i = 0; i<len; i++) {
      const NNOutput& other = *(others[i]);
      for(int pos = 0; pos<NNPos::MAX_NN_POLICY_SIZE; pos++) {
        if(i > 0 && (policyProbs[pos] < 0) != (other.policyProbs[pos] < 0))
          mismatch = true;
        policyProbs[pos] += other.policyProbs[pos];
      }
    }
    //In case of mismatch, just take the first one
    //This should basically never happen, only on true hash collisions
    if(mismatch) {
      const NNOutput& other = *(others[0]);
      std::copy(other.policyProbs, other.policyProbs + NNPos::MAX_NN_POLICY_SIZE, policyProbs);
    }
    else {
      for(int pos = 0; pos<NNPos::MAX_NN_POLICY_SIZE; pos++)
        policyProbs[pos] /= floatLen;
    }
  }

}

NNOutput& NNOutput::operator=(const NNOutput& other) {
  if(&other == this)
    return *this;
  nnHash = other.nnHash;
  whiteWinProb = other.whiteWinProb;
  whiteLossProb = other.whiteLossProb;
  whiteNoResultProb = other.whiteNoResultProb;
  whiteScoreMean = other.whiteScoreMean;
  whiteScoreMeanSq = other.whiteScoreMeanSq;
  whiteLead = other.whiteLead;
  varTimeLeft = other.varTimeLeft;

  nnXLen = other.nnXLen;
  nnYLen = other.nnYLen;
  if(whiteOwnerMap != NULL)
    delete[] whiteOwnerMap;
  if(other.whiteOwnerMap != NULL) {
    whiteOwnerMap = new float[nnXLen * nnYLen];
    std::copy(other.whiteOwnerMap, other.whiteOwnerMap + nnXLen * nnYLen, whiteOwnerMap);
  }
  else
    whiteOwnerMap = NULL;
  if(noisedPolicyProbs != NULL)
    delete[] noisedPolicyProbs;
  if(other.noisedPolicyProbs != NULL) {
    noisedPolicyProbs = new float[NNPos::MAX_NN_POLICY_SIZE];
    std::copy(other.noisedPolicyProbs, other.noisedPolicyProbs + NNPos::MAX_NN_POLICY_SIZE, noisedPolicyProbs);
  }
  else
    noisedPolicyProbs = NULL;

  std::copy(other.policyProbs, other.policyProbs+NNPos::MAX_NN_POLICY_SIZE, policyProbs);

  return *this;
}


NNOutput::~NNOutput() {
  if(whiteOwnerMap != NULL) {
    delete[] whiteOwnerMap;
    whiteOwnerMap = NULL;
  }
  if(noisedPolicyProbs != NULL) {
    delete[] noisedPolicyProbs;
    noisedPolicyProbs = NULL;
  }
}


void NNOutput::debugPrint(ostream& out, const Board& board) {
  out << "Win " << Global::strprintf("%.2fc",whiteWinProb*100) << endl;
  out << "Loss " << Global::strprintf("%.2fc",whiteLossProb*100) << endl;
  out << "NoResult " << Global::strprintf("%.2fc",whiteNoResultProb*100) << endl;
  out << "ScoreMean " << Global::strprintf("%.1f",whiteScoreMean) << endl;
  out << "ScoreMeanSq " << Global::strprintf("%.1f",whiteScoreMeanSq) << endl;
  out << "Lead " << Global::strprintf("%.1f",whiteLead) << endl;
  out << "VarTimeLeft " << Global::strprintf("%.1f",varTimeLeft) << endl;

  out << "Policy" << endl;
  for(int y = 0; y<board.y_size; y++) {
    for(int x = 0; x<board.x_size; x++) {
      int pos = NNPos::xyToPos(x,y,nnXLen);
      float prob = policyProbs[pos];
      if(prob < 0)
        out << "   - ";
      else
        out << Global::strprintf("%4d ", (int)round(prob * 1000));
    }
    out << endl;
  }

  if(whiteOwnerMap != NULL) {
    for(int y = 0; y<board.y_size; y++) {
      for(int x = 0; x<board.x_size; x++) {
        int pos = NNPos::xyToPos(x,y,nnXLen);
        float whiteOwn = whiteOwnerMap[pos];
        out << Global::strprintf("%5d ", (int)round(whiteOwn * 1000));
      }
      out << endl;
    }
    out << endl;
  }
}


//-------------------------------------------------------------------------------------------------------------

static void setRowBin(float* rowBin, int pos, int feature, float value, int posStride, int featureStride) {
  rowBin[pos * posStride + feature * featureStride] = value;
}

//Currently does NOT depend on history (except for marking ko-illegal spots)
Hash128 NNInputs::getHash(
  const Board& board, const BoardHistory& hist, Player nextPlayer,
  const MiscNNInputParams& nnInputParams
) {
  int xSize = board.x_size;
  int ySize = board.y_size;

  //Note that board.pos_hash also incorporates the size of the board.
  Hash128 hash = board.pos_hash;
  hash ^= Board::ZOBRIST_PLAYER_HASH[nextPlayer];

  float selfKomi = hist.currentSelfKomi(nextPlayer,nnInputParams.drawEquivalentWinsForWhite);

  //Discretize the komi for the purpose of matching hash, so that extremely close effective komi we just reuse nn cache hits
  int64_t komiDiscretized = (int64_t)(selfKomi*256.0f);
  uint64_t komiHash = Hash::murmurMix((uint64_t)komiDiscretized);
  hash.hash0 ^= komiHash;
  hash.hash1 ^= Hash::basicLCong(komiHash);

  //Fold in the ko, scoring, and suicide rules
  hash ^= Rules::ZOBRIST_SCORING_RULE_HASH[hist.rules.scoringRule];

  //Fold in whether a pass ends this phase
  if(nnInputParams.conservativePass)
    hash ^= MiscNNInputParams::ZOBRIST_CONSERVATIVE_PASS;
  //Fold in whether the game is over or not, since this affects how we compute input features
  //but is not a function necessarily of previous hashed values.
  //If the history is in a weird prolonged state, also treat it similarly.
  if(hist.isGameFinished )
    hash ^= Board::ZOBRIST_GAME_IS_OVER;

  //Fold in asymmetric playout indicator
  if(nnInputParams.playoutDoublingAdvantage != 0) {
    int64_t playoutDoublingsDiscretized = (int64_t)(nnInputParams.playoutDoublingAdvantage*256.0f);
    hash.hash0 += Hash::splitMix64((uint64_t)playoutDoublingsDiscretized);
    hash.hash1 += Hash::basicLCong((uint64_t)playoutDoublingsDiscretized);
    hash ^= MiscNNInputParams::ZOBRIST_PLAYOUT_DOUBLINGS;
  }
  return hash;
}

//===========================================================================================
//INPUTSVERSION 3
//===========================================================================================


void NNInputs::fillRowV7(
  const Board& board, const BoardHistory& hist, Player nextPlayer,
  const MiscNNInputParams& nnInputParams,
  int nnXLen, int nnYLen, bool useNHWC, float* rowBin, float* rowGlobal
) {
  assert(nnXLen <= NNPos::MAX_BOARD_LEN);
  assert(nnYLen <= NNPos::MAX_BOARD_LEN);
  assert(board.x_size <= nnXLen);
  assert(board.y_size <= nnYLen);
  std::fill(rowBin,rowBin+NUM_FEATURES_SPATIAL_V7*nnXLen*nnYLen,false);
  std::fill(rowGlobal,rowGlobal+NUM_FEATURES_GLOBAL_V7,0.0f);

  Player pla = nextPlayer;
  Player opp = getOpp(pla);
  int xSize = board.x_size;
  int ySize = board.y_size;

  int featureStride;
  int posStride;
  if(useNHWC) {
    featureStride = 1;
    posStride = NNInputs::NUM_FEATURES_SPATIAL_V7;
  }
  else {
    featureStride = nnXLen * nnYLen;
    posStride = 1;
  }

  for(int y = 0; y<ySize; y++) {
    for(int x = 0; x<xSize; x++) {
      int pos = NNPos::xyToPos(x,y,nnXLen);
      Loc loc = Location::getLoc(x,y,xSize);

      //Feature 0 - on board
      setRowBin(rowBin,pos,0, 1.0f, posStride, featureStride);

      Color stone = board.colors[loc];

      //Features 1,2 - pla,opp stone
      //Features 3,4,5 - 1,2,3 libs
      if(stone == pla)
        setRowBin(rowBin,pos,1, 1.0f, posStride, featureStride);
      else if(stone == opp)
        setRowBin(rowBin,pos,2, 1.0f, posStride, featureStride);

    }
  }

  //Hide history from the net if a pass would end things and we're behaving as if a pass won't.
  //Or if the game is in fact over right now!




  //History isn't important
  /*
  bool hideHistory =
    hist.isGameFinished ||
    (nnInputParams.conservativePass && hist.consecutiveEndingPasses>=1);

  //Features 9,10,11,12,13
  if(!hideHistory) {
    const vector<Move>& moveHistory = hist.moveHistory;
    size_t moveHistoryLen = moveHistory.size();
    //Also effectively wipe history as we change phase
    assert(moveHistoryLen >= hist.numTurnsThisPhase);
    int numTurnsThisPhase = hist.numTurnsThisPhase;

    if(numTurnsThisPhase >= 1 && moveHistory[moveHistoryLen-1].pla == opp) {
      Loc prev1Loc = moveHistory[moveHistoryLen-1].loc;
      if(prev1Loc == Board::PASS_LOC)
        rowGlobal[0] = 1.0;
      else if(prev1Loc != Board::NULL_LOC) {
        int pos = NNPos::locToPos(prev1Loc,xSize,nnXLen,nnYLen);
        setRowBin(rowBin,pos,9, 1.0f, posStride, featureStride);
      }
      if(numTurnsThisPhase >= 2 && moveHistory[moveHistoryLen-2].pla == pla) {
        Loc prev2Loc = moveHistory[moveHistoryLen-2].loc;
        if(prev2Loc == Board::PASS_LOC)
          rowGlobal[1] = 1.0;
        else if(prev2Loc != Board::NULL_LOC) {
          int pos = NNPos::locToPos(prev2Loc,xSize,nnXLen,nnYLen);
          setRowBin(rowBin,pos,10, 1.0f, posStride, featureStride);
        }
        if(numTurnsThisPhase >= 3 && moveHistory[moveHistoryLen-3].pla == opp) {
          Loc prev3Loc = moveHistory[moveHistoryLen-3].loc;
          if(prev3Loc == Board::PASS_LOC)
            rowGlobal[2] = 1.0;
          else if(prev3Loc != Board::NULL_LOC) {
            int pos = NNPos::locToPos(prev3Loc,xSize,nnXLen,nnYLen);
            setRowBin(rowBin,pos,11, 1.0f, posStride, featureStride);
          }
          if(numTurnsThisPhase >= 4 && moveHistory[moveHistoryLen-4].pla == pla) {
            Loc prev4Loc = moveHistory[moveHistoryLen-4].loc;
            if(prev4Loc == Board::PASS_LOC)
              rowGlobal[3] = 1.0;
            else if(prev4Loc != Board::NULL_LOC) {
              int pos = NNPos::locToPos(prev4Loc,xSize,nnXLen,nnYLen);
              setRowBin(rowBin,pos,12, 1.0f, posStride, featureStride);
            }
            if(numTurnsThisPhase >= 5 && moveHistory[moveHistoryLen-5].pla == opp) {
              Loc prev5Loc = moveHistory[moveHistoryLen-5].loc;
              if(prev5Loc == Board::PASS_LOC)
                rowGlobal[4] = 1.0;
              else if(prev5Loc != Board::NULL_LOC) {
                int pos = NNPos::locToPos(prev5Loc,xSize,nnXLen,nnYLen);
                setRowBin(rowBin,pos,13, 1.0f, posStride, featureStride);
              }
            }
          }
        }
      }
    }
  }
  */



  //Global features.
  //The first 5 of them were set already above to flag which of the past 5 moves were passes.

  //Komi and any score adjustments
  float selfKomi = hist.currentSelfKomi(nextPlayer,nnInputParams.drawEquivalentWinsForWhite);
  float bArea = xSize * ySize;
  //Bound komi just in case
  if(selfKomi > bArea+1.0f)
    selfKomi = bArea+1.0f;
  if(selfKomi < -bArea-1.0f)
    selfKomi = -bArea-1.0f;
  rowGlobal[5] = selfKomi/8.0f;

  
  //Used for handicap play
  //Parameter 15 is used because there's actually a discontinuity in how training behavior works when this is
  //nonzero, no matter how slightly.
  if(nnInputParams.playoutDoublingAdvantage != 0) {
    rowGlobal[15] = 1.0;
    rowGlobal[16] = (float)(0.5 * nnInputParams.playoutDoublingAdvantage);
  }


  //Provide parity information about the board size and komi
  //This comes from the following observation:
  //From white's perspective:
  //Komi = 0.0 - Draw possible
  //Komi = 0.5 - Win the games we would have drawn with komi 0.0
  //Komi = 1.0 - Usually no difference from komi 0.5
  //Komi = 1.5 - Usually no difference from komi 0.5
  //Komi = 2.0 - Draw possible
  //If we were to assign an "effective goodness" to these komis in order it would look like
  //0 1 1 1 2 3 3 3 4 5 5 5 6 ...
  //since when away from the right parity, increasing the komi doesn't help us except in cases of seki with odd numbers of dame.
  //If we were to add 0.5 times a vector like:
  //0 -1 0 1 0 -1 0 1 0 -1 0 ...
  //Then this would become a linear function and hopefully easier for a neural net to learn.
  //We expect that this is hard for a neural net to learn since it depends on the parity of the board size
  //and is very "xor"like.
  //So we provide it as an input.
  //Since we are using a model where games are jittered by 0.5 (see BoardHistory::whiteKomiAdjustmentForDraws)
  //in theory right thing to first order to provide should be a triangular wave with a period of 2 komi points:
  //  ../\........
  //  ./..\.......
  //  /....\..../.
  //  ......\../..
  //  .......\/...
  //The upsloping part of the wave is centered around the komi value where you could draw
  //since komi is extra valuable when it turns losses into draws into wins, peaking at the komi value where you could draw + 0.5.
  //It's downsloping around the komi value where you can't draw, since the marginal komi there is nearly useless, not causing you to win
  //more games except in case of odd-dame seki.

  if(hist.rules.scoringRule == Rules::SCORING_AREA ) {
    bool boardAreaIsEven = (xSize*ySize) % 2 == 0;

    //What is the parity of the komi values that can produce jigos?
    bool drawableKomisAreEven = boardAreaIsEven;

    //Find the difference between the komi viewed from our perspective and the nearest drawable komi below it.
    float komiFloor;
    if(drawableKomisAreEven)
      komiFloor = floor(selfKomi / 2.0f) * 2.0f;
    else
      komiFloor = floor((selfKomi-1.0f) / 2.0f) * 2.0f + 1.0f;

    //Cap just in case we have floating point weirdness
    float delta = selfKomi - komiFloor;
    assert(delta >= -0.0001f);
    assert(delta <= 2.0001f);
    if(delta < 0.0f)
      delta = 0.0f;
    if(delta > 2.0f)
      delta = 2.0f;

    //Create the triangle wave based on the difference
    float wave;
    if(delta < 0.5f)
      wave = delta;
    else if(delta < 1.5f)
      wave = 1.0f-delta;
    else
      wave = delta-2.0f;

    //NOTE: If ever changing which feature this is, must also update index in model.py where we multiply it into the scorebelief parity vector
    rowGlobal[18] = wave;
  }

}

#ifndef GAME_RULES_H_
#define GAME_RULES_H_

#include "../core/global.h"
#include "../core/hash.h"

struct Rules {

  static const int SCORING_AREA = 0;
  int scoringRule;

  float komi;
  //Min and max acceptable komi in various places involving user input validation
  static constexpr float MIN_USER_KOMI = -150.0f;
  static constexpr float MAX_USER_KOMI = 150.0f;

  Rules();
  Rules(int koRule, int scoringRule, int taxRule, bool multiStoneSuicideLegal, bool hasButton, int whiteHandicapBonusRule, float komi);
  ~Rules();

  bool operator==(const Rules& other) const;
  bool operator!=(const Rules& other) const;

  bool gameResultWillBeInteger() const;

  static Rules getTrompTaylorish();

  static std::set<std::string> scoringRuleStrings();
  static int parseScoringRule(const std::string& s);
  static std::string writeScoringRule(int scoringRule);

  static bool komiIsIntOrHalfInt(float komi);

  static Rules parseRules(const std::string& str);
  static Rules parseRulesWithoutKomi(const std::string& str, float komi);
  static bool tryParseRules(const std::string& str, Rules& buf);
  static bool tryParseRulesWithoutKomi(const std::string& str, Rules& buf, float komi);

  static Rules updateRules(const std::string& key, const std::string& value, Rules priorRules);

  friend std::ostream& operator<<(std::ostream& out, const Rules& rules);
  std::string toString() const;
  std::string toStringNoKomi() const;
  std::string toJsonString() const;
  std::string toJsonStringNoKomi() const;

  static const Hash128 ZOBRIST_SCORING_RULE_HASH[2];
};

#endif  // GAME_RULES_H_

#include "../game/rules.h"

#include "../external/nlohmann_json/json.hpp"

#include <sstream>

using namespace std;
using json = nlohmann::json;

Rules::Rules() {
  //Defaults if not set - closest match to TT rules
  scoringRule = SCORING_AREA;
  komi = -0.5f;
}

Rules::Rules(int kRule, int sRule, int tRule, bool suic, bool button, int whbRule, float km)
  :scoringRule(sRule),komi(km)
{}

Rules::~Rules() {
}

bool Rules::operator==(const Rules& other) const {
  return
    scoringRule == other.scoringRule &&
    komi == other.komi;
}

bool Rules::operator!=(const Rules& other) const {
  return
    scoringRule != other.scoringRule ||
    komi != other.komi;
}

bool Rules::gameResultWillBeInteger() const {
  bool komiIsInteger = ((int)komi) == komi;
  return komiIsInteger ;
}

Rules Rules::getTrompTaylorish() {
  Rules rules;
  rules.scoringRule = SCORING_AREA;
  rules.komi = -0.5f;
  return rules;
}


bool Rules::komiIsIntOrHalfInt(float komi) {
  return std::isfinite(komi) && komi * 2 == (int)(komi * 2);
}

set<string> Rules::scoringRuleStrings() {
  return {"AREA"};
}

int Rules::parseScoringRule(const string& s) {
  if(s == "AREA") return Rules::SCORING_AREA;
  else throw IOError("Rules::parseScoringRule: Invalid scoring rule: " + s);
}
string Rules::writeScoringRule(int scoringRule) {
  if(scoringRule == Rules::SCORING_AREA) return string("AREA");
  return string("UNKNOWN");
}

ostream& operator<<(ostream& out, const Rules& rules) {
	out << "score" << Rules::writeScoringRule(rules.scoringRule);
	out << "komi" << rules.komi;
  return out;
}

string Rules::toStringNoKomi() const {
  ostringstream out;
  out << "score" << Rules::writeScoringRule(scoringRule);
  return out.str();
}

string Rules::toString() const {
  ostringstream out;
  out << (*this);
  return out.str();
}

string Rules::toJsonString() const {
  json ret;
  ret["scoring"] = writeScoringRule(scoringRule);
  ret["komi"] = komi;
  return ret.dump();
}

string Rules::toJsonStringNoKomi() const {
  json ret;
  ret["scoring"] = writeScoringRule(scoringRule);
  return ret.dump();
}

Rules Rules::updateRules(const string& k, const string& v, Rules oldRules) {
  Rules rules = oldRules;
  string key = Global::trim(k);
  string value = Global::trim(Global::toUpper(v));
  if(key == "score") rules.scoringRule = Rules::parseScoringRule(value);
  else if(key == "scoring") rules.scoringRule = Rules::parseScoringRule(value);
  else throw IOError("Unknown rules option: " + key);
  return rules;
}

static Rules parseRulesHelper(const string& sOrig, bool allowKomi) {
  Rules rules;


  return rules;
  /*
  string lowercased = Global::trim(Global::toLower(sOrig));
  if(lowercased == "japanese" || lowercased == "korean") {
    rules.scoringRule = Rules::SCORING_TERRITORY;
    rules.koRule = Rules::KO_SIMPLE;
    rules.taxRule = Rules::TAX_SEKI;
    rules.multiStoneSuicideLegal = false;
    rules.hasButton = false;
    rules.whiteHandicapBonusRule = Rules::WHB_ZERO;
    rules.komi = 6.5;
  }
  else if(lowercased == "chinese") {
    rules.scoringRule = Rules::SCORING_AREA;
    rules.koRule = Rules::KO_SIMPLE;
    rules.taxRule = Rules::TAX_NONE;
    rules.multiStoneSuicideLegal = false;
    rules.hasButton = false;
    rules.whiteHandicapBonusRule = Rules::WHB_N;
    rules.komi = 7.5;
  }
  else if(
    lowercased == "ancient-area" || lowercased == "ancient_area" || lowercased == "ancient area" ||
    lowercased == "stone-scoring" || lowercased == "stone_scoring" || lowercased == "stone scoring"
  ) {
    rules.scoringRule = Rules::SCORING_AREA;
    rules.koRule = Rules::KO_SIMPLE;
    rules.taxRule = Rules::TAX_ALL;
    rules.multiStoneSuicideLegal = false;
    rules.hasButton = false;
    rules.whiteHandicapBonusRule = Rules::WHB_ZERO;
    rules.komi = 7.5;
  }
  else if(lowercased == "ancient-territory" || lowercased == "ancient_territory" || lowercased == "ancient territory") {
    rules.scoringRule = Rules::SCORING_TERRITORY;
    rules.koRule = Rules::KO_SIMPLE;
    rules.taxRule = Rules::TAX_ALL;
    rules.multiStoneSuicideLegal = false;
    rules.hasButton = false;
    rules.whiteHandicapBonusRule = Rules::WHB_ZERO;
    rules.komi = 6.5;
  }
  else if(lowercased == "aga-button" || lowercased == "aga_button" || lowercased == "aga button") {
    rules.scoringRule = Rules::SCORING_AREA;
    rules.koRule = Rules::KO_SITUATIONAL;
    rules.taxRule = Rules::TAX_NONE;
    rules.multiStoneSuicideLegal = false;
    rules.hasButton = true;
    rules.whiteHandicapBonusRule = Rules::WHB_N_MINUS_ONE;
    rules.komi = 7.0;
  }
  else if(lowercased == "aga" || lowercased == "bga" || lowercased == "french") {
    rules.scoringRule = Rules::SCORING_AREA;
    rules.koRule = Rules::KO_SITUATIONAL;
    rules.taxRule = Rules::TAX_NONE;
    rules.multiStoneSuicideLegal = false;
    rules.hasButton = false;
    rules.whiteHandicapBonusRule = Rules::WHB_N_MINUS_ONE;
    rules.komi = 7.5;
  }
  else if(lowercased == "nz" || lowercased == "new zealand" || lowercased == "new-zealand" || lowercased == "new_zealand") {
    rules.scoringRule = Rules::SCORING_AREA;
    rules.koRule = Rules::KO_SITUATIONAL;
    rules.taxRule = Rules::TAX_NONE;
    rules.multiStoneSuicideLegal = true;
    rules.hasButton = false;
    rules.whiteHandicapBonusRule = Rules::WHB_ZERO;
    rules.komi = 7.5;
  }
  else if(lowercased == "tromp-taylor" || lowercased == "tromp_taylor" || lowercased == "tromp taylor" || lowercased == "tromptaylor") {
    rules.scoringRule = Rules::SCORING_AREA;
    rules.koRule = Rules::KO_POSITIONAL;
    rules.taxRule = Rules::TAX_NONE;
    rules.multiStoneSuicideLegal = true;
    rules.hasButton = false;
    rules.whiteHandicapBonusRule = Rules::WHB_ZERO;
    rules.komi = 7.5;
  }
  else if(lowercased == "goe" || lowercased == "ing") {
    rules.scoringRule = Rules::SCORING_AREA;
    rules.koRule = Rules::KO_POSITIONAL;
    rules.taxRule = Rules::TAX_NONE;
    rules.multiStoneSuicideLegal = true;
    rules.hasButton = false;
    rules.whiteHandicapBonusRule = Rules::WHB_ZERO;
    rules.komi = 7.5;
  }
  else if(sOrig.length() > 0 && sOrig[0] == '{') {
    //Default if not specified
    rules = Rules::getTrompTaylorish();
    bool komiSpecified = false;
    bool taxSpecified = false;
    try {
      json input = json::parse(sOrig);
      string s;
      for(json::iterator iter = input.begin(); iter != input.end(); ++iter) {
        string key = iter.key();
        if(key == "ko")
          rules.koRule = Rules::parseKoRule(iter.value().get<string>());
        else if(key == "score")
          rules.scoringRule = Rules::parseScoringRule(iter.value().get<string>());
        else if(key == "scoring")
          rules.scoringRule = Rules::parseScoringRule(iter.value().get<string>());
        else if(key == "tax") {
          rules.taxRule = Rules::parseTaxRule(iter.value().get<string>()); taxSpecified = true;
        }
        else if(key == "suicide")
          rules.multiStoneSuicideLegal = iter.value().get<bool>();
        else if(key == "hasButton")
          rules.hasButton = iter.value().get<bool>();
        else if(key == "whiteHandicapBonus")
          rules.whiteHandicapBonusRule = Rules::parseWhiteHandicapBonusRule(iter.value().get<string>());
        else if(key == "komi") {
          if(!allowKomi)
            throw IOError("Unknown rules option: " + key);
          rules.komi = iter.value().get<float>();
          komiSpecified = true;
          if(rules.komi < Rules::MIN_USER_KOMI || rules.komi > Rules::MAX_USER_KOMI || !Rules::komiIsIntOrHalfInt(rules.komi))
            throw IOError("Komi value is not a half-integer or is too extreme");
        }
        else
          throw IOError("Unknown rules option: " + key);
      }
    }
    catch(nlohmann::detail::exception&) {
      throw IOError("Could not parse rules: " + sOrig);
    }
    if(!taxSpecified)
      rules.taxRule = (rules.scoringRule == Rules::SCORING_TERRITORY ? Rules::TAX_SEKI : Rules::TAX_NONE);
    if(!komiSpecified) {
      //Drop default komi to 6.5 for territory rules, and to 7.0 for button
      if(rules.scoringRule == Rules::SCORING_TERRITORY)
        rules.komi = 6.5f;
      else if(rules.hasButton)
        rules.komi = 7.0f;
    }
  }

  //This is more of a legacy internal format, not recommended for users to provide
  else {
    auto startsWithAndStrip = [](string& str, const string& prefix) {
      bool matches = str.length() >= prefix.length() && str.substr(0,prefix.length()) == prefix;
      if(matches)
        str = str.substr(prefix.length());
      str = Global::trim(str);
      return matches;
    };

    //Default if not specified
    rules = Rules::getTrompTaylorish();

    string s = sOrig;
    s = Global::trim(s);

    //But don't allow the empty string
    if(s.length() <= 0)
      throw IOError("Could not parse rules: " + sOrig);

    bool komiSpecified = false;
    bool taxSpecified = false;
    while(true) {
      if(s.length() <= 0)
        break;

      if(startsWithAndStrip(s,"komi")) {
        if(!allowKomi)
          throw IOError("Could not parse rules: " + sOrig);
        int endIdx = 0;
        while(endIdx < s.length() && !Global::isAlpha(s[endIdx] && !Global::isWhitespace(s[endIdx])))
          endIdx++;
        float komi;
        bool suc = Global::tryStringToFloat(s.substr(0,endIdx),komi);
        if(!suc)
          throw IOError("Could not parse rules: " + sOrig);
        if(!std::isfinite(komi) || komi > 1e5 || komi < -1e5)
          throw IOError("Could not parse rules: " + sOrig);
        rules.komi = komi;
        komiSpecified = true;
        s = s.substr(endIdx);
        s = Global::trim(s);
        continue;
      }
      if(startsWithAndStrip(s,"ko")) {
        if(startsWithAndStrip(s,"SIMPLE")) rules.koRule = Rules::KO_SIMPLE;
        else if(startsWithAndStrip(s,"POSITIONAL")) rules.koRule = Rules::KO_POSITIONAL;
        else if(startsWithAndStrip(s,"SITUATIONAL")) rules.koRule = Rules::KO_SITUATIONAL;
        else if(startsWithAndStrip(s,"SPIGHT")) rules.koRule = Rules::KO_SPIGHT;
        else throw IOError("Could not parse rules: " + sOrig);
        continue;
      }
      if(startsWithAndStrip(s,"scoring")) {
        if(startsWithAndStrip(s,"AREA")) rules.scoringRule = Rules::SCORING_AREA;
        else if(startsWithAndStrip(s,"TERRITORY")) rules.scoringRule = Rules::SCORING_TERRITORY;
        else throw IOError("Could not parse rules: " + sOrig);
        continue;
      }
      if(startsWithAndStrip(s,"score")) {
        if(startsWithAndStrip(s,"AREA")) rules.scoringRule = Rules::SCORING_AREA;
        else if(startsWithAndStrip(s,"TERRITORY")) rules.scoringRule = Rules::SCORING_TERRITORY;
        else throw IOError("Could not parse rules: " + sOrig);
        continue;
      }
      if(startsWithAndStrip(s,"tax")) {
        if(startsWithAndStrip(s,"NONE")) {rules.taxRule = Rules::TAX_NONE; taxSpecified = true;}
        else if(startsWithAndStrip(s,"SEKI")) {rules.taxRule = Rules::TAX_SEKI; taxSpecified = true;}
        else if(startsWithAndStrip(s,"ALL")) {rules.taxRule = Rules::TAX_ALL; taxSpecified = true;}
        else throw IOError("Could not parse rules: " + sOrig);
        continue;
      }
      if(startsWithAndStrip(s,"sui")) {
        if(startsWithAndStrip(s,"1")) rules.multiStoneSuicideLegal = true;
        else if(startsWithAndStrip(s,"0")) rules.multiStoneSuicideLegal = false;
        else throw IOError("Could not parse rules: " + sOrig);
        continue;
      }
      if(startsWithAndStrip(s,"button")) {
        if(startsWithAndStrip(s,"1")) rules.hasButton = true;
        else if(startsWithAndStrip(s,"0")) rules.hasButton = false;
        else throw IOError("Could not parse rules: " + sOrig);
        continue;
      }
      if(startsWithAndStrip(s,"whb")) {
        if(startsWithAndStrip(s,"0")) {rules.whiteHandicapBonusRule = Rules::WHB_ZERO;}
        else if(startsWithAndStrip(s,"N-1")) {rules.whiteHandicapBonusRule = Rules::WHB_N_MINUS_ONE;}
        else if(startsWithAndStrip(s,"N")) {rules.whiteHandicapBonusRule = Rules::WHB_N;}
        else throw IOError("Could not parse rules: " + sOrig);
        continue;
      }

      //Unknown rules format
      else throw IOError("Could not parse rules: " + sOrig);
    }
    if(!taxSpecified)
      rules.taxRule = (rules.scoringRule == Rules::SCORING_TERRITORY ? Rules::TAX_SEKI : Rules::TAX_NONE);
    if(!komiSpecified) {
      //Drop default komi to 6.5 for territory rules, and to 7.0 for button
      if(rules.scoringRule == Rules::SCORING_TERRITORY)
        rules.komi = 6.5f;
      else if(rules.hasButton)
        rules.komi = 7.0f;
    }
  }

  return rules;*/
}

Rules Rules::parseRules(const string& sOrig) {
  return parseRulesHelper(sOrig,true);
}
Rules Rules::parseRulesWithoutKomi(const string& sOrig, float komi) {
  Rules rules = parseRulesHelper(sOrig,false);
  rules.komi = komi;
  return rules;
}

bool Rules::tryParseRules(const string& sOrig, Rules& buf) {
  Rules rules;
  try { rules = parseRulesHelper(sOrig,true); }
  catch(const StringError&) { return false; }
  buf = rules;
  return true;
}
bool Rules::tryParseRulesWithoutKomi(const string& sOrig, Rules& buf, float komi) {
  Rules rules;
  try { rules = parseRulesHelper(sOrig,false); }
  catch(const StringError&) { return false; }
  rules.komi = komi;
  buf = rules;
  return true;
}


const Hash128 Rules::ZOBRIST_SCORING_RULE_HASH[2] = {
  //Based on sha256 hash of Rules::SCORING_AREA, but also mixing none tax rule hash, to preserve legacy hashes
  Hash128(0x8b3ed7598f901494ULL ^ 0x72eeccc72c82a5e7ULL, 0x1dfd47ac77bce5f8ULL ^ 0x0d1265e413623e2bULL),
  //Based on sha256 hash of Rules::SCORING_TERRITORY, but also mixing seki tax rule hash, to preserve legacy hashes
  Hash128(0x381345dc357ec982ULL ^ 0x125bfe48a41042d5ULL, 0x03ba55c026026b56ULL ^ 0x061866b5f2b98a79ULL),
};